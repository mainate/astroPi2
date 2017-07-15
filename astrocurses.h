#ifndef ASTROCURSES_H
#define ASTROCURSES_H

#define LAPTOP

#define VERSION     0
#define SUBVERSION  4

#define DRIVERSXMLFILE  "/usr/share/indi/drivers.xml"
#define CONFIGXMLFILE   "/astroconfig.xml"
#define INDISERVERNAME  "indiserver"

#define TIMETOSLEEP     5   // value in second before switch off power light on screen
#define REFRESHPORTINT  1   // value for refresh the port number
#define REFRESHIPINT    3   // Refresh delay for IP display in second
#define SLEEPTIME   10000   // Waiting time in loops (for usleep)

#define NBRLINEDISPLAY  9   // Number of line displayed on set config drivers

#include <ncurses.h>
#include <tinyxml2.h>
#include <pthread.h>
#include <dirent.h>
#include <string>
#include <fstream>
#include <signal.h>

#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>

#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "libevdev-1.0/libevdev/libevdev.h"
#include "include/screensaver.h"

#ifndef LAPTOP
     #include <wiringPi.h>
#endif

/*  Windows height
    Windows count 0..4 from top to bottom */

#define hgtp1               3
#define hgtp2               4
#define hgtp3               6
#define hgtp0               LINES - hgtp3 - hgtp2 - hgtp1

#define die(str, args...) do { \
        perror(str); \
        exit(EXIT_FAILURE); \
    } while(0)

#define BUTTON_ON 1
#define BUTTON_OFF 0

using namespace tinyxml2;
using namespace std;

enum activeState_e {active = 1, notactive = 2};
enum kindOfBtn_e {squarebtn, rectanglebtn, squarebottombtn, rectanglebottombtn, smallbtn};
enum resp_e {NIL, CONTINUE, CANCEL, QUIT, SHUTDOWN, REBOOT, STOP, BRIGHTNESS, DECBRIGHTNESS, INCBRIGHTNESS, DISPLAYCONF, TOUCHPANEL, END,
            INDICONFIG, SCREENCONF, STOPINDI, STARTINDI, SHOWCONF, EDITCONF, PREVIOUS, NEXT, SAVE, EXIT, DRIVER, PARAMETER,
            VERBOSEM, VERBOSEP, MEMM, MEMP, PORT10000P, PORT10000M, PORT1000P, PORT1000M, PORT100P, PORT100M, PORT10P, PORT10M, PORT1P, PORT1M,
            REDM, REDP, GREENM, GREENP, BLUEM, BLUEP, BRGHTM, BRGHTP, SSAVERM, SSAVERMM, SSAVERP, SSAVERPP};
enum flush_e {ALL, DRIVERS, PARAMETERS, DISPLAY};

struct xy_t
{
    int x;
    int y;
};

struct driversIndiConfig_t
{
    char driverName[64]= {'\0'};
    char driverFile[64] = {'\0'};
    char driverVersion[8] = {'\0'};
    driversIndiConfig_t* nextDriver = NULL;
};

struct color_t
{
    int red;
    int green;
    int blue;
};

struct parametersIndiConfig_t
{
    char verbose[8];// = "vv";
    char maxMemClient[8];// = "64";
    char ipPort[8];// = "7624";
};

struct screenConfig_t
{
    int brightness = {6};
    int screensaver = {120};
    color_t colorbkgnd = {4, 0, 0},
            colortext = {5, 5, 5};
    xy_t offsetscreen;
};

struct lineSelection_t
{
    char driverName[32] = {'\0'};
    char driverFile[32] = {'\0'};
    char driverVersion[32] = {'\0'};
    bool isSelected = false;
};

struct pageConfig_t
{
    int pageNumber = 0;
    char groupDev[32] = {'\0'};
    int numLine = 0;
    lineSelection_t line[9];
};

// Structure for configuration pages
struct pageList_t
{
    pageConfig_t page;
    int numPage = 0;
    pageList_t *nextPage = NULL;
    pageList_t *previousPage = NULL;
};

/***********************************************************************************
*
*   Classes definition
*
***********************************************************************************/
class buttonWindow;

