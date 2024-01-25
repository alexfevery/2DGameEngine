#include "Input.h"

using namespace std;
using namespace Input;

TextInputState TextInputState::InputState;

void Input::CloseOverlay()
{
	if (hInputWindow == NULL) { return; }
	SendMessage(hInputWindow, WM_CLOSE, 0, 0);
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
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		// Unregister the window class
		UnregisterClass(CLASS_NAME, GetModuleHandle(nullptr));
		}).detach();
}


LRESULT CALLBACK Input::ProcessInputMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Util_Assert(IsWindow(m_hwnd), L"error invalid handle");
	switch (msg)
	{
	case WM_CLOSE:
		SignalCloseWindow = true;
		return 0;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		PostMessage(GetParent(m_hwnd), msg, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
		SetFocus(hEdit);
		if (msg == WM_MOUSEWHEEL)
		{
			Util::InputManager::UpdateMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		}
		if (msg == WM_MOUSEMOVE)
		{
			POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(m_hwnd, &ptClient);
			Util::InputManager::UpdateMouseMove(ptClient.x, ptClient.y);
		}
		break;
	case WM_LBUTTONDOWN:
		SetFocus(hEdit);
		break;
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
	}
	return CallWindowProc(g_OldEditProc, hwnd, msg, wParam, lParam);
}

void InputAwaiter::TriggerInput(InputResultType type)
{
	InputAwaiter::UserAction.ActionType = type;
	InputAwaiter::TriggerAwaiter();
}

void Input::GetControls()
{
	if (GetForegroundWindow() == hInputWindow)
	{
		Util::InputManager::CaptureInput();
		if (g_onNextKeyDown && Util::InputManager::GetFirstKeyPressed() != -1)
		{
			g_onNextKeyDown();
			g_onNextKeyDown = nullptr;
		}
		bool TextInputInProgress = false;
		for (InputAwaiter* a1 : InputAwaiter::GetAwaiters())
		{
			if (!a1->IsSet()) { continue; }
			if (a1->ExpectedType == InputType::TEXTENTRY && !IMEComposing)
			{
				if ((Util::InputManager::IsControlFreshlyPressed(VK_ESCAPE) && a1->AllowCancel) || Util::InputManager::IsControlFreshlyPressed(VK_RETURN))
				{
					if (IMEActivated)
					{
						IMEActivated = false;
						continue;
					}
					a1->UserAction.vk_code = (Util::InputManager::IsControlFreshlyPressed(VK_ESCAPE) ? VK_ESCAPE : VK_RETURN);
					if (g_onEndTextEntryFunc) { SetInputText(g_onEndTextEntryFunc(GetInputText()), true, false); }
					TextInputState::SetInputState(ProcessTextInput());
					a1->TriggerInput(Util::InputManager::IsControlFreshlyPressed(VK_ESCAPE) ? InputResultType::INPUTCANCEL : InputResultType::TEXTSUBMIT);
					continue;
				}
			}

			if ((a1->ExpectedType == InputType::MOUSECLICK) && Util::InputManager::IsControlFreshlyPressed(VK_LBUTTON))
			{
				if (Util::PosDistanceClientArea(hInputWindow, Util::InputManager::GetMousePos()) == Util::Vector2(0))
				{
					a1->UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, Util::InputManager::GetMousePos());
					a1->UserAction.vk_code = VK_LBUTTON;
					a1->TriggerInput(InputResultType::MOUSECLICK);
					continue;
				}
			}

			if ((a1->ExpectedType == InputType::MOUSEWHEEL) && Util::InputManager::GetLastMouseWheelDelta() != 0)
			{
				if (Util::PosDistanceClientArea(hInputWindow, Util::InputManager::GetMousePos()) == Util::Vector2(0, 0))
				{
					a1->UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, Util::InputManager::GetMousePos());
					a1->UserAction.mouseWheelDelta = Util::InputManager::GetLastMouseWheelDelta();
					a1->TriggerInput(InputResultType::MOUSEWHEEL);
					continue;
				}
			}

			if (a1->ExpectedType == InputType::KEYPRESS || a1->ExpectedType == InputType::KEYDOWN || a1->ExpectedType == InputType::KEYUP)
			{
				int FirstKey = (a1->ExpectedType == InputType::KEYDOWN ? Util::InputManager::GetFirstKeyDown() : a1->ExpectedType == InputType::KEYUP ? Util::InputManager::GetFirstKeyReleased() : Util::InputManager::GetFirstKeyPressed());
				if (FirstKey == -1) { continue; }
				if ((a1->ExpectedKeys.empty() || std::find(a1->ExpectedKeys.begin(), a1->ExpectedKeys.end(), FirstKey) != a1->ExpectedKeys.end()))
				{
					a1->UserAction.vk_code = FirstKey;
					a1->TriggerInput(InputResultType::KEYPRESS);
					continue;
				}
			}

			if ((a1->ExpectedType == InputType::MOUSEMOVE) && Util::InputManager::GetLastMouseDelta() != Util::Vector2(0))
			{
				if (Util::PosDistanceClientArea(hInputWindow, Util::InputManager::GetMousePos()) == Util::Vector2(0, 0))
				{
					a1->UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, Util::InputManager::GetMousePos());
					a1->UserAction.MouseDelta = Util::InputManager::GetLastMouseDelta();
					a1->TriggerInput(InputResultType::MOUSEMOVE);
					continue;
				}
			}
			if (a1->ExpectedType == InputType::TEXTENTRY) { TextInputInProgress = true; }

			if (a1->ExpectedType == InputType::IDLE)
			{
				a1->UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, Util::InputManager::GetMousePos());
				a1->UserAction.MouseDelta = Util::Vector2(0, 0);
				a1->TriggerInput(InputResultType::IDLE);
				continue;
			}
		}
		if (ManualInputFlag || TextInputInProgress) { TextInputState::SetInputState(ProcessTextInput()); }
		else { TextInputState::SetInputState({}); }
		Util::InputManager::ResetInput();
	}
	else
	{
		ManualInputFlag = false;
		TextInputState::SetInputState({});
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

void Input::ManualInput(wstring inputText)
{
	ManualInputFlag = true;
	SendMessageW(hEdit, EM_SETREADONLY, TRUE, 0);
	SetInputText(inputText, true, false);
}

void Input::EndManualInput()
{
	ManualInputFlag = false;
	SendMessageW(hEdit, EM_SETREADONLY, FALSE, 0);
	SetInputText(L"", true, false);
}


void Input::InputAwaiter::GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* promiseFulfilled)
{
	InputAwaiter* test = this;
	SetInputText(UserAction.PrefillInputText, true, false);
	SetFocus(hEdit);
	while (Util::GetTimeStamp() <= Timeout) { std::this_thread::yield(); }
	SetAwaiter();
	while (!promiseFulfilled->load())
	{
		if (SignalCloseWindow) { break; }
		if (IsTriggered() || (Interupt > 0 && Util::GetTimeStamp() >= Interupt))
		{
			if (Interupt > 0 && Util::GetTimeStamp() >= Interupt) { UserAction.InputTimedOut = true; }
			if (callback(UserAction))
			{
				bool expected = false;
				if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value(UserAction); }
				return;
			}
			else
			{
				SetAwaiter();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait for 10 milliseconds
	}
	bool expected = false;
	if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value({}); }  //In case of window close ensure the promise is fullfilled so the waiting function can close too. 
}


