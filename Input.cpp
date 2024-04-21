#include "Input.h"

using namespace std;
using namespace Input;

template class MouseClickAwaiter<ClickType::DOWN>;
template class MouseClickAwaiter<ClickType::UP>;
template class KeyPressAwaiter<PressType::DOWN>;
template class KeyPressAwaiter<PressType::UP>;


void Input::DestroyOverlay()
{
	if (hInputWindow == NULL) { return; }
	SendMessage(hInputWindow, WM_CLOSE, 0, 0);
}

void Input::RequestCloseWindow()
{
	SignalCloseWindow = true;
}

void Input::UpdateOverlayPosition(HWND hostWindowHandle)
{
	RECT newRect;
	GetClientRect(hostWindowHandle, &newRect);
	int width = newRect.right - newRect.left;
	int height = newRect.bottom - newRect.top;
	SetWindowPos(hInputWindow, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE);
}




void Input::AttachToWindow(HWND HostWindow)
{
	std::thread([=] {
		RECT rect = { 0, 0, 500, 300 }; // Default size for standalone window
		if (HostWindow != NULL) { GetClientRect(HostWindow, &rect); }

		const wchar_t CLASS_NAME[] = L"InputClass";
		WNDCLASSEX wc = { };
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = ProcessInputMessage;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszClassName = CLASS_NAME;
		RegisterClassExW(&wc);

		hInputWindow = CreateWindowEx(
			//WS_EX_LAYERED,
			WS_EX_TRANSPARENT,
			CLASS_NAME,
			NULL,
			WS_POPUP,
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			HostWindow, NULL, GetModuleHandle(nullptr), NULL);
		SetParent(hInputWindow, HostWindow);

		hEdit = CreateWindowExW(
			0,
			L"EDIT",
			NULL,
			//WS_CHILD | ES_AUTOHSCROLL | WS_VISIBLE,
			WS_CHILD | ES_AUTOHSCROLL,
			0, 0, 10, 10, hInputWindow, (HMENU)1, GetModuleHandle(nullptr), NULL);
		g_OldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)SubclassedEditProc);
		//SetLayeredWindowAttributes(hInputWindow, 0, 75, LWA_ALPHA);
		ShowWindow(hInputWindow, SW_SHOW);
		SetTimer(hInputWindow, 1, 1000 / 100, NULL);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		// Unregister the window class
		UnregisterClass(CLASS_NAME, GetModuleHandle(nullptr));
		hInputWindow = NULL;
		}).detach();
}


LRESULT CALLBACK Input::ProcessInputMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Util_Assert(IsWindow(m_hwnd), L"error invalid handle");
	AlertInputAwaiters(msg, wParam, lParam);

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(m_hwnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		PostMessage(GetParent(m_hwnd), msg, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEMOVE:
		break;
	case WM_KEYDOWN:
		SetFocus(hEdit);
		break;
	case WM_SETCURSOR:
		PostMessage(GetParent(m_hwnd), msg, wParam, lParam);
		return TRUE;
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetForegroundWindow(GetParent(m_hwnd));
		SetFocus(hEdit);
		break;
	case WM_ACTIVATE:
		WindowActive = (wParam != WA_INACTIVE);
		if (!WindowActive)
		{
			HWND hActiveWindow = (HWND)lParam;
			HWND hostWindow = GetParent(m_hwnd);
			if (hActiveWindow == hostWindow || IsChild(hostWindow, hActiveWindow)) {WindowActive = true;}
		}
		break;
	case WM_TIMER: {
		HWND foregroundWindow = GetForegroundWindow();
		HWND hostWindow = GetParent(m_hwnd);
		if (foregroundWindow == hostWindow && IsWindowVisible(hostWindow) && !Util::IsWindowMinimized(hostWindow))
		{
			if (Util::GetWindowRect(hostWindow).Contains(Util::GetCursorPos()))
			{
				if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0 && (GetAsyncKeyState(VK_MBUTTON) & 0x8000) == 0)
				{
					SetFocus(hEdit);
				}
			}
		}
		break;
	}
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 1)
		{
			if (g_onStartTextEntryFunc)
			{
				SetInputText(g_processTextFunc(GetInputText()), true, false);
				g_onStartTextEntryFunc = nullptr;
			}
			else if (g_processTextFunc)
			{
				SetInputText(g_processTextFunc(GetInputText()), true, false);
			}
		}
		return 0;
	default:
		return DefWindowProcW(m_hwnd, msg, wParam, lParam);
		break;
	}
	return 0;
}

