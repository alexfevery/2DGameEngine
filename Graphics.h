#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)


#include <functional>
#include <vector>
#include <windows.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <regex>
#include <future>
#include <sphelper.h>
#include <queue>
#include <atomic>
#include <gdiplus.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <map>
#include <optional>

#include "CPPUtil.h"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")


inline ULONG_PTR graphicsEngineToken = NULL;
inline ID2D1Factory* pD2DFactory = nullptr;
inline IWICImagingFactory* pIWICFactory = nullptr;
inline IDWriteFactory* pDWriteFactory = nullptr;

inline bool RenderEngineInitialized = false;

inline std::wstring CursorDesign = L"▌";


const D2D1::ColorF RED = D2D1::ColorF(205.0f / 255.0f, 49.0f / 255.0f, 49.0f / 255.0f);
const D2D1::ColorF GREEN = D2D1::ColorF(30.0f / 255.0f, 230.0f / 255.0f, 30.0f / 255.0f);
const D2D1::ColorF TRANSPARENTGREEN = D2D1::ColorF(30.0f / 255.0f, 230.0f / 255.0f, 30.0f / 255.0f,.35f);
const D2D1::ColorF DARK_GREEN = D2D1::ColorF(15.0f / 255.0f, 115.0f / 255.0f, 15.0f / 255.0f);
const D2D1::ColorF BLUE = D2D1::ColorF(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f);
const D2D1::ColorF WHITE = D2D1::ColorF(1.0f, 1.0f, 1.0f);
const D2D1::ColorF BLACK = D2D1::ColorF(0.0f, 0.0f, 0.0f);
const D2D1::ColorF YELLOW = D2D1::ColorF(1.0f, 1.0f, 153.0f / 255.0f);
const D2D1::ColorF CYAN = D2D1::ColorF(51.0f / 255.0f, 187.0f / 255.0f, 204.0f / 255.0f);
const D2D1::ColorF MAGENTA = D2D1::ColorF(204.0f / 255.0f, 51.0f / 255.0f, 204.0f / 255.0f);
const D2D1::ColorF GRAY = D2D1::ColorF(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f);
const D2D1::ColorF TextBoxColor = D2D1::ColorF(0, 0, 0, .6f);
const D2D1::ColorF TextBoxColorLight = D2D1::ColorF(0.5f, 0.5f, 0.5f, .5f);

const D2D1::ColorF SOFT_PINK = D2D1::ColorF(255.0f / 255.0f, 183.0f / 255.0f, 197.0f / 255.0f);
const D2D1::ColorF TRANSPARENT_COLOR = D2D1::ColorF(0, 0, 0, 0);
const D2D1::ColorF InputTextColor = WHITE;
const D2D1::ColorF HighlightTextColor = RED;

const int fontHeight = 72;
inline D2D1::ColorF BackgroundColor = TRANSPARENT_COLOR;



struct GUIText;
struct GUI_Image;
struct GUIBox;
struct GUITextCollection;

Util::Vector2 PxToNormalized(Util::Vector2 Position);
Util::RECTF PxToNormalized(Util::RECTF Rect);
Util::Vector2 NormalizedToPx(Util::Vector2 Position);
Util::RECTF NormalizedToPx(Util::RECTF Rect);
Util::RECTF GetStringRectN(std::wstring text, int fontHeight, int fontmodifier, Util::Vector2 originN, bool IncludeMarkup);
Util::RECTF GetSubStringRectN(std::wstring text, std::wstring substring, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup,int StartIndex = 0);
Util::RECTF GetTightStringRectN(Util::RECTF rectN, float percentage = 0.20f);
Util::Vector2 GetTextAlignCenter(std::wstring text,int fontHeight, int fontmodifier, Util::Vector2 origin);
D2D1::ColorF GetColorCode(std::wstring code);

struct BackgroundImage
{
	BackgroundImage() :imagePath(L"") {}
	BackgroundImage(std::wstring path) :imagePath(path) {}
	std::wstring imagePath;
};


class RenderBuffer {
public:
	static inline Util::Vector2 BufferSize = Util::Vector2(3840, 2160);
	static inline std::map<std::wstring, ID2D1Bitmap*> Bitmaps;
	static inline std::map<std::wstring, Gdiplus::Bitmap*> MouseHoverBitmaps;
	static inline ID2D1BitmapRenderTarget* RenderTarget = nullptr;
};


class OutputBuffer {
public:
	static inline HWND TargetWindow;
	static inline ID2D1HwndRenderTarget* RenderTarget = nullptr;
	static Util::RECTF GetOutputBufferRect();
	static Util::Vector2 WindowPosToRenderBufferPos(Util::Vector2 PosClient);
};

class GUI {
public:
	enum class ControlType { GUIImage, GUIBox, GUIText, GUIInputBox};

	GUI(Util::RECTF rect, int selectionIndex = -1);

