#include "GraphicsInterface.h"

using namespace std;
using namespace GraphicsInterface;

void Interface::internalSetPauseRendering(bool Pause)
{
	PauseRendering = Pause;
}
void Interface::internalSetCenterInput(bool Center)
{
	CenterInput = Center;
}

void Interface::internalSetMaxWrapAroundWidth(float width)
{
	InputTextWrapAroundWidthN = width;
}

int Interface::internalGetInputTextLineCount()
{
	return InputTextLineCount;
}

int Interface::internalCreateBox(float left, float top, float right, float bottom, D2D1::ColorF color, int width, int selectionIndex, D2D1::ColorF selectionColor)
{
	GUIBox box = GUIBox(color, Util::RECTF(left, top, right, bottom), width, selectionIndex, selectionColor);
	layerStack.back().TextBoxes.push_back(box);
	return box.GetID();
}

void Interface::internalLoadBackgroundImage(std::wstring ImagePath)
{
	layerStack.back().backgroundImage = BackgroundImage(ImagePath);
}

int Interface::internalAddGUIImage(std::wstring ImagePath, Util::Vector2 pos, float size, int SelectionIndex)
{
	GUI_Image img = GUI_Image(ImagePath, pos, size, SelectionIndex);
	layerStack.back().Images.push_back(img);
	return img.GetID();
}


std::vector<GUIText> Interface::internalCreateGUIText(std::wstring text, int fontModifier, D2D1::ColorF color, Util::Vector2 position, int selectionIndex, D2D1::ColorF selectionColor, D2D1::ColorF highlightColor, bool silhouette, int textID)
{
	if (textID == -1) { textID = GUI::GetGroupID(); }
	vector<GUIText> drawOperations;
	std::wregex colorPattern(L"\\[([^\\[\\]]*?)\\]\\((([A-Z]{1,2})?,?)?(([+-]\\d+))?\\)");
	Util::RECTF textRect = GetStringRectN(text, fontHeight, fontModifier, Util::Vector2(position.X, position.Y), false, true);
	std::wsmatch colorMatch;
	if (std::regex_search(text, colorMatch, colorPattern))
	{
		std::wstring inputText = text;
		float horizontalPosition = textRect.GetPosition().X;

		while (std::regex_search(inputText, colorMatch, colorPattern))
		{
			std::wstring beforeMatch = colorMatch.prefix().str();
			std::wstring colorText = colorMatch[1].str();
			std::wstring colorMarker = colorMatch[3].str();
			int fontModifierN = fontModifier;

			if (colorMatch[4].matched) {
				fontModifierN += std::stoi(colorMatch[4].str());
			}

			if (!beforeMatch.empty())
			{
				Util::RECTF beforeRect = GetStringRectN(beforeMatch, fontHeight, fontModifierN, Util::Vector2(horizontalPosition, textRect.GetPosition().Y), false, true);
				drawOperations.push_back(GUIText(textID, beforeMatch, beforeRect, color, fontModifierN, highlightColor, selectionColor, selectionIndex));
				horizontalPosition += beforeRect.Width;
			}

			D2D1::ColorF Tagcolor = color;
			if (!colorMarker.empty())
			{
				Tagcolor = GetColorCode(colorMarker);
				if (Tagcolor.a != 0) { Tagcolor.a = color.a; }
			}
			Util::RECTF colorRect = GetStringRectN(colorText, fontHeight, fontModifierN, Util::Vector2(horizontalPosition, textRect.GetPosition().Y), false, true);
			drawOperations.push_back(GUIText(textID, colorText, colorRect, Tagcolor, fontModifierN, highlightColor, selectionColor, selectionIndex));
			horizontalPosition += colorRect.Width;

			inputText = colorMatch.suffix().str();
		}

		if (!inputText.empty())
		{
			drawOperations.push_back(GUIText(textID, inputText, GetStringRectN(inputText, fontHeight, fontModifier, Util::Vector2(horizontalPosition, textRect.GetPosition().Y), false, true), color, fontModifier, highlightColor, selectionColor, selectionIndex));
		}
	}
	else
	{
		drawOperations.push_back(GUIText(textID, text, GetStringRectN(text, fontHeight, fontModifier, textRect.GetPosition(), false, true), color, fontModifier, highlightColor, selectionColor, selectionIndex));
	}
	return drawOperations;
}


