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
		KEYDOWN,
		KEYRELEASED,
		MOUSEDOWN,
		MOUSERELEASED,
		MOUSEMOVE,
		IDLE,
		MOUSECLICK,
		MOUSEWHEEL,
	};

	inline std::function<std::wstring(const std::wstring&)> g_onStartTextEntryFunc = nullptr;
	inline std::function<std::wstring(const std::wstring&)> g_processTextFunc = nullptr;
	inline std::function<std::wstring(const std::wstring&)> g_onEndTextEntryFunc = nullptr;
	inline std::function<void()> g_onNextKeyDown = nullptr;


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
		int CursorPosition = -1;
		std::optional<IMEState> IMEState;
		std::wstring PrefillInputText;
	};

	TextInputState ProcessTextInput();
	
	inline std::wstring PrefillInputText;
	inline bool InputActive = false;
	inline bool InputTextEnabled() { return InputActive; }
	inline void EnableInputText(std::wstring prefill = L"")
	{
		InputActive = true;
		PrefillInputText = prefill;
	}
	inline void DisableInputText()
	{
		InputActive = false;
		PrefillInputText.clear();
	}


	struct InputEvent
	{
	private:
		std::vector<std::vector<int>> keyCombos;
		bool inputTimedOut = false;
		friend void AlertInputAwaiters(UINT message, WPARAM wParam, LPARAM lParam);
		friend class InputAwaiter;
	public:
		Util::Vector2 mousePos;
		int mouseWheelDelta = 0;
		InputResultType ActionType = InputResultType::NONE;
		int64_t starttime = -1;
		int64_t endtime = 0;
		
		TextInputState TextState;
		
		bool InputTimedOut() { return inputTimedOut; }
		std::wstring GetFinalInputText() 
		{ 
			if (g_onEndTextEntryFunc){return Util::EraseAllSubStr(g_onEndTextEntryFunc(TextState.CurrentInputText), L"\n");}
			else { return Util::EraseAllSubStr(TextState.CurrentInputText, L"\n"); }
		}
		bool InputTextModified() { return !Util::StringsEqual(GetFinalInputText(), PrefillInputText); }
		
		bool ReturnedVKsInclude(int vkCode) { return Util::VectorContains(keyCombos[0], vkCode); }
		bool ReturnedVKIsOnly(int vkCode) {return keyCombos.size() == 1 && keyCombos[0].size() == 1 && keyCombos[0][0] == vkCode;}
		std::vector<int> GetReturnedVKs() { return (keyCombos.size() == 1 ? keyCombos[0] : std::vector<int>{}); }
	};

	inline std::map<int, bool> PreviousControlState;
	inline std::map<int, bool> CurrentControlState;

	


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
	inline const int MaxInputLength = 256;
	inline bool IMEComposing = false;
	inline bool IMECompositionComplete = false;
	inline bool IMEActivated = false;
	inline std::wstring IMECompositionText;



	inline bool SignalCloseWindow = false;



	class InputAwaiter
	{
	private:
		void GetInputAsync(std::shared_ptr<std::promise<InputEvent>> sharedPromise, std::atomic<bool>* sharedTrigger);
		friend InputEvent GetAllInputAsync(std::vector<InputAwaiter*> awaiters);

		void Set() {
			UserAction.ActionType = GetActionType();
			UserAction.keyCombos = ExpectedVK_Codes;
			UserAction.mousePos = Util::Vector2(-1);
			UserAction.mouseWheelDelta = 0;
			UserAction.inputTimedOut = false;
			ConditionMet = false;
			Abort = false;
		}

	public:
		InputAwaiter(int64_t interupt, bool allowCancel, int64_t timeout) : Interupt(interupt), AllowCancel(allowCancel), Timeout(timeout) {};

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
			Enabled = enable;
		}
		void SetPaused(bool pause)
		{
			Paused = pause;
		}

	protected:
		std::function<bool(InputEvent&)> callback = [](InputEvent&) { return true; };
		virtual InputResultType GetActionType() = 0;
		std::vector<std::vector<int>> ExpectedVK_Codes;

		
		bool AllowCancel = true;
		bool Enabled = true;
		bool Paused = false;
		std::exception_ptr Exception = nullptr;
		
		
	public:
		int64_t Timeout;
		int64_t Interupt;
		std::condition_variable cv;
		std::mutex mtx;
		bool ConditionMet = false;
		bool Abort = false;
		InputEvent GetInput();