LRESULT CALLBACK Input::SubclassedEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	AlertInputAwaiters(msg, wParam, lParam);
	switch (msg)
	{
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		PostMessage(GetParent(hInputWindow), msg, wParam, lParam);
		break;
	case WM_INPUTLANGCHANGE:
		break;
	case WM_IME_SETCONTEXT:
		if (wParam == TRUE) { hImc = ImmGetContext(hwnd); }
		else
		{
			if (hImc != NULL)
			{
				ImmReleaseContext(hwnd, hImc);
				hImc = NULL;
			}
		}
		lParam = 0;
		break;
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_COMPOSITION:
		IMEComposing = true;
		if (lParam & GCS_RESULTSTR)
		{
			IMECompositionComplete = true;
			IMEComposing = false;
			ImmReleaseContext(hwnd, hImc);
		}
		break;
	case WM_IME_ENDCOMPOSITION:
		IMEComposing = false;
		ImmReleaseContext(hwnd, hImc);
		break;
	case WM_CHAR:
		if (msg == WM_CHAR)
		{
			if (wParam == VK_RETURN || wParam == VK_ESCAPE) { return true; }
			if (IMEComposing && wParam == VK_RIGHT || wParam == VK_LEFT) { return true; }
		}
		break;
	case WM_PASTE: {
		wstring text = Util::GetClipboardText(hwnd);
		text = Util::ReplaceAllSubStr(Util::ReplaceAllSubStr(text, L"\n", L""),L"\r",L"");
		if(!text.empty())
		{ 
			wstring existing = GetInputText();
			if((text.size()+existing.size()) < MaxInputLength)
			{
				int pos = GetCursorPosition();
				SetInputText(existing.substr(0, pos) + text + existing.substr(pos),false,true);
				SendMessage(hwnd, EM_SETSEL, pos + text.length(), pos + text.length());
			}
		}
		return 0;
	}
	}
	return CallWindowProc(g_OldEditProc, hwnd, msg, wParam, lParam);
}


void Input::AlertInputAwaiters(UINT message, WPARAM wParam, LPARAM lParam) {

	int mouseWheeldelta = 0;
	int mouseX = -1;
	int mouseY = -1;
	int VKCode = -1;
	bool down = false;
	bool key = false;
	switch (message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP: {
		VKCode = static_cast<int>(wParam);
		down = (message == WM_KEYDOWN);
		key = true;
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		VKCode = VK_LBUTTON;
		down = (message == WM_LBUTTONDOWN);
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		VKCode = VK_RBUTTON;
		down = (message == WM_RBUTTONDOWN);
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		VKCode = VK_MBUTTON;
		down = (message == WM_MBUTTONDOWN);
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) { VKCode = VK_XBUTTON1; }
		else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) { VKCode = VK_XBUTTON2; }
		down = (message == WM_XBUTTONDOWN);
		break;
	case WM_MOUSEMOVE: {
		break;
	}
	case WM_MOUSEWHEEL: {
		break;
	}
	case WM_TIMER: {
		break;
	}
	default: return;
	}
	if (VKCode != -1)
	{
		PreviousControlState[VKCode] = CurrentControlState[VKCode];
		CurrentControlState[VKCode] = down;
	}
	int64_t currentTime = Util::GetTimeStamp();
	std::optional<TextInputState> currentTextState;
	std::lock_guard<std::mutex> lk(listMutex);
	for (int i = 0; i < AwaiterList.size(); i++)
	{		
		InputAwaiter* currentAwaiter = AwaiterList[i];
		if (currentAwaiter->ConditionMet) { continue; }
		if (SignalCloseWindow)
		{
			currentAwaiter->Abort = true;
			currentAwaiter->cv.notify_one();
			continue;
		}
		if (InputActive)
		{
			if (!currentTextState.has_value()) { currentTextState = ProcessTextInput(); }
			currentAwaiter->UserAction.TextState = currentTextState.value();
		}
		if (message == WM_TIMER && (currentAwaiter->UserAction.ActionType == InputResultType::IDLE || (currentAwaiter->Interupt >= currentAwaiter->UserAction.starttime && currentAwaiter->Interupt <= currentTime)))
		{
			if (currentAwaiter->Interupt >= currentAwaiter->UserAction.starttime && currentAwaiter->Interupt <= currentTime) { currentAwaiter->UserAction.inputTimedOut = true; }
			currentAwaiter->ConditionMet = true;
		}
		else if (VKCode != -1 && (currentAwaiter->UserAction.ActionType == InputResultType::MOUSEDOWN || currentAwaiter->UserAction.ActionType == InputResultType::MOUSERELEASED || currentAwaiter->UserAction.ActionType == InputResultType::KEYDOWN || currentAwaiter->UserAction.ActionType == InputResultType::KEYRELEASED))
		{
			if (((currentAwaiter->UserAction.ActionType == InputResultType::MOUSEDOWN || currentAwaiter->UserAction.ActionType == InputResultType::MOUSERELEASED) && !key) || ((currentAwaiter->UserAction.ActionType == InputResultType::KEYDOWN || currentAwaiter->UserAction.ActionType == InputResultType::KEYRELEASED) && key))
			{
				vector<vector<int>> combos = (!currentAwaiter->UserAction.keyCombos.empty() ? currentAwaiter->UserAction.keyCombos : vector<vector<int>>{ { VKCode} });
				for (int i = 0; i < combos.size(); i++)
				{
					if (Util::VectorContains(combos[i], VKCode) && std::all_of(combos[i].begin(), combos[i].end(), [&](int k)
						{
							if (currentAwaiter->UserAction.ActionType == InputResultType::KEYDOWN || currentAwaiter->UserAction.ActionType == InputResultType::MOUSEDOWN) { return CurrentControlState[k] && !PreviousControlState[k]; }
							else if (currentAwaiter->UserAction.ActionType == InputResultType::KEYRELEASED || currentAwaiter->UserAction.ActionType == InputResultType::MOUSERELEASED) { return !CurrentControlState[k] && PreviousControlState[k]; }
							return false;
						}))
					{
						currentAwaiter->ConditionMet = true;
						currentAwaiter->UserAction.keyCombos = { combos[i] };
						break;
					}
				}
			}
		}
		else if (message == WM_MOUSEMOVE && currentAwaiter->UserAction.ActionType == InputResultType::MOUSEMOVE) { currentAwaiter->ConditionMet = true; }
		else if (message == WM_MOUSEWHEEL && currentAwaiter->UserAction.ActionType == InputResultType::MOUSEWHEEL)
		{
			currentAwaiter->ConditionMet = true;
			currentAwaiter->UserAction.mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			ScreenToClient(hInputWindow, &pt);
			currentAwaiter->UserAction.mousePos = Util::Vector2(pt.x, pt.y);
		}
		if (currentAwaiter->ConditionMet && (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONUP))
		{
			mouseX = GET_X_LPARAM(lParam);
			mouseY = GET_Y_LPARAM(lParam);
			currentAwaiter->UserAction.mousePos = Util::Vector2(mouseX, mouseY);
		}
		if (currentAwaiter->ConditionMet)
		{
			if(currentAwaiter->UserAction.ActionType == InputResultType::IDLE && message == WM_TIMER){ currentAwaiter->UserAction.mousePos = Util::GetClientAreaCursorPos(hInputWindow); }
			currentAwaiter->UserAction.WindowActive = WindowActive;
			currentAwaiter->UserAction.endtime = currentTime;
			currentAwaiter->cv.notify_one();
		}
	}
}