int Interface::internalWriteFreeText(wstring text, D2D1::ColorF color, float HorizontalStartPoint, float VerticalStartPosition, bool aligncenter, int Selectionindex, int FontModifier, D2D1::ColorF selectioncolor, bool Silhouette, int textID)
{
	wstring::size_type pos = text.find(L'\n');
	wstring firstPart;
	wstring secondPart;
	if (pos != wstring::npos)
	{
		firstPart = text.substr(0, pos);
		secondPart = text.substr(pos + 1);
	}
	else { firstPart = text; }

	int id = textID;
	if (!firstPart.empty())
	{
		Util::Vector2 position = Util::Vector2(HorizontalStartPoint, VerticalStartPosition);

		vector<GUIText> OPs = (textID == -1) ? internalCreateGUIText(firstPart, FontModifier, color, position, Selectionindex, selectioncolor, TRANSPARENT_COLOR, Silhouette) : internalCreateGUIText(firstPart, FontModifier, color, position, Selectionindex, selectioncolor, TRANSPARENT_COLOR, Silhouette, textID);
		if (aligncenter)
		{
			Util::RECTF boundingRect = OPs[0].GetRectN();
			for (auto& op : OPs)
			{
				boundingRect = boundingRect.Union(op.GetRectN());
			}
			Util::Vector2 offset = position - boundingRect.GetCenter();
			for (auto& op : OPs)
			{
				op.SetRectN(Util::RECTF(Util::Vector2(op.GetRectN().GetPosition().X + offset.X, op.GetRectN().GetPosition().Y), op.GetRectN().GetSize()));
			}
		}
		id = OPs[0].GetID();
		// Lock the mutex, add to buffer, and unlock mutex
		Util::VectorAddRange(layerStack.back().GUITextItems, OPs);
	}
	if (pos != wstring::npos) { internalWriteFreeText(secondPart, color, HorizontalStartPoint, VerticalStartPosition + LineHeight, aligncenter, Selectionindex, FontModifier, selectioncolor, Silhouette, id); }
	return id;
}


int Interface::internalWriteOrderedText(wstring text, D2D1::ColorF color, float HorizontalStartPoint, int Selectionindex, int FontModifier, D2D1::ColorF selectioncolor, bool Silhouette, int textID)
{
	wstring::size_type pos = text.find(L'\n');
	wstring firstPart;
	wstring secondPart;
	if (pos != wstring::npos)
	{
		firstPart = text.substr(0, pos);
		secondPart = text.substr(pos + 1);
	}
	else { firstPart = text; }


	int id = textID;
	if (!firstPart.empty())
	{
		vector<GUIText> OPs = (textID == -1) ? internalCreateGUIText(firstPart, FontModifier, color, Util::Vector2(HorizontalStartPoint, internalGetLastVerticalPosition()), Selectionindex, selectioncolor, TRANSPARENT_COLOR, Silhouette) : internalCreateGUIText(firstPart, FontModifier, color, Util::Vector2(HorizontalStartPoint, internalGetLastVerticalPosition()), Selectionindex, selectioncolor, TRANSPARENT_COLOR, Silhouette, textID);
		id = OPs[0].GetID();
		Util::VectorAddRange(layerStack.back().GUITextItems, OPs);
	}
	if (pos != wstring::npos)
	{
		internalSetLastVerticalPosition(internalGetLastVerticalPosition() + LineHeight);
		return internalWriteOrderedText(secondPart, color, HorizontalStartPoint, Selectionindex, FontModifier, selectioncolor, Silhouette, id);
	}

	return id;
}


