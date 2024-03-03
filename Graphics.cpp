#include "Graphics.h"

using namespace D2D1;
using namespace std;



Util::Vector2 PxToNormalized(Util::Vector2 Position)
{
	return Util::Vector2(Position.X / RenderBuffer::BufferSize.X, Position.Y / RenderBuffer::BufferSize.Y);
}

Util::RECTF PxToNormalized(Util::RECTF Rect)
{
	return Util::RECTF(Rect.Left / RenderBuffer::BufferSize.X, Rect.Top / RenderBuffer::BufferSize.Y, Rect.Right / RenderBuffer::BufferSize.X, Rect.Bottom / RenderBuffer::BufferSize.Y);
}

Util::Vector2 NormalizedToPx(Util::Vector2 Position)
{
	return Util::Vector2(std::floor(Position.X * RenderBuffer::BufferSize.X), std::floor(Position.Y * RenderBuffer::BufferSize.Y));
}

Util::RECTF NormalizedToPx(Util::RECTF Rect)
{
	return Util::RECTF(std::floor(Rect.Left * RenderBuffer::BufferSize.X), std::floor(Rect.Top * RenderBuffer::BufferSize.Y), std::ceil(Rect.Right * RenderBuffer::BufferSize.X), std::ceil(Rect.Bottom * RenderBuffer::BufferSize.Y));
}


Util::RECTF OutputBuffer::GetOutputBufferRect()
{
	// Get DPI for the window
	float dpi = static_cast<float>(GetDpiForWindow(TargetWindow));
	Util::Vector2 WindowDimensions = Util::GetClientRect(TargetWindow).GetSize();
	float refdpi = 96.0f;

	// Calculate scaling ratios separately for width and height
	float widthRatio = WindowDimensions.X * refdpi / RenderBuffer::BufferSize.X / dpi;
	float heightRatio = WindowDimensions.Y * refdpi / RenderBuffer::BufferSize.Y / dpi;
	float minRatio = (std::min)(widthRatio, heightRatio); // Use std::min to find the smaller ratio

	// Calculate scaled dimensions separately for width and height
	int scaledWidth = static_cast<int>(RenderBuffer::BufferSize.X * minRatio);
	int scaledHeight = static_cast<int>(RenderBuffer::BufferSize.Y * minRatio);

	// Calculate the start position for the image to be centered, separately for x and y
	float startX = (WindowDimensions.X * refdpi / dpi - scaledWidth) / 2.0f;
	float startY = (WindowDimensions.Y * refdpi / dpi - scaledHeight) / 2.0f;

	// Create and return the final Rect
	return Util::RECTF(startX, startY, startX + scaledWidth, startY + scaledHeight);
}