/***********************************************************************************
*   Base Class BUTTON
***********************************************************************************/
/***********************************************************************************
*   Base Class objButton
***********************************************************************************/
class objButton
{
public:
    objButton();
    objButton(activeState_e setActive, void* (_ptrFunction(void*)));
    virtual ~objButton();
    virtual activeState_e btnActive();
    virtual WINDOW* getHandle();
    virtual void bntActivate(activeState_e _state);
    virtual void btnRedraw();
    virtual void btnHide();
    virtual bool isSelectedBtn(int _pos_x, int _pos_y, buttonWindow* _win);

protected:
    WINDOW* handle = NULL;
    // To know if the fonction after click on button is showing next windows (Or other functionnalities)
    bool showFunction;
    //If needed by callback function
    buttonWindow* parentContainer = NULL;
    // Call back function definition
    void* (*functPtr)(void*) = NULL;

private:
    activeState_e isActive = active;
};

/***********************************************************************************
*   Base Class squareButton
***********************************************************************************/
class squareButton : virtual public objButton
{
public:
    squareButton(WINDOW* _parentWin, const int _y, const int _x, const char* _textTop, const char* _textBottom, activeState_e _setActive, void* (_ptrFunction(void*)));
    virtual ~squareButton();
    virtual void btnRedraw();
protected:
private:
    char* btnTextTop;
    char* btnTextBottom;
    const int btnLength = 6;
    const int btnHeight = 4;
};

/***********************************************************************************
*   Base Class rectangleButton
***********************************************************************************/
class rectangleButton : virtual public objButton
{
public:
    rectangleButton(WINDOW* _parentWin, const int _y, const int _x, const char* _text, activeState_e _setActive, void* (_ptrFunction(void*)));
    virtual ~rectangleButton();
    virtual void btnRedraw();
protected:
private:
    char* btnText;
    const int btnLength = 12;
    const int btnHeight = 3;
};

/***********************************************************************************
*  Class smallButton
***********************************************************************************/
class smallButton: virtual public objButton
{
public:
    smallButton(WINDOW* _parentWin, const int _y, const int _x, const char* _text, activeState_e _setActive, void* (_ptrFunction(void*)));
    virtual ~smallButton();
    virtual void btnRedraw();
protected:
private:
    char* btnText;
    const int btnLength = 3;
    const int btnHeight = 3;
};

/***********************************************************************************
*   Base Class WINDOWS
***********************************************************************************/
/***********************************************************************************
*   Base Class objWindow
***********************************************************************************/
class objWindow
{
public:

    struct rect_t
    {
        int l, c, y, x;
    };
    /** Default constructor */
    objWindow(int _l, int _c, int _y, int _x, void* (_ptrFuncSystem(void*)));
    /** Default destructor */
    virtual ~objWindow();
    virtual WINDOW* get_winContainer() const;
    virtual void set_winContainer(WINDOW* _winptr);
    virtual WINDOW* get_winContentInfo() const;
    virtual void set_winContentInfo(WINDOW* _winptr);
    virtual void refreshWindow();
    virtual void redrawWindow();
    virtual void setRefresh(bool _refresh);
    virtual bool getRefresh();
    virtual bool isStopThread();
    virtual void startThread();
    virtual void stopThread();


protected:
    // Call back function definition
    void* (*functPtr)(buttonWindow*) = NULL;
    void* (*functIndiport)(void*) = 0;

    WINDOW* winContainer = NULL;
    WINDOW* winContentInfo = NULL;

    bool refresh = false;
    bool runThread = true;
    pthread_t threadsWin;
private:

};

/***********************************************************************************
*   Child Class textWindow
***********************************************************************************/
class textWindow : virtual public objWindow
{
public:
    textWindow(int _l, int _c, int _y, int _x, char* _textTitle, void* (_ptrFuncSystem(void*)));
    /** Default destructor */
    virtual ~textWindow();
    virtual void setTitle(int _line, char* _title, bool _center, const char _separator);
    virtual WINDOW* get_winContentTitle() const;
    virtual void set_winContentTitle(WINDOW* _winptr);
    virtual void writeline(char* _lineText, int _colPair);
    virtual void writeline(int _line, char* _lineText, int _colPair);
    virtual void writeline(int _line, int _coll, char* _lineText, int _colPair);
protected:

private:
    WINDOW* winContentTitle = NULL;
};