int Interface::internalFadeInFreeText(const std::wstring& text, D2D1::ColorF startingColor, D2D1::ColorF endingColor, int duration, float xpos, float ypos, bool align, int fontmodifier)
{
	const int steps = 30; 
	int textID = internalWriteFreeText(text, startingColor, xpos, ypos, align, -1, fontmodifier, NULL, false, -1);

	for (int i = 0; i <= steps; ++i)
	{
		float t = static_cast<float>(i) / steps;
		D2D1::ColorF currentColor = D2D1::ColorF(
			Util::lerp(startingColor.r, endingColor.r, t),
			Util::lerp(startingColor.g, endingColor.g, t),
			Util::lerp(startingColor.b, endingColor.b, t),
			Util::lerp(startingColor.a, endingColor.a, t)
		);
		internalChangeTextColor(textID, currentColor);
		std::this_thread::sleep_for(std::chrono::milliseconds(duration / steps));
	}
	return textID;
}


void Interface::internalAddContinueButton(wstring Text)
{
	internalWriteFreeText(Text, BLUE, .5f, .85f, true, ContinueIndex, 0, GREEN);
}
void Interface::internalAddBackButton(wstring Text)
{
	internalWriteFreeText(Text, BLUE, .5f, .9f, true, BackIndex, 0, GREEN);
}

void Interface::internalChangeTextColor(int ID, D2D1::ColorF newcolor)
{
	bool found = false;
	for (Layer& layer : layerStack) {
		for (GUIText& text : layer.GUITextItems) {
			if (text.GetID() == ID) {
				found = true;
				text.color = newcolor;
			}
		}
		if (found) break;
	}

	if (!found) { Util_LogErrorTerminate(L"Text ID not found"); }
}

void Interface::internalChangeTextOpacity(int ID, float newValue)
{
	bool found = false;
	for (Layer& layer : layerStack) {
		for (GUIText& text : layer.GUITextItems) {
			if (text.GetID() == ID) {
				found = true;
				text.color.a = newValue;
			}
		}
		if (found) break;
	}

	if (!found) { Util_LogErrorTerminate(L"Text ID not found"); }
}

Util::RECTF Interface::internalGetControlRect(int ID)
{
	for (Layer& layer : layerStack)
	{
		for (GUI* item : internalGetGUIItemsByLayer(layer.layerLevel))
		{
			if (item->GetID() == ID) { return item->GetRectN(); }
		}
	}
	Util_LogErrorTerminate(L"ID not found");
	return Util::RECTF();
}

void Interface::internalMoveControl(int ID, Util::Vector2 pos)
{
	bool found = false;
	for (Layer& layer : layerStack)
	{
		for (GUI* item : internalGetGUIItemsByLayer(layer.layerLevel))
		{
			if (item->GetID() == ID) 
			{ 
				Util::RECTF rect = item->GetRectN();
				rect.SetPosition(pos);
				item->SetRectN(rect);
				found = true;
			}
		}
	}
	if (!found) { Util_LogErrorTerminate(L"ID not found"); }
}

void Interface::internalRotateControl(int ID, float rotation)
{
	bool found = false;
	for (Layer& layer : layerStack)
	{
		for (GUI* item : internalGetGUIItemsByLayer(layer.layerLevel))
		{
			if (item->GetID() == ID)
			{
				item->SetRotation(rotation);
				found = true;
			}
		}
	}
	if (!found) { Util_LogErrorTerminate(L"ID not found"); }
}

bool Interface::internalRemoveControl(int ID)
{
	bool isRemoved = false;

	for (Layer& layer : layerStack)
	{
		auto it1 = std::remove_if(layer.Images.begin(), layer.Images.end(), [ID](const GUI_Image& img) { return img.GetID() == ID; });
		if (it1 != layer.Images.end())
		{
			isRemoved = true;
			layer.Images.erase(it1, layer.Images.end());
		}
		auto it2 = std::remove_if(layer.TextBoxes.begin(), layer.TextBoxes.end(), [ID](const GUIBox& box) { return box.GetID() == ID; });
		if (it2 != layer.TextBoxes.end())
		{
			isRemoved = true;
			layer.TextBoxes.erase(it2, layer.TextBoxes.end());
		}

		auto it3 = std::remove_if(layer.GUITextItems.begin(), layer.GUITextItems.end(), [ID](const GUIText& text) { return text.GetID() == ID; });
		if (it3 != layer.GUITextItems.end())
		{
			isRemoved = true;
			layer.GUITextItems.erase(it3, layer.GUITextItems.end());
		}
	}
	return isRemoved;
}




