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
	int inputBoxID = -1;
	int ItemSelection = -1;
	bool TextSubmitted = false;


	IdleAwaiter BackgroundEffects = IdleAwaiter(0, false, 0);
	MouseClickAwaiter MouseClick = MouseClickAwaiter<ClickType::DOWN>(0, false, 0, { {VK_LBUTTON} });
	KeyPressAwaiter KeyPress = KeyPressAwaiter<PressType::DOWN>(0, false, 0, { {VK_LEFT}, {VK_RIGHT},{VK_RETURN},{VK_ESCAPE} });
	BackgroundEffects.SetCallback([&](InputEvent& event) {
		girlBox.RunPhysicsStep(world);
		Interface::SetControlCenter(Image1, girlBox.GetPosition());
		Interface::RotateControl(Image1, girlBox.GetOrientation());
		int index = Interface::GetMouseHoverIndex(event.mousePos);

		if (index != -1)
		{
			Interface::SetGUISelection(index);
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(index); }
		}
		else
		{
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); }
			else { Interface::SetGUISelection(-1); }
		}
		G1->SetCursorSelectableHovered(index != -1);  //Change cursor to hand if over selectable item otherwise arrow.

		if (Input::InputTextEnabled())
		{
			Interface::BeginChainRequests();
			if (!Interface::Control(inputBoxID)) { inputBoxID = Interface::CreateInputBox(Util::RECTF(.35f, .5f, .65f, .6f), WHITE, BLACK, true, true, true); }
			Interface::UpdateInputBox(inputBoxID, event.TextState.CurrentInputText, event.TextState.CursorPosition, event.TextState.IMEState.IMECompositionText, event.TextState.IMEState.IMECursorPos, event.TextState.IMEState.guiCandidateTexts, event.TextState.IMEState.SelectedCandidate);
			Interface::EndChainRequests();
		}
		return false;
		});

	MouseClick.SetCallback([&](InputEvent& event) {
		ItemSelection = Interface::GetMouseHoverIndex(event.mousePos);
		if(ItemSelection == ActivateTypingSelectionId)
		{
			this->EnableMaintainSelectionAfterMouseHover(false);
			Interface::RemoveControl(ActivateTypingID);
			Input::EnableInputText(L"Text");
			return false;
		}
		if (ItemSelection == -1)
		{ 
			Interface::RemoveControl(inputBoxID);
			ActivateTypingID = Interface::WriteFreeText(L"Click To Type!", BLUE, .5f, .5f, true, ActivateTypingSelectionId, 0, GREEN);
			Input::DisableInputText();
			return false; 
		}
		return true;
		});

	KeyPress.SetCallback([&](InputEvent& event) {
		if (!Input::InputTextEnabled())
		{
			if (event.ReturnedVKIsOnly(VK_LEFT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(-1, false)); }
			if (event.ReturnedVKIsOnly(VK_RIGHT)) { Interface::SetGUISelection(Interface::RotateSelectableMenuItems(1, false)); }
			if (event.ReturnedVKIsOnly(VK_RETURN))
			{
				ItemSelection = Interface::GetCurrentSelectableMenuItem();
				if (ItemSelection != -1) { return true; }
			}
			return false;
		}
		else
		{
			if (event.ReturnedVKIsOnly(VK_RETURN))
			{
				TextSubmitted = true;
				return true;
			}
			if (event.ReturnedVKIsOnly(VK_ESCAPE))
			{
				Interface::RemoveControl(inputBoxID);
				ActivateTypingID = Interface::WriteFreeText(L"Click To Type!", BLUE, .5f, .5f, true, ActivateTypingSelectionId, 0, GREEN);
				Input::DisableInputText();
				return false;
			}
		}
		});

	try
	{
		InputEvent UserResponse = GetAllInputAsync({ &BackgroundEffects, &MouseClick, &KeyPress });
		if (ItemSelection == Option1SelectionID)
		{
			SetMenuResult(L"Selected 1");
		}
		else if (ItemSelection == Option2SelectionID)
		{
			SetMenuResult(L"Selected 2");
		}
		else if (ItemSelection == GirlSelectionId)
		{
			SetMenuResult(L"Girl Selected");
		}
		else if (TextSubmitted)
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