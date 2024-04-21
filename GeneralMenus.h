#pragma once

#include "Menus.h"

#include "GameWindow2D.h"
#include "GraphicsInterface.h"
#include "Input.h"

using namespace Input;
using namespace GraphicsInterface;


template<typename ResultType = NoReturnValue>
class TextSelectMenu : public Menu<ResultType>
{
public:
	TextSelectMenu(bool hideBelow = false, bool disableBelow = true)
		: Menu<ResultType>(hideBelow, disableBelow) {}

protected:
	virtual void ShowManagedMenu() = 0;
	virtual void Show() override
	{
		try {
			ShowManagedMenu();
		}
		catch (const SignalCloseWindowException&)
		{
			throw CloseWindowException();
		}

	}

	virtual int GetGUISelection(bool AllowCancel)
	{
		int Selected = -1;
		MouseMoveAwaiter a1(0, false, 0);
		a1.SetCallback([this](InputEvent& event) {
			int Hovered = Interface::GetMouseHoverIndex(event.mousePos);
			if (Hovered != -1)
			{
				Interface::SetGUISelection(Hovered);
				if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(Hovered); }
			}
			else { Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); }
			G1->SetCursorSelectableHovered(Hovered != -1);  //Change cursor to hand if over selectable item otherwise arrow.
			return false;
			});

		MouseClickAwaiter a2 = MouseClickAwaiter<ClickType::UP>(0, false, 0, { {VK_LBUTTON} });
		a2.SetCallback([this, &Selected](InputEvent& event) {
			Selected = Interface::GetMouseHoverIndex(event.mousePos);
			if (Selected != -1) { return true; }
			return false;
			});
		KeyPressAwaiter KeyAwaiter = KeyPressAwaiter<PressType::UP>(0, false, 0, { {VK_UP},{ VK_LEFT },{VK_DOWN},{ VK_RIGHT },{VK_RETURN},{VK_ESCAPE} });
		KeyAwaiter.SetCallback([this, &Selected](InputEvent& event) {
			if (event.ReturnedVKIsOnly(VK_LEFT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(-1, false)); }
			if (event.ReturnedVKIsOnly(VK_RIGHT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(1, false)); }
			if (event.ReturnedVKIsOnly(VK_UP)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(-1, false)); }
			if (event.ReturnedVKIsOnly(VK_LEFT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(1, false)); }
			if (event.ReturnedVKIsOnly(VK_RETURN))
			{
				Selected = Interface::GetCurrentSelectableMenuItem();
				return true;
			}
			if (event.ReturnedVKIsOnly(VK_ESCAPE))
			{
				Selected = -1;
				return true;
			}
			return false;
			});
		GetAllInputAsync({ &a1, &a2,&KeyAwaiter });
		return Selected;
	}

};