void Interface::internalRemoveControls(vector<int> controls)
{
	for (int i = 0; i < controls.size(); i++) { internalRemoveControl(controls[i]); }
}


std::vector<GUI*> Interface::internalGetGUIItemsByLayer(int layer)
{
	std::vector<GUI*> guiItems;

	if (layer < 0 || layer >= layerStack.size()) { return guiItems; }

	for (auto& image : layerStack[layer].Images)
		guiItems.push_back(&image);

	for (auto& textBox : layerStack[layer].TextBoxes)
		guiItems.push_back(&textBox);

	for (auto& textBuffer : layerStack[layer].GUITextItems)
		guiItems.push_back(&textBuffer);

	return guiItems;
}

GraphicsState* Interface::internalCreateFrame()
{
	GraphicsState* g1 = new GraphicsState();
	for (int layer = static_cast<int>(layerStack.size()) - 1; layer >= 0; layer--)
	{
		if (!layerStack[layer].isVisible || !layerStack[layer].backgroundImage.imagePath.empty())
		{
			g1->backgroundImage = layerStack[layer].backgroundImage;
			break;
		}
	}
	std::vector<std::pair<GUI::ControlType, size_t>> order;

	for (int layer = 0; static_cast<int>(layer < layerStack.size()); layer++)
	{
		if (!layerStack[layer].isVisible) { continue; }

		for (GUI* CurrentGUIItem : internalGetGUIItemsByLayer(layer))
		{
			if (CurrentGUIItem->GetType() == GUI::ControlType::GUIImage)
			{
				g1->Images.push_back(*static_cast<GUI_Image*>(CurrentGUIItem));
				order.emplace_back(GUI::ControlType::GUIImage, g1->Images.size() - 1);
			}
			else if (CurrentGUIItem->GetType() == GUI::ControlType::GUIBox)
			{
				g1->TextBoxes.push_back(*static_cast<GUIBox*>(CurrentGUIItem));
				order.emplace_back(GUI::ControlType::GUIBox, g1->TextBoxes.size() - 1);
			}
			else if (CurrentGUIItem->GetType() == GUI::ControlType::GUIText)
			{
				g1->GUITextItems.push_back(*static_cast<GUIText*>(CurrentGUIItem));
				order.emplace_back(GUI::ControlType::GUIText, g1->GUITextItems.size() - 1);
			}
		}
	}
	for (int i = 0; i < static_cast<int>(IMEOverlayItems.size()); i++)
	{
		g1->TextBoxes.push_back(IMEOverlayItems[i]);
		order.emplace_back(GUI::ControlType::GUIBox, g1->TextBoxes.size() - 1);
	}
	for (int i = 0; i < static_cast<int>(InputTextItems.size()); i++)
	{
		g1->GUITextItems.push_back(InputTextItems[i]);
		order.emplace_back(GUI::ControlType::GUIText, g1->GUITextItems.size() - 1);
	}

	for (const auto& [type, index] : order)
	{
		switch (type)
		{
		case GUI::ControlType::GUIImage:
			g1->AllGUIItems.push_back(&g1->Images[index]);
			break;
		case GUI::ControlType::GUIBox:
			g1->AllGUIItems.push_back(&g1->TextBoxes[index]);
			break;
		case GUI::ControlType::GUIText:
			g1->AllGUIItems.push_back(&g1->GUITextItems[index]);
			break;
		}
	}

	if (DebugNextFrame) { g1->DebugFrame = true; }
	return g1;
}

void Interface::internalClearTextInput()
{
	IMEOverlayItems.clear();
	InputTextItems.clear();
}

void Interface::internalClearLayer()
{
	if (layerStack.size() > 0)
	{
		layerStack.back().GUITextItems.clear();
		layerStack.back().Images.clear();
		layerStack.back().TextBoxes.clear();
		layerStack.back().lastVerticalPosition = 0;
	}
	else { Util_LogErrorTerminate(L"No layer to clear"); }
}