/***********************************************************************************
*   Derived Class buttonWindow
***********************************************************************************/
class buttonWindow : virtual public textWindow
{
public:
    buttonWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*)));
    virtual ~buttonWindow();
//    virtual resp_e startGetKbd();
    virtual bool addButton(const char* _name, resp_e _value, int _position, const char* _textTop, const char* _textBottom, void* (_ptrFunction(void*)));
    virtual bool addSmallButton(const char* _name, resp_e _value, int _position, const char* _text, void* (_ptrFunction(void*))) {return false;};
    virtual void delButton(const char* _name);
    virtual void delButton(int _position);
    virtual void activButton(const char* _name, activeState_e _active);
    virtual void activButton(int _position, activeState_e _active);
    virtual void activeAllButton();
    virtual void deactiveAllButton();
    virtual void activSmallButton(const char* _name, activeState_e _active) {};
    virtual void activSmallButton(int _position, activeState_e _active) {};
    virtual void showLink(int8_t _linkPlace);
    virtual void clean();
    virtual resp_e getClickPos(int _pos_x, int _pos_y);
    virtual resp_e getClickOther(int _pos_x, int _pos_y);
    virtual bool isButtonActive(const char* _name);
    virtual bool isSmallButtonActive(const char* _name){return false;};
//    virtual bool lineSelected(int lineNumber, int numPage, pageList_t *pList);
    buttonWindow* childWin = NULL;

protected:
    struct listBtn_s
    {
        objButton* button;
        char name[32];
        resp_e value;
    };
    listBtn_s listBtn[4] = {{NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}};
//    listBtn_s selectorBtn[14] = {{NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL},
  //                              {NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}};
    kindOfBtn_e btnType;
    int rectBtnCoord[4][2] = {{1, 2}, {1, 16}, {5, 2}, {5, 16}};
    int squareBtnCoord[4][2] = {{1, 1}, {1, 8}, {1, 16}, {1, 23}};
//    int smallBtnCoord[14][2] = {{2, 4}, {2, 22}, {8, 4}, {8, 22}, {16, 3}, {24, 3}, {16, 8}, {24, 8}, {16, 13}, {24, 13}, {16, 18}, {24, 18}, {16, 23}, {24, 23}};

private:
};

/***********************************************************************************
*   Derived Class configIndiWindow
***********************************************************************************/
class configIndiWindow : virtual public buttonWindow
{
public:
    configIndiWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*)));
    virtual ~configIndiWindow();
    virtual bool readDriversList();
    virtual bool saveDriversConfig();
    virtual bool isDriverAlreadyInList(const char *name, pageList_t *pList);
    virtual void writeDeviceInfo(char *deviceName, char *fileName, int line, bool active);
    virtual void writeDeviceInfoNextPage();
    virtual void writeDeviceInfoPreviousPage();
    virtual void writeGroupTitle(char *groupdev, int page, int numPages);
    virtual bool lineSelected(int lineNumber);
    virtual resp_e getClickOther(int _pos_x, int _pos_y);

protected:

private:
    pageList_t *pagesList = NULL;
    pageList_t *actualptr = NULL;
    int pos_x = 0, pos_y = 0;
    int pageDisplayed;
};

/***********************************************************************************
*   Derived Class configIndiParamWindow
***********************************************************************************/
class configIndiParamWindow : virtual public buttonWindow
{
public:
    configIndiParamWindow(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*)));
    virtual ~configIndiParamWindow();
    virtual void readParamsList();
    virtual bool saveParamsConfig();
    virtual void writeDeviceInfo(char *deviceName, char *fileName, int line, bool active);
    virtual void writeGroupTitle(char *groupdev, int page, int numPages);
    virtual resp_e getClickOther(int _pos_x, int _pos_y);
    virtual void initialValue();
    virtual bool isPortValid();
    virtual bool addSmallButton(const char* _name, resp_e _value, int _position, const char* _text, void* (_ptrFunction(void*)));
    virtual void activSmallButton(const char* _name, activeState_e _active);
    virtual void activSmallButton(int _position, activeState_e _active);
    virtual bool isSmallButtonActive(const char* _name);

