#include "astrocurses.h"

extern pthread_mutex_t  mutexNcurses;
extern configurationXML* configXML;
// Windows instance used by the program
//extern objWindow* portWin;
//extern objWindow* verWin;
//extern textWindow* mainWin;
//extern textWindow* ipWin;
//extern buttonWindow* bottomWin;

/***********************************************************************************
*  Class objWindow
***********************************************************************************/
objWindow::objWindow(int _l, int _c, int _y, int _x, void* (_ptrFuncSystem(void*)))
{
    functIndiport = _ptrFuncSystem;
    pthread_mutex_lock(&mutexNcurses);
    // Build container
    set_winContainer(newwin(_l, _c, _y, _x));
    wattron(get_winContainer(), COLOR_PAIR(2));
    wbkgd(get_winContainer(), COLOR_PAIR(2));
    // Buid text window
    box(get_winContainer(), 0, 0);
    set_winContentInfo(subwin(get_winContainer(), _l -2, _c -2, _y +1, _x + 1));
    pthread_mutex_unlock(&mutexNcurses);

    refreshWindow();
}

objWindow::~objWindow()
{
    pthread_mutex_lock(&mutexNcurses);
    delwin(get_winContentInfo());
    delwin(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
}

WINDOW* objWindow::get_winContainer() const
{
    return winContainer;
}

void objWindow::set_winContainer(WINDOW* _winptr)
{
    winContainer = _winptr;
}

WINDOW* objWindow::get_winContentInfo() const
{
    return winContentInfo;
}

void objWindow::set_winContentInfo(WINDOW* _winptr)
{
    winContentInfo = _winptr;
}

void objWindow::refreshWindow()
{
    pthread_mutex_lock(&mutexNcurses);
    wrefresh(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
}

void objWindow::redrawWindow()
{
    pthread_mutex_lock(&mutexNcurses);
    redrawwin(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
}

void objWindow::setRefresh(bool _refresh)
{
    refresh = _refresh;
}

bool objWindow::getRefresh()
{
    return refresh;
}

bool objWindow::isStopThread()
{
    return runThread;
}

void objWindow::startThread()
{
    // Start the thread
    runThread = true;
    if (functIndiport != 0) pthread_create(&threadsWin, NULL, functIndiport, (void*) &mutexNcurses);
}

void objWindow::stopThread()
{
    struct timespec ts;
    runThread = false;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    pthread_timedjoin_np(threadsWin, NULL, &ts);
}
/***********************************************************************************
*  Class textWindow
***********************************************************************************/
textWindow::textWindow(int _l, int _c, int _y, int _x, char* _textTitle, void* (_ptrFuncSystem(void*))) : objWindow(_l, _c, _y, _x, _ptrFuncSystem)
{
    // Build text window
    if (_textTitle)
    {
        pthread_mutex_lock(&mutexNcurses);
        set_winContentTitle(subwin(get_winContainer(), 1, _c -2, _y +1, _x + 1));
        set_winContentInfo(subwin(get_winContainer(), _l -4, _c -2, _y +3, _x +1));
        scrollok(get_winContentInfo(), true);
        pthread_mutex_unlock(&mutexNcurses);
        setTitle(0, _textTitle, true, '\0');
    }
    else
    {
        pthread_mutex_lock(&mutexNcurses);
        set_winContentInfo(subwin(get_winContainer(), _l -2, _c -2, _y +1, _x +1));
        scrollok(get_winContentInfo(), true);
        pthread_mutex_unlock(&mutexNcurses);
    }
    pthread_mutex_lock(&mutexNcurses);
    wrefresh(get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
}

textWindow::~textWindow()
{
    pthread_mutex_lock(&mutexNcurses);
    delwin(get_winContentTitle());
    pthread_mutex_unlock(&mutexNcurses);
}

WINDOW* textWindow::get_winContentTitle() const
{
    return winContentTitle;
}

void textWindow::set_winContentTitle(WINDOW* _winptr)
{
    winContentTitle = _winptr;
}

void textWindow::setTitle(int _line, char* _title, bool _center, const char _separator)
{
    pthread_mutex_lock(&mutexNcurses);
    wattron(get_winContentTitle(), A_BOLD | A_UNDERLINE);
    wmove(get_winContentTitle(), _line, 0);
    wclrtoeol(get_winContentTitle());
    if (!_center) mvwprintw(get_winContentTitle(), _line, 0, _title);
    else mvwprintw(get_winContentTitle(), _line, (get_winContentTitle()->_maxx / 2) - (strlen(_title)/2), _title);
    mvwprintw(get_winContentTitle(), _line, get_winContentTitle()->_maxx -1, &_separator);
    wrefresh(get_winContentTitle());
    wattroff(get_winContentTitle(), A_BOLD | A_UNDERLINE);
    pthread_mutex_unlock(&mutexNcurses);
}

void textWindow::writeline(char* _lineText, int _colPair)
{
    writeline(-1, -1, _lineText, _colPair);
}

void textWindow::writeline(int _line, char* _lineText, int _colPair)
{
    writeline(_line, 0, _lineText, _colPair);
}

void textWindow::writeline(int _line, int _coll, char* _lineText, int _colPair)
{
    pthread_mutex_lock(&mutexNcurses);
    if ((_line <= 0) && (_coll <= 0))
    {
        wprintw(get_winContentInfo(), _lineText);
    }
    else
    {
        wmove(get_winContentInfo(), _line, _coll);
        wclrtoeol(get_winContentInfo());
        mvwprintw(get_winContentInfo(), _line, _coll, _lineText);
    }
    pthread_mutex_unlock(&mutexNcurses);
}

/***********************************************************************************
*  Class buttonWindow
***********************************************************************************/
buttonWindow::buttonWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*))) : objWindow(_l, _c, _y, _x, _ptrFuncSystem), textWindow(_l, _c, _y, _x, _textTitle, NULL)
{
    // set handles to NULL
    for(int i = 0; i<=3; i++) listBtn[i].button = NULL;
    btnType = _btnType;
    showLink(_linkPlace);
}

buttonWindow::~buttonWindow()
{
    for (int i=0; i <= 3; i++) delButton(i);
}

bool buttonWindow::addButton(const char* _name, resp_e _value, int _position, const char* _textTop, const char* _textBottom, void* (_ptrFunction(void*)))
{
    strncpy(listBtn[_position].name, _name, 32);
    listBtn[_position].value = _value;

    pthread_mutex_lock(&mutexNcurses);

    switch (btnType)
    {
    case rectanglebtn :
        listBtn[_position].button = new rectangleButton(get_winContainer(), rectBtnCoord[_position][0], rectBtnCoord[_position][1], _textTop, active, _ptrFunction);
         break;

    case squarebtn :
        listBtn[_position].button = new squareButton(get_winContainer(), squareBtnCoord[_position][0], squareBtnCoord[_position][1], _textTop, _textBottom, active, _ptrFunction);
        break;

    case rectanglebottombtn :
        listBtn[_position].button = new rectangleButton(get_winContainer(),  get_winContainer()->_maxy - 4, rectBtnCoord[_position][1], _textTop, active, _ptrFunction);
        break;

    case squarebottombtn :
        listBtn[_position].button = new squareButton(get_winContainer(), get_winContainer()->_maxy - 5, squareBtnCoord[_position][1], _textTop, _textBottom, active, _ptrFunction);
        break;

    case smallbtn : break;
    }

    wrefresh(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
    return true;
}

void buttonWindow::delButton(const char* _name)
{
    for (int i=0; i <= 3; i++)
    {
        if (strcmp(listBtn[i].name, _name) == 0)
        {
            delButton(i);
            break;
        }
    }
}

void buttonWindow::delButton(int _position)
{
    if (listBtn[_position].button != NULL)
    {
        pthread_mutex_lock(&mutexNcurses);
        delete listBtn[_position].button;
        listBtn[_position].button = NULL;
        pthread_mutex_unlock(&mutexNcurses);
    }
}

void buttonWindow::activButton(const char* _name, activeState_e _active)
{
    for (int i=0; i <= 3; i++)
    {
        if (strcmp(listBtn[i].name, _name) == 0)
        {
            activButton(i, _active);
            break;
        }
    }
}

void buttonWindow::activButton(int _position, activeState_e _active)
{
    if (listBtn[_position].button != NULL)
    {
        pthread_mutex_lock(&mutexNcurses);
        listBtn[_position].button->bntActivate(_active);
        pthread_mutex_unlock(&mutexNcurses);
    }
}

void buttonWindow::activeAllButton()
{
    for (int i=0; i<=3; i++) activButton(i, active);
}

void buttonWindow::deactiveAllButton()
{
    for (int i=0; i<=3; i++) activButton(i, notactive);
}

void buttonWindow::showLink(int8_t _linkPlace)
{
    pthread_mutex_lock(&mutexNcurses);
    switch(_linkPlace)
    {
        case -4:    wmove(winContainer, winContainer->_maxy, squareBtnCoord[3][1] +2);
                    break;
        case -3:    wmove(winContainer, winContainer->_maxy, squareBtnCoord[2][1] +2);
                    break;
        case -2:    wmove(winContainer, winContainer->_maxy, squareBtnCoord[1][1] +2);
                    break;
        case -1:    wmove(winContainer, winContainer->_maxy, squareBtnCoord[0][1] +2);
                    break;
        case 1:     wmove(winContainer, 0, squareBtnCoord[0][1] +2);
                    break;
        case 2:     wmove(winContainer, 0, squareBtnCoord[1][1] +2);
                    break;
        case 3:     wmove(winContainer, 0, squareBtnCoord[2][1] +2);
                    break;
        case 4:     wmove(winContainer, 0, squareBtnCoord[3][1] +2);
                    break;
    };
    if (_linkPlace != 0)
    {
        waddch(winContainer, ACS_DIAMOND);
        waddch(winContainer, ACS_DIAMOND);
    }
    else
    {
        box(winContainer, 0, 0);
    }
    wrefresh(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
}

void buttonWindow::clean()
{
    showLink(0);
    activeAllButton();
    refreshWindow();
//    wrefresh(winContentInfo);
}

resp_e buttonWindow::getClickPos(int _pos_x, int _pos_y)
{
    bool resp = false;
    for (int i=0; i<=3; i++)
    {
        if (listBtn[i].button != NULL)
        {
            resp = listBtn[i].button->isSelectedBtn(_pos_x, _pos_y, this);
            if (resp) return listBtn[i].value;
        }
    }
    if (!resp)
    {
        // Forward to next windowbutton
        if (childWin != NULL) return childWin->getClickPos(_pos_x, _pos_y);
            else return getClickOther(_pos_x, _pos_y);
    }
    return CONTINUE;
}

resp_e buttonWindow::getClickOther(int _pos_x, int _pos_y)
{
    return CONTINUE;
}

bool buttonWindow::isButtonActive(const char* _name)
{
    for (int i=0; i <= 3; i++)
    {
        if (strcmp(listBtn[i].name, _name) == 0)
        {
            if (listBtn[i].button->btnActive() == active)
                return true;
                else return false;
        }

        /*if (strcmp(selectorBtn[i].name, _name) == 0)
        {
            if (selectorBtn[i].button->btnActive() == active)
                return true;
                else return false;
        } */
    }
    return false;
}

/***********************************************************************************
*   Derived Class configIndiWindow
***********************************************************************************/
configIndiWindow::configIndiWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*))): objWindow(_l, _c, _y, _x, _ptrFuncSystem), textWindow(_l, _c, _y, _x, _textTitle, NULL), buttonWindow(_l, _c, _y, _x, _textTitle, _linkPlace, _btnType, _ptrFuncSystem)
{
    pagesList = new pageList_t;
    pagesList->page.pageNumber = -1;
    pagesList->nextPage = NULL;
    pagesList->previousPage = NULL;
    readDriversList();

    actualptr = pagesList;
    pageDisplayed = actualptr->page.pageNumber;

    //Display first page of available/configured driver
    writeGroupTitle(actualptr->page.groupDev, pageDisplayed, pagesList->numPage);
    for (int i=0; i <= actualptr->page.numLine; ++i)
    {
        writeDeviceInfo(actualptr->page.line[i].driverName, actualptr->page.line[i].driverFile, i, actualptr->page.line[i].isSelected);
    }

    pthread_mutex_lock(&mutexNcurses);
    wrefresh(get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
}

configIndiWindow::~configIndiWindow()
{
    pageList_t *tempoptr;
    for (int i= 0; i <= pagesList->numPage; i++)
    {
        tempoptr = pagesList->nextPage;
        delete pagesList;
        pagesList = tempoptr;
    }
}

bool configIndiWindow::isDriverAlreadyInList(const char *name, pageList_t *pList)
{
    // check via recursive call if driver not already in the list
    if (pList->page.numLine <= NBRLINEDISPLAY)
    for (int i = 0; i <= pList->page.numLine; ++i)
    {

        if (strcmp(name, pList->page.line[i].driverFile) == 0)
            return true;
    }
    if (pList->nextPage != NULL)
        return isDriverAlreadyInList(name, pList->nextPage);

    return false;
}

bool configIndiWindow::readDriversList()
{
    actualptr = pagesList;
    int lineNumber = 0;
    int pageNumber = 0;

    // Open the xml file to iterate through
    XMLDocument xmldoc;
    FILE *fileDrivers = fopen(DRIVERSXMLFILE, "r");
    if (xmldoc.LoadFile(fileDrivers) != XML_SUCCESS)
        die("error: open drivers.xml");

    // Get first group
    XMLElement *pListGroup = NULL;
    pListGroup = xmldoc.FirstChildElement("devGroup");

    while (pListGroup != NULL)
    {
        char *groupName = strdup(pListGroup->Attribute("group"));
        strncpy(actualptr->page.groupDev, groupName, 31);
        free(groupName);
        actualptr->page.pageNumber = pageNumber;
        pagesList->numPage = pageNumber;

        XMLElement * pListDevice = pListGroup->FirstChildElement( "device" );

        while (pListDevice != NULL)
        {
            if (lineNumber == NBRLINEDISPLAY)
            {
                lineNumber = 0;
                ++pageNumber;
                pagesList->numPage = pageNumber;
                actualptr->nextPage = new pageList_t;
                actualptr->nextPage->previousPage = actualptr;
                actualptr = actualptr->nextPage;
                actualptr->nextPage = NULL;
                // Get for new page, groupdev name and page number.
                char *groupName = strdup(pListGroup->Attribute("group"));
                strncpy(actualptr->page.groupDev, groupName, 31);
                free(groupName);
                actualptr->page.pageNumber = pageNumber;
            }

            // Get Driver and version form XML file
            char* drvName = strdup(pListDevice->FirstChildElement("driver")->Attribute("name"));
            char* drvFile = strdup(pListDevice->FirstChildElement("driver")->GetText());
            char* drvVersion = strdup(pListDevice->FirstChildElement("version")->GetText());

            // if driver alreaddy in the list don't add it again
            if (!isDriverAlreadyInList(drvFile, pagesList))
            {
                strncpy(actualptr->page.line[lineNumber].driverFile, drvFile, 31);
                strncpy(actualptr->page.line[lineNumber].driverName, drvName, 31);
                strncpy(actualptr->page.line[lineNumber].driverVersion, drvVersion, 31);
                // set the flag isSelectd (From configuration)
                if (configXML->isConfigured(drvFile))
                    actualptr->page.line[lineNumber].isSelected = true;
                    else actualptr->page.line[lineNumber].isSelected = false;

                // Write the number of line for this driver on a page
                actualptr->page.numLine = lineNumber;
                // End testing
                #ifdef DEBUG

                if (pageNumber >= 0)
                {
                    //fprintf(pferror, "%i Lecture d'un device \n", __LINE__);
                    fprintf(pferror, "%i Lecture d'un drivers name : %s\n", __LINE__, actualptr->page.line[lineNumber].driverName);
                    fprintf(pferror, "%i Num page : %i\n", __LINE__, pageNumber);
                    fprintf(pferror, "%i Num ligne : %i\n", __LINE__, actualptr->page.numLine);
                    fprintf(pferror, "%i Compteur ligne : %i\n", __LINE__, lineNumber);
                    //fprintf(pferror, "\n");
                }
                #endif // DEBUG

                ++lineNumber;
            }
            free(drvFile);
            free(drvName);
            free(drvVersion);
            //Point to next device
            pListDevice = pListDevice->NextSiblingElement("device");
        }
        ++pageNumber;
        lineNumber = 0;
        actualptr->nextPage = new pageList_t;
        actualptr->nextPage->previousPage = actualptr;
        actualptr = actualptr->nextPage;
        actualptr->nextPage = NULL;
        pListGroup = pListGroup->NextSiblingElement("devGroup");
    }
    fclose(fileDrivers);
    actualptr = NULL;
    return true;
}

bool configIndiWindow::saveDriversConfig()
{
    // First delete all devices from the config file TODO
    configXML->cleanDrivers();

    // iteration through pList to add selected driver
    actualptr = pagesList;
    while (actualptr != NULL)
    {
        for (int i = 0; i <= 8; i++)
        {
            if (actualptr->page.line[i].isSelected)
                configXML->addDrivers(actualptr->page.line[i].driverFile, actualptr->page.line[i].driverName, actualptr->page.line[i].driverVersion);
        }
        actualptr = actualptr->nextPage;
    }

    // save config on file
    configXML->flushConfig(DRIVERS);
    return true;
}

void configIndiWindow::writeDeviceInfo(char *deviceName, char *fileName, int line, bool active)
{
    int i;
    // Check length line before print
    char deviceN[this->get_winContentInfo()->_maxx +1] = "Device: ";
    strncat(deviceN, deviceName, get_winContentInfo()->_maxx - 7);
    // Fillin with space the line in case of reserse attibute for display
    if ((short)strlen(deviceN) <= get_winContentInfo()->_maxx)
        for(i = strlen(deviceN); i <= get_winContentInfo()->_maxx; i++)
            strcat(deviceN, " ");
    if ((short)strlen(deviceN) <= get_winContentInfo()->_maxx) strcat(deviceN, "\n");

    char fileN[get_winContentInfo()->_maxx +1] = "driver: ";
    strncat(fileN, fileName, get_winContentInfo()->_maxx - 7);
    // Fillin with space the line in case of reserse attibute for display
    if ((short)strlen(fileN) <= get_winContentInfo()->_maxx)
        for(i = strlen(fileN); i <= get_winContentInfo()->_maxx; i++)
            strcat(fileN, " ");
    if ((short)strlen(fileN) <= get_winContentInfo()->_maxx) strcat(fileN, "\n");

    pthread_mutex_lock(&mutexNcurses);
    if (active) wattron(get_winContentInfo(), A_REVERSE);
    // Move the cursor at begin of the correct line on windows screen
    wmove(get_winContentInfo(), (line +1) * 3, 0);
    wprintw(get_winContentInfo(), "%s", deviceN);
    wprintw(get_winContentInfo(), "%s\n", fileN);
    if (active) wattroff(get_winContentInfo(), A_REVERSE);
    pthread_mutex_unlock(&mutexNcurses);
}

void configIndiWindow::writeDeviceInfoNextPage()
{
    // Selection "Next"
    actualptr = actualptr->nextPage;
    writeGroupTitle(actualptr->page.groupDev, actualptr->page.pageNumber, pagesList->numPage);
    pageDisplayed = actualptr->page.pageNumber;

    for (int i=0; i <= actualptr->page.numLine; ++i)
    {
        writeDeviceInfo(actualptr->page.line[i].driverName, actualptr->page.line[i].driverFile, i, actualptr->page.line[i].isSelected);
    }
    // If last page disable next button
    if (pageDisplayed == pagesList->numPage) activButton("NEXT", notactive);
    if (pageDisplayed == 1) activButton("PREVIOUS", active);
    pthread_mutex_lock(&mutexNcurses);
    wrefresh(get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
}

void configIndiWindow::writeDeviceInfoPreviousPage()
{
    //Selection "Previous"
    actualptr = actualptr->previousPage;
    writeGroupTitle(actualptr->page.groupDev, actualptr->page.pageNumber, pagesList->numPage);
    pageDisplayed = actualptr->page.pageNumber;

    for (int i=0; i <= actualptr->page.numLine; ++i)
    {
        writeDeviceInfo(actualptr->page.line[i].driverName, actualptr->page.line[i].driverFile, i, actualptr->page.line[i].isSelected);
    }
    // If first page disable previous button
    if (pageDisplayed == 0) activButton("PREVIOUS", notactive);
    if (pageDisplayed == pagesList->numPage -1) activButton("NEXT", active);
    pthread_mutex_lock(&mutexNcurses);
    wrefresh(get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
}

void configIndiWindow::writeGroupTitle(char *groupdev, int page, int numPages)
{
    pthread_mutex_lock(&mutexNcurses);
    //Clear the part of window without the bottom buttons
    for (int i = 0; i <= winContentInfo->_maxy - 5; i++)
    {
        wmove(winContentInfo, i, 0);
        wclrtoeol(winContentInfo);
    }
    wmove(winContentInfo, 0, 0);
    wattron(winContentInfo, A_BOLD | A_UNDERLINE);
    wprintw(winContentInfo, "%s", groupdev);
    wattroff(winContentInfo, A_BOLD | A_UNDERLINE);
    char text[16];
    sprintf (text, "Page: %d/%d", page +1 , numPages +1);
    mvwprintw(winContentInfo, 0, winContentInfo->_maxx - strlen(text) +1, "%s\n\n", text);
    pthread_mutex_unlock(&mutexNcurses);
}

bool configIndiWindow::lineSelected(int lineNumber)
{
    actualptr->page.line[lineNumber].isSelected = !actualptr->page.line[lineNumber].isSelected;
    if (strlen(actualptr->page.line[lineNumber].driverName) != 0)
    {
        writeDeviceInfo(actualptr->page.line[lineNumber].driverName, actualptr->page.line[lineNumber].driverFile, lineNumber, actualptr->page.line[lineNumber].isSelected);
        pthread_mutex_lock(&mutexNcurses);
        wrefresh(winContentInfo);
        pthread_mutex_unlock(&mutexNcurses);
        return true;
    } else return false;
}

resp_e configIndiWindow::getClickOther(int _pos_x, int _pos_y)
{
    // X and Y are absolute position form screen.
    // should be adapted for the current windows
    _pos_y = _pos_y - winContentInfo->_begy;

    switch (_pos_y)
    {
    case 2:
    case 3:
    case 4: lineSelected(0);
        break;
    case 5:
    case 6:
    case 7: lineSelected(1);
        break;
    case 8:
    case 9:
    case 10: lineSelected(2);
        break;
    case 11:
    case 12:
    case 13: lineSelected(3);
        break;
    case 14:
    case 15:
    case 16: lineSelected(4);
        break;
    case 17:
    case 18:
    case 19: lineSelected(5);
        break;
    case 20:
    case 21:
    case 22: lineSelected(6);
        break;
    case 23:
    case 24:
    case 25: lineSelected(7);
        break;
    case 26:
    case 27:
    case 28: lineSelected(8);
        break;
    }
    return CONTINUE;
}


/***********************************************************************************
*   Derived Class configIndiParamWindow
***********************************************************************************/
configIndiParamWindow::configIndiParamWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*))): objWindow(_l, _c, _y, _x, _ptrFuncSystem), textWindow(_l, _c, _y, _x, _textTitle, NULL), buttonWindow(_l, _c, _y, _x, _textTitle, _linkPlace, _btnType, _ptrFuncSystem)
{
    readParamsList();
    _10000 = (atoi(parameters.ipPort) % 100000) / 10000;
    _1000 = (atoi(parameters.ipPort) % 10000) / 1000;
    _100 = (atoi(parameters.ipPort) % 1000) / 100;
    _10 = (atoi(parameters.ipPort) % 100) / 10;
    _1 = (atoi(parameters.ipPort) % 10) / 1;
}

