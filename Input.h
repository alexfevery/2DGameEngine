#pragma once

#include <windowsx.h>
#include "CPPUtil.h"

#include <future>
#include <functional>


namespace Input
{

	enum InputType
	{
		NONE,
		KEYDOWN,
		KEYUP,
		MOUSEMOVE,
		MOUSECLICK,
		MOUSEWHEEL,
		TEXTENTRY,
		KEYPRESS,
		IDLE
	};

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
	inline bool ManualInputFlag = false;

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

	inline std::mutex mtx;


	inline HWND hostWindowHandle;

	struct InputEvent
	{
		InputType Awaiter = InputType::NONE;
		InputResultType ActionType = InputResultType::NONE;
		int mouseWheelDelta = 0;
		std::wstring PrefillInputText;
		std::vector<int> vk_codes = { { -1} };
		Util::Vector2 MousePos;
		Util::Vector2 MouseDelta;
		bool InputTimedOut = false;
		int64_t starttime = -1;
		int64_t endtime = 0;
		int SelectedIndex = -1;

		std::wstring GetFinalInputText() { return Util::EraseAllSubStr(CurrentInputText, L"\n"); }
		bool InputTextModified() { return !Util::StringsEqual(GetFinalInputText(), PrefillInputText); }
	};


	struct TextInputState
	{
		bool TextInputInProgress = false;
		std::wstring CurrentInputText;
		std::wstring IMECompositionText;
		std::vector<std::wstring> guiCandidateTexts;
		int SelectedCandidate = 0;
		int IMECursorPos = 0;
		bool IMEComposing = false;
		bool ManualInput = false;

		static void SetInputState(const TextInputState state)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			InputState = state;
		}

