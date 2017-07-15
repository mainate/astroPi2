#include "astrocurses.h"

// Windows instance used by the program
extern objWindow* portWin;
//extern objWindow* verWin;
//extern textWindow* mainWin;
//extern textWindow* ipWin;
//extern buttonWindow* bottomWin;

/***********************************************************************************
*  Class objButton
***********************************************************************************/
objButton::objButton()
{
}

objButton::objButton(activeState_e setActive, void* (_ptrFunction(void*)))
{
    isActive = setActive;
    // Function adress copy to pointer
    functPtr = _ptrFunction;
}

objButton::~objButton()
{
    btnHide();
    delwin(handle);
}

WINDOW* objButton::getHandle()
{
    return handle;
}

activeState_e objButton::btnActive()
{
    return isActive;
}

void objButton::bntActivate(activeState_e _state)
{
    if (isActive != _state)
        isActive = _state;
        btnRedraw();
}

void objButton::btnRedraw()
{
    wattron(handle, COLOR_PAIR(btnActive()));
    wbkgd(handle, COLOR_PAIR(btnActive()));
    wrefresh(handle);
}

void objButton::btnHide()
{
    wattron(handle, COLOR_PAIR(notactive));
    wbkgd(handle, COLOR_PAIR(notactive));
    wclear(handle);
    wrefresh(handle);
}

bool objButton::isSelectedBtn(int _pos_x, int _pos_y, buttonWindow* _win)
{
    if ((btnActive() == active) &&
        (_pos_x >= handle->_begx) && (_pos_x <= (handle->_maxx + handle->_begx + 1)) &&
        (_pos_y >= handle->_begy) && (_pos_y <= (handle->_maxy + handle->_begy + 1)))
    {
        if (functPtr != NULL) functPtr(_win);
        return true;
    }
    else return false;
}

/***********************************************************************************
*  Class squareButton
***********************************************************************************/
squareButton::squareButton(WINDOW* _parentWin, const int _y, const int _x, const char* _textTop, const char* _textBottom, activeState_e _setActive, void* (_ptrFunction(void*))):
    objButton(_setActive, _ptrFunction)
{
    btnTextTop = strdup(_textTop);
    btnTextBottom = strdup(_textBottom);
    // Build container
    handle = (derwin(_parentWin, btnHeight, btnLength, _y, _x));
    wattron(handle, COLOR_PAIR(btnActive()));
    wbkgd(handle, COLOR_PAIR(btnActive()));
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnTextTop) / 2), btnTextTop);
    mvwprintw(handle, 2, (btnLength / 2) - (strlen(btnTextBottom) / 2), btnTextBottom);
    wrefresh(handle);
}

squareButton::~squareButton()
{
}

void squareButton::btnRedraw()
{
    objButton::btnRedraw();
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnTextTop) / 2), btnTextTop);
    mvwprintw(handle, 2, (btnLength / 2) - (strlen(btnTextBottom) / 2), btnTextBottom);
    wrefresh(handle);
}

/***********************************************************************************
*  Class rectangleButton
***********************************************************************************/
rectangleButton::rectangleButton(WINDOW* _parentWin, const int _y, const int _x, const char* _text, activeState_e _setActive, void* (_ptrFunction(void*))):
    objButton(_setActive, _ptrFunction)
{
    btnText = strdup(_text);
    handle = derwin(_parentWin, btnHeight, btnLength, _y, _x);
    wattron(handle, COLOR_PAIR(btnActive()));
    wbkgd(handle, COLOR_PAIR(btnActive()));
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnText) / 2), btnText);
    wrefresh(handle);
}

rectangleButton::~rectangleButton()
{
}

void rectangleButton::btnRedraw()
{
    objButton::btnRedraw();
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnText) / 2), btnText);
    wrefresh(handle);
}

/***********************************************************************************
*  Class smallButton
***********************************************************************************/
smallButton::smallButton(WINDOW* _parentWin, const int _y, const int _x, const char* _text, activeState_e _setActive, void* (_ptrFunction(void*))):
    objButton(_setActive, _ptrFunction)
{
    btnText = strdup(_text);
    handle = derwin(_parentWin, btnHeight, btnLength, _y, _x);
    wattron(handle, COLOR_PAIR(btnActive()));
    wbkgd(handle, COLOR_PAIR(btnActive()));
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnText) / 2), btnText);
    wrefresh(handle);
}

smallButton::~smallButton()
{
}

void smallButton::btnRedraw()
{
    objButton::btnRedraw();
    mvwprintw(handle, 1, (btnLength / 2) - (strlen(btnText) / 2), btnText);
    wrefresh(handle);
}