configIndiParamWindow::~configIndiParamWindow()
{

}

void configIndiParamWindow::initialValue()
{
    char tempo[8];
    //Draw the content of the window
    pthread_mutex_lock(&mutexNcurses);
    mvwprintw(winContentInfo, 3, (winContentInfo->_maxx / 2) - (strlen(parameters.verbose) / 2 - 1), parameters.verbose);
    mvwprintw(winContentInfo, 9, (winContentInfo->_maxx / 2) - (strlen(parameters.maxMemClient) / 2 - 1), parameters.maxMemClient);

    sprintf(tempo,"%d",_10000);
    mvwprintw(winContentInfo, 21, 4, tempo);
    sprintf(tempo,"%d",_1000);
    mvwprintw(winContentInfo, 21, 9, tempo);
    sprintf(tempo,"%d",_100);
    mvwprintw(winContentInfo, 21, 14, tempo);
    sprintf(tempo,"%d",_10);
    mvwprintw(winContentInfo, 21, 19, tempo);
    sprintf(tempo,"%d",_1);
    mvwprintw(winContentInfo, 21, 24, tempo);
    pthread_mutex_unlock(&mutexNcurses);

    //Determination for state of smallbutton
    if (_10000 == 0) activSmallButton("PORT10000M", notactive); else if (_10000 == 6) activSmallButton("PORT10000P", notactive);
    if (_1000 == 0) activSmallButton("PORT1000M", notactive); else if (_1000 == 9) activSmallButton("PORT1000P", notactive);
    if (_100 == 0) activSmallButton("PORT100M", notactive); else if (_100 == 9) activSmallButton("PORT100P", notactive);
    if (_10 == 0) activSmallButton("PORT10M", notactive); else if (_10 == 9) activSmallButton("PORT10P", notactive);
    if (_1 == 0) activSmallButton("PORT1M", notactive); else if (_1 == 9) activSmallButton("PORT1P", notactive);

    pthread_mutex_lock(&mutexNcurses);
    wrefresh(winContentInfo);
    pthread_mutex_unlock(&mutexNcurses);
}

