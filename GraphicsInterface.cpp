#include "GraphicsInterface.h"

using namespace std;
using namespace GraphicsInterface;

void Interface::internalSetPauseRendering(bool Pause)
{
	PauseRendering = Pause;
}

int Interface::internalCreateBox(float left, float top, float right, float bottom, D2D1::ColorF color, int width, int selectionIndex, D2D1::ColorF selectionColor)
{
	GUIBox box = GUIBox(color, Util::RECTF(left, top, right, bottom), width, selectionIndex, selectionColor);
	layerStack.back().TextBoxes.push_back(box);
	return box.GetID();
}

int Interface::internalCreateInputBox(Util::RECTF rect, D2D1::ColorF textColor, D2D1::ColorF boxColor, bool centerInput, bool wrapText, bool dynamicSizing, int selectionIndex)
{
	GUI_InputBox box = GUI_InputBox(rect, textColor, boxColor, centerInput, wrapText, dynamicSizing, selectionIndex);
	layerStack.back().InputBoxes.push_back(box);
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
	Util::RECTF textRect = GetStringRectN(text, fontHeight, fontModifier, Util::Vector2(position.X, position.Y), false);
	std::wsmatch colorMatch;
	if (std::regex_search(text, colorMatch, colorPattern))
	{
		std::wstring inputText = text;
		float horizontalPosition = textRect.GetTopLeft().X;

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
				Util::RECTF beforeRect = GetStringRectN(beforeMatch, fontHeight, fontModifierN, Util::Vector2(horizontalPosition, textRect.GetTopLeft().Y), false);
				drawOperations.push_back(GUIText(textID, beforeMatch, beforeRect, color, fontModifierN, highlightColor, selectionColor, selectionIndex));
				horizontalPosition += beforeRect.Width;
			}

			D2D1::ColorF Tagcolor = color;
			if (!colorMarker.empty())
			{
				Tagcolor = GetColorCode(colorMarker);
				if (Tagcolor.a != 0) { Tagcolor.a = color.a; }
			}
			Util::RECTF colorRect = GetStringRectN(colorText, fontHeight, fontModifierN, Util::Vector2(horizontalPosition, textRect.GetTopLeft().Y), false);
			drawOperations.push_back(GUIText(textID, colorText, colorRect, Tagcolor, fontModifierN, highlightColor, selectionColor, selectionIndex));
			horizontalPosition += colorRect.Width;

			inputText = colorMatch.suffix().str();
		}

		if (!inputText.empty())
		{
			drawOperations.push_back(GUIText(textID, inputText, GetStringRectN(inputText, fontHeight, fontModifier, Util::Vector2(horizontalPosition, textRect.GetTopLeft().Y), false), color, fontModifier, highlightColor, selectionColor, selectionIndex));
		}
	}
	else
	{
		drawOperations.push_back(GUIText(textID, text, GetStringRectN(text, fontHeight, fontModifier, textRect.GetTopLeft(), false), color, fontModifier, highlightColor, selectionColor, selectionIndex));
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
				op.SetRectN(Util::RECTF(Util::Vector2(op.GetRectN().GetTopLeft().X + offset.X, op.GetRectN().GetTopLeft().Y), op.GetRectN().GetSize()));
			}
		}
		id = OPs[0].GetID();
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
	for (auto op : internalGetControls(ID))
	{
		GUIText* item = dynamic_cast<GUIText*>(op);
		item->color = newcolor;
	}
}

void Interface::internalChangeTextOpacity(int ID, float newValue)
{
	for (auto op : internalGetControls(ID))
	{
		GUIText* item = dynamic_cast<GUIText*>(op);
		item->color.a = newValue;
	}
}

Util::RECTF Interface::internalGetControlRect(int ID)
{
	return internalGetControl(ID)->GetRectN();
}

int Interface::internalGetInputBoxLineCount(int ID)
{
	GUI_InputBox* inputBox = dynamic_cast<GUI_InputBox*>(internalGetControl(ID));
	return inputBox->GetLineCount();
}

