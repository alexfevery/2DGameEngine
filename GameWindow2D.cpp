#include "GameWindow2D.h"

using namespace std;

GameWindow2D::GameWindow2D(float screenWidthPercent, float screenHeightPercent)
{


	WindowThread = std::thread([this, screenWidthPercent, screenHeightPercent]()
		{
			std::unique_lock<std::mutex> lk(cv_m);
			cv.wait(lk, [this] { return RunWindow.load(); });
			SetThreadDescription(GetCurrentThread(), L"WndProc");

			int CurrentWidth = 800;
			int CurrentHeight = 600;
			int LocationX = CW_USEDEFAULT;
			int LocationY = CW_USEDEFAULT;
			windowStyle = WS_OVERLAPPEDWINDOW; 

			const wchar_t CLASS_NAME[] = L"GDIPlusWindowClass";
			WNDCLASS wc = { 0, ProcessWindowMessage, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, CLASS_NAME };
			RegisterClass(&wc);
			m_hwnd = CreateWindowEx(0, CLASS_NAME, L"Game Window", windowStyle, LocationX, LocationY, CurrentWidth, CurrentHeight, NULL, NULL, GetModuleHandle(NULL), this);
			UpdateWindowSize(screenWidthPercent, screenHeightPercent,true,false);
			if (this->onLoad) { this->onLoad(); }
			SetTimer(m_hwnd, 1, 1000 / 100, NULL);
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (trayIconData.isUpdated) { ProcessTray(); }
			}
		});
	WindowThread.detach();
}


void GameWindow2D::GetInput(int FPS, bool Show)
{
	this->TargetFrameRate = FPS;
	RunWindow.store(true);
	cv.notify_one();
	while (!this->m_hwnd) { std::this_thread::yield(); }
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	if (Show){this->ShowWindow(false);}
}


void GameWindow2D::ProcessTray()
{
	if (trayIconData.CurrentTrayData.hWnd != NULL)
	{
		trayIconData.CurrentTrayData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &(trayIconData.CurrentTrayData));
		trayIconData.CurrentTrayData = {};
	}

	// Set up the tray icon
	trayIconData.CurrentTrayData.cbSize = sizeof(NOTIFYICONDATA);
	trayIconData.CurrentTrayData.hWnd = m_hwnd;
	trayIconData.CurrentTrayData.uVersion = NOTIFYICON_VERSION_4;
	trayIconData.CurrentTrayData.uID = 5000;
	trayIconData.CurrentTrayData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	trayIconData.CurrentTrayData.uCallbackMessage = WM_APP;
	trayIconData.CurrentTrayData.hIcon = trayIconData.hIcon;
	wcscpy_s(trayIconData.CurrentTrayData.szTip, trayIconData.title.c_str());
	// Set up notification if available
	if (!trayIconData.NotificationTitle.empty() && !trayIconData.NotificationMessage.empty())
	{
		wcscpy_s(trayIconData.CurrentTrayData.szInfo, trayIconData.NotificationMessage.c_str());
		wcscpy_s(trayIconData.CurrentTrayData.szInfoTitle, trayIconData.NotificationTitle.c_str());
		trayIconData.CurrentTrayData.uTimeout = trayIconData.DurationMs;
		trayIconData.CurrentTrayData.dwInfoFlags = NIIF_INFO;
	}

	Shell_NotifyIconW(NIM_ADD, &trayIconData.CurrentTrayData);

	// Create the menu
	hMenu = CreatePopupMenu();
	for (int i = 0; i < trayIconData.menuItems.size(); ++i)
	{
		AppendMenuW(hMenu, MF_STRING, static_cast<UINT_PTR>(5001 + i), trayIconData.menuItems[i].c_str());
	}
	trayIconData.NotificationTitle.clear();
	trayIconData.NotificationMessage.clear();
	trayIconData.DurationMs = 0;
	trayIconData.isUpdated = false;
}