Util::Vector2 OutputBuffer::WindowPosToRenderBufferPos(Util::Vector2 PosClient)
{
	float dpi = static_cast<float>(GetDpiForWindow(TargetWindow));
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



GUI_Image::GUI_Image(const std::wstring& imagePath, Util::Vector2 pos, float size, int SelectionIndex) : GUI(Util::RECTF(pos), SelectionIndex), imagePath(imagePath), size(size)
{
	Gdiplus::Bitmap originalBitmap = Gdiplus::Bitmap(imagePath.c_str());
	Gdiplus::Bitmap* resizedBitmap = new Gdiplus::Bitmap(static_cast<INT>(originalBitmap.GetWidth() * size), static_cast<INT>(originalBitmap.GetHeight() * size), originalBitmap.GetPixelFormat());
	Gdiplus::Graphics graphics(resizedBitmap);
	graphics.DrawImage(&originalBitmap, 0, 0, resizedBitmap->GetWidth(), resizedBitmap->GetHeight());
	RenderBuffer::MouseHoverBitmaps[imagePath] = resizedBitmap;
	Util::Vector2 pixelSizeNCenter = PxToNormalized(Util::Vector2(static_cast<float>(resizedBitmap->GetWidth()), static_cast<float>(resizedBitmap->GetHeight()))) / 2.0f;
	RectN = Util::RECTF(RectN.Left - pixelSizeNCenter.X, RectN.Top - pixelSizeNCenter.Y, RectN.Left + pixelSizeNCenter.X, RectN.Top + pixelSizeNCenter.Y);
}

void GUI_Image::DeleteImage() const
{
	delete RenderBuffer::MouseHoverBitmaps[imagePath];
	RenderBuffer::MouseHoverBitmaps.erase(imagePath);
	RenderBuffer::Bitmaps[imagePath]->Release();
	RenderBuffer::Bitmaps.erase(imagePath);
}

bool GUI_InputBox::IsHovered(Util::Vector2 MousePosClient)
{
	Util_Assert(RenderEngineInitialized, L"Render engine not initialized");
	return Util::RotatedRECTF(NormalizedToPx(RectN), rotation).Contains(OutputBuffer::WindowPosToRenderBufferPos(MousePosClient));
}

bool GUIText::IsHovered(Util::Vector2 MousePosClient)
{
	Util_Assert(RenderEngineInitialized, L"Render engine not initialized");
	return Util::RotatedRECTF(NormalizedToPx(RectN), rotation).Contains(OutputBuffer::WindowPosToRenderBufferPos(MousePosClient));
}

bool GUIBox::IsHovered(Util::Vector2 MousePosClient)
{
	Util_Assert(RenderEngineInitialized, L"Render engine not initialized");
	return Util::RotatedRECTF(NormalizedToPx(RectN), rotation).Contains(OutputBuffer::WindowPosToRenderBufferPos(MousePosClient));
}

bool GUI_Image::IsHovered(Util::Vector2 MousePosClient)
{
	Util_Assert(RenderEngineInitialized, L"Render engine not initialized");
	if (!RenderBuffer::MouseHoverBitmaps[imagePath]) { return false; }
	Util::Vector2 mousePos = OutputBuffer::WindowPosToRenderBufferPos(MousePosClient);
	Util::RotatedRECTF transformed = Util::RotatedRECTF(NormalizedToPx(RectN), rotation);
	if (transformed.Contains(mousePos))
	{
		Util::Vector2 temp = transformed.GetTopLeft();
		mousePos -= temp;
		mousePos = Util::Vector2::RotatePoint(mousePos, Util::Vector2(0, 0), -rotation);
		Gdiplus::Color color;
		if (RenderBuffer::MouseHoverBitmaps[imagePath]->GetPixel(static_cast<INT>(std::round(mousePos.X)), static_cast<INT>(std::round(mousePos.Y)), &color) == Gdiplus::Status::Ok)
		{
			if (color.GetA() != 0) { return true; }
		}
	}
	return false;
}

Util::RECTF GUI_Image::GetPhysicsRect()
{
	Util_Assert(RenderEngineInitialized, L"Render engine not initialized");
	Gdiplus::Bitmap* bitmap = RenderBuffer::MouseHoverBitmaps[imagePath];

	int left = static_cast<int>(bitmap->GetWidth()), right = 0, top = static_cast<int>(bitmap->GetWidth()), bottom = 0;
	Gdiplus::Color pixelColor;
	for (int x = 0; x < static_cast<int>(bitmap->GetWidth()); ++x)
	{
		for (int y = 0; y < static_cast<int>(bitmap->GetHeight()); ++y)
		{
			if (bitmap->GetPixel(x, y, &pixelColor) == Gdiplus::Status::Ok)
			{
				int test = pixelColor.GetA();
				if (test > 10)
				{
					if (x < left) { left = x; }
					if (x > right) { right = x; }
					if (y < top) { top = y; }
					if (y > bottom) { bottom = y; }
				}
			}
		}
	}

	if (left > right || top > bottom) { return Util::RECTF(0, 0, 0, 0); } // No non-transparent pixels found

	Util::Vector2 topLeft = Util::Vector2(left, top);
	Util::Vector2 bottomRight = Util::Vector2(right, bottom);
	topLeft = RectN.GetTopLeft()+PxToNormalized(topLeft);
	bottomRight = RectN.GetTopLeft() + PxToNormalized(bottomRight);

	return Util::RECTF(topLeft.X, topLeft.Y, bottomRight.X, bottomRight.Y);
}

void Direct2DShutdown()
{
	if (!RenderEngineInitialized) { return; }
	ReleaseBuffers();
	CoUninitialize();
	if (pD2DFactory != nullptr)
	{
		ULONG refCount = pD2DFactory->Release();
		pD2DFactory = nullptr;
	}
	if (pIWICFactory != nullptr)
	{
		ULONG refCount = pIWICFactory->Release();
		pIWICFactory = nullptr;
	}
	if (pDWriteFactory != nullptr)
	{
		ULONG refCount = pDWriteFactory->Release();
		pDWriteFactory = nullptr;
	}
	Gdiplus::GdiplusShutdown(graphicsEngineToken);
	RenderEngineInitialized = false;
}



void ReleaseBuffers()
{
	if (RenderBuffer::RenderTarget != nullptr)
	{
		ULONG refCount = RenderBuffer::RenderTarget->Release();
		RenderBuffer::RenderTarget = nullptr;
	}
	if (OutputBuffer::RenderTarget != nullptr)
	{
		ULONG refCount = OutputBuffer::RenderTarget->Release();
		OutputBuffer::RenderTarget = nullptr;
	}
	for (auto& bitmap : RenderBuffer::Bitmaps)
	{
		if (bitmap.second != nullptr)
		{
			ULONG refCount = bitmap.second->Release();
			bitmap.second = nullptr;
		}
	}
	RenderBuffer::Bitmaps.clear();
}

void CreateBuffers(HWND handle)
{
	if (!RenderEngineInitialized) { return; }
	ReleaseBuffers();  // Clear existing buffers
	OutputBuffer::TargetWindow = handle;
	Util::Vector2 WindowDimensions = Util::GetClientRect(handle).GetSize();
	Util_D2DCall(pD2DFactory->CreateHwndRenderTarget(RenderTargetProperties(), HwndRenderTargetProperties(handle, SizeU(static_cast<int>(WindowDimensions.X), static_cast<int>(WindowDimensions.Y))), &OutputBuffer::RenderTarget));
	Util_D2DCall(OutputBuffer::RenderTarget->CreateCompatibleRenderTarget(SizeF(RenderBuffer::BufferSize.X, RenderBuffer::BufferSize.Y), &RenderBuffer::RenderTarget));
}

void LoadGUIImage(wstring imagePath)
{
	// Release previously loaded assets if they exist
	if (RenderBuffer::Bitmaps[imagePath] != nullptr) { RenderBuffer::Bitmaps[imagePath]->Release(); }
	IWICBitmapDecoder* pDecoder = nullptr;
	Util_D2DCall(pIWICFactory->CreateDecoderFromFilename(imagePath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder));
	IWICBitmapFrameDecode* pFrame = nullptr;
	Util_D2DCall(pDecoder->GetFrame(0, &pFrame));
	IWICFormatConverter* pConverter = nullptr;
	Util_D2DCall(pIWICFactory->CreateFormatConverter(&pConverter));
	Util_D2DCall(pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeMedianCut));

	// Create a new bitmap and add it to the Buffer's map
	ID2D1Bitmap* newBitmap;
	Util_D2DCall(RenderBuffer::RenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, &newBitmap));
	RenderBuffer::Bitmaps[imagePath] = newBitmap;

	pDecoder->Release();
	pFrame->Release();
	pConverter->Release();

}