InputEvent UserAction = {};
	};

	inline std::mutex listMutex;
	inline std::vector<InputAwaiter*> AwaiterList;


	enum class PressType { DOWN, UP };
	template<PressType Type>
	class KeyPressAwaiter : public InputAwaiter {
	public:
		KeyPressAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, std::vector<std::vector<int>> keys) : InputAwaiter(interupt, allowCancel, timeout) { ExpectedVK_Codes = keys; }
		virtual InputResultType GetActionType() override { return Type == PressType::DOWN ? InputResultType::KEYDOWN : InputResultType::KEYRELEASED; }

	};

	enum class ClickType { DOWN, UP };
	template<ClickType Type>
	class MouseClickAwaiter : public InputAwaiter {
	public:
		MouseClickAwaiter(int64_t interupt, bool allowCancel, int64_t timeout, std::vector<std::vector<int>> buttons) : InputAwaiter(interupt, allowCancel, timeout) { ExpectedVK_Codes = buttons; }
		virtual InputResultType GetActionType() override { return Type == ClickType::DOWN ? InputResultType::MOUSEDOWN : InputResultType::MOUSERELEASED; }

	};

	class MouseMoveAwaiter : public InputAwaiter
	{
	public:
		MouseMoveAwaiter(int64_t interupt, bool allowCancel, int64_t timeout) : InputAwaiter(interupt, allowCancel, timeout) { }
		virtual InputResultType GetActionType() override { return InputResultType::MOUSEMOVE; }
	};

	class MouseWheelAwaiter : public InputAwaiter
	{
	public:
		MouseWheelAwaiter(int64_t interupt, bool allowCancel, int64_t timeout) : InputAwaiter(interupt, allowCancel, timeout) {}
		virtual InputResultType GetActionType() override { return InputResultType::MOUSEWHEEL; }
	};

	class IdleAwaiter : public InputAwaiter
	{
	public:
		IdleAwaiter(int64_t interupt, bool allowCancel, int64_t timeout) : InputAwaiter(interupt, allowCancel, timeout) { }
		virtual InputResultType GetActionType() override { return InputResultType::IDLE; }
	};

	inline InputEvent GetAllInputAsync(std::vector<InputAwaiter*> awaiters)
	{
		auto sharedPromise = std::make_shared<std::promise<InputEvent>>();
		std::atomic<bool> sharedAtomicFlag(false);
		std::vector<std::thread> threads;
		SignalCloseWindow = false;
		for (auto awaiter : awaiters)
		{
			threads.emplace_back([awaiter, &sharedAtomicFlag, &sharedPromise] {awaiter->GetInputAsync(sharedPromise, &sharedAtomicFlag); });
		}
		InputEvent result = sharedPromise->get_future().get();
		for (auto awaiter : awaiters)
		{
			awaiter->Abort = true;
			awaiter->cv.notify_one();
		}
		for (auto& t : threads)
		{
			t.join();
		}
		if (SignalCloseWindow)
		{
			throw SignalCloseWindowException();
		}
		for (auto awaiter : awaiters)
		{
			if (awaiter->Exception != nullptr)
			{
				std::rethrow_exception(awaiter->Exception);
			}
		}
		DisableInputText();
		return result;
	}

	class AwaiterGroup
	{
	public:
		AwaiterGroup(std::vector<InputAwaiter*> Awaiters)
		{
			awaiters = Awaiters;
		}
		void EnableAll()
		{
			for (int i = 0; i < awaiters.size(); i++) { awaiters[i]->SetEnabled(true); }
		}
		void DisableAll()
		{
			for (int i = 0; i < awaiters.size(); i++) { awaiters[i]->SetEnabled(false); }
		}
		void PauseAll()
		{
			for (int i = 0; i < awaiters.size(); i++) { awaiters[i]->SetPaused(true); }
		}
		void UnpauseAll()
		{
			for (int i = 0; i < awaiters.size(); i++) { awaiters[i]->SetPaused(false); }
		}
		InputEvent GetAllInputAsync() { return Input::GetAllInputAsync(awaiters); }
	private:
		std::vector<InputAwaiter*> awaiters;
	};

	void UpdateOverlayPosition(HWND hostWindowHandle);
	void DestroyOverlay();
	void AttachToWindow(HWND windowHandle);

	inline void SetTextProcessingFunction(std::function<std::wstring(const std::wstring&)> func) { g_processTextFunc = func; }
	inline void SetOnStartTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onStartTextEntryFunc = func; }
	inline void SetOnEndTypingFunction(std::function<std::wstring(const std::wstring&)> func) { g_onEndTextEntryFunc = func; }
	inline void SetOnNextKeyDownFunction(std::function<void()> func) { g_onNextKeyDown = func; }
	void RequestCloseWindow();
	std::wstring GetInputText();
	int GetCursorPosition();
	void SetInputText(std::wstring text, bool SetCursorToEnd, bool TriggerMessage);
	static LRESULT CALLBACK ProcessInputMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK SubclassedEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void AlertInputAwaiters(UINT message, WPARAM wParam, LPARAM lParam);
}	
