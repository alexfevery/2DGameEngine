#pragma once

#include <windowsx.h>
#include "CPPUtil.h"
#include <optional>
#include <future>
#include <functional>


namespace Input
{

	enum class InputResultType
	{
		NONE,
		KEYPRESS,
		MOUSEMOVE,
		IDLE,
		MOUSECLICK,
		MOUSEWHEEL,
		TEXTSUBMIT,
		INPUTCANCEL,
	};

	class KeyboardMouseState
	{
	private:
		enum KEYSTATE { KEY_NULL, KEY_UP, KEY_DOWN };
	public:
		void CaptureInput()
		{
			ButtonsDown.clear();
			ButtonsPressed.clear();
			ButtonsReleased.clear();
			keysDown.clear();
			keysPressed.clear();
			keysReleased.clear();
			lastMousePos = mousePos;
			lastMouseWheelPos = mouseWheelPosition;

			for (int key = 0; key < 256; key++)
			{
				prevKeyStates[key] = keyStates[key];
				keyStates[key] = (GetAsyncKeyState(key) & 0x8000) ? KEY_DOWN : KEY_UP;
				if (IsControlDown(key) && key < 7) { ButtonsDown.push_back(key); }
				if (IsControlFreshlyPressed(key) && key < 7) { ButtonsPressed.push_back(key); }
				if (IsControlReleased(key) && key < 7) { ButtonsReleased.push_back(key); }
				if (IsControlDown(key) && key > 7) { keysDown.push_back(key); }
				if (IsControlFreshlyPressed(key) && key > 7) { keysPressed.push_back(key); }
				if (IsControlReleased(key) && key > 7) { keysReleased.push_back(key); }
			}
		}

		bool IsControlFreshlyPressed(int VK_keycode)
		{
			return keyStates[VK_keycode] == KEY_DOWN && prevKeyStates[VK_keycode] == KEY_UP;
		}

		bool IsControlReleased(int VK_keycode)
		{
			return keyStates[VK_keycode] == KEY_UP;
		}

		bool IsControlDown(int VK_keycode)
		{
			return keyStates[VK_keycode] == KEY_DOWN;
		}

		std::vector<int> GetKeysPressed()
		{
			return keysPressed;
		}

		std::vector<int> GetKeysDown()
		{
			return keysDown;
		}

		std::vector<int> GetKeysReleased()
		{
			return keysReleased;
		}

		std::vector<int> GetButtonsPressed()
		{
			return ButtonsPressed;
		}

		std::vector<int> GetButtonsDown()
		{
			return ButtonsDown;
		}

		std::vector<int> GetButtonsReleased()
		{
			return ButtonsReleased;
		}

		int GetLastMouseWheelDelta(bool Reset)
		{
			int delta = (mouseWheelPosition - lastMouseWheelPos);
			if (Reset) { lastMouseWheelPos = mouseWheelPosition; }
			return delta;
		}

		Util::Vector2 GetLastMouseDelta(bool Reset)
		{
			Util::Vector2 delta = mousePos - lastMousePos;
			if (Reset) { lastMousePos = mousePos; }
			return delta;
		}

		static void UpdateMouseWheel(int wheelDelta)
		{
			mouseWheelPosition += (wheelDelta / WHEEL_DELTA);
		}

		static void UpdateMouseMove(int x, int y)
		{
			mousePos = Util::Vector2(static_cast<float>(x), static_cast<float>(y));
		}
		static Util::Vector2 GetMousePos()
		{
			return mousePos;
		}

	private:
		std::vector<int> ButtonsPressed; //key that is down in the current frame but was up in the previous frame.Captures freshly pressed keys
		std::vector<int> ButtonsReleased;  //key that was down in the previous frame but is now up.  Captures freshly released keys.
		std::vector<int> ButtonsDown; //Key that is down regardless of previous state.  Captures keys being held down. 
		std::vector<int> keysPressed; //key that is down in the current frame but was up in the previous frame.Captures freshly pressed keys
		std::vector<int> keysReleased;  //key that was down in the previous frame but is now up.  Captures freshly released keys.
		std::vector<int> keysDown; //Key that is down regardless of previous state.  Captures keys being held down. 
		KEYSTATE keyStates[256] = { KEY_NULL };
		KEYSTATE prevKeyStates[256] = { KEY_NULL };
		int lastMouseWheelPos = 0;
		Util::Vector2 lastMousePos = Util::Vector2();

		static inline int mouseWheelPosition = 0;
		static inline Util::Vector2 mousePos = Util::Vector2();
	};

	class SignalCloseWindowException : public std::exception {
	public:
		const char* what() const noexcept override {
			return "Signal to close window received.";
		}
	};

	inline HWND hEdit = nullptr; // Global handle to the edit control
	inline HIMC hImc = nullptr; // handle to IME context
	inline HWND hInputWindow;
	inline WNDPROC g_OldEditProc;

	inline RECT Hostrect;

	inline bool EnableKeyboardSelect = true;

	inline std::function<std::wstring(const std::wstring&)> g_onStartTextEntryFunc = nullptr;
	inline std::function<std::wstring(const std::wstring&)> g_processTextFunc = nullptr;
	inline std::function<std::wstring(const std::wstring&)> g_onEndTextEntryFunc = nullptr;
	inline std::function<void()> g_onNextKeyDown = nullptr;
	inline const int MaxInputLength = 256;
	inline std::wstring CurrentInputText;
	inline bool IMEComposing = false;
	inline bool IMECompositionComplete = false;
	inline bool IMEActivated = false;
	inline std::wstring IMECompositionText;
	inline bool SignalCloseWindow = false;

	inline HWND hostWindowHandle;