LRESULT CALLBACK ProcessWindowMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) { SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams); }
	GameWindow2D* pThis = (GameWindow2D*)GetWindowLongPtr(m_hwnd, GWLP_USERDATA);
	if (!pThis) { return DefWindowProc(m_hwnd, msg, wParam, lParam); }
	switch (msg)
	{
	case WM_APP:
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP)
		{
			POINT cursorPosition;
			GetCursorPos(&cursorPosition);
			SetForegroundWindow(m_hwnd);
			UINT clicked = TrackPopupMenu(pThis->hMenu, TPM_RETURNCMD | TPM_NONOTIFY, cursorPosition.x, cursorPosition.y, 0, m_hwnd, NULL);
			SendMessage(m_hwnd, WM_NULL, 0, 0);
			if (clicked >= 5000)
			{
				pThis->trayIconData.callback(clicked - 5000);
			}
			return 0;
		}
		if (lParam == 1029) { pThis->trayIconData.callback(-1); }
		break;
	case WM_CLOSE:
		if (pThis->onClose) { pThis->onClose(); }
		return 0;
	case WM_SIZE:
	case WM_MOVE:
		if (pThis->onResize) { pThis->onResize(0, 0); }
		break;
	case WM_PAINT:
	{
		auto start = std::chrono::high_resolution_clock::now(); 		
		if (pThis->onRenderFrame) { pThis->onRenderFrame(); }
		pThis->UpdateFps(start, 0);
		ValidateRect(m_hwnd, NULL);
		break;
	}
	case WM_SYSKEYUP:
		break;
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
		{
			if (pThis->windowStyle == WS_POPUP) { pThis->UpdateWindowSize(.5, .5, true); }
			else { pThis->UpdateWindowSize(-1, -1); }
		}
		break;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(LoadCursor(NULL, pThis->currentCursor == CT_HAND ? IDC_HAND : IDC_ARROW));
		}
		else { return DefWindowProc(m_hwnd, msg, wParam, lParam); }
		return 1;
	case WM_TIMER:
		if (pThis->onUpdateFrame) { pThis->onUpdateFrame(0.0); }
		InvalidateRect(m_hwnd, NULL, TRUE);
		break;
	default:
		return DefWindowProc(m_hwnd, msg, wParam, lParam);
	}
	return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void GameWindow2D::UpdateWindowSize(float screenWidthPercent, float screenHeightPercent,bool center,bool Update)
{
	Util_Assert(IsWindow(m_hwnd), L"Error window handle is null");
	SetProcessDPIAware();

	RECT rect;
	GetWindowRect(m_hwnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	if (screenWidthPercent == 0) { screenWidthPercent = static_cast<float>(width); }
	if (screenHeightPercent == 0) { screenHeightPercent = static_cast<float>(height); }

	int systemScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int systemScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	int LocationX = 0;
	int LocationY = 0;
	int CurrentWidth = 800;
	int CurrentHeight = 600;
	if (screenWidthPercent == -1 && screenHeightPercent == -1)
	{
		CurrentWidth = systemScreenWidth;
		CurrentHeight = systemScreenHeight;
		windowStyle = WS_POPUP;
		LocationX = 0;
		LocationY = 0;
	}
	else
	{
		CurrentWidth = static_cast<int>(systemScreenWidth * screenWidthPercent);
		CurrentHeight = static_cast<int>(systemScreenHeight * screenHeightPercent);
		LocationX = (systemScreenWidth - CurrentWidth) / 2;
		LocationY = (systemScreenHeight - CurrentHeight) / 2;
		windowStyle = WS_OVERLAPPEDWINDOW; 
	}

	if (screenWidthPercent == 1 && screenHeightPercent == 1) 
	{ 
		LocationX = 0;
		LocationY = 0;
		showWindowCommand = SW_SHOWMAXIMIZED; 
	}
	else { showWindowCommand = SW_SHOW; }
	
	SetWindowLongPtr(m_hwnd, GWL_STYLE, windowStyle);
	SetWindowPos(m_hwnd, HWND_TOP, LocationX, LocationY, CurrentWidth, CurrentHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
	if (screenWidthPercent > 0 && screenHeightPercent > 0 && screenWidthPercent < 1 && screenHeightPercent < 1 && center) 
	{ 
		RECT rectWindow;
		GetWindowRect(m_hwnd, &rectWindow);
		int windowWidth = rectWindow.right - rectWindow.left;
		int windowHeight = rectWindow.bottom - rectWindow.top;
		int screenWidth = systemScreenWidth;
		int screenHeight = systemScreenHeight;
		HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
		GetMonitorInfo(hMonitor, &monitorInfo);
		int centerX = (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2;
		int centerY = (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2;
		int newX = centerX - windowWidth / 2;
		int newY = centerY - windowHeight / 2;
		SetWindowPos(m_hwnd, NULL, newX, newY, windowWidth, windowHeight, SWP_NOZORDER);
	}
	if (!IsIconic(m_hwnd) && Update) { ::ShowWindow(m_hwnd, showWindowCommand); }
}


void GameWindow2D::UpdateFps(std::chrono::high_resolution_clock::time_point start, float targetFPS)
{
	static int FrameCount = 0;
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();

	if (targetFPS > 0)
	{
		std::chrono::duration<float, std::milli> renderTimeMs = end - start;
		std::chrono::duration<float, std::milli> desiredFrameTime = std::chrono::duration<float, std::milli>(1000.0f / targetFPS);
		if (renderTimeMs < desiredFrameTime)
		{
			std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(0.75f * (desiredFrameTime - renderTimeMs).count()));

			end = std::chrono::high_resolution_clock::now();
			renderTimeMs = end - start;

			while (renderTimeMs < desiredFrameTime)
			{
				end = std::chrono::high_resolution_clock::now();
				renderTimeMs = end - start;
			}
		}
	}
	FrameCount++;
	std::chrono::duration<float, std::milli> elapsedTime = end - startTime;
	if (elapsedTime.count() >= 1000.0f)
	{
		FrameRate = FrameCount;
		FrameCount = 0; 
		startTime = std::chrono::high_resolution_clock::now();
	}
}

void GameWindow2D::ShowTrayNotification(wstring title, wstring message, int durationMs)
{
	trayIconData.NotificationTitle = title;
	trayIconData.NotificationMessage = message;
	trayIconData.DurationMs = durationMs;
	trayIconData.isUpdated = true;
}

void GameWindow2D::ShowWindow(bool Centered)
{
	::ShowWindow(m_hwnd, showWindowCommand);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	SetForegroundWindow(m_hwnd);
	WindowOpen = true;
}
void GameWindow2D::HideWindow()
{
	::ShowWindow(m_hwnd, SW_HIDE);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	WindowOpen = false;
}

void GameWindow2D::CloseWindow()
{
	SendMessage(m_hwnd, WM_CLOSE, 0, 0);
}

void GameWindow2D::updateTrayIconData(HICON hIcon, wstring title, vector<wstring> menuItems, std::function<void(int)> callback)
{
	if (hIcon == nullptr) { trayIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION); }
	else { trayIconData.hIcon = hIcon; }
	trayIconData.title = title;
	trayIconData.menuItems = menuItems;
	trayIconData.callback = callback;
	trayIconData.isUpdated = true; 
}
