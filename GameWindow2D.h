#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

#include "CPPUtil.h"




enum CursorType { CT_ARROW, CT_HAND };
struct TrayIconData
{
	HICON hIcon = nullptr; // the icon to use
	std::wstring title; // the title for the icon
	std::wstring NotificationTitle; // Title of the notification
	std::wstring NotificationMessage; // Notification message content
	int DurationMs = 0; // Duration for the notification
	std::vector<std::wstring> menuItems; // the items in the menu
	NOTIFYICONDATA CurrentTrayData = {}; // the notify icon data
	std::function<void(int)> callback; // the callback function
	bool isUpdated = false; // the flag indicating whether the icon data has been updated
};

typedef LRESULT(CALLBACK* WndProcFunction)(HWND, UINT, WPARAM, LPARAM);

class CloseWindowException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Window Close Signal Received";
	}
};

static LRESULT CALLBACK ProcessWindowMessage(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


class GameWindow2D
{
public:
	GameWindow2D(float screenWidthPercent, float screenHeightPercent);

    void Run(int FPS, bool Show);


	HWND m_hwnd;
	HMENU hMenu;
	HICON m_hIcon;


	std::function<void()> onLoad;
	std::function<void(int width, int height)> onResize;
	std::function<void(double deltaTime)> onUpdateFrame;
	std::function<void()> onRenderFrame;
	std::function<void()> onClose;

	TrayIconData trayIconData;

	std::condition_variable cv;
	std::mutex cv_m;
	std::atomic<bool> RunWindow {false};

	bool DestroyWindowFlag = false;
	bool WindowOpen = false;
	int showWindowCommand = SW_SHOW;
	DWORD windowStyle = WS_OVERLAPPEDWINDOW;
	std::thread WindowThread;

	int TargetFrameRate = 60;
	int FrameRate = 0;
	LPCWSTR currentCursor = IDC_ARROW;
	bool cursorEnabled = true;

	void EnableCursor(bool enable) 
	{
		cursorEnabled = enable;
		while ((enable && ShowCursor(TRUE) < 0) || (!enable && ShowCursor(FALSE) >= 0));
	}
	void SetCursorSelectableHovered(bool enable)
	{
		if (enable) { currentCursor = IDC_HAND; }
		else { currentCursor = IDC_ARROW; }
	}
	void updateTrayIconData(HICON hIcon, std::wstring title, std::vector<std::wstring> menuItems, std::function<void(int)> callback);
	void ShowWindow(bool Centered);
	void HideWindow();
	void CloseWindow();
	void UpdateFps(std::chrono::high_resolution_clock::time_point start, float targetFPS);
	void ShowTrayNotification(std::wstring title, std::wstring message, int durationMs);
	void ProcessTray();
	void UpdateWindowSize(float screenWidthPercent = 0, float screenHeightPercent = 0, bool center = false, bool Update = true);
};