	struct IMEState
	{
		int SelectedCandidate = 0;
		int IMECursorPos = 0;
		std::wstring IMECompositionText;
		std::vector<std::wstring> guiCandidateTexts;
	};

	struct TextInputState
	{
		std::wstring CurrentInputText;
		std::optional<IMEState> IMEState;
	};

	struct InputEvent
	{
		InputResultType ActionType = InputResultType::NONE;
		int mouseWheelPosition = 0;
		std::wstring PrefillInputText;
		std::vector<int> vk_codes = { { -1} };
		Util::Vector2 MousePos;
		Util::Vector2 MouseDelta;
		bool InputTimedOut = false;
		int64_t starttime = -1;
		int64_t endtime = 0;
		int SelectedIndex = -1;
		TextInputState InputState;
		std::wstring GetFinalInputText() { return Util::EraseAllSubStr(CurrentInputText, L"\n"); }
		bool InputTextModified() { return !Util::StringsEqual(GetFinalInputText(), PrefillInputText); }
	};

	class InputAwaiter
	{
	private:
		void GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* sharedTrigger);
		friend InputEvent GetAllInputAsync(std::vector<InputAwaiter*> awaiters);

	public:
		InputAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, int pollingInterval = 10) : Interupt(interupt), AllowCancel(allowCancel), Timeout(timeout), CheckInterval(pollingInterval) {};

		template <typename T>
		struct is_correct_signature : std::false_type {};
		template <typename T>
		struct is_correct_signature<bool(T::*)(InputEvent&) const> : std::true_type {};
		template<typename T>
		void SetCallback(T callBack) {
			static_assert(is_correct_signature<decltype(&T::operator())>::value,
				"Callback must take a reference to InputEvent");
			callback = callBack;
		}
		virtual void SetEnabled(bool enable) 
		{
			UserAction = {};
			Enabled = enable;
		}
	protected:
		int CheckInterval;
		std::function<bool(InputEvent&)> callback = [](InputEvent&) { return true; };
		virtual bool CheckCondition() = 0;
		KeyboardMouseState KMState = {};
		int64_t Timeout;
		int64_t Interupt;

		InputEvent UserAction = {};
		bool AllowCancel = true;
		bool Enabled = true;

	public:
		InputEvent GetInput();

	};


	enum class PressType { PRESS, DOWN, UP };
	template<PressType Type>
	class KeyPressAwaiter : public InputAwaiter {
	public:
		KeyPressAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, std::vector<std::vector<int>> keys, int pollingInterval = 10) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval), ExpectedKeys(keys) {}
		bool CheckCondition() override;

	private:
		std::vector<std::vector<int>> ExpectedKeys;
	};

	enum class ClickType { PRESS, DOWN, UP };
	template<ClickType Type>
	class MouseClickAwaiter : public InputAwaiter {
	public:
		MouseClickAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, std::vector<std::vector<int>> buttons, int pollingInterval = 10) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval), ExpectedButtons(buttons) {}
		bool CheckCondition() override;

	private:
		std::vector<std::vector<int>> ExpectedButtons;
	};

	class MouseMoveAwaiter : public InputAwaiter
	{
	public:
		MouseMoveAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, int pollingInterval = 10) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval) { }
		bool CheckCondition() override;
	};

	class MouseWheelAwaiter : public InputAwaiter
	{
	public:
		MouseWheelAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, int pollingInterval = 50) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval) {}
		bool CheckCondition() override;
	};

	class TextEntryAwaiter : public InputAwaiter
	{
	public:
		TextEntryAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, int pollingInterval = 10) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval) {}
		bool CheckCondition() override;
		void SetPrefill(std::wstring prefill)
		{
			prefillInputText = prefill;
			UserAction.PrefillInputText = prefill;
		}
		void SetEnabled(bool enable) override
		{
			UserAction = {};
			UserAction.PrefillInputText = prefillInputText;
			Enabled = enable;
		}
		std::wstring prefillInputText;
	};

	class IdleAwaiter : public InputAwaiter
	{
	public:
		IdleAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, int pollingInterval = 10) : InputAwaiter(interupt, allowCancel, timeout, pollingInterval) { }
		bool CheckCondition() override;
	};

	inline InputEvent GetAllInputAsync(std::vector<InputAwaiter*> awaiters)
	{
		auto sharedPromise = std::make_shared<std::promise<InputEvent>>();
		std::atomic<bool> sharedAtomicFlag(false);
		std::vector<std::thread> threads;

		for (auto awaiter : awaiters)
		{
			threads.emplace_back([awaiter, &sharedAtomicFlag, &sharedPromise] {awaiter->GetInputAsync(sharedPromise, &sharedAtomicFlag); });
		}
		InputEvent result = sharedPromise->get_future().get();
		for (auto& t : threads)
		{
			t.join();
		}
		if (SignalCloseWindow)
		{
			SignalCloseWindow = false;
			throw SignalCloseWindowException();
		}
		return result;
	}

	void UpdateOverlayPosition(HWND hostWindowHandle);
	void CloseOverlay();
	void AttachToWindow(HWND windowHandle);

	inline void SetTextProcessingFunction(std::function<std::wstring(const std::wstring&)> func) { g_processTextFunc = func; }
	inline void SetOnStartTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onStartTextEntryFunc = func; }
	inline void SetOnEndTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onEndTextEntryFunc = func; }
	inline void SetOnNextKeyDownFunction(std::function<void()> func) { g_onNextKeyDown = func; }

	TextInputState ProcessTextInput();
	std::wstring GetInputText();
	void SetInputText(std::wstring text, bool SetCursorToEnd, bool TriggerMessage);
	static LRESULT CALLBACK ProcessInputMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK SubclassedEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}