void configIndiParamWindow::readParamsList()
{
    parameters = configXML->getFullParameters();
}

bool configIndiParamWindow::saveParamsConfig()
{
    /* As exemple from driver backup has to be adapted for param backup */
    // First delete all devices from the config file TODO
    configXML->cleanParams();
    configXML->addParams(parameters.verbose, parameters.maxMemClient, parameters.ipPort);
   // save config on file
    configXML->flushConfig(PARAMETERS);
    return true;
}

void configIndiParamWindow::writeDeviceInfo(char *deviceName, char *fileName, int line, bool active)
{
}

void configIndiParamWindow::writeGroupTitle(char *groupdev, int page, int numPages)
{
    pthread_mutex_lock(&mutexNcurses);
    pthread_mutex_unlock(&mutexNcurses);
}

resp_e configIndiParamWindow::getClickOther(int _pos_x, int _pos_y)
{
    // X and Y are absolute position form screen.
    // should be adapted for the current windows
    _pos_y = _pos_y - winContentInfo->_begy;
    _pos_x = _pos_x - winContentInfo->_begx;

    // Line one means option -v
    if ((_pos_y >= smallBtnCoord[0][0]) && (_pos_y <= smallBtnCoord[0][0] + 2))
    {
        // length of the string of the v parameter
        int dim = strlen(parameters.verbose);
        // Minus
//        if ((_pos_x >= smallBtnCoord[0][1]) && (_pos_x <= smallBtnCoord[0][1] + 2) && (dim >= 2))
        if ((_pos_x >= smallBtnCoord[0][1]) && (_pos_x <= smallBtnCoord[0][1] + 2) && (isSmallButtonActive("VERBOSEM")))        {
            parameters.verbose[dim -1] = '\0';
            pthread_mutex_lock(&mutexNcurses);
            mvwprintw(winContentInfo, 3, (winContentInfo->_maxx / 2) - 3, "       ");
            mvwprintw(winContentInfo, 3, (winContentInfo->_maxx / 2) - (strlen(parameters.verbose) / 2 - 1), parameters.verbose);
            wrefresh(winContentInfo);
            pthread_mutex_unlock(&mutexNcurses);
            activSmallButton("VERBOSEP", active);
            if(strlen(parameters.verbose) == 1) activSmallButton("VERBOSEM", notactive);
        }
        else
        // Plus
//        if ((_pos_x >= smallBtnCoord[1][1]) && (_pos_x <= smallBtnCoord[1][1] + 2) && (dim <= 2))
        if ((_pos_x >= smallBtnCoord[1][1]) && (_pos_x <= smallBtnCoord[1][1] + 2) && (isSmallButtonActive("VERBOSEP")))
        {
            strcat(parameters.verbose, "v");
            pthread_mutex_lock(&mutexNcurses);
            mvwprintw(winContentInfo, 3, (winContentInfo->_maxx / 2) - 3, "       ");
            mvwprintw(winContentInfo, 3, (winContentInfo->_maxx / 2) - (strlen(parameters.verbose) / 2 - 1), parameters.verbose);
            wrefresh(winContentInfo);
            pthread_mutex_unlock(&mutexNcurses);
            activSmallButton("VERBOSEM", active);
            if(strlen(parameters.verbose) == 3) activSmallButton("VERBOSEP", notactive);
        }
    } else
        // Line two means option -m
        if ((_pos_y >= smallBtnCoord[2][0]) && (_pos_y <= smallBtnCoord[2][0] + 2))
        {
            int val = atoi(parameters.maxMemClient);
            // Minus
//            if ((_pos_x >= smallBtnCoord[2][1]) && (_pos_x <= smallBtnCoord[2][1] + 2) && (val >= 48))
            if ((_pos_x >= smallBtnCoord[2][1]) && (_pos_x <= smallBtnCoord[2][1] + 2) && (isSmallButtonActive("MEMM")))
            {
                val = val - 16;
                sprintf(parameters.maxMemClient, "%d", val);
                pthread_mutex_lock(&mutexNcurses);
                mvwprintw(winContentInfo, 9, (winContentInfo->_maxx / 2) - 3, "       ");
                mvwprintw(winContentInfo, 9, (winContentInfo->_maxx / 2) - (strlen(parameters.maxMemClient) / 2 - 1), parameters.maxMemClient);
                wrefresh(winContentInfo);
                pthread_mutex_unlock(&mutexNcurses);
                activSmallButton("MEMP", active);
                if(val <= 32) activSmallButton("MEMM", notactive);
            }
            else
            // Plus
//            if ((_pos_x >= smallBtnCoord[3][1]) && (_pos_x <= smallBtnCoord[3][1] + 2) && (val <= 496))
            if ((_pos_x >= smallBtnCoord[3][1]) && (_pos_x <= smallBtnCoord[3][1] + 2) && (isSmallButtonActive("MEMP")))
            {
                val = val + 16;
                sprintf(parameters.maxMemClient, "%d", val);
                pthread_mutex_lock(&mutexNcurses);
                mvwprintw(winContentInfo, 9, (winContentInfo->_maxx / 2) - 3, "       ");
                mvwprintw(winContentInfo, 9, (winContentInfo->_maxx / 2) - (strlen(parameters.maxMemClient) / 2 - 1), parameters.maxMemClient);
                wrefresh(winContentInfo);
                pthread_mutex_unlock(&mutexNcurses);
                activSmallButton("MEMM", active);
                if(val >= 512) activSmallButton("MEMP", notactive);
            }

        } else
        {
            char tempo[2];
            // Line three means option increase port value
            if ((_pos_y >= smallBtnCoord[4][0]) && (_pos_y <= smallBtnCoord[4][0] + 2))
            {
                // 10000
//                if ((_pos_x >= smallBtnCoord[4][1]) && (_pos_x <= smallBtnCoord[4][1] + 2) && (_10000 <= 5))
                if ((_pos_x >= smallBtnCoord[4][1]) && (_pos_x <= smallBtnCoord[4][1] + 2) && (isSmallButtonActive("PORT10000P")))
                {
                    ++_10000;
                    if (!isPortValid()) --_10000;
                    else
                    {
                        sprintf(tempo, "%d", _10000);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 4, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_10000 == 1) activSmallButton("PORT10000M", active);
                        if(_10000 == 6) activSmallButton("PORT10000P", notactive);
                    }
                }
                else
                // 1000
//                if ((_pos_x >= smallBtnCoord[6][1]) && (_pos_x <= smallBtnCoord[6][1] + 2) && (_1000 <= 8))
                if ((_pos_x >= smallBtnCoord[6][1]) && (_pos_x <= smallBtnCoord[6][1] + 2) && (isSmallButtonActive("PORT1000P")))
                {
                    ++_1000;
                    if (!isPortValid()) --_1000;
                    else
                    {
                        sprintf(tempo, "%d", _1000);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 9, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_1000 == 1) activSmallButton("PORT1000M", active);
                        if(_1000 == 9) activSmallButton("PORT1000P", notactive);
                    }
                }
                // 100
//                if ((_pos_x >= smallBtnCoord[8][1]) && (_pos_x <= smallBtnCoord[8][1] + 2) && (_100<= 8))
                if ((_pos_x >= smallBtnCoord[8][1]) && (_pos_x <= smallBtnCoord[8][1] + 2) && (isSmallButtonActive("PORT100P")))
                {
                    ++_100;
                    if (!isPortValid()) --_100;
                    else
                    {
                        sprintf(tempo, "%d", _100);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 14, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_100 == 1) activSmallButton("PORT100M", active);
                        if(_100 == 9) activSmallButton("PORT100P", notactive);
                    }
                }
                else
                // 10
