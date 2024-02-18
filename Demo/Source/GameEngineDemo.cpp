#include "GameEngineDemo.h"
#include "Physics2D.h"

using namespace Input;


void DemoMenu::Show()
{
	Interface::LoadBackgroundImage(L"Background.png");
	Interface::CreateBox(.15f, .25f, .85f, .75f, TextBoxColor);
	Interface::WriteFreeText(Prompt, WHITE, .5f, .3f, true, -1, 30);
	Interface::WriteFreeText(L"Select an item with either the mouse or the left/right arrow keys and enter.", YELLOW, .5f, .4f, true, -1, 10);

	int Option1SelectionID = 0;
	Interface::WriteFreeText(L"Selectable 1", BLUE, .4f, .6f, true, Option1SelectionID, 0, GREEN);
	Interface::AddSelectableMenuItem(Option1SelectionID);
	int Option2SelectionID = 1;
	Interface::WriteFreeText(L"Selectable 2", BLUE, .6f, .6f, true, Option2SelectionID, 0, GREEN);
	Interface::AddSelectableMenuItem(Option2SelectionID);
	Interface::RotateSelectableMenuItemsToValue(Option2SelectionID);
	Interface::SetGUISelection(Option2SelectionID);


	int GirlSelectionId = 5;
	int Image1 = Interface::AddGUIImage(L"girl.png", Util::Vector2(0.5f, 0.5f), .5f, GirlSelectionId);
	Util::RECTF physicsRect = Interface::GetPhsyicsRect(Image1).Resize(.9f, true);
	GameWorld world = GameWorld(RenderBuffer::BufferSize.X, RenderBuffer::BufferSize.Y);
	PhysicsBox girlBox = PhysicsBox(physicsRect, Util::Vector2(0.0025f, 0.0025f), 0, .5f);

	int ActivateTypingSelectionId = 6;
	int ActivateTypingID = Interface::WriteFreeText(L"Click To Type!", BLUE, .5, .5, true, ActivateTypingSelectionId, 0, GREEN);

	IdleAwaiter a0 = IdleAwaiter(0, false, 0);
	MouseClickAwaiter a2 = MouseClickAwaiter<ClickType::DOWN>(0, false, 0, { {VK_LBUTTON} });
	KeyPressAwaiter KeyAwaiter = KeyPressAwaiter<PressType::DOWN>(0, false, 0, { {VK_LEFT}, {VK_RIGHT},{VK_RETURN} });
	TextEntryAwaiter Text = TextEntryAwaiter(0, true, 0);

	a0.SetCallback([this, Image1, &world, &girlBox](InputEvent& event) {
		girlBox.RunPhysicsStep(world);
		Interface::SetControlCenter(Image1, girlBox.GetPosition());
		Interface::RotateControl(Image1, girlBox.GetOrientation());
		event.SelectedIndex = Interface::GetMouseHoverIndex(event.MousePos);
		if (event.SelectedIndex != -1)
		{
			Interface::SetGUISelection(event.SelectedIndex);
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(event.SelectedIndex); }
		}
		else
		{
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); }
			else { Interface::SetGUISelection(-1); }
		}
		G1->SetCursorSelectableHovered(event.SelectedIndex != -1);  //Change cursor to hand if over selectable item otherwise arrow.
		return false;
		});

	a2.SetCallback([this,&KeyAwaiter,ActivateTypingID,ActivateTypingSelectionId,&Text](InputEvent& event) {
		event.SelectedIndex = Interface::GetMouseHoverIndex(event.MousePos);
		if (event.SelectedIndex == -1) { return false; }
		if(event.SelectedIndex == ActivateTypingSelectionId)
		{
			this->EnableMaintainSelectionAfterMouseHover(false);
			KeyAwaiter.SetEnabled(false);
			Text.SetEnabled(true);
			return false;
		}
		return true;
		});

	KeyAwaiter.SetCallback([this](InputEvent& event) {
		if (Util::VectorContains(event.vk_codes, VK_LEFT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(-1, false)); }
		if (Util::VectorContains(event.vk_codes, VK_RIGHT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(1, false)); }
		if (Util::VectorContains(event.vk_codes, VK_RETURN))
		{
			event.SelectedIndex = Interface::GetCurrentSelectableMenuItem();
			if (event.SelectedIndex != -1) { return true; }
		}
		return false;
		});

	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	//TO DO: IMPROVE PERFORMANCE OF INPUT BOX RENDERING.  IME FLICKERS. 
	//	
	//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	Text.SetPrefill(L"Text");
	int inputBoxID = -1;
	Text.SetCallback([&Text,&KeyAwaiter, &inputBoxID,&ActivateTypingID, ActivateTypingSelectionId](InputEvent& event) {
		if (event.ActionType == InputResultType::TEXTSUBMIT) { return true; }
		if (event.ActionType == InputResultType::INPUTCANCEL)
		{
			Interface::RemoveControl(inputBoxID);
			ActivateTypingID = Interface::WriteFreeText(L"Click To Type!", BLUE, .5f, .5f, true, ActivateTypingSelectionId, 0, GREEN);
			KeyAwaiter.SetEnabled(true);
			Text.SetEnabled(false);
			return false;
		}
		Interface::RemoveControl(ActivateTypingID);
		if(!Interface::Control(inputBoxID)){inputBoxID = Interface::CreateInputBox(Util::RECTF(.35f, .5f, .65f, .6f), WHITE, BLACK, true, true, true);}
		Interface::UpdateInputBox(inputBoxID, event.GetTextState().CurrentInputText, event.GetTextState().IMEState.has_value() ? -1 : event.GetTextState().CursorPosition);
		if (event.GetTextState().IMEState.has_value()) { Interface::AddIMEOverlay(inputBoxID, event.GetTextState().IMEState->IMECompositionText, event.GetTextState().IMEState->IMECursorPos, event.GetTextState().IMEState->guiCandidateTexts, event.GetTextState().IMEState->SelectedCandidate); }
		return false;
		});
	Text.SetEnabled(false);
	try
	{
		InputEvent UserResponse = GetAllInputAsync({ &a0, &a2,&KeyAwaiter, &Text });
		if (UserResponse.SelectedIndex == Option1SelectionID)
		{
			SetMenuResult(L"Selected 1");
		}
		else if (UserResponse.SelectedIndex == Option2SelectionID)
		{
			SetMenuResult(L"Selected 2");
		}
		else if (UserResponse.SelectedIndex == GirlSelectionId)
		{
			SetMenuResult(L"Girl Selected");
		}
		else if (UserResponse.ActionType == InputResultType::TEXTSUBMIT)
		{
			SetMenuResult(L"Entered Text: " + UserResponse.GetFinalInputText());
		}
		return;
	}
	catch (const Input::SignalCloseWindowException&)
	{
		throw CloseWindowException();
	}
}

