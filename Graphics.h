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



struct GUIText;
struct GUI_Image;
struct GUIBox;
struct GUITextCollection;

Util::Vector2 PxToNormalized(Util::Vector2 Position);
Util::RECTF PxToNormalized(Util::RECTF Rect);
Util::Vector2 NormalizedToPx(Util::Vector2 Position);
Util::RECTF NormalizedToPx(Util::RECTF Rect);
Util::RECTF GetStringRectN(std::wstring text, int fontHeight, int fontmodifier, Util::Vector2 originN, bool IncludeMarkup, bool IncludeCursor);
Util::RECTF GetSubStringRectN(std::wstring text, std::wstring substring, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup, bool IncludeCursor,int StartIndex = 0);
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
	static inline HWND WindowHandle;
	static inline ID2D1HwndRenderTarget* RenderTarget = nullptr;
	static inline int CurrentWidth = 2560;
	static inline int CurrentHeight = 1440;

	static Util::RECTF GetOutputBufferRect()
	{
		// Get DPI for the window
		float dpi = static_cast<float>(GetDpiForWindow(WindowHandle));
		float refdpi = 96.0f;

		// Calculate scaling ratios separately for width and height
		float widthRatio = CurrentWidth * refdpi / RenderBuffer::BufferSize.X / dpi;
		float heightRatio = CurrentHeight * refdpi / RenderBuffer::BufferSize.Y / dpi;
		float minRatio = (std::min)(widthRatio, heightRatio); // Use std::min to find the smaller ratio

		// Calculate scaled dimensions separately for width and height
		int scaledWidth = static_cast<int>(RenderBuffer::BufferSize.X * minRatio);
		int scaledHeight = static_cast<int>(RenderBuffer::BufferSize.Y * minRatio);

		// Calculate the start position for the image to be centered, separately for x and y
		float startX = (CurrentWidth * refdpi / dpi - scaledWidth) / 2.0f;
		float startY = (CurrentHeight * refdpi / dpi - scaledHeight) / 2.0f;

		// Create and return the final Rect
		return Util::RECTF(startX, startY, startX + scaledWidth, startY + scaledHeight);
	}

	static Util::Vector2 WindowPosToRenderBufferPos(Util::Vector2 PosClient)
	{
		float dpi = static_cast<float>(GetDpiForWindow(WindowHandle));
		// Scale cursor position with DPI
		Util::Vector2 DpiScaledPos = PosClient * 96.0f / dpi;

		// Get the scaled and centered Rect
		Util::RECTF rect = OutputBuffer::GetOutputBufferRect();
		if (rect.Right - rect.Left == 0 || rect.Bottom - rect.Top == 0) { return Util::Vector2(0, 0); }
		// Calculate scale ratios
		Util::Vector2 Ratio = rect.GetSize() / RenderBuffer::BufferSize;

		// Transform mouse position to bitmap's coordinate system
		Util::Vector2 TransformedPos = (DpiScaledPos - rect.GetTopLeft()) / Ratio;
		return TransformedPos;
	}

};

class GUI {
public:
	enum class ControlType { GUIImage, GUIBox, GUIText };

	GUI(Util::RECTF rect, int selectionIndex = -1) : ID(++counter), SelectionIndex(selectionIndex), RectN(rect)
	{
		if (counter > 999999) { counter = 0; }
	}

	int GetID() const { return ID; }
	bool IsSelectable() { return SelectionIndex != -1; }
	Util::RECTF GetRectN() { return RectN; }
	Util::RECTF GetRectP() { return NormalizedToPx(RectN); }
	void SetRectN(Util::RECTF rectN){RectN = rectN;}
	void SetRotation(float rot) { rotation = rot; }
	virtual bool IsHovered(Util::Vector2) = 0;
	virtual ControlType GetType() const = 0;
	virtual void Render(ID2D1RenderTarget*) = 0;
	static int GetGroupID()
	{
		counter++;
		if (counter > 999999) { counter = 0; }
		return counter;
	}

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
	GUI_Image(const std::wstring& imagePath, Util::Vector2 pos, float size, int SelectionIndex) : GUI(Util::RECTF(pos), SelectionIndex), imagePath(imagePath), size(size)
	{
		Gdiplus::Bitmap originalBitmap = Gdiplus::Bitmap(imagePath.c_str());
		Gdiplus::Bitmap* resizedBitmap = new Gdiplus::Bitmap(static_cast<INT>(originalBitmap.GetWidth() * size), static_cast<INT>(originalBitmap.GetHeight() * size), originalBitmap.GetPixelFormat());
		Gdiplus::Graphics graphics(resizedBitmap);
		graphics.DrawImage(&originalBitmap, 0, 0, resizedBitmap->GetWidth(), resizedBitmap->GetHeight());
		RenderBuffer::MouseHoverBitmaps[imagePath] = resizedBitmap;
		Util::Vector2 pixelSizeNCenter = PxToNormalized(Util::Vector2(static_cast<float>(resizedBitmap->GetWidth()), static_cast<float>(resizedBitmap->GetHeight()))) / 2.0f;
		RectN = Util::RECTF(RectN.Left-pixelSizeNCenter.X, RectN.Top - pixelSizeNCenter.Y, RectN.Left + pixelSizeNCenter.X, RectN.Top + pixelSizeNCenter.Y);
	}

	void DeleteImage()
	{
		delete RenderBuffer::MouseHoverBitmaps[imagePath];
		RenderBuffer::MouseHoverBitmaps.erase(imagePath);
		RenderBuffer::Bitmaps[imagePath]->Release();
		RenderBuffer::Bitmaps.erase(imagePath);
	}

	bool IsHovered(Util::Vector2 MousePosClient) override;
	Util::RECTF GetPhysicsRect();
	ControlType GetType() const override { return ControlType::GUIImage; }
	void Render(ID2D1RenderTarget* Target) override;

	std::wstring imagePath;
	float size;
};

struct GUIBox : public GUI
{
	GUIBox(D2D1::ColorF color, Util::RECTF rect, int width = 0, int selectionindex = -1, D2D1::ColorF selectioncolor = D2D1::ColorF(0, 0, 0, 0))
		: GUI(rect, selectionindex), color(color), borderWidth(width), selectionColor(selectioncolor) {}

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
	int fontModifier;
	D2D1::ColorF highlightColor;
	D2D1::ColorF selectionColor;
	bool wrap = false;

	bool IsHovered(Util::Vector2 MousePosClient) override;
	ControlType GetType() const override { return ControlType::GUIText; }
	void Render(ID2D1RenderTarget* Target) override;

	Util::RECTF GetTightStringRectP(float percentage = 0.20f) { return NormalizedToPx(GetTightStringRectN(RectN)); }
	GUIText(int guiID, const std::wstring& t, const Util::RECTF& r, const D2D1::ColorF& c, const int& f, const D2D1::ColorF& hc, const D2D1::ColorF& sc, const int index)
		: GUI(r, index), text(t), color(c), fontModifier(f), highlightColor(hc), selectionColor(sc)
	{
		if (guiID != -1) { ID = guiID; }
	}
};

struct GraphicsState
{
	bool DebugFrame = false;
	BackgroundImage backgroundImage = {};
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