Util::RECTF Interface::internalGetInputBoxLineRect(int ID, int LineIndex)
{
	GUI_InputBox* inputBox = dynamic_cast<GUI_InputBox*>(internalGetControl(ID));
	return inputBox->GetLineRect(LineIndex);
}

Util::Vector2 Interface::internalGetInputBoxCursorPos(int ID)
{
	GUI_InputBox* inputBox = dynamic_cast<GUI_InputBox*>(internalGetControl(ID));
	return inputBox->GetCursorPos();
}

Util::RECTF Interface::internalGetPhysicsRect(int ID)
{
	GUI_Image* image = dynamic_cast<GUI_Image*>(internalGetControl(ID));
	return image->GetPhysicsRect();
}

void Interface::internalSetControlCenter(int ID, Util::Vector2 newCenter)
{
	GUI* item = internalGetControl(ID);
	Util::RECTF rect = item->GetRectN();
	Util::Vector2 size = rect.GetSize();
	Util::Vector2 newTopLeft = newCenter - size / 2.0f;
	rect.SetPositionTopLeft(newTopLeft);
	item->SetRectN(rect);
}

void Interface::internalRotateControl(int ID, float rotation)
{
	internalGetControl(ID)->SetRotation(rotation);
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

		auto it4 = std::remove_if(layer.InputBoxes.begin(), layer.InputBoxes.end(), [ID](const GUI_InputBox& inputbox) { return inputbox.GetID() == ID; });
		if (it4 != layer.InputBoxes.end())
		{
			isRemoved = true;
			layer.InputBoxes.erase(it4, layer.InputBoxes.end());
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

	for (auto& inputTextBuffer : layerStack[layer].InputBoxes)
		guiItems.push_back(&inputTextBuffer);

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

			else if (CurrentGUIItem->GetType() == GUI::ControlType::GUIInputBox)
			{
				g1->GUIInputBoxes.push_back(*static_cast<GUI_InputBox*>(CurrentGUIItem));
				order.emplace_back(GUI::ControlType::GUIInputBox, g1->GUIInputBoxes.size() - 1);
			}
		}
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
		case GUI::ControlType::GUIInputBox:
			g1->AllGUIItems.push_back(&g1->GUIInputBoxes[index]);
			break;
		}
	}

	if (DebugNextFrame) { g1->DebugFrame = true; }
	return g1;
}

void Interface::internalClearLayer()
{
	if (layerStack.size() > 0)
	{
		layerStack.back().GUITextItems.clear();
		layerStack.back().Images.clear();
		layerStack.back().TextBoxes.clear();
		layerStack.back().InputBoxes.clear();
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
		if (!Util::VectorContains(items, item))
		{
			auto it = std::lower_bound(items.begin(), items.end(), item);
			items.insert(it, item);
		}
	}
}

GUI* Interface::internalGetControl(int ID)
{
	for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
	{
		for (GUI* item : internalGetGUIItemsByLayer(it->layerLevel))
		{
			if (item->GetID() == ID)
			{
				return item;
			}
		}
	}
	Util_LogErrorTerminate(L"ID not found");
}

bool Interface::internalControl(int ID)
{
	if (ID == -1) { return false; }
	for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
	{
		for (GUI* item : internalGetGUIItemsByLayer(it->layerLevel))
		{
			if (item->GetID() == ID) { return true; }
		}
	}
	return false;
}

vector<GUI*> Interface::internalGetControls(int ID)
{
	vector<GUI*> controls;
	for (auto it = layerStack.rbegin(); it != layerStack.rend(); ++it)
	{
		for (GUI* item : internalGetGUIItemsByLayer(it->layerLevel))
		{
			if (item->GetID() == ID) { controls.push_back(item); }
		}
	}
	if (controls.empty()) { Util_LogErrorTerminate(L"ID not found"); }
	return controls;
}


