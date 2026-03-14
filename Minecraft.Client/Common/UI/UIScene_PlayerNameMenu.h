#pragma once

#include "UIScene.h"

class UIScene_PlayerNameMenu : public UIScene
{
private:
	enum EControls
	{
		eControl_NameInput,  // maps to SignEntryMenu "Line1"
		eControl_Save        // maps to SignEntryMenu "Confirm"
	};

	wstring m_name;
	UIControl_TextInput m_textName;
	UIControl_Button m_buttonSave;
#ifdef _WINDOWS64
	bool m_bNeedsInitialEdit;
#endif

	// Reuse SignEntryMenu movie (Line1 + Confirm); no Cancel button - use Back key
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_textName, "Line1")
		UI_MAP_ELEMENT(m_buttonSave, "Confirm")
	UI_END_MAP_ELEMENTS_AND_NAMES()

public:
	UIScene_PlayerNameMenu(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_PlayerNameMenu();

	virtual EUIScene getSceneType() { return eUIScene_PlayerNameMenu; }
	virtual void updateTooltips();
	virtual void tick();

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);
#ifdef _WINDOWS64
	virtual void getDirectEditInputs(vector<UIControl_TextInput*> &inputs);
	virtual void onDirectEditFinished(UIControl_TextInput *input, UIControl_TextInput::EDirectEditResult result);
	virtual bool handleMouseClick(F32 x, F32 y);
#endif

protected:
	void handlePress(F64 controlId, F64 childId);
	void applyNameAndBack();
};
