#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

#include "CPPUtil.h"
#include "Graphics.h"


namespace GraphicsInterface
{

	struct OptionStruct
	{
		std::wstring Prompt; 
		std::wstring Click; 
		std::vector<std::wstring> optionnames;
		int OriginalSelection;
	};

	struct Layer
	{
		float lastVerticalPosition;
		bool isVisible;
		bool isInteractive;
		int layerLevel;
		BackgroundImage backgroundImage = {};
		std::vector<GUI_InputBox> InputBoxes;
		std::vector<GUIText> GUITextItems;
		std::vector<GUIBox> TextBoxes;
		std::vector<GUI_Image> Images;
		std::vector<int> SelectableMenuItems;
		Layer(int level, float position = 0.0f, bool visible = true, bool interactive = true) : layerLevel(level), lastVerticalPosition(position), isVisible(visible), isInteractive(interactive) {}
	};

	
	const int BackIndex = 12000;
	const int ContinueIndex = 12001;
	const float LineHeight = .04f;
	inline bool PauseRendering = false;
	inline bool ItemHovered = false;

	inline bool DebugNextFrame = false;


	class Interface
	{
	private:
		static inline std::vector<Layer> layerStack = { Layer(0) };



	private:
		template<typename F, typename... Args>
		static auto addRequest(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
		{
			Util_Assert(!dedicatedThread || std::this_thread::get_id() != dedicatedThread->get_id(), L"Error: Cannot call external interface function from internal interface function");
			Util_Assert(RenderEngineInitialized, L"Error: Render Engine is not initialized");
			if (!dedicatedThread)
			{
				dedicatedThread = new std::thread(&Interface::processQueue);
			}
			
			using return_type = decltype(f(args...));
			auto task = std::make_shared<std::packaged_task<return_type()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);
			std::future<return_type> res = task->get_future();
			{
				std::unique_lock<std::mutex> lock(mtx);
				requests.push([task]() { (*task)(); });
			}
			cv.notify_one();
			return res;
		}

		static void processQueue()
		{
			SetThreadDescription(GetCurrentThread(), L"GraphicsInterfaceThread");
			while (true)
			{
				std::function<void()> request;
				{
					std::unique_lock<std::mutex> lock(mtx);
					cv.wait(lock, [] { return !requests.empty(); });
					request = requests.front();
					requests.pop();
				}
				request(); 
				if (!RenderEngineInitialized) 
				{
					delete dedicatedThread;
					dedicatedThread = nullptr;
					break;
				}
			}
		}
		static inline std::thread* dedicatedThread = nullptr;
		static inline std::mutex mtx;
		static inline std::queue<std::function<void()>> requests;
		static inline std::condition_variable cv;