//                if ((_pos_x >= smallBtnCoord[10][1]) && (_pos_x <= smallBtnCoord[10][1] + 2) && (_10 <= 8))
                if ((_pos_x >= smallBtnCoord[10][1]) && (_pos_x <= smallBtnCoord[10][1] + 2) && (isSmallButtonActive("PORT10P")))
                {
                    ++_10;
                    if (!isPortValid()) --_10;
                    else
                    {
                        sprintf(tempo, "%d", _10);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 19, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_10 == 1) activSmallButton("PORT10M", active);
                        if(_10 == 9) activSmallButton("PORT10P", notactive);
                    }
                }
                // 1
//                if ((_pos_x >= smallBtnCoord[12][1]) && (_pos_x <= smallBtnCoord[12][1] + 2) && (_1 <= 8))
                if ((_pos_x >= smallBtnCoord[12][1]) && (_pos_x <= smallBtnCoord[12][1] + 2) && (isSmallButtonActive("PORT1P")))
                {
                    ++_1;
                    if (!isPortValid()) --_1;
                    else
                    {
                        sprintf(tempo, "%d", _1);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 24, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_1 == 1) activSmallButton("PORT1M", active);
                        if(_1 == 9) activSmallButton("PORT1P", notactive);
                    }
                }
            }
            else
            if ((_pos_y >= smallBtnCoord[5][0]) && (_pos_y <= smallBtnCoord[5][0] + 2))
            {
                // Line three means option decrease port value
                // 10000
//                if ((_pos_x >= smallBtnCoord[5][1]) && (_pos_x <= smallBtnCoord[5][1] + 2) && (_10000 >= 1))
                if ((_pos_x >= smallBtnCoord[5][1]) && (_pos_x <= smallBtnCoord[5][1] + 2) && (isSmallButtonActive("PORT10000M")))
                {
                    --_10000;
                    if (!isPortValid()) ++_10000;
                    else
                    {
                        sprintf(tempo, "%d", _10000);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 4, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_10000 <= 5) activSmallButton("PORT10000P", active);
                        if(_10000 == 0) activSmallButton("PORT10000M", notactive);
                    }
                }
                else
                // 1000