void Update(double deltaTime)
{
	Util::VariableMonitor::PrintMonitorValues(G1->m_hwnd);
}

void Load()
{
	AttachToWindow(G1->m_hwnd);
	Util::VariableMonitor::AddValueMonitor(L"fps", &G1->FrameRate);
}

void Resize(int width, int height)
{
	CreateBuffers(G1->m_hwnd);
	UpdateOverlayPosition(G1->m_hwnd);
}

void Render()
{
	GraphicsState* g1 = Interface::CreateFrame();
	RenderFrame(g1);
	delete g1;
}

bool Close()
{
	Input::RequestCloseWindow();
	return false;
}

int main()
{
	std::wcout << L"Welcome to 2D Game Engine Demo" << std::endl;
	std::wcout << L"Creating Game Window" << std::endl;
	G1 = new GameWindow2D(.5, .5);
	G1->onLoad = []() { Load(); };
	G1->onResize = [](int width, int height) { Resize(width, height); };
	G1->onUpdateFrame = [](double deltaTime) { Update(deltaTime); };
	G1->onRenderFrame = []() { Render(); };
	G1->onClose = []() {return Close(); };
	std::wcout << L"Initializing game window at 60fps" << std::endl;
	G1->Run(60, false);
	std::wcout << L"Starting graphics rendering engine" << std::endl;
	Direct2DStartup(G1->m_hwnd);
	std::wcout << L"Opening window" << std::endl;
	G1->ShowWindow(true);
	std::wcout << L"Game window now open and rendering..." << std::endl;
	DemoMenu m1 = DemoMenu(L"2D Game Engine Demo");
	try
	{
		m1.Display();
		std::wcout << m1.Result() << std::endl;
	}
	catch (const CloseWindowException&)
	{
		std::wcout << L"Window closed by user (X button clicked)" << std::endl;
	}
	if (G1->WindowOpen)
	{
		std::wcout << L"Closing game window" << std::endl;
		G1->CloseWindow();
	}
	std::wcout << L"Shutting down rendering engine" << std::endl;
	Direct2DShutdown();
	std::wcout << L"2D Game Engine Demo has completed" << std::endl;
	std::cin.get();
	return 0;
}