	public:
		static GraphicsState* CreateFrame() { return addRequest(&Interface::internalCreateFrame).get(); }
		static void SetPauseRendering(bool Pause){ return addRequest(&Interface::internalSetPauseRendering,Pause).get(); }
		static int FadeInFreeText(const std::wstring& text, D2D1::ColorF startingColor, D2D1::ColorF endingColor, int duration, float xpos, float ypos, bool align = true, int fontmodifier = 0) { return addRequest(&Interface::internalFadeInFreeText, text, startingColor, endingColor, duration, xpos, ypos, align, fontmodifier).get(); }
		static int WriteFreeText(std::wstring text, D2D1::ColorF color, float xpos, float ypos, bool aligncenter = true, int Selectionindex = -1, int FontModifier = 0, D2D1::ColorF selectioncolor = NULL, bool Silhouette = false, int textID = -1) { return addRequest(&Interface::internalWriteFreeText, std::move(text), color, xpos, ypos, aligncenter, Selectionindex, FontModifier, selectioncolor, Silhouette, textID).get(); }
		static int WriteOrderedText(std::wstring text, D2D1::ColorF color, float HorizontalStartPoint = 0, int Selectionindex = -1, int FontModifier = 0, D2D1::ColorF selectioncolor = NULL, bool Silhouette = false, int textID = -1) { return addRequest(&Interface::internalWriteOrderedText, std::move(text), color, HorizontalStartPoint, Selectionindex, FontModifier, selectioncolor, Silhouette, textID).get(); }
		static void AddContinueButton(std::wstring Text = L"Continue") { addRequest(&Interface::internalAddContinueButton, std::move(Text)).get(); }
		static void AddBackButton(std::wstring Text = L"Go back") { addRequest(&Interface::internalAddBackButton, std::move(Text)).get(); }
		static void ClearTextBoxes() { addRequest(&Interface::internalClearTextBoxes).get(); }
		static int CreateBox(float left, float top, float right, float bottom, D2D1::ColorF color, int width = 0, int selectionIndex = -1, D2D1::ColorF selectionColor = TRANSPARENT_COLOR) { return addRequest(&Interface::internalCreateBox, left, top, right, bottom, color, width, selectionIndex, selectionColor).get(); }
		static int CreateInputBox(Util::RECTF rect, D2D1::ColorF textColor, D2D1::ColorF boxColor, bool centerInput, bool wrapText, bool dynamicSizing, int selectionIndex = -1) {return addRequest(&Interface::internalCreateInputBox, rect, textColor, boxColor, centerInput, wrapText, dynamicSizing, selectionIndex).get();}		
		static void ClearImages() { addRequest(&Interface::internalClearImages).get(); }
		static void ClearText() { addRequest(&Interface::internalClearText).get(); }
		static int AddGUIImage(std::wstring ImagePath, Util::Vector2 pos, float size, int SelectionIndex = -1) { return addRequest(&Interface::internalAddGUIImage, std::move(ImagePath), pos, size, SelectionIndex).get(); }
		static void LoadBackgroundImage(std::wstring ImagePath) { addRequest(&Interface::internalLoadBackgroundImage, std::move(ImagePath)).get(); }
		static void ChangeTextOpacity(int ID, float newValue) { addRequest(&Interface::internalChangeTextOpacity, ID, newValue).get(); }
		static void ChangeTextColor(int ID, D2D1::ColorF newcolor) { addRequest(&Interface::internalChangeTextColor, ID, newcolor).get(); }
		static Util::RECTF GetControlRect(int ID) { return addRequest(&Interface::internalGetControlRect, ID).get(); }
		static Util::RECTF GetPhsyicsRect(int ID) { return addRequest(&Interface::internalGetPhysicsRect, ID).get(); }
		static bool RemoveControl(int ID) { return addRequest(&Interface::internalRemoveControl, ID).get(); }
		static void RemoveControls(std::vector<int> controls) { addRequest(&Interface::internalRemoveControls, std::move(controls)).get(); }
		static void SetControlCenter(int ID, Util::Vector2 newCenter) { addRequest(&Interface::internalSetControlCenter, ID, newCenter).get(); }
		static void RotateControl(int ID, float rotation) { addRequest(&Interface::internalRotateControl, ID, rotation).get(); }
		static std::vector<GUIText> CreateGUIText(std::wstring text, int fontModifier, D2D1::ColorF color, Util::Vector2 position, int selectionIndex, D2D1::ColorF selectionColor, D2D1::ColorF highlightColor, bool silhouette, int textID = -1) { return addRequest(&Interface::internalCreateGUIText, std::move(text), fontModifier, color, position, selectionIndex, selectionColor, highlightColor, silhouette, textID).get(); }
		static std::vector<GUI*> GetGUIItemsByLayer(int layer) { return addRequest(&Interface::internalGetGUIItemsByLayer, layer).get(); }
		static void ClearLayer() { addRequest(&Interface::internalClearLayer).get(); }
		static float GetLastVerticalPosition() { return addRequest(&Interface::internalGetLastVerticalPosition).get(); }
		static void SetLastVerticalPosition(float value) { addRequest(&Interface::internalSetLastVerticalPosition, value).get(); }
		static int PushLayer(bool hideBelow = false, bool disableBelow = true) { return addRequest(&Interface::internalPushLayer, hideBelow, disableBelow).get(); }
		static int PopLayer() { return addRequest(&Interface::internalPopLayer).get(); }
		static void PopAllLayers() { addRequest(&Interface::internalPopAllLayers).get(); }
		static int GetMouseHoverIndex(Util::Vector2 mousePosC) { return addRequest(&Interface::internalGetMouseHoverIndex, mousePosC).get(); }
		static void SetGUISelection(int Index) { return addRequest(&Interface::internalSetGUISelection, Index).get(); }
		static int RotateSelectableMenuItems(int rotateBy, bool allowWraparound) { return addRequest(&Interface::internalRotateSelectableMenuItems, rotateBy, allowWraparound).get(); }
		static void RotateSelectableMenuItemsToValue(int value) { addRequest(&Interface::internalRotateSelectableMenuItemsToValue, value).get(); }
		static bool SelectableMenuItemsAtStart() { return addRequest(&Interface::internalSelectableMenuItemsAtStart).get(); }
		static bool SelectableMenuItemsAtEnd() { return addRequest(&Interface::internalSelectableMenuItemsAtEnd).get(); }
		static bool IsSelectableMenuItemsEmpty() { return addRequest(&Interface::internalIsSelectableMenuItemsEmpty).get(); }
		static int GetCurrentSelectableMenuItem() { return addRequest(&Interface::internalGetCurrentSelectableMenuItem).get(); }
		static void ClearSelectableMenuItems() { addRequest(&Interface::internalClearSelectableMenuItems).get(); }
		static void AddSelectableMenuItem(int item) { addRequest(&Interface::internalAddSelectableMenuItem, item).get(); }
		static void AddIMEOverlay(int InputBoxID, std::wstring IMEcompositionText, std::vector<std::wstring> guiCandidateTexts, int SelectedCandidate){ addRequest(&Interface::internalAddIMEOverlay, InputBoxID, std::move(IMEcompositionText),  std::move(guiCandidateTexts), SelectedCandidate).get(); }
		static void UpdateInputBox(int id, std::wstring newText) { addRequest(&Interface::internalUpdateInputBox,id ,std::move(newText)).get(); }
		static int GetInputBoxLineCount(int ID) { return addRequest(&Interface::internalGetInputBoxLineCount, ID).get(); }
		static Util::RECTF GetInputBoxLineRect(int ID, int LineIndex) { return addRequest(&Interface::internalGetInputBoxLineRect, ID, LineIndex).get(); }
		static Util::Vector2 GetInputBoxCursorPos(int ID) { return addRequest(&Interface::internalGetInputBoxCursorPos, ID).get(); }
		static bool Control(int ID) { return addRequest(&Interface::internalControl, ID).get(); }