void RenderFrame(GraphicsState* Frame)
{
	if (!RenderEngineInitialized || Frame == nullptr) { return; } //When rendering paused frame will be nullptr
	if (RenderBuffer::RenderTarget && OutputBuffer::RenderTarget)
	{
		RenderBuffer::RenderTarget->BeginDraw();
		RenderBuffer::RenderTarget->Clear(ColorF(ColorF::Black));
		if (Frame->DebugFrame) { __debugbreak(); }
		DrawBackground(RenderBuffer::RenderTarget, Frame->backgroundImage);
		for (int i = 0; i < Frame->AllGUIItems.size(); i++)
		{
			Frame->AllGUIItems[i]->Render(RenderBuffer::RenderTarget);
		}
		Util_D2DCall(RenderBuffer::RenderTarget->EndDraw());
		ID2D1Bitmap* RenderBufferImage = nullptr;
		Util_D2DCall(RenderBuffer::RenderTarget->GetBitmap(&RenderBufferImage));
		Util_Assert(RenderBufferImage != nullptr, L"Error: Bitmap was null");
		OutputBuffer::RenderTarget->BeginDraw();
		OutputBuffer::RenderTarget->Clear(ColorF(ColorF::Black));
		OutputBuffer::RenderTarget->DrawBitmap(RenderBufferImage, OutputBuffer::GetOutputBufferRect());
		RenderBufferImage->Release();
		Util_D2DCall(OutputBuffer::RenderTarget->EndDraw());
	}
}


