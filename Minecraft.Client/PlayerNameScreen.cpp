#include "stdafx.h"
#include "PlayerNameScreen.h"
#include "Button.h"
#include "User.h"
#include "..\Minecraft.World\StringHelpers.h"
#ifdef _WINDOWS64
#include <Windows.h>
#endif

const wstring PlayerNameScreen::allowedChars = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";

PlayerNameScreen::PlayerNameScreen(Screen *lastScreen, const wstring& currentName)
{
    frame = 0;
    this->lastScreen = lastScreen;
    this->name = currentName;
    title = L"Player name";
}

void PlayerNameScreen::init()
{
    buttons.clear();
    Keyboard::enableRepeatEvents(true);
    buttons.push_back(new Button(0, width / 2 - 100, height / 4 + 24 * 5, L"Save"));
    buttons.push_back(new Button(1, width / 2 - 100, height / 4 + 24 * 6, L"Cancel"));
    buttons[0]->active = trimString(name).length() >= 1 && trimString(name).length() <= 16;
}

void PlayerNameScreen::removed()
{
    Keyboard::enableRepeatEvents(false);
}

void PlayerNameScreen::tick()
{
    frame++;
}

void PlayerNameScreen::buttonClicked(Button *button)
{
    if (!button->active) return;

    if (button->id == 0 && trimString(name).length() >= 1 && trimString(name).length() <= 16)
    {
        wstring newName = trimString(name);
        if (minecraft->user != nullptr)
        {
            minecraft->user->name = newName;
        }
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
        minecraft->setScreen(lastScreen);
    }
    if (button->id == 1)
    {
        minecraft->setScreen(lastScreen);
    }
}

void PlayerNameScreen::keyPressed(wchar_t ch, int eventKey)
{
    if (eventKey == Keyboard::KEY_BACK && name.length() > 0)
        name = name.substr(0, name.length() - 1);
    if (allowedChars.find(ch) != wstring::npos && name.length() < 16)
    {
        name += ch;
    }
    if (buttons.size() > 0)
        buttons[0]->active = trimString(name).length() >= 1 && trimString(name).length() <= 16;
}

void PlayerNameScreen::render(int xm, int ym, float a)
{
    renderBackground();
    drawCenteredString(font, title, width / 2, 40, 0xffffff);

    int bx = width / 2 - 100;
    int by = height / 2 - 10;
    int bw = 200;
    int bh = 20;
    fill(bx - 1, by - 1, bx + bw + 1, by + bh + 1, 0xffa0a0a0);
    fill(bx, by, bx + bw, by + bh, 0xff000000);
    drawString(font, name + (frame / 6 % 2 == 0 ? L"_" : L""), bx + 4, by + (bh - 8) / 2, 0xe0e0e0);

    Screen::render(xm, ym, a);
}
