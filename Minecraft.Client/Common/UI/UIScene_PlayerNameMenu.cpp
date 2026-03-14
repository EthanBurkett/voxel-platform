#include "stdafx.h"
#include "UI.h"
#include "UIScene_PlayerNameMenu.h"
#include "..\..\Minecraft.h"
#include "..\..\User.h"
#include "..\..\..\Minecraft.World\StringHelpers.h"
#ifdef _WINDOWS64
#include <Windows.h>
#include "..\..\Windows64\KeyboardMouseInput.h"
#endif

UIScene_PlayerNameMenu::UIScene_PlayerNameMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	initialiseMovie();

#ifdef _WINDOWS64
	m_bNeedsInitialEdit = true;
#endif

	Minecraft *mc = Minecraft::GetInstance();
	m_name = (mc->user != nullptr && !mc->user->name.empty()) ? mc->user->name : L"Player";

	m_textName.init(UIString(m_name), eControl_NameInput);
	m_textName.SetCharLimit(16);
	m_buttonSave.init(UIString(L"Save"), eControl_Save);

	parentLayer->addComponent(iPad, eUIComponent_MenuBackground);
}

UIScene_PlayerNameMenu::~UIScene_PlayerNameMenu()
{
	m_parentLayer->removeComponent(eUIComponent_MenuBackground);
}

wstring UIScene_PlayerNameMenu::getMoviePath()
{
	// Reuse SignEntryMenu layout (one line + Confirm button); Cancel = Back key
	return L"SignEntryMenu";
}

void UIScene_PlayerNameMenu::updateTooltips()
{
	ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_PlayerNameMenu::tick()
{
	UIScene::tick();
#ifdef _WINDOWS64
	if (m_bNeedsInitialEdit)
	{
		m_bNeedsInitialEdit = false;
		if (g_KBMInput.IsKBMActive())
		{
			SetFocusToElement(eControl_NameInput);
			m_textName.beginDirectEdit(16);
		}
	}
#endif
}

void UIScene_PlayerNameMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
#ifdef _WINDOWS64
	if (isDirectEditBlocking()) { handled = true; return; }
#endif
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);
	switch (key)
	{
	case ACTION_MENU_CANCEL:
		if (pressed)
		{
			navigateBack();
			ui.PlayUISFX(eSFX_Back);
		}
		handled = true;
		break;
	case ACTION_MENU_OK:
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		sendInputToMovie(key, repeat, pressed, released);
		handled = true;
		break;
	}
}

#ifdef _WINDOWS64
void UIScene_PlayerNameMenu::getDirectEditInputs(vector<UIControl_TextInput*> &inputs)
{
	inputs.push_back(&m_textName);
}

void UIScene_PlayerNameMenu::onDirectEditFinished(UIControl_TextInput *input, UIControl_TextInput::EDirectEditResult result)
{
	if (input != &m_textName) return;
	if (result == UIControl_TextInput::eDirectEdit_Cancelled)
		navigateBack();
}

bool UIScene_PlayerNameMenu::handleMouseClick(F32 x, F32 y)
{
	if (m_textName.isDirectEditing())
	{
		m_buttonSave.UpdateControl();
		S32 cx = m_buttonSave.getXPos();
		S32 cy = m_buttonSave.getYPos();
		S32 cw = m_buttonSave.getWidth();
		S32 ch = m_buttonSave.getHeight();
		if (cw > 0 && ch > 0 && x >= cx && x <= cx + cw && y >= cy && y <= cy + ch)
		{
			m_textName.confirmDirectEdit();
			applyNameAndBack();
		}
		return true;
	}
	return UIScene::handleMouseClick(x, y);
}
#endif

void UIScene_PlayerNameMenu::handlePress(F64 controlId, F64 childId)
{
	ui.PlayUISFX(eSFX_Press);
	switch (static_cast<int>(controlId))
	{
	case eControl_Save:
		applyNameAndBack();
		break;
	}
}

void UIScene_PlayerNameMenu::applyNameAndBack()
{
#ifdef _WINDOWS64
	if (m_textName.isDirectEditing())
		m_textName.confirmDirectEdit();
#endif
	const wchar_t *pLabel = m_textName.getLabel();
	wstring newName = pLabel ? trimString(wstring(pLabel)) : L"";
	if (newName.length() < 1 || newName.length() > 16)
		return;

	Minecraft *mc = Minecraft::GetInstance();
	if (mc->user != nullptr)
		mc->user->name = newName;

#ifdef _WINDOWS64
	extern char g_Win64Username[17];
	extern wchar_t g_Win64UsernameW[17];
	size_t len = newName.length();
	if (len > 16) len = 16;
	for (size_t i = 0; i < len; i++)
		g_Win64UsernameW[i] = newName[i];
	g_Win64UsernameW[len] = L'\0';
	WideCharToMultiByte(CP_ACP, 0, g_Win64UsernameW, -1, g_Win64Username, 17, nullptr, nullptr);
	g_Win64Username[16] = '\0';

	char exePath[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	char *lastSlash = strrchr(exePath, '\\');
	if (lastSlash)
		*(lastSlash + 1) = '\0';
	char filePath[MAX_PATH] = {};
	_snprintf_s(filePath, sizeof(filePath), _TRUNCATE, "%susername.txt", exePath);
	FILE *f = nullptr;
	if (fopen_s(&f, filePath, "w") == 0 && f)
	{
		fprintf(f, "%s\n", g_Win64Username);
		fclose(f);
	}
#endif
	navigateBack();
}