void Input::SetInputText(wstring text, bool SetCursorToEnd, bool TriggerMessage)
{
	if (!Util::StringsEqual(text, GetInputText()))
	{
		SetWindowText(hEdit, text.c_str());
		if (SetCursorToEnd) { SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(MaxInputLength - 1), static_cast<LPARAM>(MaxInputLength - 1)); }
		if (TriggerMessage) { SendMessageW(hEdit, WM_COMMAND, MAKEWPARAM(1, EN_CHANGE), (LPARAM)hEdit); }
	}
}

wstring Input::GetInputText()
{
	wchar_t text[MaxInputLength] = {};
	int textLength = GetWindowTextLength(hEdit) + 1; // +1 for null terminator
	Util_Assert(textLength < MaxInputLength, L"Max input length exceeded.");
	GetWindowText(hEdit, text, textLength);
	text[MaxInputLength - 1] = L'\0';
	return text;
}

int Input::GetCursorPosition()
{
	DWORD pos;
	SendMessageW(hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&pos), NULL);
	int textLength = GetWindowTextLength(hEdit);
	if (textLength == 0) { return 0; }
	if (pos > static_cast<DWORD>(textLength)) { return textLength; }
	return static_cast<int>(pos);
}


void Input::InputAwaiter::GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* promiseFulfilled)
{
Restart:
	while (!Enabled || Util::GetTimeStamp() <= Timeout)
	{
		if (promiseFulfilled->load()) { return; }
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
	SetInputText(PrefillInputText, true, false);
	if (GetForegroundWindow() == GetParent(hInputWindow)) { SetFocus(hEdit); }
	UserAction.starttime = Util::GetTimeStamp();
	try
	{
		while (!promiseFulfilled->load())
		{
			if (!Enabled) { goto Restart; }
			if (SignalCloseWindow) { break; }
			{
				std::lock_guard<std::mutex> lk(listMutex);
				AwaiterList.push_back(this);
			}
			Set();
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [&] { return ConditionMet || Abort; });
			{
				std::lock_guard<std::mutex> lk(listMutex);
				auto it = std::find(AwaiterList.begin(), AwaiterList.end(), this);
				if (it != AwaiterList.end()) { AwaiterList.erase(it); }
			}
			if (Paused) { continue; }
			if ((UserAction.InputTimedOut() || Abort || callback(UserAction)))
			{
				bool expected = false;
				if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value(UserAction); }
				break;
			}
		}
	}
	catch (...)
	{
		Exception = std::current_exception();
	}
	bool expected = false;
	if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value({}); }
}