//                if ((_pos_x >= smallBtnCoord[7][1]) && (_pos_x <= smallBtnCoord[7][1] + 2) && (_1000 >= 1))
                if ((_pos_x >= smallBtnCoord[7][1]) && (_pos_x <= smallBtnCoord[7][1] + 2) && (isSmallButtonActive("PORT1000M")))
                {
                    --_1000;
                    if (!isPortValid()) ++_1000;
                    else
                    {
                        sprintf(tempo, "%d", _1000);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 9, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_1000 <= 8) activSmallButton("PORT1000P", active);
                        if(_1000 == 0) activSmallButton("PORT1000M", notactive);
                    }
                }
                // 100
//                if ((_pos_x >= smallBtnCoord[9][1]) && (_pos_x <= smallBtnCoord[9][1] + 2) && (_100 >= 1))
                if ((_pos_x >= smallBtnCoord[9][1]) && (_pos_x <= smallBtnCoord[9][1] + 2) && (isSmallButtonActive("PORT100M")))
                {
                    --_100;
                    if (!isPortValid()) ++_100;
                    else
                    {
                        sprintf(tempo, "%d", _100);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 14, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_100 <= 8) activSmallButton("PORT100P", active);
                        if(_100 == 0) activSmallButton("PORT100M", notactive);
                    }
                }
                else
                // 10