void Direct2DStartup(HWND hwnd)
{
	Util_Assert(!RenderEngineInitialized, L"Render engine already started");
	Util_D2DCall(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
	Util_D2DCall(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory));
	Util_D2DCall(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pIWICFactory));
	Util_D2DCall(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory)));
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&graphicsEngineToken, &gdiplusStartupInput, NULL);
	RenderEngineInitialized = true;
	CreateBuffers(hwnd);
}

GUI::GUI(Util::RECTF rect, int selectionIndex) : ID(++counter), SelectionIndex(selectionIndex), RectN(rect)
{
	if (counter > 999999) { counter = 0; }
}

int GUI::GetGroupID()
{
	counter++;
	if (counter > 999999) { counter = 0; }
	return counter;
}

void DrawBackground(ID2D1RenderTarget* Target, BackgroundImage backgroundImage, D2D1::ColorF backgroundColor)
{
	if (backgroundImage.imagePath.empty()) { return; }
	Target->SetTransform(D2D1::Matrix3x2F::Identity());
	if (RenderBuffer::Bitmaps[backgroundImage.imagePath] == nullptr) { LoadGUIImage(backgroundImage.imagePath); }
	Target->DrawBitmap(RenderBuffer::Bitmaps[backgroundImage.imagePath], NormalizedToPx(Util::RECTF(0, 0, 1, 1)), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}


GUIBox::GUIBox(D2D1::ColorF color, Util::RECTF rect, int width, int selectionindex, D2D1::ColorF selectioncolor): GUI(rect, selectionindex), color(color), borderWidth(width), selectionColor(selectioncolor)
{

}

void GUIBox::Render(ID2D1RenderTarget* Target)
{
	D2D1::ColorF DrawColor = color;
	if (Selected) { DrawColor = selectionColor; }
	Util::RECTF rect = GetRectP();
	ID2D1SolidColorBrush* pBrush = nullptr;
	Util_D2DCall(Target->CreateSolidColorBrush(DrawColor, &pBrush));
	Util_Assert(pBrush, L"pBrush was null.");
	Target->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, rect.GetCenter()));
	if (borderWidth == 0) { Target->FillRectangle(rect, pBrush); }
	else { Target->DrawRectangle(rect, pBrush, static_cast<FLOAT>(borderWidth)); }
	Target->SetTransform(D2D1::Matrix3x2F::Identity());
	pBrush->Release();
}