InputEvent Input::InputAwaiter::GetInput()
{
Restart:
	SignalCloseWindow = false;
	while (!Enabled || Util::GetTimeStamp() <= Timeout) { std::this_thread::sleep_for(std::chrono::milliseconds(30)); }
	SetInputText(PrefillInputText, true, false);
	SetFocus(hEdit);
	UserAction.starttime = Util::GetTimeStamp();
	while (true)
	{
		if (!Enabled) { goto Restart; }
		if (SignalCloseWindow) { throw SignalCloseWindowException(); }
		Set();
		{
			std::lock_guard<std::mutex> lk(listMutex);
			AwaiterList.push_back(this);
		}
		std::unique_lock<std::mutex> lk(mtx);
		cv.wait(lk, [&] { return ConditionMet || Abort; });
		{
			std::lock_guard<std::mutex> lk(listMutex);
			auto it = std::find(AwaiterList.begin(), AwaiterList.end(), this);
			if (it != AwaiterList.end()) { AwaiterList.erase(it); }
		}
		if (Paused) { continue; }
		if ((UserAction.InputTimedOut() || Abort || callback(UserAction))) { break; }
	}
	return UserAction;
}

TextInputState Input::ProcessTextInput()
{
	TextInputState CurrentTextInputState = {};
	CurrentTextInputState.PrefillInputText = PrefillInputText;
	CurrentTextInputState.CurrentInputText = GetInputText();
	if(WindowActive){CurrentTextInputState.CursorPosition = GetCursorPosition();}
	else { CurrentTextInputState.CursorPosition = -1; }
	if (IMEComposing && hImc != NULL)
	{
		IMEActivated = true;
		DWORD dwCompLen = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);
		std::vector<wchar_t> compositionBuffer(dwCompLen / sizeof(wchar_t));
		ImmGetCompositionString(hImc, GCS_COMPSTR, compositionBuffer.data(), dwCompLen);
		std::wstring NewCompositionText(compositionBuffer.begin(), compositionBuffer.end());
		if (!NewCompositionText.empty() && !IMECompositionComplete)
		{
			IMECompositionText = NewCompositionText;
			CurrentTextInputState.IMEState.IMECompositionText = NewCompositionText;
			if (WindowActive) { CurrentTextInputState.IMEState.IMECursorPos = ImmGetCompositionString(hImc, GCS_CURSORPOS, NULL, 0); }
			else { CurrentTextInputState.IMEState.IMECursorPos = -1; }
			CANDIDATELIST* pCandidateList;
			DWORD dwSize = ImmGetCandidateList(hImc, 0, NULL, 0);
			pCandidateList = (CANDIDATELIST*) new char[dwSize + sizeof(wchar_t)]; // Allocate extra space
			ImmGetCandidateList(hImc, 0, pCandidateList, dwSize);
			DWORD* OffSet = pCandidateList->dwOffset;
			DWORD TotalSize = pCandidateList->dwSize;
			int CandidateCount = pCandidateList->dwCount;
			CurrentTextInputState.IMEState.SelectedCandidate = pCandidateList->dwSelection;
			for (int i = 0; i < min(CandidateCount, 5); i++)
			{
				wchar_t* start = (wchar_t*)((BYTE*)pCandidateList + OffSet[i]);
				if ((BYTE*)start >= ((BYTE*)pCandidateList + TotalSize)) { break; }
				wchar_t* end = 0;
				if (i + 1 < CandidateCount) { end = (wchar_t*)((BYTE*)pCandidateList + OffSet[i + 1]); }
				else { end = (wchar_t*)((BYTE*)pCandidateList + TotalSize); }
				if ((BYTE*)end > ((BYTE*)pCandidateList + TotalSize)) { end = (wchar_t*)((BYTE*)pCandidateList + TotalSize); }
				wchar_t* real_end = start;
				while (real_end < end && real_end < ((wchar_t*)pCandidateList + dwSize / sizeof(wchar_t)) && *real_end) { ++real_end; } // Check bounds
				CurrentTextInputState.IMEState.guiCandidateTexts.push_back(std::wstring(start, real_end - start));
			}
			delete[] pCandidateList;
		}
	}
	else { IMEActivated = false; }
	if (IMECompositionComplete && !IMECompositionText.empty())
	{
		//SetInputText(GetInputText() + IMECompositionText, true, true);
		IMECompositionText = L"";
		IMECompositionComplete = false;
	}
	return CurrentTextInputState;

}