InputEvent Input::InputAwaiter::GetInput()
{
	SetFocus(hEdit);
	while (Util::GetTimeStamp() <= Timeout) { std::this_thread::yield(); }
	SetAwaiter();
	while (!IsTriggered())
	{
		if (SignalCloseWindow)
		{
			SignalCloseWindow = false;
			throw SignalCloseWindowException();
		}
		if (Interupt > 0 && Util::GetTimeStamp() >= Interupt)
		{
			UserAction.InputTimedOut = true;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait for 10 milliseconds
	}
	std::lock_guard<std::mutex> lock(mtx);
	Awaiters.erase(std::find(Awaiters.begin(), Awaiters.end(), this));
	return UserAction;
}


static wstring IMEcomptext = L"";
static wstring SelectedCandidate = L"";
static int IMECusorpos = 0;
static bool IMEcomp = false;
static int IMECandidateCount = 0;
TextInputState Input::ProcessTextInput()
{
	TextInputState CurrentTextInputState;
	CurrentTextInputState.TextInputInProgress = true;
	CurrentTextInputState.ManualInput = ManualInputFlag;
	CurrentInputText = GetInputText();
	CurrentTextInputState.CurrentInputText = CurrentInputText;

	if (IMEComposing && hImc != NULL)
	{
		IMEActivated = true;
		CurrentTextInputState.IMEComposing = true;
		DWORD dwCompLen = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);
		std::vector<wchar_t> compositionBuffer(dwCompLen / sizeof(wchar_t));
		ImmGetCompositionString(hImc, GCS_COMPSTR, compositionBuffer.data(), dwCompLen);
		std::wstring NewCompositionText(compositionBuffer.begin(), compositionBuffer.end());
		if (!NewCompositionText.empty() && !IMECompositionComplete)
		{
			IMECompositionText = NewCompositionText;
			CurrentTextInputState.IMECompositionText = NewCompositionText;
			//CurrentTextInputState.IMECursorPos = ImmGetCompositionString(hImc, GCS_CURSORPOS, NULL, 0);
			CurrentTextInputState.IMECursorPos = static_cast<int>(NewCompositionText.size());
			CANDIDATELIST* pCandidateList;
			DWORD dwSize = ImmGetCandidateList(hImc, 0, NULL, 0);
			pCandidateList = (CANDIDATELIST*) new char[dwSize + sizeof(wchar_t)]; // Allocate extra space
			ImmGetCandidateList(hImc, 0, pCandidateList, dwSize);
			DWORD* OffSet = pCandidateList->dwOffset;
			DWORD TotalSize = pCandidateList->dwSize;
			int CandidateCount = pCandidateList->dwCount;
			CurrentTextInputState.SelectedCandidate = pCandidateList->dwSelection;
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
				CurrentTextInputState.guiCandidateTexts.push_back(std::wstring(start, real_end - start));
			}
			IMECandidateCount = min(CandidateCount, 5);
			delete[] pCandidateList;
		}
	}
	if (IMECompositionComplete && !IMECompositionText.empty())
	{
		//SetInputText(GetInputText() + IMECompositionText, true, true);
		IMECompositionText = L"";
		IMECompositionComplete = false;
	}
	IMEcomptext = CurrentTextInputState.IMECompositionText;
	IMECusorpos = CurrentTextInputState.IMECursorPos;
	IMEcomp = CurrentTextInputState.IMEComposing;
	return CurrentTextInputState;

}