void GUI_InputBox::Render(ID2D1RenderTarget* Target)
{
	Util::RECTF rect = GetRectP();
	Target->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, rect.GetCenter()));
	if (InputBox) { InputBox->Render(Target); }
	for (int i = 0; i < GUITexts.size(); i++)
	{
		GUITexts[i].Render(Target);
	}
	if (Cursor) { Cursor->Render(Target); }
	if (IMEBox) { IMEBox->Render(Target); }
	for (int i = 0; i < IMETextItems.size(); i++)
	{
		IMETextItems[i].Render(Target);
	}
	Target->SetTransform(D2D1::Matrix3x2F::Identity());
}



void GUI_Image::Render(ID2D1RenderTarget* Target)
{
	if (RenderBuffer::Bitmaps[imagePath] == nullptr) { LoadGUIImage(imagePath); }
	Util::RECTF rect = Selected ? GetRectP().Resize(1.2f, true) : GetRectP();
	Target->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, rect.GetCenter()));
	Target->DrawBitmap(RenderBuffer::Bitmaps[imagePath], rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	//ID2D1SolidColorBrush* pBrush = nullptr;
	//Util_D2DCall(Target->CreateSolidColorBrush(RED, &pBrush));
	//Target->DrawRectangle(rect, pBrush, static_cast<FLOAT>(3));
	Target->SetTransform(D2D1::Matrix3x2F::Identity());
}

GUIText::GUIText(int guiID, const std::wstring& t, const Util::RECTF& r, const D2D1::ColorF& c, const int& f, const D2D1::ColorF& hc, const D2D1::ColorF& sc, const int index): GUI(r, index), text(t), color(c), fontModifier(f), highlightColor(hc), selectionColor(sc)
{
	if (guiID != -1) { ID = guiID; }
}


void GUIText::Render(ID2D1RenderTarget* Target)
{
	Util::RECTF rect = GetRectP();
	Target->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, rect.GetCenter()));

	if (highlightColor.a != 0)
	{
		ID2D1SolidColorBrush* HighlightBrush = nullptr;
		Util_D2DCall(Target->CreateSolidColorBrush(highlightColor, &HighlightBrush));
		Target->FillRectangle(NormalizedToPx(GetTightStringRectN(RectN)), HighlightBrush);
		HighlightBrush->Release();
	}

	IDWriteTextFormat* pTextFormat = nullptr;
	Util_D2DCall(pDWriteFactory->CreateTextFormat(L"Yu Mincho", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, (static_cast<float>(fontHeight) + fontModifier), L"", &pTextFormat));

	if (pTextFormat)
	{
		if (!wrap) { pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP); }
		D2D1::ColorF color = Selected ? selectionColor : this->color;

		if (color.a != 0)
		{
			ID2D1SolidColorBrush* TextBrush = nullptr;
			Util_D2DCall(Target->CreateSolidColorBrush(color, &TextBrush));

			if (TextBrush)
			{
				Target->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), pTextFormat, GetRectP(), TextBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
				TextBrush->Release();
			}
			else { Util_LogErrorTerminate(L"TextBrush was null."); }
		}
		pTextFormat->Release();
	}
	else { Util_LogErrorTerminate(L"PTextFormat was null."); }
	Target->SetTransform(D2D1::Matrix3x2F::Identity());

}


Util::Vector2 GetTextDimensionsP(const wstring& text, int fontHeight, int fontModifier)
{
	DWRITE_TEXT_METRICS textMetrics = {};
	IDWriteFactory* factoryInstance = NULL;
	if (!pDWriteFactory) { Util_D2DCall(DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&factoryInstance))); }
	else { factoryInstance = pDWriteFactory; }
	IDWriteTextFormat* pTextFormat = nullptr;
	Util_D2DCall(factoryInstance->CreateTextFormat(L"Yu Mincho", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<float>(fontHeight + fontModifier), L"", &pTextFormat));
	Util_Assert(pTextFormat, L"pTextFormat was null.");
	IDWriteTextLayout* pTextLayout = nullptr;
	Util_D2DCall(factoryInstance->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.length()), pTextFormat, 4000.0f, 2000.0f, &pTextLayout));
	Util_Assert(pTextLayout, L"pTextLayout was null.");
	Util_D2DCall(pTextLayout->GetMetrics(&textMetrics));
	if (factoryInstance != pDWriteFactory) { factoryInstance->Release(); }
	pTextLayout->Release();
	pTextFormat->Release();
	return Util::Vector2(textMetrics.width, textMetrics.height);
}

