#include "Input.h"

using namespace std;
using namespace Input;

template class MouseClickAwaiter<ClickType::PRESS>;
template class MouseClickAwaiter<ClickType::DOWN>;
template class MouseClickAwaiter<ClickType::UP>;
template class KeyPressAwaiter<PressType::PRESS>;
template class KeyPressAwaiter<PressType::DOWN>;
template class KeyPressAwaiter<PressType::UP>;


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
			KeyboardMouseState::UpdateMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		}
		if (msg == WM_MOUSEMOVE)
		{
			POINT ptClient = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(m_hwnd, &ptClient);
			KeyboardMouseState::UpdateMouseMove(ptClient.x, ptClient.y);
		}
		break;
	case WM_SETCURSOR:
		PostMessage(GetParent(m_hwnd), msg, wParam, lParam);
		return TRUE;
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

template<PressType Type>
bool Input::KeyPressAwaiter<Type>::CheckCondition()
{
	KMState.CaptureInput();
	vector<int> Keys;
	if (Type == PressType::PRESS) { Keys = KMState.GetKeysPressed(); }
	else if (Type == PressType::DOWN) { Keys = KMState.GetKeysDown(); }
	else { Keys = KMState.GetKeysReleased(); }
	if (!Keys.empty())
	{
		bool triggered = false;
		for (int i = 0; i < ExpectedKeys.size(); i++)
		{
			bool allMet = true;
			for (int u = 0; u < ExpectedKeys[i].size(); u++)
			{
				if (!Util::VectorContains(Keys, ExpectedKeys[i][u]))
				{
					allMet = false;
					break;
				}
			}
			if (allMet)
			{
				triggered = true;
				break;
			}
		}
		if (ExpectedKeys.empty() || triggered)
		{
			UserAction.vk_codes = Keys;
			InputAwaiter::UserAction.ActionType = InputResultType::KEYPRESS;
			UserAction.endtime = Util::GetTimeStamp();
			return true;
		}
	}
	return false;
}

template<ClickType Type>
bool Input::MouseClickAwaiter<Type>::CheckCondition()
{
	KMState.CaptureInput();
	if (Util::PosDistanceClientArea(hInputWindow, KeyboardMouseState::GetMousePos()) == Util::Vector2(0, 0))
	{
		vector<int> Buttons;
		if (Type == ClickType::PRESS) { Buttons = KMState.GetButtonsPressed(); }
		else if (Type == ClickType::DOWN) { Buttons = KMState.GetButtonsDown(); }
		else { Buttons = KMState.GetButtonsReleased(); }
		if (!Buttons.empty())
		{
			bool triggered = false;
			for (int i = 0; i < ExpectedButtons.size(); i++)
			{
				bool allMet = true;
				for (int u = 0; u < ExpectedButtons[i].size(); u++)
				{
					if (!Util::VectorContains(Buttons, ExpectedButtons[i][u]))
					{
						allMet = false;
						break;
					}
				}
				if (allMet)
				{
					triggered = true;
					break;
				}
			}
			if (ExpectedButtons.empty() || triggered)
			{
				UserAction.vk_codes = Buttons;
				UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, KeyboardMouseState::GetMousePos());
				UserAction.MouseDelta = KMState.GetLastMouseDelta(false);
				InputAwaiter::UserAction.ActionType = InputResultType::MOUSECLICK;
				UserAction.endtime = Util::GetTimeStamp();
				return true;
			}
		}
	}
	return false;
}

bool Input::TextEntryAwaiter::CheckCondition()
{
	KMState.CaptureInput();
	if (g_onNextKeyDown && !KMState.GetKeysPressed().empty())
	{
		g_onNextKeyDown();
		g_onNextKeyDown = nullptr;
	}
	if (!IMEComposing)
	{
		if ((KMState.IsControlFreshlyPressed(VK_ESCAPE) && AllowCancel) || KMState.IsControlFreshlyPressed(VK_RETURN))
		{
			if (IMEActivated)
			{
				IMEActivated = false;
			}
			else
			{
				UserAction.vk_codes = { (KMState.IsControlFreshlyPressed(VK_ESCAPE) ? VK_ESCAPE : VK_RETURN) };
				if (g_onEndTextEntryFunc) { SetInputText(g_onEndTextEntryFunc(GetInputText()), true, false); }
				InputAwaiter::UserAction.ActionType = KMState.IsControlFreshlyPressed(VK_ESCAPE) ? InputResultType::INPUTCANCEL : InputResultType::TEXTSUBMIT;
				UserAction.endtime = Util::GetTimeStamp();
			}
		}
	}
	UserAction.InputState = ProcessTextInput();
	return true;
}

bool Input::IdleAwaiter::CheckCondition()
{
	UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, KeyboardMouseState::GetMousePos());
	UserAction.MouseDelta = Util::Vector2(0, 0);
	InputAwaiter::UserAction.ActionType = InputResultType::IDLE;
	UserAction.endtime = Util::GetTimeStamp();
	return true;
}