protected:

    listBtn_s selectorBtn[14] = {{NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL},
                                {NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}};

    int smallBtnCoord[14][2] = {{2, 4}, {2, 22}, {8, 4}, {8, 22}, {16, 3}, {24, 3}, {16, 8}, {24, 8}, {16, 13}, {24, 13}, {16, 18}, {24, 18}, {16, 23}, {24, 23}};

private:
    parametersIndiConfig_t parameters;
    int _10000;
    int _1000;
    int _100;
    int _10;
    int _1;

};

/***********************************************************************************
*   Class screenConfiguration
***********************************************************************************/
class screenConfiguration : virtual public buttonWindow
{
    public:
        enum plusMinus_e {MINUS, PLUS};

        screenConfiguration(int _l, int _c, int _y, int _x, char* _textTitle, int8_t _linkPlace, kindOfBtn_e _btnType, void* (_ptrFuncSystem(void*)));
        virtual ~screenConfiguration();
        virtual void drawContent();
        virtual void readScreenList();
        void saveScreenConfig();
        virtual void moveRed(plusMinus_e direction);
        virtual void moveGreen(plusMinus_e direction);
        virtual void moveBlue(plusMinus_e direction);
        virtual void moveSaverDelay(plusMinus_e direction, int _factor);
        virtual resp_e getClickOther(int _pos_x, int _pos_y);
        virtual bool addSmallButton(const char* _name, resp_e _value, int _position, const char* _text, void* (_ptrFunction(void*)));
        virtual void activSmallButton(const char* _name, activeState_e _active);
        virtual void activSmallButton(int _position, activeState_e _active);
        virtual bool isSmallButtonActive(const char* _name);
        virtual void setBackgroundActive(bool isActive);
        virtual void restaureConfigOrg();

    protected:

        listBtn_s selectorBtn[10] = {{NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL},
                                {NULL, {'\0'}, NIL},{NULL, {'\0'}, NIL}, {NULL, {'\0'}, NIL}};

//        int smallBtnCoord[8][2] = {{3, 4}, {18, 4}, {3, 12}, {18, 12}, {3, 20}, {18, 20}, {25, 3}, {25, 22}};
        int smallBtnCoord[10][2] = {{3, 4}, {18, 4}, {3, 12}, {18, 12}, {3, 20}, {18, 20}, {25, 6}, {25, 19}, {25, 1}, {25, 24}};
    private:
        screenConfig_t screenConf, screenOrg;
        bool isBackgroundActive = true;
};


/***********************************************************************************
*   Other Classese
***********************************************************************************/

/***********************************************************************************
*   Class configurationXML
***********************************************************************************/
class configurationXML
{
public:
    configurationXML(const char* _fileName);
    ~configurationXML();
    color_t getScreenBackgroundColor();
    bool setScreenBackgroundColor(color_t _color);
    int getScreenBrithness();
    void setScreenBrightness(int _brightness);
    int getScreenSaveDelay();
    void setScreenSaveDelay(int _delay);
    screenConfig_t getScreenConfig();
    color_t getScreenTextColor();
    bool setScreenTextColor(color_t _color);
    parametersIndiConfig_t getFullParameters();
    char* getParameters();
    bool setParameters(const char* _verbose, int _maxMemClient, int _ipPort);
    const driversIndiConfig_t* getFullDrivers();
    char* getDrivers();
    bool flushConfig(flush_e selection);
    bool refreshConfig();
    bool isConfigValid();
    bool isConfigured(const char *name);
    void cleanDrivers();
    bool addDrivers(char* _drvFile, char* _drvName, char* _drvVer);
    void cleanParams();
    void addParams(char* _verbose, char* _clientmem, char* _portip);
    void cleanScreen();

protected:

private:
    char configFileName[PATH_MAX + strlen(".xml")];
//    char configFileName[PATH_MAX + strlen(".xml")];
    bool ConfigValid = false;
    driversIndiConfig_t* drivers = NULL;
    parametersIndiConfig_t parameters;
    screenConfig_t screenSetting;
};

/***********************************************************************************
*
*   Functions definition
*
***********************************************************************************/
/***********************************************************************************
*  Initialisation of Input function
***********************************************************************************/
xy_t initProg();

/***********************************************************************************
*  Function intiCurses
***********************************************************************************/
xy_t initCurses();