//                if ((_pos_x >= smallBtnCoord[11][1]) && (_pos_x <= smallBtnCoord[11][1] + 2) && (_10 >= 1))
                if ((_pos_x >= smallBtnCoord[11][1]) && (_pos_x <= smallBtnCoord[11][1] + 2) && (isSmallButtonActive("PORT10M")))                {
                    --_10;
                    if (!isPortValid()) ++_10;
                    else
                    {
                        sprintf(tempo, "%d", _10);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 19, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_10 <= 8) activSmallButton("PORT10P", active);
                        if(_10 == 0) activSmallButton("PORT10M", notactive);
                    }
                }
                // 1
//                if ((_pos_x >= smallBtnCoord[13][1]) && (_pos_x <= smallBtnCoord[13][1] + 2) && (_1 >= 1))
                if ((_pos_x >= smallBtnCoord[13][1]) && (_pos_x <= smallBtnCoord[13][1] + 2) && (isSmallButtonActive("PORT1M")))
                {
                    --_1;
                    if (!isPortValid()) ++_1;
                    else
                    {
                        sprintf(tempo, "%d", _1);
                        pthread_mutex_lock(&mutexNcurses);
                        mvwprintw(winContentInfo, 21, 24, tempo);
                        wrefresh(winContentInfo);
                        pthread_mutex_unlock(&mutexNcurses);
                        if(_1 <= 8) activSmallButton("PORT1P", active);
                        if(_1 == 0) activSmallButton("PORT1M", notactive);
                    }
                }
            }
            sprintf(parameters.ipPort, "%d", (_10000 * 10000 + _1000 * 1000 + _100 * 100 + _10 * 10 + _1));
        }
    return CONTINUE;
}