float Interface::internalGetLastVerticalPosition() {
	if (!layerStack.empty()) {
		return layerStack.back().lastVerticalPosition;
	}
	return 0.0f;
}

void Interface::internalSetLastVerticalPosition(float value) {
	if (!layerStack.empty()) {
		layerStack.back().lastVerticalPosition = value;
	}
}

void Interface::internalSetInputLocation(Util::Vector2 pos)
{
	InputLocation = pos;
}

int Interface::internalPushLayer(bool hideBelow, bool disableBelow) //returns new current layer level
{
	if (!layerStack.empty())
	{
		if (hideBelow) { layerStack.back().isVisible = false; }
		if (disableBelow) { layerStack.back().isInteractive = false; }
	}
	layerStack.push_back(Layer(layerStack.back().layerLevel + 1));
	if (layerStack.size() > 100) { Util_LogErrorTerminate(L"GUI Layer leak detected"); }
	return layerStack.back().layerLevel;
}

int Interface::internalPopLayer() //returns new current layer level or -1 if no pop occurred because already at 0.
{
	if (layerStack.size() >= 1)
	{
		if (layerStack.size() > 1) { layerStack.pop_back(); }
		layerStack.back().isVisible = true;
		layerStack.back().isInteractive = true;
	}
	return layerStack.back().layerLevel;
}

void Interface::internalPopAllLayers()
{
	while (internalPopLayer() > 0);
}

void Interface::internalClearImages()
{
	layerStack.back().Images.clear();
}

void Interface::internalClearTextBoxes()
{
	layerStack.back().TextBoxes.clear();
}
void Interface::internalClearText()
{
	layerStack.back().GUITextItems.clear();
	layerStack.back().lastVerticalPosition = 0;
}

void Interface::internalSetGUISelection(int Index)
{
	for (int layer = 0; layer < layerStack.size(); layer++)
	{
		if (!layerStack[layer].isInteractive || !layerStack[layer].isVisible) { continue; }
		for (GUI* CurrentGUIItem : internalGetGUIItemsByLayer(layer))
		{
			CurrentGUIItem->Selected = (Index == -1 ? false : (CurrentGUIItem->SelectionIndex == Index));
		}
	}
}

int Interface::internalGetMouseHoverIndex(Util::Vector2 mousePosC)
{
	for (int layer = 0; layer < layerStack.size(); layer++)
	{
		if (!layerStack[layer].isInteractive || !layerStack[layer].isVisible) { continue; }
		for (GUI* CurrentGUIItem : internalGetGUIItemsByLayer(layer))
		{
			if (CurrentGUIItem->SelectionIndex == -1) { continue; }
			if (mousePosC != Util::Vector2(0) && CurrentGUIItem->IsHovered(mousePosC))
			{
				return CurrentGUIItem->SelectionIndex;
			}
		}
	}
	return -1;
}


int Interface::internalRotateSelectableMenuItems(int rotateBy, bool allowWraparound)
{
	if (!layerStack.empty() && !layerStack.back().SelectableMenuItems.empty())
	{
		if (rotateBy < 0)
		{
			for (int i = 0; i < -rotateBy; ++i)
			{
				if (allowWraparound || !internalSelectableMenuItemsAtStart())
				{
					std::rotate(layerStack.back().SelectableMenuItems.rbegin(), layerStack.back().SelectableMenuItems.rbegin() + 1, layerStack.back().SelectableMenuItems.rend());
				}
			}
		}
		else
		{
			for (int i = 0; i < rotateBy; ++i)
			{
				if (allowWraparound || !internalSelectableMenuItemsAtEnd())
				{
					std::rotate(layerStack.back().SelectableMenuItems.begin(), layerStack.back().SelectableMenuItems.begin() + 1, layerStack.back().SelectableMenuItems.end());
				}
			}
		}
	}
	return internalGetCurrentSelectableMenuItem();
}



