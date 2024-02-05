#pragma once
#include "Graphics.h"
#include "GraphicsInterface.h"

using namespace GraphicsInterface;


struct NoReturnValue {};

template<typename ResultType>
struct MenuResultProxy {
	ResultType Result;
};

template<typename ResultType = NoReturnValue>
class Menu
{
protected:
	Menu(bool hideBelow = false, bool disableBelow = true) : hideBelow_(hideBelow), disableBelow_(disableBelow) {}


public:
	MenuResultProxy<ResultType> Display()
	{
		int ExistingLayerLevel = Interface::PushLayer(hideBelow_, disableBelow_) - 1;
		try
		{
			Show();
		}
		catch (const CloseWindowException&)
		{
			Interface::PopAllLayers();
			Interface::SetPauseRendering(false);
			throw; 
		}
		int newback = Interface:: PopLayer();
		Util_Assert(newback == ExistingLayerLevel, L"GUI Layer leak detected!");
		Interface::SetPauseRendering(false);
		result_.displayCalled = true; 
		return { Result() };
	}

public:
	ResultType Result()
	{
		Util_Assert(result_.displayCalled, L"Display() was not called");
		if (!std::is_same<ResultType, NoReturnValue>::value)
		{
			Util_Assert(result_.isSet, L"Result was not set");
		}
		return result_.value;
	}

protected:

	void SetMenuResult(const ResultType& value)
	{
		result_.value = value;
		result_.isSet = true;
	}

	bool MenuResultSet()
	{
		return result_.isSet;
	}

	void EnableSelectableWrapAround(bool enable)
	{
		SelectableWrapAround_ = enable;
	}

	void EnableMaintainSelectionAfterMouseHover(bool enable)
	{
		MaintainSelectionAfterMouseHover_ = enable;
	}

	bool GetSelectableWrapAround()  {
		return SelectableWrapAround_;
	}

	bool GetMaintainSelectionAfterMouseHover()  {
		return MaintainSelectionAfterMouseHover_;
	}

private:
	virtual void Show() = 0;
	bool hideBelow_;
	bool disableBelow_;
	struct MenuResult
	{
		ResultType value = {};
		bool displayCalled = false;
		bool isSet = false;
	} result_;

	bool SelectableWrapAround_ = true;
	bool MaintainSelectionAfterMouseHover_ = true;
};


