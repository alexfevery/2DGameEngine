# 2D Game Engine - Overview

## Introduction
This 2D game engine provides a comprehensive set of tools and interfaces for developing 2D games. It features a modular architecture, allowing for easy customization and extension. Key components include a game window manager, graphics handling, user input processing, and menu systems.

## Key Features
- **GameWindow2D**: Manages the game window, offering functionality for window creation, resizing, and rendering.
Functions in GameWindow2D
GameWindow2D(float screenWidthPercent, float screenHeightPercent): Constructor to initialize the game window with screen size percentages.
void Run(int FPS, bool Show): Starts the game loop with specified frames per second (FPS) and visibility.
void ShowWindow(bool Centered): Displays the game window, optionally centered.
void HideWindow(): Hides the game window.
void CloseWindow(): Closes the game window.
void ShowTrayNotification(std::wstring title, std::wstring message, int durationMs): Displays a notification in the system tray.
void ProcessTray(): Processes the system tray icon and its interactions.
void UpdateWindowSize(float screenWidthPercent = 0, float screenHeightPercent = 0, bool center = false, bool Update = true): Updates the size of the game window.

- **Graphics & GraphicsInterface**: Handles all graphics-related operations, including image rendering, control manipulation (move, rotate), and frame creation.
- **Input**: Processes various input types such as keyboard and mouse events, allowing for interactive game experiences.
- **Menus**: Supports the creation and management of in-game menus, with customizable options and selections.