bool Input::MouseWheelAwaiter::CheckCondition()
{
	int delta = KMState.GetLastMouseWheelDelta(true);
	if (delta != 0)
	{
		if (Util::PosDistanceClientArea(hInputWindow, KeyboardMouseState::GetMousePos()) == Util::Vector2(0, 0))
		{
			UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, KeyboardMouseState::GetMousePos());
			UserAction.mouseWheelPosition = delta;
			InputAwaiter::UserAction.ActionType = InputResultType::MOUSEWHEEL;
			UserAction.endtime = Util::GetTimeStamp();
			return true;
		}
	}
	return false;
}

bool Input::MouseMoveAwaiter::CheckCondition()
{
	Util::Vector2 delta = KMState.GetLastMouseDelta(true);
	if (delta != Util::Vector2(0))
	{
		if (Util::PosDistanceClientArea(hInputWindow, KeyboardMouseState::GetMousePos()) == Util::Vector2(0, 0))
		{
			UserAction.MousePos = Util::GetClientAreaPos(hInputWindow, KeyboardMouseState::GetMousePos());
			UserAction.MouseDelta = delta;
			InputAwaiter::UserAction.ActionType = InputResultType::MOUSEMOVE;
			UserAction.endtime = Util::GetTimeStamp();
			return true;
		}
	}
	return false;
}

void Input::InputAwaiter::GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* promiseFulfilled)
{
	Restart:
	while (!Enabled || Util::GetTimeStamp() <= Timeout) 
	{ 
		if (promiseFulfilled->load()) { return; }
		std::this_thread::sleep_for(std::chrono::milliseconds(CheckInterval)); 
	}
	SetInputText(UserAction.PrefillInputText, true, false);
	SetFocus(hEdit);
	UserAction.starttime = Util::GetTimeStamp();
	while (!promiseFulfilled->load())
	{
		if (!Enabled) { goto Restart; }
		if (SignalCloseWindow) { break; }
		if (Interupt > 0 && Util::GetTimeStamp() >= Interupt)
		{
			UserAction.InputTimedOut = true;
			bool expected = false;
			if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value(UserAction); }
			break;
		}
		if (CheckCondition() && callback(UserAction))
		{
			bool expected = false;
			if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value(UserAction); }
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(CheckInterval));
	}
	bool expected = false;
	if (promiseFulfilled->compare_exchange_strong(expected, true)) { sharedPromise->set_value({}); } 
}


InputEvent Input::InputAwaiter::GetInput()
{
Restart:
	while (!Enabled || Util::GetTimeStamp() <= Timeout) { std::this_thread::sleep_for(std::chrono::milliseconds(CheckInterval)); }
	SetInputText(UserAction.PrefillInputText, true, false);
	SetFocus(hEdit);
	UserAction.starttime = Util::GetTimeStamp();
	while (true)
	{
		if (!Enabled) { goto Restart; }
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
		if (CheckCondition() && callback(UserAction)) { break; }
		std::this_thread::sleep_for(std::chrono::milliseconds(CheckInterval));
	}
	return UserAction;
}

TextInputState Input::ProcessTextInput()
{
	TextInputState CurrentTextInputState;
	CurrentInputText = GetInputText();
	CurrentTextInputState.CurrentInputText = CurrentInputText;

	if (IMEComposing && hImc != NULL)
	{
		IMEActivated = true;
		CurrentTextInputState.IMEState = IMEState();
		DWORD dwCompLen = ImmGetCompositionStringW(hImc, GCS_COMPSTR, NULL, 0);
		std::vector<wchar_t> compositionBuffer(dwCompLen / sizeof(wchar_t));
		ImmGetCompositionString(hImc, GCS_COMPSTR, compositionBuffer.data(), dwCompLen);
		std::wstring NewCompositionText(compositionBuffer.begin(), compositionBuffer.end());
		if (!NewCompositionText.empty() && !IMECompositionComplete)
		{
			IMECompositionText = NewCompositionText;
			CurrentTextInputState.IMEState->IMECompositionText = NewCompositionText;
			//CurrentTextInputState.IMECursorPos = ImmGetCompositionString(hImc, GCS_CURSORPOS, NULL, 0);
			CurrentTextInputState.IMEState->IMECursorPos = static_cast<int>(NewCompositionText.size());
			CANDIDATELIST* pCandidateList;
			DWORD dwSize = ImmGetCandidateList(hImc, 0, NULL, 0);
			pCandidateList = (CANDIDATELIST*) new char[dwSize + sizeof(wchar_t)]; // Allocate extra space
			ImmGetCandidateList(hImc, 0, pCandidateList, dwSize);
			DWORD* OffSet = pCandidateList->dwOffset;
			DWORD TotalSize = pCandidateList->dwSize;
			int CandidateCount = pCandidateList->dwCount;
			CurrentTextInputState.IMEState->SelectedCandidate = pCandidateList->dwSelection;
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
				CurrentTextInputState.IMEState->guiCandidateTexts.push_back(std::wstring(start, real_end - start));
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