void Interface::internalAddIMEOverlay(int ID, wstring IMEcompositionText, int imeCursorPos, vector<wstring> guiCandidateTexts, int SelectedCandidate)
{
	GUI_InputBox* box = static_cast<GUI_InputBox*>(internalGetControl(ID));
	box->IMETextItems.clear();
	Util::VectorAddRange(box->IMETextItems, internalCreateGUIText(IMEcompositionText, 0, InputTextColor, box->GetCursorPos(), -1, WHITE, RED, false, -1));

	if (GetTickCount64() % (2 * box->CursorBlinkInterval) < box->CursorBlinkInterval)
	{
		wstring cursorSubstring = IMEcompositionText.substr(0, imeCursorPos);
		Util::RECTF cursorRect = GetSubStringRectN(IMEcompositionText, cursorSubstring, fontHeight, 0, box->GetCursorPos(), false, 0);
		Util::VectorAddRange(box->IMETextItems, internalCreateGUIText(CursorDesign, 0, InputTextColor, cursorRect.GetTopRight(), -1, WHITE, TRANSPARENT_COLOR, false, -1));
	}
	float MaxLength = 0;
	for (int i = 0; i < guiCandidateTexts.size(); i++)
	{
		Util::RECTF rect = GetStringRectN(guiCandidateTexts[i], fontHeight, 0, Util::Vector2(box->GetCursorPos().X, box->GetCursorPos().Y + LineHeight * (i + 1)), false);
		if (i == SelectedCandidate) { Util::VectorAddRange(box->IMETextItems, internalCreateGUIText(guiCandidateTexts[i], 0, WHITE, rect.GetTopLeft(), -1, WHITE, GREEN, false, -1)); }
		else { Util::VectorAddRange(box->IMETextItems, internalCreateGUIText(guiCandidateTexts[i], 0, WHITE, rect.GetTopLeft(), -1, WHITE, TRANSPARENT_COLOR, false, -1)); }
		MaxLength = max(MaxLength, rect.Width);
	}
	box->IMEBox = GUIBox(BLACK, Util::RECTF(box->GetCursorPos().X, box->GetCursorPos().Y, box->GetCursorPos().X + MaxLength, box->GetCursorPos().Y + LineHeight * (guiCandidateTexts.size() + 1.3f)));
	return;
}


