#pragma once
#include "GameWindow2D.h"
#include "Graphics.h"
#include "GraphicsInterface.h"
#include "Menus.h"
#include "Input.h"
#include <iostream>

class DemoMenu : public Menu<std::wstring>
{
public:
	DemoMenu(const std::wstring& prompt):Prompt(prompt){}
private:
	virtual void Show() override;
	std::wstring Prompt;
};

GameWindow2D* G1;