/***********************************************************************************
*  Function releaseCurses
***********************************************************************************/
void releaseCurses();

/***********************************************************************************
*  Function initScreen
***********************************************************************************/
bool initScreen();

/***********************************************************************************
*  Function readKbd
***********************************************************************************/
bool readKbd(int* _x, int* _y);

/***********************************************************************************
*  Function mainLoop
***********************************************************************************/
resp_e mainLoop();

/***********************************************************************************
*  Function callBtnWinLeaveProgram
***********************************************************************************/
//WINDOW* callBtnWinLeaveProgram(buttonWindow* _Win);
void* callBtnWinLeaveProgram(void* _a);

/***********************************************************************************
*  Function callBtnWinIndiserver
***********************************************************************************/
void* callBtnWinIndiServer(void* _a);

/***********************************************************************************
*  Function callBtnWinScreenConfig
***********************************************************************************/
void* callBtnWinScreenConfig(void* _a);

/***********************************************************************************
*  Function callBtnWinScreenConfigSave
***********************************************************************************/
void* callBtnWinScreenConfigSave(void* _a);

/***********************************************************************************
*  Function callBtnBrigthness
***********************************************************************************/
void* callBtnBrigthness(void* _a);

/***********************************************************************************
*  Function getProcIdByName
***********************************************************************************/
int getProcIdByName(string procName);

/***********************************************************************************
*  Fucntion indiport
***********************************************************************************/
void *callIndiport(void *_mutex);

/***********************************************************************************
*  CallBack function indiserver
***********************************************************************************/
void *callIndiserver(void *_mutex);

/***********************************************************************************
*  CallBack function ipaddress
***********************************************************************************/
void *callMyIpaddress(void *_mutex);

/***********************************************************************************
*  CallBack function stopIndiserver
***********************************************************************************/
void* callBtnStopIndiserver(void* _a);

/***********************************************************************************
*  CallBack function startIndiserver
***********************************************************************************/
void* callBtnStartIndiserver(void* _a);

/***********************************************************************************
*  CallBack function showConfigIndiserver
***********************************************************************************/
void* callBtnShowConfigIndiserver(void* _a);

/***********************************************************************************
*  CallBack function editConfigServer
***********************************************************************************/
void* callBtnEditConfigServer(void* _a);

/***********************************************************************************
*  CallBack function callBtnEditParamIndiserver
***********************************************************************************/
void* callBtnEditParamIndiserver(void* _a);

/***********************************************************************************
*  CallBack function callBtnEditParamIndiserverSave
***********************************************************************************/
void* callBtnEditParamIndiserverSave(void* _a);

/***********************************************************************************
*  CallBack function editConfigIndiserver
***********************************************************************************/
void* callBtnEditConfigIndiserver(void* _a);

/***********************************************************************************
*  CallBack function editConfigIndiserverSave
***********************************************************************************/
void* callBtnEditConfigIndiserverSave(void* _a);

/***********************************************************************************
*  CallBack function editConfigIndiserverNext
***********************************************************************************/
void* callBtnEditConfigIndiserverNext(void* _a);

/***********************************************************************************
*  CallBack function editConfigIndiserverPrevious
***********************************************************************************/
void* callBtnEditConfigIndiserverPrevious(void* _a);

/***********************************************************************************
*  CallBack function increaseBrightness
***********************************************************************************/
void* callBtnBrightnessDec(void* _a);

/***********************************************************************************
*  CallBack function decreaseBrightness
***********************************************************************************/
void* callBtnBrightnessInc(void* _a);

/***********************************************************************************
*  CallBack function callBtnConfDisplay
***********************************************************************************/
void* callBtnConfDisplay(void* _a);

/***********************************************************************************
*  CallBack function callBtnCalTouch
***********************************************************************************/
void* callBtnCalTouch(void* _a);

/***********************************************************************************
*  CallBack function frozeScreen
***********************************************************************************/
void frozeScreen(bool froze);

/***********************************************************************************
*  CallBack function callBtnActiveBkgnd
***********************************************************************************/
void* callBtnActiveBkgnd(void * _a);

/***********************************************************************************
*  CallBack function callBtnActiveTxt
***********************************************************************************/
void* callBtnActiveTxt(void * _a);

#endif // ASTROCURSES_H