void Interface::internalRotateSelectableMenuItemsToValue(int value)
{
	if (value == -1) { return; }
	if (!layerStack.empty() && !layerStack.back().SelectableMenuItems.empty())
	{
		std::rotate(layerStack.back().SelectableMenuItems.begin(), std::find(layerStack.back().SelectableMenuItems.begin(), layerStack.back().SelectableMenuItems.end(), value), layerStack.back().SelectableMenuItems.end());
	}
}

bool Interface::internalSelectableMenuItemsAtStart()
{
	if (!layerStack.empty() && !layerStack.back().SelectableMenuItems.empty())
	{
		return layerStack.back().SelectableMenuItems[0] == *std::min_element(layerStack.back().SelectableMenuItems.begin(), layerStack.back().SelectableMenuItems.end());
	}
	return true;
}

bool Interface::internalSelectableMenuItemsAtEnd() {
	if (!layerStack.empty() && !layerStack.back().SelectableMenuItems.empty())
	{
		return layerStack.back().SelectableMenuItems[0] == *std::max_element(layerStack.back().SelectableMenuItems.begin(), layerStack.back().SelectableMenuItems.end());
	}
	return true;
}

bool Interface::internalIsSelectableMenuItemsEmpty()
{
	if (!layerStack.empty())
	{
		return layerStack.back().SelectableMenuItems.empty();
	}
	return true;
}

int Interface::internalGetCurrentSelectableMenuItem()
{
	if (!layerStack.empty() && !layerStack.back().SelectableMenuItems.empty())
	{
		return layerStack.back().SelectableMenuItems[0];
	}
	return -1;
}

void Interface::internalClearSelectableMenuItems()
{
	if (!layerStack.empty())
	{
		layerStack.back().SelectableMenuItems.clear();
	}
}

void Interface::internalAddSelectableMenuItem(int item)
{
	if (!layerStack.empty())
	{
		auto& items = layerStack.back().SelectableMenuItems;
		Util_Assert(!Util::VectorContains(items, item), L"Error item already exists in vector");
		auto it = std::lower_bound(items.begin(), items.end(), item);
		items.insert(it, item);
	}
}

vector<GUIText> Interface::CreateInputLine(wstring currentInputText, Util::Vector2 Pos, bool ManualInputFlag)
{
	vector<GUIText> InputControls;
	Util::RECTF inputRect = GetStringRectN(currentInputText, fontHeight, 0, Pos, false, false);
	if (CenterInput) { inputRect.SetPosition(GetTextAlignCenter(currentInputText, fontHeight, 0, inputRect.GetPosition())); }  //manually center aligned to prevent jumping back and forth as cursor blinks
	wstring DisplayInputText = currentInputText;
	if (g_altDisplayTextFunc) { DisplayInputText = g_altDisplayTextFunc(DisplayInputText); }
	if (ManualInputFlag) { DisplayInputText = L"[" + DisplayInputText + L"](TP)"; }
	Util::VectorAddRange(InputControls, internalCreateGUIText(DisplayInputText, 0, InputTextColor, Util::Vector2(inputRect.Left, Pos.Y), -1, WHITE, TRANSPARENT_COLOR, false, -1));
	return InputControls;
}