	private:
		static GraphicsState* internalCreateFrame();
		static GUI* internalGetControl(int ID);
		static std::vector<GUI*> internalGetControls(int ID);
		static void internalSetPauseRendering(bool pause);
		static int internalFadeInFreeText(const std::wstring& text, D2D1::ColorF startingColor, D2D1::ColorF endingColor, int duration, float xpos, float ypos, bool align = true, int fontmodifier = 0);
		static int internalWriteFreeText(std::wstring text, D2D1::ColorF color, float xpos, float ypos, bool aligncenter = true, int Selectionindex = -1, int FontModifier = 0, D2D1::ColorF selectioncolor = NULL, bool Silhouette = false, int textID = -1);
		static int internalWriteOrderedText(std::wstring text, D2D1::ColorF color, float HorizontalStartPoint = 0, int Selectionindex = -1, int FontModifier = 0, D2D1::ColorF selectioncolor = NULL, bool Silhouette = false, int textID = -1);
		static void internalAddContinueButton(std::wstring Text = L"Continue");
		static void internalAddBackButton(std::wstring Text = L"Go back");
		static void internalClearTextBoxes();
		static int internalCreateBox(float left, float top, float right, float bottom, D2D1::ColorF color, int width = 0, int selectionIndex = -1, D2D1::ColorF selectionColor = TRANSPARENT_COLOR);
		static int internalCreateInputBox(Util::RECTF rect, D2D1::ColorF textColor, D2D1::ColorF boxColor, bool centerInput, bool wrapText, bool dynamicSizing, int selectionIndex = -1);
		static void internalClearImages();
		static void internalClearText();
		static int internalAddGUIImage(std::wstring ImagePath, Util::Vector2 pos, float size, int SelectionIndex = -1);
		static void internalLoadBackgroundImage(std::wstring ImagePath);
		static void internalChangeTextOpacity(int ID, float newValue);
		static void internalChangeTextColor(int ID, D2D1::ColorF newcolor);
		static Util::RECTF internalGetControlRect(int ID);
		static Util::RECTF internalGetPhysicsRect(int ID);
		static void internalSetControlCenter(int ID, Util::Vector2 newCenter);
		static bool internalRemoveControl(int ID);
		static void internalRemoveControls(std::vector<int> controls);
		static std::vector<GUIText> internalCreateGUIText(std::wstring text, int fontModifier, D2D1::ColorF color, Util::Vector2 position, int selectionIndex, D2D1::ColorF selectionColor, D2D1::ColorF highlightColor, bool silhouette, int textID = -1);
		static std::vector<GUI*> internalGetGUIItemsByLayer(int layer);
		static void internalClearLayer();
		static float internalGetLastVerticalPosition();
		static void internalSetLastVerticalPosition(float value);
		static int internalPushLayer(bool hideBelow = false, bool disableBelow = true);
		static int internalPopLayer();
		static void internalPopAllLayers();
		static int internalGetMouseHoverIndex(Util::Vector2 mousePosC);
		static int internalRotateSelectableMenuItems(int rotateBy,bool allowWraparound);
		static void internalRotateSelectableMenuItemsToValue(int value);
		static bool internalSelectableMenuItemsAtStart();
		static bool internalSelectableMenuItemsAtEnd();
		static bool internalIsSelectableMenuItemsEmpty();
		static int internalGetCurrentSelectableMenuItem();
		static void internalAddSelectableMenuItem(int item);
		static void internalClearSelectableMenuItems();
		static void internalSetGUISelection(int Index);
		static void internalAddIMEOverlay(int InputBoxID, std::wstring IMEcompositionText, std::vector<std::wstring> guiCandidateTexts, int SelectedCandidate);
		static void internalRotateControl(int ID, float rotation);
		static bool internalControl(int ID);
		static void internalUpdateInputBox(int ID, const std::wstring& CurrentInput);
		static int internalGetInputBoxLineCount(int ID);
		static Util::RECTF internalGetInputBoxLineRect(int ID, int LineIndex);
		static Util::Vector2 internalGetInputBoxCursorPos(int ID);

	};

}