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
	OutputBuffer::WindowHandle = handle;
	OutputBuffer::CurrentWidth = static_cast<int>(Util::GetClientRect(handle).Width);
	OutputBuffer::CurrentHeight = static_cast<int>(Util::GetClientRect(handle).Height);
	Util_D2DCall(pD2DFactory->CreateHwndRenderTarget(RenderTargetProperties(), HwndRenderTargetProperties(handle, SizeU(OutputBuffer::CurrentWidth, OutputBuffer::CurrentHeight)), &OutputBuffer::RenderTarget));
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
	if (!RenderEngineInitialized) { return; }
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

void DrawBackground(ID2D1RenderTarget* Target, BackgroundImage backgroundImage, D2D1::ColorF backgroundColor)
{
	if (backgroundImage.imagePath.empty()) { return; }
	Target->SetTransform(D2D1::Matrix3x2F::Identity());
	if (RenderBuffer::Bitmaps[backgroundImage.imagePath] == nullptr) { LoadGUIImage(backgroundImage.imagePath); }
	Target->DrawBitmap(RenderBuffer::Bitmaps[backgroundImage.imagePath], NormalizedToPx(Util::RECTF(0, 0, 1, 1)), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
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


void GUIText::Render(ID2D1RenderTarget* Target)
{
	Util::RECTF rect = GetRectP();
	Target->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, rect.GetCenter()));

	if (highlightColor.a != 0)
	{
		ID2D1SolidColorBrush* HighlightBrush = nullptr;
		Util_D2DCall(Target->CreateSolidColorBrush(highlightColor, &HighlightBrush));
		Target->FillRectangle(GetTightStringRectP(), HighlightBrush);
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
	IDWriteTextFormat* pTextFormat = nullptr;
	Util_D2DCall(pDWriteFactory->CreateTextFormat(L"Yu Mincho", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<float>(fontHeight + fontModifier), L"", &pTextFormat));
	Util_Assert(pTextFormat, L"pTextFormat was null.");
	IDWriteTextLayout* pTextLayout = nullptr;
	Util_D2DCall(pDWriteFactory->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.length()), pTextFormat, 4000.0f, 2000.0f, &pTextLayout));
	Util_Assert(pTextLayout, L"pTextLayout was null.");
	Util_D2DCall(pTextLayout->GetMetrics(&textMetrics));
	pTextLayout->Release();
	pTextFormat->Release();
	return Util::Vector2(textMetrics.width, textMetrics.height);
}

Util::RECTF GetStringRectN(wstring text, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup, bool IncludeCursor)
{
	if (!IncludeMarkup)
	{
		std::wregex colorPattern(L"\\[([^\\[\\]]*?)\\]\\((([RGBYCMW])?,?)?(([+-]\\d+))?\\)");
		text = std::regex_replace(text, colorPattern, L"$1");
	}
	if (!IncludeCursor) { text = Util::ReplaceAllSubStr(text, CursorDesign, L""); }

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

Util::RECTF GetSubStringRectN(wstring text, wstring substring, int fontHeight, int fontModifier, Util::Vector2 originN, bool IncludeMarkup, bool IncludeCursor, int StartIndex)
{
	size_t pos = text.find(substring, StartIndex);

	Util_Assert(pos != string::npos, L"Substring not found in text.")
		wstring preText = text.substr(0, pos);
	Util::Vector2 preTextMetricsP = GetTextDimensionsP(preText, fontHeight, fontModifier);
	Util::Vector2 preTextMetricsN = PxToNormalized(preTextMetricsP);

	Util::Vector2 subTextOriginN = Util::Vector2(originN.X + preTextMetricsN.X, originN.Y);

	return GetStringRectN(text.substr(pos, substring.length()), fontHeight, fontModifier, subTextOriginN, IncludeMarkup, IncludeCursor);
}

Util::RECTF GetTightStringRectN(Util::RECTF rectN, float percentage)
{
	float adjustment = rectN.Height * percentage;
	return Util::RECTF(rectN.Left, rectN.Top + adjustment, rectN.Right, rectN.Bottom - (.25f * adjustment));
}


Util::Vector2 GetTextAlignCenter(wstring text, int fontHeight, int fontmodifier, Util::Vector2 origin)
{
	Util::RECTF textRect = GetStringRectN(text, fontHeight, fontmodifier, origin, false, false);
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