class LoadingMenu : public TextSelectMenu<>
{
public:
	LoadingMenu(const std::wstring& prompt) : m_prompt(prompt) {}
	void TriggerLoadingComplete()
	{
		isLoaded = true;
		Util_Assert(displayThread.joinable(), L"Thread management error");
		displayThread.join();
	}
	void UpdateLoadingText(std::wstring text)
	{
		m_prompt = text;
	}
	void DisplayAsync()
	{
		displayThread = std::thread([this]() {this->Display(); });
	}
private:
	void ShowManagedMenu() override
	{
		Util_Assert(displayThread.get_id() != std::thread::id(), L"ShowManagedMenu must be called with DisplayAsync");
		Interface::CreateBox(0, 0, 1, 1, D2D1::ColorF(0, 0, 0, .85f));
		int LoadingTextID = Interface::WriteFreeText(m_prompt, WHITE, .5f, .5f, true);

		int numPromptLines = static_cast<int>(std::count(m_prompt.begin(), m_prompt.end(), L'\n') + 1);

		Util::RECTF rect = GetStringRectN(m_prompt, fontHeight, 0, Util::Vector2(0, 0), false);
		float promptWidth = rect.GetSize().X;

		float textBoxStartX = min(.275f, .5f - promptWidth / 2.0f);
		float textBoxEndX = max(.725f, .5f + promptWidth / 2.0f);

		float textBoxEndPosition = .65f + numPromptLines * LineHeight;
		std::wstring originalvalue = m_prompt;
		Interface::CreateBox(textBoxStartX - .05f, .475f, textBoxEndX + .05f, textBoxEndPosition, D2D1::ColorF(0.5f, 0.5f, 0.5f, .5f));
		IdleAwaiter idleAwaiter = IdleAwaiter(0, false, 0);
		idleAwaiter.SetCallback([this, &originalvalue, &LoadingTextID](InputEvent& event) {
			int Selected = Interface::GetMouseHoverIndex(event.mousePos);
			if (Selected != -1)
			{
				Interface::SetGUISelection(Selected);
				if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(Selected); }
			}
			else
			{
				if (this->GetMaintainSelectionAfterMouseHover()) { Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); }
				else { Interface::SetGUISelection(-1); }
			}
			if (this->m_prompt != originalvalue)
			{
				Interface::RemoveControl(LoadingTextID);
				LoadingTextID = Interface::WriteFreeText(this->m_prompt, WHITE, .5f, .5f, true);
				originalvalue = this->m_prompt;
			}
			return isLoaded.load();
			});

		GetAllInputAsync({ &idleAwaiter });

	}



	std::wstring m_prompt;
	std::atomic<bool> isLoaded = false;
	std::thread displayThread;
};