Util::RECTF GetStringRectN(wstring text, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup)
{
	if (!IncludeMarkup)
	{
		std::wregex colorPattern(L"\\[([^\\[\\]]*?)\\]\\((([RGBYCMW])?,?)?(([+-]\\d+))?\\)");
		text = std::regex_replace(text, colorPattern, L"$1");
	}
	Util::Vector2 textMetrics = GetTextDimensionsP(text, fontHeight, fontModifier);

	int leadingSpaces = static_cast<int>(text.find_first_not_of(' '));
	int trailingSpaces = static_cast<int>(text.length() - text.find_last_not_of(' ') - 1);

	float spaceWidth = 0.0f;
	if (leadingSpaces > 0 || trailingSpaces > 0)
	{
		// Measure the width of a space by comparing "o o" with "o"
		spaceWidth = GetTextDimensionsP(L"o o", fontHeight, fontModifier).X - (2 * GetTextDimensionsP(L"o", fontHeight, fontModifier).X);
	}

	float textWidth = textMetrics.X + (leadingSpaces + trailingSpaces) * spaceWidth;
	Util::Vector2 textSize = PxToNormalized(Util::Vector2(textWidth, textMetrics.Y));
	float left = originN.X;
	float top = originN.Y;
	float right = originN.X + textSize.X;
	float bottom = originN.Y + textSize.Y;
	Util::RECTF textRect = Util::RECTF(left, top, right, bottom);
	return textRect;
}

Util::RECTF GetSubStringRectN(wstring text, wstring substring, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup, int StartIndex)
{
	size_t pos = text.find(substring, StartIndex);

	Util_Assert(pos != string::npos, L"Substring not found in text.")
		wstring preText = text.substr(0, pos);
	Util::Vector2 preTextMetricsP = GetTextDimensionsP(preText, fontHeight, fontModifier);
	Util::Vector2 preTextMetricsN = PxToNormalized(preTextMetricsP);

	Util::Vector2 subTextOriginN = Util::Vector2(originN.X + preTextMetricsN.X, originN.Y);

	return GetStringRectN(text.substr(pos, substring.length()), fontHeight, fontModifier, subTextOriginN, IncludeMarkup);
}

Util::RECTF GetTightStringRectN(Util::RECTF rectN, float percentage)
{
	float adjustment = rectN.Height * percentage;
	return Util::RECTF(rectN.Left, rectN.Top + adjustment, rectN.Right, rectN.Bottom - (.25f * adjustment));
}


Util::Vector2 GetTextAlignCenter(wstring text, int fontHeight, int fontmodifier, Util::Vector2 origin)
{
	Util::RECTF textRect = GetStringRectN(text, fontHeight, fontmodifier, origin, false);
	return Util::Vector2(textRect.Left - textRect.Width / 2.0f, textRect.Top);
}

D2D1::ColorF GetColorCode(std::wstring code)
{
	if (code == L"R") return RED;
	else if (code == L"G") return GREEN;
	else if (code == L"B") return BLUE;
	else if (code == L"Y") return YELLOW;
	else if (code == L"C") return CYAN;
	else if (code == L"M") return MAGENTA;
	else if (code == L"W") return WHITE;
	else if (code == L"BK") return BLACK;
	else if (code == L"GY") return GRAY;
	else if (code == L"TP") return TRANSPARENT_COLOR;
	throw std::runtime_error("Bad Color Code");
}