	int GetID() const { return ID; }
	bool IsSelectable() const { return SelectionIndex != -1; }
	Util::RECTF GetRectN() const { return RectN; }
	Util::RECTF GetRectP() const { return NormalizedToPx(RectN); }
	void SetRectN(Util::RECTF rectN){RectN = rectN;}
	void SetRotation(float rot) { rotation = rot; }
	virtual bool IsHovered(Util::Vector2) = 0;
	virtual ControlType GetType() const = 0;
	virtual void Render(ID2D1RenderTarget*) = 0;
	static int GetGroupID();

	bool Selected = false;
	int SelectionIndex;

protected:
	int ID;
	Util::RECTF RectN;
	float rotation = 0;

private:
	static inline int counter;
};

struct GUI_Image : public GUI
{
	GUI_Image(const std::wstring& imagePath, Util::Vector2 pos, float size, int SelectionIndex);

	void DeleteImage() const;

	bool IsHovered(Util::Vector2 MousePosClient) override;
	Util::RECTF GetPhysicsRect();
	ControlType GetType() const override { return ControlType::GUIImage; }
	void Render(ID2D1RenderTarget* Target) override;

	std::wstring imagePath;
	float size;
};

struct GUIBox : public GUI
{
	GUIBox(D2D1::ColorF color, Util::RECTF rect, int width = 0, int selectionindex = -1, D2D1::ColorF selectioncolor = D2D1::ColorF(0, 0, 0, 0));

	bool IsHovered(Util::Vector2 MousePosClient) override;
	ControlType GetType() const override { return ControlType::GUIBox; }

	void Render(ID2D1RenderTarget* Target) override;

	D2D1::ColorF color;
	int borderWidth = 0;
	D2D1::ColorF selectionColor;
};


struct GUIText : public GUI
{
public:
	std::wstring text;
	D2D1::ColorF color;
	int fontModifier = 0;
	D2D1::ColorF highlightColor;
	D2D1::ColorF selectionColor;
	bool wrap = false;

	bool IsHovered(Util::Vector2 MousePosClient) override;
	ControlType GetType() const override { return ControlType::GUIText; }
	void Render(ID2D1RenderTarget* Target) override;
	GUIText(int guiID, const std::wstring& t, const Util::RECTF& r, const D2D1::ColorF& c, const int& f, const D2D1::ColorF& highlightColor, const D2D1::ColorF& selectionColor, const int index);
};


struct GUI_InputBox : public GUI {
	GUI_InputBox(Util::RECTF rect, D2D1::ColorF textColor, D2D1::ColorF boxColor,bool centerInput,bool wrapText,bool dynamicSizing,  int selectionIndex = -1): GUI(rect, selectionIndex), TextColor(textColor), BoxColor(boxColor), CenterInput(centerInput),AllowWrap(wrapText),DynamicBox(dynamicSizing)
	{
		MinWidth = min(rect.Width, .2f);
	}

	bool IsHovered(Util::Vector2 MousePosClient) override;
	ControlType GetType() const override { return ControlType::GUIInputBox; }
	void Render(ID2D1RenderTarget* Target) override;

	Util::Vector2 GetCursorPos()
	{
		return GUITexts.size()> 0? cursorPos :CenterInput?Util::Vector2(RectN.GetCenter().X,RectN.Top):RectN.GetTopLeft();
	}

	int GetLineCount() const
	{
		return static_cast<int>(GUITexts.size());
	}

	Util::RECTF GetLineRect(int LineIndex) const
	{
		return GUITexts[LineIndex].GetRectN();
	}

	std::vector<GUIText> GUITexts;
	std::optional<GUIText> Cursor;
	std::optional<GUIBox> InputBox;
	std::optional<GUIBox> IMEBox;
	std::vector<GUIText> IMETextItems;
	bool CenterInput = true;
	D2D1::ColorF TextColor;
	D2D1::ColorF BoxColor;
	Util::Vector2 cursorPos;
	bool DisplayCursor = true;
	bool AllowWrap = true;
	bool DynamicBox = false;
	int CursorBlinkInterval = 500;
	float MinWidth;
	int WidthIncrement = 4;
	float WrapThreshold = .8f;
};




struct GraphicsState
{
	bool DebugFrame = false;
	BackgroundImage backgroundImage = {};
	std::vector<GUI_InputBox> GUIInputBoxes = {};
	std::vector<GUIText> GUITextItems = {};
	std::vector<GUIBox> TextBoxes = {};
	std::vector<GUI_Image> Images = {};
	std::vector<GUI*> AllGUIItems;
};


void LoadGUIImage(std::wstring img);
void RenderFrame(GraphicsState* Frame);
void Direct2DStartup(HWND hwnd);
void Direct2DShutdown();
void ReleaseBuffers();
void CreateBuffers(HWND handle);
void DrawBackground(ID2D1RenderTarget* Target, BackgroundImage backgroundImage, D2D1::ColorF backgroundColor = D2D1::ColorF(0, 0, 0, 0));
Util::Vector2 GetTextDimensionsP(const std::wstring& text, int fontHeight, int fontModifier);