bool configIndiParamWindow::isPortValid()
{
    int value = _10000 * 10000 + _1000 * 1000 + _100 * 100 + _10 * 10 + _1;
    if ((value > 65535) || (value < 1024)) return false;
        else return true;
}

bool configIndiParamWindow::addSmallButton(const char* _name, resp_e _value, int _position, const char* _text, void* (_ptrFunction(void*)))
{
    strncpy(selectorBtn[_position].name, _name, 32);
    selectorBtn[_position].value = _value;

    pthread_mutex_lock(&mutexNcurses);

    selectorBtn[_position].button = new smallButton(get_winContentInfo(), smallBtnCoord[_position][0], smallBtnCoord[_position][1], _text, active, _ptrFunction);

    wrefresh(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
    return true;
}

void configIndiParamWindow::activSmallButton(const char* _name, activeState_e _active)
{
    for (int i=0; i <= 13; i++)
    {
        if (strcmp(selectorBtn[i].name, _name) == 0)
        {
            activSmallButton(i, _active);
            break;
        }
    }
}

void configIndiParamWindow::activSmallButton(int _position, activeState_e _active)
{
    if (selectorBtn[_position].button != NULL)
    {
        pthread_mutex_lock(&mutexNcurses);
        selectorBtn[_position].button->bntActivate(_active);
        pthread_mutex_unlock(&mutexNcurses);
    }
}

bool configIndiParamWindow::isSmallButtonActive(const char* _name)
{
    for (int i=0; i <= 13; i++)
    {
        if (strcmp(selectorBtn[i].name, _name) == 0)
        {
            if (selectorBtn[i].button->btnActive() == active)
                return true;
                else return false;
        }
    }
    return false;
}

/***********************************************************************************
*  Class screenConfiguration
***********************************************************************************/
//screenConfiguration::screenConfiguration(int l, int c, int y, int x, int colPair, pthread_mutex_t mutex)
screenConfiguration::screenConfiguration(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*))): objWindow(_l, _c, _y, _x, _ptrFuncSystem), textWindow(_l, _c, _y, _x, _textTitle, NULL), buttonWindow(_l, _c, _y, _x, _textTitle, _linkPlace, _btnType, _ptrFuncSystem)
{
    readScreenList();
    drawContent();
};

screenConfiguration::~screenConfiguration()
{
};

void screenConfiguration::readScreenList()
{
    screenConf = configXML->getScreenConfig();
    screenOrg = screenConf;
}

void screenConfiguration::saveScreenConfig()
{
    configXML->cleanScreen();
    // save config on file
    configXML->setScreenBackgroundColor(screenConf.colorbkgnd);
    configXML->setScreenTextColor(screenConf.colortext);
    configXML->setScreenSaveDelay(screenConf.screensaver);
    configXML->flushConfig(DISPLAY);
}

void screenConfiguration::drawContent()
{
    int x_center = smallBtnCoord[2][1] + 1;
    int x_left = smallBtnCoord[0][1] + 1;
    int x_right = smallBtnCoord[4][1] + 1;

    pthread_mutex_lock(&mutexNcurses);
    if (isBackgroundActive)
        init_color(COLOR_RED, screenConf.colorbkgnd.red * 100, screenConf.colorbkgnd.green * 100, screenConf.colorbkgnd.blue * 100);
        else init_color(COLOR_WHITE, screenConf.colortext.red * 100, screenConf.colortext.green * 100, screenConf.colortext.blue * 100);
    int i;

    // Cursor for RED color
    mvwprintw(get_winContentInfo(), 1, x_left -1, "RED");
    for (i=7; i<= 16; i++)
    {
        wmove(get_winContentInfo(), i, x_left);
        waddch(get_winContentInfo(), ACS_VLINE);
    }
    if(isBackgroundActive) wmove(get_winContentInfo(), 16 -screenConf.colorbkgnd.red, x_left);
        else wmove(get_winContentInfo(), 16 -screenConf.colortext.red, x_left);
    waddch(get_winContentInfo(), ACS_CKBOARD);

    // Cursor for GREEN color
    mvwprintw(get_winContentInfo(), 1, x_center -2, "GREEN");
    for (i=7; i<= 16; i++)
    {
        wmove(get_winContentInfo(), i, x_center);
        waddch(get_winContentInfo(), ACS_VLINE);
    }
    if(isBackgroundActive) wmove(get_winContentInfo(), 16 -screenConf.colorbkgnd.green, x_center);
        else wmove(get_winContentInfo(), 16 -screenConf.colortext.green, x_center);
    waddch(get_winContentInfo(), ACS_CKBOARD);

    // Cursuor for BLUE color
    mvwprintw(get_winContentInfo(), 1, x_right- 1, "BLUE");
    for (i=7; i<= 16; i++)
    {
        wmove(get_winContentInfo(), i, x_right);
        waddch(get_winContentInfo(), ACS_VLINE);
    }
    if(isBackgroundActive) wmove(get_winContentInfo(), 16 -screenConf.colorbkgnd.blue, x_right);
        else wmove(get_winContentInfo(), 16 - screenConf.colortext.blue, x_right);
    waddch(get_winContentInfo(), ACS_CKBOARD);

    // Value for ScreenSaver delay
    char saver[8];
    sprintf(saver,"%i", screenConf.screensaver);
    mvwprintw(get_winContentInfo(), 23, x_center -5, "SCREENSAVER");
    mvwprintw(get_winContentInfo(), 26, x_center -3, "      ");
    mvwprintw(get_winContentInfo(), 26, x_center - strlen(saver) / 2, saver);

    redrawwin(get_winContentInfo());
    wrefresh(get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
}

void screenConfiguration::moveRed(plusMinus_e direction)
{
    if(direction == PLUS)
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.red < 9)) screenConf.colorbkgnd.red++;
        else if (screenConf.colortext.red < 9) screenConf.colortext.red++;
    } else
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.red > 0)) screenConf.colorbkgnd.red--;
        else if (screenConf.colortext.red > 0) screenConf.colortext.red--;
    }
    drawContent();
};

void screenConfiguration::moveGreen(plusMinus_e direction)
{
    if(direction == PLUS)
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.green < 9)) screenConf.colorbkgnd.green++;
        else if (screenConf.colortext.green < 9) screenConf.colortext.green++;
    } else
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.green > 0)) screenConf.colorbkgnd.green--;
        else if (screenConf.colortext.green > 0) screenConf.colortext.green--;
    }
    drawContent();
};