void Interface::internalProcessInputText(wstring currentInputText, wstring IMEcompositionText, std::vector<std::wstring> guiCandidateTexts, int SelectedCandidate, int IMECursorPos, bool IMEComposing, bool ManualInputFlag)
{
	vector<GUIText> InputControls;
	int TotalLines = 0;
	wstring remainingText = currentInputText;
	Util::Vector2 currentPosition = InputLocation;
	while (!remainingText.empty())
	{
		Util::RECTF textRect = GetStringRectN(remainingText, fontHeight, 0, currentPosition, false, false);
		float textWidth = textRect.Right - textRect.Left;
		if (textWidth <= InputTextWrapAroundWidthN) { break; }
		else
		{
			// Find split position by reducing it until it fits
			size_t splitPos = remainingText.size();
			while (splitPos > 0)
			{
				wstring testLine = remainingText.substr(0, splitPos);
				textRect = GetStringRectN(testLine, fontHeight, 0, currentPosition, false, false);
				textWidth = textRect.Right - textRect.Left;
				if (textWidth <= InputTextWrapAroundWidthN) { break; }
				splitPos--;  // Reduce split position to shorten line
			}

			// Create line from start to splitPos
			wstring lineText = remainingText.substr(0, splitPos);
			vector<GUIText> lineControls = CreateInputLine(lineText, currentPosition, ManualInputFlag);
			Util::VectorAddRange(InputControls, lineControls);

			// Update remaining text and position for next line
			remainingText = remainingText.substr(splitPos);
			currentPosition.Y += LineHeight;
			TotalLines++;
		}
	}

	// Update currentInputText to only include the text for the last line
	currentInputText = remainingText;


	wstring Combined = currentInputText + IMEcompositionText;
	wstring CombinedCursor = currentInputText + IMEcompositionText.substr(0, IMECursorPos);

	Util::RECTF inputRect = GetStringRectN(currentInputText, fontHeight, 0, currentPosition, false, false);
	Util::RECTF imecursorCompRect = Util::RECTF(0, 0, 0, 0);
	if (IMEComposing && !IMEcompositionText.empty())
	{
		imecursorCompRect = GetStringRectN(CombinedCursor, fontHeight, 0, currentPosition, false, false);
	}
	if (CenterInput) { inputRect.SetPosition(GetTextAlignCenter(Combined, fontHeight, 0, inputRect.GetPosition())); }  //manually center aligned to prevent jumping back and forth as cursor blinks
	imecursorCompRect.SetPosition(inputRect.GetPosition());
	wstring DisplayInputText = currentInputText;
	if (g_altDisplayTextFunc) { DisplayInputText = g_altDisplayTextFunc(DisplayInputText); }
	if (ManualInputFlag) { DisplayInputText = L"[" + DisplayInputText + L"](TP)"; }

	Util::VectorAddRange(InputControls, internalCreateGUIText(DisplayInputText + ((!IMEComposing && GetTickCount64() % 1000 < 500) ? CursorDesign : L""), 0, InputTextColor, Util::Vector2(inputRect.Left, currentPosition.Y), -1, WHITE, TRANSPARENT_COLOR, false, -1));
	if (IMEComposing)
	{
		Util::VectorAddRange(InputControls, internalCreateGUIText(IMEcompositionText, 0, InputTextColor, Util::Vector2(inputRect.Right, currentPosition.Y), -1, WHITE, RED, false, -1));
		Util::VectorAddRange(InputControls, internalCreateGUIText(((GetTickCount64() % 1000 < 500) ? CursorDesign : L""), 0, InputTextColor, Util::Vector2(imecursorCompRect.Right, currentPosition.Y), -1, WHITE, TRANSPARENT_COLOR, false, -1));
		float MaxLength = 0;
		for (int i = 0; i < guiCandidateTexts.size(); i++)
		{
			Util::RECTF rect = GetStringRectN(guiCandidateTexts[i], fontHeight, 0, Util::Vector2(inputRect.Right, currentPosition.Y + LineHeight * (i + 1)), false, false);
			if (i == SelectedCandidate) { Util::VectorAddRange(InputControls, internalCreateGUIText(guiCandidateTexts[i], 0, WHITE, rect.GetPosition(), -1, WHITE, GREEN, false, -1)); }
			else { Util::VectorAddRange(InputControls, internalCreateGUIText(guiCandidateTexts[i], 0, WHITE, rect.GetPosition(), -1, WHITE, TRANSPARENT_COLOR, false, -1)); }
			MaxLength = max(MaxLength, rect.Width);
		}
		GUIBox b1 = GUIBox(BLACK, Util::RECTF(inputRect.Right, GetTightStringRectN(inputRect).Top, inputRect.Right + MaxLength, currentPosition.Y + LineHeight * (guiCandidateTexts.size() + 1.3f)));
		IMEOverlayItems = { b1 };
	}
	else
	{
		IMEOverlayItems.clear();
	}
	TotalLines++;
	InputTextLineCount = TotalLines;
	InputTextItems = InputControls;
}









