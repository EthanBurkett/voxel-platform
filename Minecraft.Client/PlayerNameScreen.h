#pragma once
#include "Screen.h"

class PlayerNameScreen : public Screen
{
private:
    Screen *lastScreen;
protected:
    wstring title;
private:
    wstring name;
    int frame;
public:
    PlayerNameScreen(Screen *lastScreen, const wstring& currentName);
    virtual void init();
    virtual void removed();
    virtual void tick();
protected:
    virtual void buttonClicked(Button *button);
private:
    static const wstring allowedChars;
protected:
    virtual void keyPressed(wchar_t ch, int eventKey);
public:
    virtual void render(int xm, int ym, float a);
};