		static TextInputState GetInputState()
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return InputState;
		}

	private:
		inline static std::mutex mutex_;
		static TextInputState InputState;
	};


	class InputAwaiter
	{
	private:
		enum State { IANotSet = 0, IASet = 1, IATriggered = 2 };
		std::atomic<int> state = IANotSet; // 0 - not set, 1 - set, 2 - triggered
		std::function<bool(InputEvent&)> callback = [](InputEvent&) { return true; };
		void GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* sharedTrigger);

	public:
		InputAwaiter(InputType type, int64_t interupt, bool allowCancel, int64_t timeout, std::vector<std::vector<int>> keys = {}) : ExpectedType(type), Interupt(interupt), AllowCancel(allowCancel), Timeout(timeout), ExpectedKeys(keys)
		{
			UserAction.Awaiter = type;
			std::lock_guard<std::mutex> lock(mtx);
			Awaiters.push_back(this);
		};

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

		int64_t Timeout;
		int64_t Interupt;
		InputType ExpectedType = InputType::NONE;
		std::vector<std::vector<int>> ExpectedKeys;
		InputEvent UserAction = {};
		bool AllowCancel = true;
		void SetAwaiter()
		{
			state.store(IASet);
			UserAction.starttime = Util::GetTimeStamp();
		}
		void SetEnabled(bool Enable)
		{
			UserAction = {};
			if(!Enable){state.store(IANotSet);}
			else {SetAwaiter(); }
		}
		bool IsSet() { return state.load() == IASet; }
		void TriggerAwaiter()
		{
			Util_Assert(state.load() == IASet, L"InputAwaiter not set");
			if (state.load() == IASet)
			{
				UserAction.endtime = Util::GetTimeStamp();
				state.store(IATriggered);
			}
		}
		bool IsTriggered() { return state.load() == IATriggered; }
		void TriggerInput(InputResultType type);

		InputEvent GetInput();
		
		static InputEvent GetAllInputAsync(std::vector<InputAwaiter*> awaiters)
		{
			auto sharedPromise = std::make_shared<std::promise<InputEvent>>();
			std::atomic<bool> sharedAtomicFlag(false);
			std::vector<std::thread> threads;

			for (auto awaiter : awaiters)
			{
				threads.emplace_back([awaiter, &sharedAtomicFlag, &sharedPromise] {awaiter->GetInputAsync(sharedPromise, &sharedAtomicFlag); });
			}
			InputEvent result = sharedPromise->get_future().get();
			std::lock_guard<std::mutex> lock(mtx);
			Awaiters.erase(std::remove_if(Awaiters.begin(), Awaiters.end(), [&awaiters](const InputAwaiter* a) {return std::find(awaiters.begin(), awaiters.end(), a) != awaiters.end(); }), Awaiters.end());
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


		void SetPrefill(std::wstring prefill)
		{
			UserAction.PrefillInputText = prefill;
		}

		inline static std::vector<InputAwaiter*> Awaiters;

		static std::vector<InputAwaiter*> GetAwaiters()
		{
			std::lock_guard<std::mutex> lock(mtx);
			return std::vector<InputAwaiter*>(Awaiters);
		}
	};

	class KeyboardMouseState
	{
	public:
		static void CaptureInput()
		{
			keysPressed.clear();
			keysDown.clear();
			keysReleased.clear();

			for (int key = 0; key < 256; key++)
			{
				prevKeyStates[key] = keyStates[key];
				keyStates[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
				if (IsControlDown(key) && key > 7) { keysDown.push_back(key); }
				if (IsControlFreshlyPressed(key) && key > 7) { keysPressed.push_back(key); }
				if (IsControlReleased(key) && key > 7) { keysReleased.push_back(key); }
			}
		}

		static bool IsControlFreshlyPressed(int VK_keycode)
		{
			return keyStates[VK_keycode] && !prevKeyStates[VK_keycode];
		}

		static bool IsControlReleased(int VK_keycode)
		{
			return !keyStates[VK_keycode] && prevKeyStates[VK_keycode];
		}

		static bool IsControlDown(int VK_keycode)
		{
			return keyStates[VK_keycode];
		}

		static std::vector<int> GetKeysPressed()
		{
			return keysPressed;
		}

		static std::vector<int> GetKeysDown()
		{
			return keysDown;
		}

		static std::vector<int> GetKeysReleased()
		{
			return keysReleased;
		}

		static void UpdateMouseWheel(int wheelDelta)
		{
			mouseWheelDelta += wheelDelta;
		}

		static void UpdateMouseMove(int x, int y)
		{
			Util::Vector2 newMousePos(static_cast<float>(x), static_cast<float>(y));
			mouseDelta += (newMousePos - mousePos);
			mousePos = newMousePos;
		}

		static void ResetInput()
		{
			keysPressed.clear();
			keysDown.clear();
			keysReleased.clear();
			mouseWheelDelta = 0;
			mouseDelta = Util::Vector2(0);
		}

		static int GetLastMouseWheelDelta()
		{
			return mouseWheelDelta / WHEEL_DELTA;
		}

		static Util::Vector2 GetMousePos()
		{
			return mousePos;
		}

		static Util::Vector2 GetLastMouseDelta()
		{
			return mouseDelta;
		}

	private:
		static inline bool keyStates[256] = { false };
		static inline Util::Vector2 mousePos = Util::Vector2();
		static inline bool prevKeyStates[256] = { false };
		static inline Util::Vector2 mouseDelta = Util::Vector2();
		static inline int mouseWheelDelta = 0;
		static inline std::vector<int> keysPressed; //key that is down in the current frame but was up in the previous frame.Captures freshly pressed keys
		static inline std::vector<int> keysReleased;  //key that was down in the previous frame but is now up.  Captures freshly released keys.
		static inline std::vector<int> keysDown; //Key that is down regardless of previous state.  Captures keys being held down. 
	};



	void UpdateOverlayPosition(HWND hostWindowHandle);
	void CloseOverlay();
	void AttachToWindow(HWND windowHandle);
	void GetControls();

	inline void SetTextProcessingFunction(std::function<std::wstring(const std::wstring&)> func) { g_processTextFunc = func; }
	inline void SetOnStartTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onStartTextEntryFunc = func; }
	inline void SetOnEndTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onEndTextEntryFunc = func; }
	inline void SetOnNextKeyDownFunction(std::function<void()> func) { g_onNextKeyDown = func; }

	void ManualInput(std::wstring InputText);
	void EndManualInput();
	TextInputState ProcessTextInput();
	std::wstring GetInputText();
	void SetInputText(std::wstring text, bool SetCursorToEnd, bool TriggerMessage);
	static LRESULT CALLBACK ProcessInputMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK SubclassedEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}