class ConfirmationMenu : public TextSelectMenu<int>
{
public:
	ConfirmationMenu(const std::vector<std::wstring>& options, const std::wstring& prompt, int defaultOption = 0)
		: TextSelectMenu(false, true), options_(options), prompt_(prompt), defaultOption_(defaultOption)
	{}

private:
	void ShowManagedMenu() override
	{
		Interface::CreateBox(0, 0, 1, 1, D2D1::ColorF(0, 0, 0, .85f));
		Interface::WriteFreeText(prompt_, WHITE, .5f, .5f, true);
		int numPromptLines = static_cast<int>(std::count(prompt_.begin(), prompt_.end(), L'\n')) + 1;

		float optionsStartPosition = .575f + numPromptLines * LineHeight;

		Util::RECTF rect = GetStringRectN(prompt_, fontHeight, 0, Util::Vector2(0, 0), false);
		float promptWidth = rect.GetSize().X;

		float textBoxStartX = min(.275f, .5f - promptWidth / 2.0f);
		float textBoxEndX = max(.725f, .5f + promptWidth / 2.0f);

		if (options_.size() == 1)
		{
			Interface::WriteFreeText(options_[0], BLUE, .5f, optionsStartPosition, true, 0, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
		}
		else if (options_.size() == 2)
		{
			Interface::WriteFreeText(options_[0], BLUE, .425f, optionsStartPosition, true, 0, 0, GREEN);
			Interface::WriteFreeText(options_[1], BLUE, .575f, optionsStartPosition, true, 1, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
		}
		else if (options_.size() == 3)
		{
			Interface::WriteFreeText(options_[0], BLUE, .40f, optionsStartPosition, true, 0, 0, GREEN);
			Interface::WriteFreeText(options_[1], BLUE, .50f, optionsStartPosition, true, 1, 0, GREEN);
			Interface::WriteFreeText(options_[2], BLUE, .60f, optionsStartPosition, true, 2, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
			Interface::AddSelectableMenuItem(2);

		}
		else if (options_.size() == 4)
		{
			Interface::WriteFreeText(options_[0], BLUE, .4f, optionsStartPosition, true, 0, 0, GREEN);
			Interface::WriteFreeText(options_[1], BLUE, .6f, optionsStartPosition, true, 1, 0, GREEN);
			Interface::WriteFreeText(options_[2], BLUE, .4f, optionsStartPosition + LineHeight, true, 2, 0, GREEN);
			Interface::WriteFreeText(options_[3], BLUE, .6f, optionsStartPosition + LineHeight, true, 3, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
			Interface::AddSelectableMenuItem(2);
			Interface::AddSelectableMenuItem(3);

		}
		else
		{
			Util_NotImplemented();
		}

		float textBoxEndPosition = .65f + numPromptLines * LineHeight;
		if (options_.size() > 3) { textBoxEndPosition += LineHeight; }
		Interface::CreateBox(textBoxStartX - .05f, .475f, textBoxEndX + .05f, textBoxEndPosition, D2D1::ColorF(0.5f, 0.5f, 0.5f, .5f));
		Interface::RotateSelectableMenuItemsToValue(defaultOption_);
		Interface::SetGUISelection(defaultOption_);
		int InputResult = GetGUISelection(true);
		SetMenuResult(InputResult);
	}

private:
	std::vector<std::wstring> options_;
	std::wstring prompt_;
	int defaultOption_;
};

template<typename ResultType = NoReturnValue>
class QuickMenu : public TextSelectMenu<ResultType>
{
public:
	using MenuContentFunction = std::function<ResultType(QuickMenu<ResultType>&)>;
	QuickMenu(bool hideBelow = false, bool disableBelow = true, MenuContentFunction menuContent = nullptr)
		: TextSelectMenu<ResultType>(hideBelow, disableBelow)
	{
		if (menuContent != nullptr)
		{
			this->menuContent_ = menuContent;
		}
	}

protected:
	virtual void ShowManagedMenu() override
	{
		if (menuContent_)
		{
			ResultType result = menuContent_(*this);
			this->SetMenuResult(result);
		}
	}

private:
	MenuContentFunction menuContent_;
};




class TextInputMenu : public TextSelectMenu<std::wstring>
{
public:
	TextInputMenu(const std::wstring& prompt, std::wstring prefill = L"", std::wstring confirm = L"Submit", std::wstring cancel = L"Cancel")
		: TextSelectMenu(false, true), Prompt(prompt), Prefill(prefill), ConfirmLabel(confirm), CancelLabel(cancel)
	{
		EnableMaintainSelectionAfterMouseHover(false);
	}

private:
	void ShowManagedMenu() override
	{
		Interface::CreateBox(0, 0, 1, 1, D2D1::ColorF(0, 0, 0, .85f));
		Interface::WriteFreeText(Prompt, WHITE, .5f, .45f, true);
		int numPromptLines = static_cast<int>(std::count(Prompt.begin(), Prompt.end(), L'\n')) + 1;

		float optionsStartPosition = .655f + numPromptLines * LineHeight;
		Util::RECTF rect = GetStringRectN(Prompt, fontHeight, 0, Util::Vector2(0, 0), false);
		float promptWidth = rect.GetSize().X;

		float textBoxStartX = min(.275f, .5f - promptWidth / 2.0f);
		float textBoxEndX = max(.725f, .5f + promptWidth / 2.0f);
		float textBoxEndPosition = .75f + numPromptLines * LineHeight;

		Interface::CreateBox(textBoxStartX - .05f, .375f, textBoxEndX + .05f, textBoxEndPosition, D2D1::ColorF(0.5f, 0.5f, 0.5f, .5f));

		int ConfirmValue = 0;
		int CancelValue = 1;
		Interface::WriteFreeText(ConfirmLabel, BLUE, .425f, optionsStartPosition, true, ConfirmValue, 0, GREEN);
		Interface::WriteFreeText(CancelLabel, BLUE, .575f, optionsStartPosition, true, CancelValue, 0, GREEN);

		Interface::AddSelectableMenuItem(0);
		Interface::AddSelectableMenuItem(1);
		Interface::SetGUISelection(0);


		MouseMoveAwaiter a1 = MouseMoveAwaiter(0, false, 0);
		a1.SetCallback([this](InputEvent& event) {
			int Hovered = Interface::GetMouseHoverIndex(event.mousePos);
			if (Hovered != -1)
			{
				Interface::SetGUISelection(Hovered);
				if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(Hovered); }
			}
			else { Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); }
			return false;
			});

		MouseClickAwaiter a2 = MouseClickAwaiter<ClickType::DOWN>(0, false, 0, { {VK_LBUTTON} });
		a2.SetCallback([&](InputEvent& event) {
			int Selected = Interface::GetMouseHoverIndex(event.mousePos);
			if (Selected == CancelValue)
			{
				SetMenuResult(Prefill);
				return true;
			}
			if (Selected == ConfirmValue)
			{
				SetMenuResult(Util::Trim(event.GetFinalInputText()));
				return true;
			}
			return false;
			});

		int inputBoxID = Interface::CreateInputBox(Util::RECTF(.35f, .5f, .65f, .6f), WHITE, BLACK, true, true, true);
		Input::EnableInputText(Prefill);
		KeyPressAwaiter KeyAwaiter = KeyPressAwaiter<PressType::UP>(0, true, 0, { {VK_RETURN},{VK_ESCAPE} });
		KeyAwaiter.SetCallback([&](InputEvent& event) {

			if (IMEComposing || IMEActivated) { return false; }

			if (event.ReturnedVKIsOnly(VK_ESCAPE))
			{
				SetMenuResult(Prefill);
			}
			else
			{
				std::wstring result = event.GetFinalInputText();
				if (g_onEndTextEntryFunc) { result = g_onEndTextEntryFunc(result); }
				SetMenuResult(Util::Trim(result));
			}
			return true;
			});

		IdleAwaiter Idle = IdleAwaiter(0, false, 0);
		Idle.SetCallback([this, &Idle, &inputBoxID](InputEvent& event)
			{
				Interface::UpdateInputBox(inputBoxID, event.TextState.CurrentInputText, event.TextState.CursorPosition, event.TextState.IMEState.IMECompositionText, event.TextState.IMEState.IMECursorPos, event.TextState.IMEState.guiCandidateTexts, event.TextState.IMEState.SelectedCandidate);
				return false;
			});
		GetAllInputAsync({ &a1,&a2,&KeyAwaiter,&Idle });
		Interface::RemoveControl(inputBoxID);
	}

private:
	std::vector<std::wstring> options_;
	std::wstring Prompt;
	std::wstring Prefill;
	std::wstring ConfirmLabel;
	std::wstring CancelLabel;
};

class SelectionMenu : public TextSelectMenu<int> {
private:
	std::vector<std::wstring> options;
	std::wstring prompt;
	float PromptAlign;
	float PromptBoxRightAlign;
	float OptionAlign;
	std::wstring EscOption;
	int defaultOption = -1;

public:
	// Constructor
	SelectionMenu(const std::vector<std::wstring>& options,
		const std::wstring& prompt,
		float PromptAlign = .5,
		float PromptBoxRightAlign = 1,
		float OptionAlign = 0,
		std::wstring EscOption = L"",
		int defaultOption = 0,
		bool hideBelow = false,
		bool disableBelow = true)
		: TextSelectMenu(hideBelow, disableBelow),
		options(options),
		prompt(prompt),
		PromptAlign(PromptAlign),
		PromptBoxRightAlign(PromptBoxRightAlign),
		OptionAlign(OptionAlign),
		EscOption(EscOption),
		defaultOption(defaultOption)
	{}

private:
	void ShowManagedMenu() override
	{
		int numOptions = static_cast<int>(options.size());
		Util_Assert(numOptions > 0, L"Error: There must be at least 1 option");
		std::wstring promptText = prompt;
		Interface::WriteFreeText(promptText, WHITE, PromptAlign, .05f);

		float startingPosition = 1 - 0.1f;
		float VerticalSpacing = .075f;
		int skip = 0;
		for (int i = 0; i < numOptions; i++)
		{
			if (options[numOptions - i - 1].empty())
			{
				skip++;
				continue;
			}
			float verticalPosition = startingPosition - ((i - skip) * VerticalSpacing);
			if (options[numOptions - i - 1].find(L"[*]") == 0)
			{
				Interface::WriteFreeText(options[numOptions - i - 1].substr(3), BLUE, OptionAlign, verticalPosition, (OptionAlign != 0), -1, 0, GREEN);
			}
			else
			{
				Interface::WriteFreeText(options[numOptions - i - 1], BLUE, OptionAlign, verticalPosition, (OptionAlign != 0), numOptions - i - 1, 0, GREEN);
				Interface::AddSelectableMenuItem(numOptions - i - 1);
			}
		}
		if (!EscOption.empty()) { Interface::WriteFreeText(EscOption, BLUE, .45f, .9f, true, numOptions, 0, GREEN); }

		numOptions = static_cast<int>(std::count_if(options.begin(), options.end(), [](const std::wstring& str) { return !str.empty(); }));

		float left = 0, top = 0, right = PromptBoxRightAlign, bottom;
		int numPromptLines = static_cast<int>(std::count(promptText.begin(), promptText.end(), L'\n') + 1);
		bottom = (numPromptLines * LineHeight) + (.5f * LineHeight);
		if (!promptText.empty()) { Interface::CreateBox(left, top, right, bottom, TextBoxColor); }

		left = 0; 
		top = max(1 - ((((numOptions)*VerticalSpacing) + VerticalSpacing * .4f) + .05f), 0);
		right = OptionAlign == 0 ? OptionAlign + 0.15f : 1;
		bottom = 1; 

		Interface::CreateBox(left, top, right, bottom, TextBoxColor);
		int InputResult = GetGUISelection(true);
		SetMenuResult(InputResult);
	}
};



class ConsoleMultiPromptMenu : public TextSelectMenu<std::vector<int>>
{
public:
	ConsoleMultiPromptMenu(std::vector<OptionStruct>& options, const std::wstring& mainPrompt, bool hideBelow = false, bool disableBelow = true) : TextSelectMenu(hideBelow, disableBelow), options_(options), mainPrompt_(mainPrompt)
	{

	}

private:
	void ShowManagedMenu() override
	{
		std::vector<int> IDList;
		Interface::WriteFreeText(mainPrompt_, WHITE, .5f, .05f, true);
		float VerticalSpacing = .075f;

		for (int i = 0; i < static_cast<int>(options_.size()); i++)
		{
			Interface::WriteFreeText(options_[i].Prompt, WHITE, .25f, .15f + i * VerticalSpacing, true, -1, 0, GREEN);
			IDList.push_back(Interface::WriteFreeText(options_[i].optionnames[options_[i].OriginalSelection], BLUE, .75f, .15f + i * VerticalSpacing, true, i, 1, GREEN));
			Interface::AddSelectableMenuItem(i);
			optionSelections_.push_back(options_[i].OriginalSelection);
		}

		Interface::WriteFreeText(L"Go Back", BLUE, .5f, .15f + options_.size() * VerticalSpacing, true, static_cast<int>(options_.size()), 0, GREEN);
		Interface::AddSelectableMenuItem(static_cast<int>(options_.size()));
		Interface::CreateBox(0, .05f, 1, .15f + (options_.size() + 1) * VerticalSpacing, TextBoxColor);


		int InputResult = GetGUISelection(true);
		if (InputResult < options_.size())
		{
			optionSelections_[InputResult] = ConfirmationMenu(options_[InputResult].optionnames, options_[InputResult].Click, options_[InputResult].OriginalSelection).Display().Result;
			SetMenuResult(optionSelections_);
		}
		if (InputResult == options_.size())
		{
			SetMenuResult({});
		}
	}


private:
	std::vector<OptionStruct>& options_;
	const std::wstring& mainPrompt_;
	std::vector<int> optionSelections_;
};

class MainMenu : public TextSelectMenu<>
{
public:
	MainMenu()
	{}

private:
	void ShowManagedMenu() override;


};

class SettingsMenu : public TextSelectMenu<>
{
public:
	SettingsMenu() {}

private:
	void ShowManagedMenu() override;
};

class ChooseChapterMenu : public TextSelectMenu<>
{
private:
	void ShowManagedMenu() override;

public:
	ChooseChapterMenu(int preselect = -1) : TextSelectMenu(true, true), Preselect(preselect)
	{
		EnableMaintainSelectionAfterMouseHover(false);
	}
	int Preselect;
};

class SpeechDialogMenu : public TextSelectMenu<int>
{
public:
	SpeechDialogMenu(std::wstring& characterName, D2D1::ColorF& characterColor, std::wstring text, std::vector<std::wstring> responses, std::wstring ImagePath, Util::Vector2 position, float size, bool HideBelow = true)
		: TextSelectMenu<int>(HideBelow, true), character(characterName), character_color(characterColor), text(text), responses(responses), ImagePath(ImagePath), position(position), size(size)
	{

	}


private:
	void ShowManagedMenu() override
	{
		if (!ImagePath.empty())
		{
			Interface::AddGUIImage(ImagePath, position, size, -1);
			if (Util::StringContains(ImagePath, L"0.png"))
			{
				std::wstring desc = Util::ReplaceAllSubStr(Util::GetFileName(ImagePath, false), L"0", L"");
				desc = Util::ReplaceAllSubStr(desc, L",", L"\n");
				Interface::WriteFreeText(desc, WHITE, position.X, position.Y, true, -1, -4, 0, false);
			}
		}

		Interface::CreateBox(0, .8, 1, 1, TextBoxColor);
		float YPos = .8;
		Interface::WriteFreeText(character, character_color, .5, YPos);
		YPos += LineHeight;
		Interface::WriteFreeText(text, WHITE, .5, YPos);
		YPos += LineHeight * 2;
		if (responses.size() == 0)
		{
			Interface::WriteFreeText(L"[...]", BLUE, .5, YPos, true, 0, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
		}
		else if (responses.size() == 1)
		{
			Interface::WriteFreeText(responses[0], BLUE, .5, YPos, true, 0, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
		}
		else if (responses.size() == 2)
		{
			Interface::WriteFreeText(responses[0], BLUE, .5 / 3.0, YPos, true, 0, 0, GREEN);
			Interface::WriteFreeText(responses[1], BLUE, 2.5 / 3.0, YPos, true, 1, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
		}
		else if (responses.size() == 3)
		{
			Interface::WriteFreeText(responses[0], BLUE, .5 / 3.0, YPos, true, 0, 0, GREEN);
			Interface::WriteFreeText(responses[1], BLUE, 2.5 / 3.0, YPos, true, 1, 0, GREEN);
			Interface::WriteFreeText(responses[2], BLUE, .5, YPos, true, 2, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
			Interface::AddSelectableMenuItem(2);
		}
		else if (responses.size() == 4)
		{
			Interface::WriteFreeText(responses[0], BLUE, .5 / 3.0, YPos, true, 0, 0, GREEN);
			Interface::WriteFreeText(responses[1], BLUE, 2.5 / 3.0, YPos, true, 1, 0, GREEN);
			YPos += LineHeight;
			Interface::WriteFreeText(responses[2], BLUE, .5 / 3.0, YPos, true, 2, 0, GREEN);
			Interface::WriteFreeText(responses[3], BLUE, 2.5 / 3.0, YPos, true, 3, 0, GREEN);
			Interface::AddSelectableMenuItem(0);
			Interface::AddSelectableMenuItem(1);
			Interface::AddSelectableMenuItem(2);
			Interface::AddSelectableMenuItem(3);
		}
		else { Util_NotImplemented(); }
		SetMenuResult(GetGUISelection(true));
	}

private:
	std::wstring character;
	D2D1::ColorF& character_color;
	std::wstring text;
	std::vector<std::wstring> responses;
	std::wstring ImagePath;
	Util::Vector2 position;
	float size;
};