void Interface::internalUpdateInputBox(int ID, const std::wstring& CurrentInput, int cursorPos)
{
	GUI_InputBox* box = static_cast<GUI_InputBox*>(internalGetControl(ID));
	box->GUITexts.clear();
	box->IMEBox = nullopt;
	box->IMETextItems.clear();
	box->Cursor = nullopt;
	vector<wstring> lines;
	wstring remainingText = CurrentInput;
	Util::Vector2 currentPosition = box->CenterInput ? Util::Vector2(box->GetRectN().GetCenter().X, box->GetRectN().Top) : Util::Vector2(box->GetRectN().Left, box->GetRectN().Top);
	int accumulatedLength = 0;
	int cursorLine = 0;
	int cursorIndex = -1;
	while (!remainingText.empty())
	{
		Util::RECTF textRect = GetStringRectN(remainingText, fontHeight, 0, currentPosition, false);
		float textWidth = textRect.Right - textRect.Left;
		if (textWidth <= box->GetRectN().GetSize().X)
		{
			lines.push_back(remainingText);
			if (cursorPos != -1)
			{
				if (cursorIndex == -1 && cursorPos <= accumulatedLength + remainingText.size())
				{
					cursorIndex = cursorPos - accumulatedLength;
				}
			}
			break;
		}
		else
		{
			int splitPos = static_cast<int>(remainingText.size());
			while (splitPos > 0)
			{
				wstring testLine = remainingText.substr(0, splitPos);
				textRect = GetStringRectN(testLine, fontHeight, 0, currentPosition, false);
				textWidth = textRect.Right - textRect.Left;
				if (textWidth <= box->GetRectN().GetSize().X)
				{
					lines.push_back(testLine);
					remainingText = remainingText.substr(splitPos);
					if (cursorPos != -1)
					{
						accumulatedLength += splitPos;
						if (cursorIndex == -1 && cursorPos <= accumulatedLength)
						{
							cursorIndex = cursorPos - (accumulatedLength - splitPos);
						}
						else { cursorLine++; }
					}
					break;
				}
				splitPos--;
			}
		}
	}
	Util::RECTF boxRect = box->GetRectN();
	Util::RECTF inputRect;
	float leftMost = box->CenterInput ? boxRect.GetCenter().X : boxRect.Left;
	float rightMost = box->CenterInput ? boxRect.Left : boxRect.GetCenter().X;
	for (int i = 0; i < lines.size(); i++)
	{
		Util::RECTF inputRect = GetStringRectN(lines[i], fontHeight, 0, Util::Vector2(currentPosition.X, currentPosition.Y + (i * LineHeight)), false);
		if (box->CenterInput) { inputRect.SetPositionTopLeft(GetTextAlignCenter(lines[i], fontHeight, 0, inputRect.GetTopLeft())); }
		if (inputRect.Bottom >= box->GetRectN().Bottom) { break; }
		box->GUITexts.push_back(GUIText(-1, lines[i], inputRect, box->TextColor, 0, TRANSPARENT_COLOR, box->TextColor, -1));
		if (cursorPos != -1 && i == cursorLine)
		{
			wstring cursorSubstring = lines[i].substr(0, cursorIndex);
			Util::RECTF cursorRect = GetSubStringRectN(lines[i], cursorSubstring, fontHeight, 0, inputRect.GetTopLeft(), false, 0);
			box->cursorPos = Util::Vector2(cursorRect.Right, cursorRect.Top);
		}
		if (box->CenterInput) { leftMost = min(leftMost, inputRect.Left); }
		rightMost = max(rightMost, inputRect.Right);
	}
	if (box->BoxColor.a != 0)
	{
		if (box->DynamicBox)
		{
			float maxWidth = boxRect.Width;
			float WidthIncrement = maxWidth / box->WidthIncrement;
			float targetWidth = box->MinWidth;

			float currentWidth = rightMost - leftMost;
			for (int i = 1; i <= box->WidthIncrement; i++)
			{
				if (currentWidth <= WidthIncrement * i * box->WrapThreshold)
				{
					targetWidth = max(targetWidth, WidthIncrement * i);
					break;
				}
			}
			if (currentWidth > WidthIncrement * box->WidthIncrement * box->WrapThreshold) { targetWidth = maxWidth; }

			if (box->CenterInput)
			{
				float centerX = boxRect.GetCenter().X;
				leftMost = centerX - (targetWidth / 2);
				rightMost = centerX + (targetWidth / 2);
			}
			else
			{
				rightMost = boxRect.Left + targetWidth;
			}
			leftMost = max(leftMost, boxRect.Left);
			rightMost = min(rightMost, boxRect.Right);
			box->InputBox = GUIBox(TextBoxColor, Util::RECTF(leftMost, boxRect.Top, rightMost, min(boxRect.Top + max(lines.size(), 1) * (1.5f * LineHeight), boxRect.Bottom)), 0, -1, TextBoxColor);
		}
		else
		{
			box->InputBox = GUIBox(TextBoxColor, box->GetRectN(), 0, -1, TextBoxColor);
		}
	}

	if (box->DisplayCursor && cursorPos != -1)
	{
		if (GetTickCount64() % (2 * box->CursorBlinkInterval) < box->CursorBlinkInterval) { box->Cursor = GUIText(-1, CursorDesign, GetStringRectN(CursorDesign, fontHeight, 0, box->GetCursorPos(), false), box->TextColor, 0, TRANSPARENT_COLOR, box->TextColor, -1); }
	}
	return;
}

