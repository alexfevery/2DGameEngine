#include "GameEngineDemo.h"
#include "Physics2D.h"

using namespace Input;


void DemoMenu::Show()
{
	Interface::LoadBackgroundImage(L"Background.png");
	Interface::CreateBox(.15f, .25f, .85f, .75f, TextBoxColor);
	Interface::WriteFreeText(Prompt, WHITE, .5f, .3f, true, -1, 30);
	Interface::WriteFreeText(L"Select an item with either the mouse or the left/right arrow keys and enter", YELLOW, .5f, .4f, true, -1, 10);

	int Option1SelectionID = 0;
	Interface::WriteFreeText(L"Option #1", BLUE, .4f, .6f, true, Option1SelectionID, 0, GREEN);
	Interface::AddSelectableMenuItem(Option1SelectionID);
	int Option2SelectionID = 1;
	Interface::WriteFreeText(L"Exit", BLUE, .6f, .6f, true, Option2SelectionID, 0, GREEN);
	Interface::AddSelectableMenuItem(Option2SelectionID);
	Interface::RotateSelectableMenuItemsToValue(Option2SelectionID);
	Interface::SetGUISelection(Option2SelectionID);

	
	int GirlSelectionId = 5;
	int Image1 = Interface::AddGUIImage(L"girl.png", Util::Vector2(0.5f, 0.5f), .5f, GirlSelectionId);
	Util::RECTF physicsRect = Interface::GetPhsyicsRect(Image1).Resize(.9f,true);
	GameWorld world = GameWorld(RenderBuffer::BufferSize.X, RenderBuffer::BufferSize.Y);
	PhysicsBox girlBox = PhysicsBox(physicsRect, Util::Vector2(0.0025f, 0.0025f), 0,.5f);
	InputAwaiter a0 = InputAwaiter(InputType::IDLE, 0, false, 0);
	a0.SetCallback([this, Image1, &world,&girlBox](InputEvent& event) {
		girlBox.RunPhysicsStep(world);
		Interface::SetControlCenter(Image1,girlBox.GetPosition());
		Interface::RotateControl(Image1, girlBox.GetOrientation());
		event.SelectedIndex = Interface::GetMouseHoverIndex(event.MousePos);
		if (event.SelectedIndex != -1)
		{
			Interface::SetGUISelection(event.SelectedIndex);
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(event.SelectedIndex); }
		}
		else
		{
			Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem());
		}
		G1->SetCursorSelectableHovered(event.SelectedIndex != -1);  //Change cursor to hand if over selectable item otherwise arrow.
		return false;
		});

	InputAwaiter a1 = InputAwaiter(InputType::MOUSEMOVE, 0, false, 0);
	a1.SetCallback([this](InputEvent& event) {
		event.SelectedIndex = Interface::GetMouseHoverIndex(event.MousePos);
		if (event.SelectedIndex != -1)
		{
			Interface::SetGUISelection(event.SelectedIndex);
			if (this->GetMaintainSelectionAfterMouseHover()) { Interface::RotateSelectableMenuItemsToValue(event.SelectedIndex); }
		}
		else 
		{ 
			Interface::SetGUISelection(Interface::GetCurrentSelectableMenuItem()); 
		}
		G1->SetCursorSelectableHovered(event.SelectedIndex != -1);  //Change cursor to hand if over selectable item otherwise arrow.
		return false;
		});

	InputAwaiter a2 = InputAwaiter(InputType::MOUSECLICK, 0, false, 0);
	a2.SetCallback([this](InputEvent& event) {
		event.SelectedIndex = Interface::GetMouseHoverIndex(event.MousePos);
		if (event.SelectedIndex != -1) { return true; }
		return false;
		});

	InputAwaiter leftKeyAwaiter = InputAwaiter(InputType::KEYPRESS, 0, false, 0, { {VK_LEFT} });
	leftKeyAwaiter.SetCallback([this](InputEvent& event) {
		Interface::SetGUISelection(Interface::RotateSelectableMenuItems(-1, this->GetSelectableWrapAround()));
		return false;
		});

	InputAwaiter rightKeyAwaiter = InputAwaiter(InputType::KEYPRESS, 0, false, 0, { {VK_RIGHT} });
	rightKeyAwaiter.SetCallback([this](InputEvent& event) {
		Interface::SetGUISelection(Interface::RotateSelectableMenuItems(1, this->GetSelectableWrapAround()));
		return false;
		});

	InputAwaiter returnKeyAwaiter = InputAwaiter(InputType::KEYPRESS, 0, false, 0, { {VK_RETURN} });
	returnKeyAwaiter.SetCallback([&](InputEvent& event) {
		event.SelectedIndex = Interface::GetCurrentSelectableMenuItem();
		if (event.SelectedIndex != -1) { return true; }
		return false;
		});

	try
	{
		InputEvent UserResponse = InputAwaiter::GetAllInputAsync({ &a0, &a1, &a2,&leftKeyAwaiter, &rightKeyAwaiter,&returnKeyAwaiter });
		if (UserResponse.SelectedIndex == Option2SelectionID)
		{
			SetMenuResult(L"Exit Selected");
		}
		else if(UserResponse.SelectedIndex == Option1SelectionID)
		{
			SetMenuResult(L"Option 1 selected");
		}
		else if (UserResponse.SelectedIndex == GirlSelectionId)
		{
			SetMenuResult(L"Girl Selected");
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
	GetControls();
	TextInputState InputState = Input::TextInputState::GetInputState();
	if (InputState.TextInputInProgress) { Interface::ProcessInputText(InputState.CurrentInputText, InputState.IMECompositionText, InputState.guiCandidateTexts, InputState.SelectedCandidate, InputState.IMECursorPos, InputState.IMEComposing, InputState.ManualInput); }
}

void Load()
{
	AttachToWindow(G1->m_hwnd);
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

void Close()
{
	Input::CloseOverlay();
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
	G1->onClose = []() {Close(); };
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