void screenConfiguration::moveBlue(plusMinus_e direction)
{
    if(direction == PLUS)
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.blue < 9)) screenConf.colorbkgnd.blue++;
        else if (screenConf.colortext.blue < 9) screenConf.colortext.blue++;
    } else
    {
        if (isBackgroundActive && (screenConf.colorbkgnd.blue > 0)) screenConf.colorbkgnd.blue--;
        else if (screenConf.colortext.blue > 0) screenConf.colortext.blue--;
    }
    drawContent();
};

void screenConfiguration::moveSaverDelay(plusMinus_e direction, int _factor)
{
    int value =  screenConf.screensaver;
    if(direction == PLUS)
    {
        if (((_factor == 1) && (value <= 599)) || ((_factor == 10) && (value <= 590)))
            screenConf.screensaver = value + _factor;
    } else
    {
        if (((_factor == 1) && (value >= 11)) || ((_factor == 10) && (value >= 20)))
            screenConf.screensaver = value - _factor;
    }
    drawContent();
};

resp_e screenConfiguration::getClickOther(int _pos_x, int _pos_y)
{
    // X and Y are absolute position form screen.
    // should be adapted for the current windows
    _pos_y = _pos_y - winContentInfo->_begy;
    _pos_x = _pos_x - winContentInfo->_begx;

    if ((_pos_y >= smallBtnCoord[0][0]) && (_pos_y <= smallBtnCoord[0][0] + 2) && (_pos_x >= smallBtnCoord[0][1]) && (_pos_x <= smallBtnCoord[0][1] + 2))
    {
        moveRed(PLUS);
    }
    else if ((_pos_y >= smallBtnCoord[1][0]) && (_pos_y <= smallBtnCoord[1][0] + 2) && (_pos_x >= smallBtnCoord[1][1]) && (_pos_x <= smallBtnCoord[1][1] + 2))
    {
        moveRed(MINUS);
    }

    if ((_pos_y >= smallBtnCoord[2][0]) && (_pos_y <= smallBtnCoord[2][0] + 2) && (_pos_x >= smallBtnCoord[2][1]) && (_pos_x <= smallBtnCoord[2][1] + 2))
    {
        moveGreen(PLUS);
    }
    else if ((_pos_y >= smallBtnCoord[3][0]) && (_pos_y <= smallBtnCoord[3][0] + 2) && (_pos_x >= smallBtnCoord[3][1]) && (_pos_x <= smallBtnCoord[3][1] + 2))
    {
        moveGreen(MINUS);
    }

    if ((_pos_y >= smallBtnCoord[4][0]) && (_pos_y <= smallBtnCoord[4][0] + 2) && (_pos_x >= smallBtnCoord[4][1]) && (_pos_x <= smallBtnCoord[4][1] + 2))
    {
        moveBlue(PLUS);
    }
    else if ((_pos_y >= smallBtnCoord[5][0]) && (_pos_y <= smallBtnCoord[5][0] + 2) && (_pos_x >= smallBtnCoord[5][1]) && (_pos_x <= smallBtnCoord[5][1] + 2))
    {
        moveBlue(MINUS);
    }

    if ((_pos_y >= smallBtnCoord[6][0]) && (_pos_y <= smallBtnCoord[6][0] + 2) && (_pos_x >= smallBtnCoord[6][1]) && (_pos_x <= smallBtnCoord[6][1] + 2))
    {
        moveSaverDelay(MINUS, 1);
    }
    else if ((_pos_y >= smallBtnCoord[7][0]) && (_pos_y <= smallBtnCoord[7][0] + 2) && (_pos_x >= smallBtnCoord[7][1]) && (_pos_x <= smallBtnCoord[7][1] + 2))
    {
        moveSaverDelay(PLUS, 1);
    }

    if ((_pos_y >= smallBtnCoord[8][0]) && (_pos_y <= smallBtnCoord[8][0] + 2) && (_pos_x >= smallBtnCoord[8][1]) && (_pos_x <= smallBtnCoord[8][1] + 2))
    {
        moveSaverDelay(MINUS, 10);
    }
    else if ((_pos_y >= smallBtnCoord[9][0]) && (_pos_y <= smallBtnCoord[9][0] + 2) && (_pos_x >= smallBtnCoord[9][1]) && (_pos_x <= smallBtnCoord[9][1] + 2))
    {
        moveSaverDelay(PLUS, 10);
    }

    return CONTINUE;
}

bool screenConfiguration::addSmallButton(const char* _name, resp_e _value, int _position, const char* _text, void* (_ptrFunction(void*)))
{
    strncpy(selectorBtn[_position].name, _name, 32);
    selectorBtn[_position].value = _value;

    pthread_mutex_lock(&mutexNcurses);

    selectorBtn[_position].button = new smallButton(get_winContentInfo(), smallBtnCoord[_position][0], smallBtnCoord[_position][1], _text, active, _ptrFunction);

    wrefresh(get_winContainer());
    pthread_mutex_unlock(&mutexNcurses);
    return true;
}

void screenConfiguration::activSmallButton(const char* _name, activeState_e _active)
{
    for (int i=0; i <= 13; i++)
    {
        if (strcmp(selectorBtn[i].name, _name) == 0)
        {
            activSmallButton(i, _active);
            break;
        }
    }
}

void screenConfiguration::activSmallButton(int _position, activeState_e _active)
{
    if (selectorBtn[_position].button != NULL)
    {
        pthread_mutex_lock(&mutexNcurses);
        selectorBtn[_position].button->bntActivate(_active);
        pthread_mutex_unlock(&mutexNcurses);
    }
}

bool screenConfiguration::isSmallButtonActive(const char* _name)
{
    for (int i=0; i <= 13; i++)
    {
        if (strcmp(selectorBtn[i].name, _name) == 0)
        {
            if (selectorBtn[i].button->btnActive() == active)
                return true;
                else return false;
        }
    }
    return false;
}

void screenConfiguration::setBackgroundActive(bool isActive)
{
    isBackgroundActive = isActive;
}

void screenConfiguration::restaureConfigOrg()
{
    init_color(COLOR_RED, screenOrg.colorbkgnd.red * 100, screenOrg.colorbkgnd.green * 100, screenOrg.colorbkgnd.blue * 100);
    init_color(COLOR_WHITE, screenOrg.colortext.red * 100, screenOrg.colortext.green * 100, screenOrg.colortext.blue * 100);
}
