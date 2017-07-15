#include "astrocurses.h"

#define LAPTOP

xy_t    offsetScreen,       // Offset for touch screen form config file, != 0 if screen calibrated
        screenResolution,   // Get the terminal screen resolution in pixel
        screenSize;         // Get the nomber of COLS and LINES of the terminal

// Windows instance used by the program
objWindow* portWin;
objWindow* verWin;
textWindow* mainWin;
textWindow* ipWin;
buttonWindow* bottomWin;
buttonWindow* showConfWin;
configIndiWindow* editConfWin;
configIndiParamWindow* editParamWin;
screenConfiguration* editScreenConfig;
configurationXML* configXML;
screenSaver_c* screenSaver;

//array of thread
pthread_t threads[3];

//mutex
pthread_mutex_t mutexNcurses;

//int procID = -1;

#ifdef LAPTOP
    #include <X11/Xlib.h>
    // For emultation touch screen on laptop devellopement
    Display *dpy;
    Window root;
    Window ret_root;
    Window ret_child;
    Window inputWin;
    int root_x;
    int root_y;
    int win_x;
    int win_y;
    unsigned int mask;
    XWindowAttributes windowattr;
    int revert_to;
#else
    #include <fcntl.h>
    #include <linux/fb.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>
//    #include <wiringPi.h>
    struct libevdev *dev;
    int fd;
    int rc;
    struct input_event     ev[10];
    bool isClick = false;
#endif // LAPTOP

/***********************************************************************************
*   Initialisation of Input function
***********************************************************************************/
xy_t initProg()
{
    offsetScreen.x = offsetScreen.y = 0;
    // Get screen resolution of the terminal to find the relation between nbr pixel by FONT CHAR
#ifdef LAPTOP
    // Get screen display info
    // active windows
    //windows size in pixel
    dpy = XOpenDisplay(NULL);
    if (dpy != NULL)
    {
        root = XDefaultRootWindow(dpy);

        XGetInputFocus(dpy, &inputWin, &revert_to); // see man To get the active windows

        if (XGetWindowAttributes(dpy, inputWin, &windowattr) == 0)
        {
            fprintf(stderr, "%s: failed to get window attributes.\n", "0");
        // Fill attribute structure with information about root window
        }
        screenResolution.x =  windowattr.width;
        screenResolution.y =  windowattr.height;
    } else
    {
        screenResolution.x =  240;
        screenResolution.y =  320;
    }

#else
    fd = open("/dev/input/touchscreen", O_RDONLY);
    if(fd < 0)
        printf("error: open /dev/input/touchscreen");
    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0)
        fprintf(stderr, "error: %d %s\n", -rc, strerror(-rc));

    screenResolution.x =  libevdev_get_abs_maximum(dev, ABS_X);
    screenResolution.y =  libevdev_get_abs_maximum(dev, ABS_Y);

    //init GPIO library
    wiringPiSetupGpio();
    pinMode(18, PWM_OUTPUT);
    pwmSetRange(10);
    pwmWrite(18, configXML->getScreenBrithness());
#endif // LAPTOP
    return screenResolution;
}

/***********************************************************************************
*   Initialisation of ncurses
***********************************************************************************/
xy_t initCurses()
{
    //mutex
    if (pthread_mutex_init(&mutexNcurses, NULL) != 0)
    {
        printf("\n mutex 'mutexNcurses' init failed\n");
        exit(EXIT_FAILURE);
    }

    // get screen parameter from astroconfig.xml file
    /*    readScreenConfig();
    */
    initscr();      // Start curses mode
    cbreak();       // Line buffering disabled, Pass on everty thing to me
    noecho();       // Deacivate echo from keyboard

    // Keyboard setting for reading mouse click
    mmask_t Mask;
//    mousemask(ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION, &Mask);
    mousemask(BUTTON1_RELEASED|REPORT_MOUSE_POSITION, &Mask);
    nodelay(stdscr, true);    // To have a no blocking function of getchr() on laptop. To simulate the touch screen reading
    keypad(stdscr, true);

    start_color();
    if (configXML == NULL)
    {
        // Color initialisation by default
        init_color(COLOR_RED, 400, 0 , 0);
        init_color(COLOR_WHITE, 500, 500, 500);
    } else
    {
        screenConfig_t screenSetting = configXML->getScreenConfig();
        // Color initialisation
        init_color(COLOR_RED, screenSetting.colorbkgnd.red *100, screenSetting.colorbkgnd.green *100, screenSetting.colorbkgnd.blue *100);
        init_color(COLOR_WHITE, screenSetting.colortext.red *100, screenSetting.colortext.green *100, screenSetting.colortext.blue *100);
    }

    init_pair(1, COLOR_RED, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_RED);


    // Hide cursor from the screen
    curs_set(0);
    refresh();

    // Set the returned value
    screenSize.x = COLS;
    screenSize.y = LINES;
    return screenSize;
};

/***********************************************************************************
*   Release of ncurses
***********************************************************************************/
void releaseCurses()
{
    portWin->stopThread();
    ipWin->stopThread();
    mainWin->stopThread();
    screenSaver->stopThread();
    delete portWin;
    delete mainWin;
    delete ipWin;
    delete bottomWin;
    delete configXML;
    delete screenSaver;
    pthread_mutex_destroy(&mutexNcurses);

    endwin();
    if (system("reset") == -1)
        printw("The reset of the console has failed!");
};

/***********************************************************************************
*  Function initScreen
***********************************************************************************/
bool initScreen()
{
    // Window for displaying the version of the program
    char ver[16];
    int len = 2;
    sprintf(ver, "Ver: %d.%d", VERSION, SUBVERSION);
    len += strlen(ver);

    verWin = new objWindow(3, len, 0, COLS - 10, NULL);
    pthread_mutex_lock(&mutexNcurses);
    wprintw(verWin->get_winContentInfo(), ver);
    wrefresh(verWin->get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);
    // Window to display the indiserver port number if running
    portWin = new objWindow(3, COLS - len , 0, 0, &callIndiport);
    // Main window to display indiserver messages
    mainWin = new textWindow(hgtp0, COLS, hgtp1, 0, (char*) "output", &callIndiserver);
    // Window to display IP adresses alternate with Mac adresses
    ipWin = new textWindow(hgtp2, COLS, hgtp0 + hgtp1, 0, NULL, &callMyIpaddress);
    // Window with basic button command
    bottomWin = new buttonWindow(hgtp3, COLS, hgtp0 + hgtp1 + hgtp2, 0, NULL, 0, squarebtn, NULL);

    // init window system refresh;
    frozeScreen(false);

    // initialisation screensaver function
    screenSaver = new screenSaver_c(configXML->getScreenSaveDelay(), 18);
 //   screenSaver = new screenSaver_c(5, 18);

    return true;
};

/***********************************************************************************
*  Function readKbd
***********************************************************************************/
bool readKbd(int* _x, int* _y)
{
    bool onClick = false;

#ifdef LAPTOP
    int ch;
    MEVENT event;

    ch = getch();
    if (ch == KEY_MOUSE)
    {
        if(getmouse(&event) == OK)
        {
            *_x = event.x;
            *_y = event.y;
            onClick = true;
        }
    }

#else

    do
    {
        struct input_event ev;

        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC)
            {
                //printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
                while (rc == LIBEVDEV_READ_STATUS_SYNC)
                {
                    rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
                }
                //printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
            } else
                if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
                {
                    if (ev.type == EV_KEY && ev.code == 330 && ev.value == 0)
                    {
                        //printf("Event type is %d & Event code is TOUCH(330) & Event value is 0 = Touch Finished\n", ev[i].type);
                        const struct input_absinfo *abs;
                        abs = libevdev_get_abs_info(dev, ABS_X);
                        *_x= abs->value;
                        abs = libevdev_get_abs_info(dev, ABS_Y);
                        *_y = abs->value;
                        *_x = *_x + offsetScreen.x;
                        *_y = *_y + offsetScreen.y;
                        if (screenSaver->isSaverActive())
                            onClick = false;
                            else onClick = true;
                        screenSaver->resetTimer(configXML->getScreenBrithness());
                    }
                }
    } while ((rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN) && !onClick);
    if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN)
        fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));

    // Return value in COLS / LINE as Curses coordonat
    // Invert the coord as 0.0 not on left botom
    *_x = (screenResolution.x - *_x) / (screenResolution.x / screenSize.x);
    *_y = (screenResolution.y - *_y) / (screenResolution.y / screenSize.y);
#endif // LAPTOP

    return onClick;
};
/***********************************************************************************
*  Function mainLoop
***********************************************************************************/
resp_e mainLoop()
{
    // Start Windows Threads
    portWin->startThread();
    usleep(1000);
    ipWin->startThread();
    usleep(1000);
    mainWin->startThread();
    usleep(1000);
    screenSaver->startThread();

    resp_e resp;

    int pos_x = 0, pos_y = 0;
#ifdef LAPTOP
    bool isActiveSaveScreen = false;
#endif // LAPTOP

    do
    {
        while (!readKbd(&pos_x, &pos_y))
        {
#ifdef LAPTOP
            if (screenSaver->isSaverActive() && !isActiveSaveScreen)
            {
                // start screensaver
                isActiveSaveScreen = true;
                pthread_mutex_lock(&mutexNcurses);
                scr_dump("scrDump.ncurses");
                init_pair(1, COLOR_BLACK, COLOR_BLACK);
                init_pair(2, COLOR_BLACK, COLOR_BLACK);
                redrawwin(stdscr);
                wrefresh(stdscr);
                pthread_mutex_unlock(&mutexNcurses);
            }
            usleep(500);
//#endif // LAPTOP
        }
//#ifdef LAPTOP
        if (screenSaver->isSaverActive())
        {
            screenSaver->resetTimer(configXML->getScreenBrithness());
            isActiveSaveScreen = false;
            pthread_mutex_lock(&mutexNcurses);
            init_pair(1, COLOR_RED, COLOR_WHITE);
            init_pair(2, COLOR_WHITE, COLOR_RED);
            scr_restore("scrDump.ncurses");
            refresh();
            pthread_mutex_unlock(&mutexNcurses);
            resp = CONTINUE;
        }
        else
        {
            screenSaver->resetTimer(configXML->getScreenBrithness());
            resp = bottomWin->getClickPos(pos_x, pos_y);
        }
#else
        resp = bottomWin->getClickPos(pos_x, pos_y);
#endif // LAPTOP
    } while ((resp != QUIT) && (resp != SHUTDOWN) && (resp != REBOOT));

    return resp;
};

/***********************************************************************************
*  Function callBtnWinLeaveProgram
***********************************************************************************/
void* callBtnWinLeaveProgram(void* _a)
{
    buttonWindow* Win = static_cast<buttonWindow*>(_a);
    bottomWin->activButton("STOP", notactive);
    bottomWin->activButton("BRIGHTNESS", notactive);
    bottomWin->activButton("SCREENCONF", notactive);

    if (Win->childWin == NULL)
    {
        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, creat the new one
        buttonWindow* selectionWin = new buttonWindow(9, COLS, Win->get_winContainer()->_begy -9, 0, NULL, -4, rectanglebtn, NULL);
        Win->showLink(4);
 //       selectionWin->addButton("CANCEL", CANCEL, 1, "cancel", "", NULL);
        selectionWin->addButton("QUIT", QUIT, 3, "quit", "", NULL);
        selectionWin->addButton("SHUTDOWN", SHUTDOWN, 0, "shutdown", "", NULL);
        selectionWin->addButton("REBOOT", REBOOT, 2, "reboot", "", NULL);
        Win->childWin = selectionWin;
        return selectionWin->get_winContainer();
    } else
        {
            // ChildWindow exist for bottom Window, destroy it
            bottomWin->clean();
            Win->childWin = NULL;
            mainWin->redrawWindow();
            mainWin->refreshWindow();
            ipWin->redrawWindow();
            ipWin->refreshWindow();
            // unfroze windows
            frozeScreen(false);
            return NULL;
        }
};

/***********************************************************************************
*  Function callBtnWinIndiServer
***********************************************************************************/
void* callBtnWinIndiServer(void* _a)
{
    if (bottomWin->childWin == NULL)
    {
        bottomWin->activButton("BRIGHTNESS", notactive);
        bottomWin->activButton("SCREENCONF", notactive);
        bottomWin->activButton("END", notactive);
        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, create the new one
        buttonWindow* selectionWin = new buttonWindow(9, COLS, bottomWin->get_winContainer()->_begy -9, 0, NULL, -1, rectanglebtn, NULL);
        bottomWin->showLink(1);
        selectionWin->addButton("STOPINDI", STOPINDI, 2, "stop", "", &callBtnStopIndiserver);
        selectionWin->addButton("STARTINDI", STARTINDI, 0, "start", "", &callBtnStartIndiserver);
        selectionWin->addButton("SHOWCONF", SHOWCONF, 3, "show conf", "", &callBtnShowConfigIndiserver);
        selectionWin->addButton("EDITCONF", EDITCONF, 1, "edit conf", "", &callBtnEditConfigServer);

        if (getProcIdByName("indiserver") == -1)
        {
            selectionWin->activButton("STOPINDI", notactive);
            selectionWin->activButton("SHOWCONF", notactive);
        }
        else
        {
            selectionWin->activButton("STARTINDI", notactive);
            selectionWin->activButton("EDITCONF", notactive);
        }
        bottomWin->childWin = selectionWin;
        return selectionWin->get_winContainer();
    } else
        {
            // ChildWindow exist for bottom Window, destroy it
            bottomWin->clean();
            delete bottomWin->childWin;
            bottomWin->childWin = NULL;
            mainWin->redrawWindow();
            mainWin->refreshWindow();
            ipWin->redrawWindow();
            ipWin->refreshWindow();
            // unfroze windows
            frozeScreen(false);
            return NULL;
        }
};

/***********************************************************************************
*  Function callBtnWinScreenConfig
***********************************************************************************/
void* callBtnWinScreenConfig(void* _a)
{
    bottomWin->activButton("STOP", notactive);
    bottomWin->activButton("BRIGHTNESS", notactive);
    bottomWin->activButton("END", notactive);

    if (bottomWin->childWin == NULL)
    {
        bottomWin->activButton("SCREENCONF", notactive);
        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, creat the new one
//        screenConfiguration* editScreenConfig = new screenConfiguration(LINES - hgtp1, COLS, hgtp1, 0, NULL, 0, squarebottombtn, NULL);
        editScreenConfig = new screenConfiguration(LINES - hgtp1, COLS, hgtp1, 0, NULL, 0, squarebottombtn, NULL);
        editScreenConfig->addButton("SAVE", SAVE, 0, "Save", "", callBtnWinScreenConfigSave);
        editScreenConfig->addButton("EXIT", EXIT, 1, "Exit", "", callBtnWinScreenConfig);
        editScreenConfig->addButton("BCKGND", EXIT, 2, "BACK", "GROUND", callBtnActiveBkgnd);
        editScreenConfig->addButton("TEXT", EXIT, 3, "TEXT", "", callBtnActiveTxt);

        editScreenConfig->activButton("BCKGND", notactive);
        // No child Window from Bottom window, create the new one

        editScreenConfig->addSmallButton("REDP", REDP, 0, " + ", NULL);
        editScreenConfig->addSmallButton("REDM", REDM, 1, " - ", NULL);
        editScreenConfig->addSmallButton("GREENP", GREENP, 2, " + ", NULL);
        editScreenConfig->addSmallButton("GREENM", GREENM, 3, " - ", NULL);
        editScreenConfig->addSmallButton("BLUEP", BLUEP, 4, " + ", NULL);
        editScreenConfig->addSmallButton("BLUEM", BLUEM, 5, " - ", NULL);
//        editScreenConfig->addSmallButton("BRGHTM", BRGHTM, 6, " - ", NULL);
//        editScreenConfig->addSmallButton("BRGHTP", BRGHTP, 7, " + ", NULL);
        editScreenConfig->addSmallButton("SSAVERM", SSAVERM, 6, " - ", NULL);
        editScreenConfig->addSmallButton("SSAVERP", SSAVERP, 7, " + ", NULL);
        editScreenConfig->addSmallButton("SSAVERMM", SSAVERMM, 8, "---", NULL);
        editScreenConfig->addSmallButton("SSAVERPP", SSAVERPP, 9, "+++", NULL);

        bottomWin->childWin = editScreenConfig;
        return editScreenConfig->get_winContainer();
/*        buttonWindow* selectionWin = new buttonWindow(5, COLS, Win->get_winContainer()->_begy -5, 0, NULL, -3, rectanglebtn, NULL);
        Win->showLink(3);
        selectionWin->addButton("DISPLAYCONF", DISPLAYCONF, 0, "display", "", &callBtnConfDisplay);
        selectionWin->addButton("TOUCHPANEL", TOUCHPANEL, 1, "touchpanel", "", &callBtnCalTouch);
        Win->childWin = selectionWin;
        return selectionWin->get_winContainer();  */
    } else
        {
            // ChildWindow exist for bottom Window, destroy it
            editScreenConfig->restaureConfigOrg();
            bottomWin->clean();
            bottomWin->childWin = NULL;
            mainWin->redrawWindow();
            mainWin->refreshWindow();
            ipWin->redrawWindow();
            ipWin->refreshWindow();
            // unfroze windows
            frozeScreen(false);
            return NULL;
        }
}

/***********************************************************************************
*  Function callBtnWinScreenConfigSave
***********************************************************************************/
void* callBtnWinScreenConfigSave(void* _a)
{
    editScreenConfig->saveScreenConfig();
    bottomWin->clean();
    bottomWin->childWin = NULL;
    mainWin->redrawWindow();
    mainWin->refreshWindow();
    ipWin->redrawWindow();
    ipWin->refreshWindow();
    // unfroze windows
    frozeScreen(false);
    return NULL;
}

/***********************************************************************************
*  Function callBtnBrigthness
***********************************************************************************/
void* callBtnBrigthness(void* _a)
{
    buttonWindow* Win = static_cast<buttonWindow*>(_a);
    bottomWin->activButton("STOP", notactive);
    bottomWin->activButton("SCREENCONF", notactive);
    bottomWin->activButton("END", notactive);
    pthread_mutex_lock(&mutexNcurses);
    wrefresh(bottomWin->get_winContentInfo());
    pthread_mutex_unlock(&mutexNcurses);

    if (Win->childWin == NULL)
    {
        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, create the new one
        buttonWindow* selectionWin = new buttonWindow(5, COLS, Win->get_winContainer()->_begy -5, 0, NULL, -2, rectanglebtn, NULL);
        Win->showLink(2);
        selectionWin->addButton("DECBRIGHTNESS", DECBRIGHTNESS, 0, "--", "", &callBtnBrightnessDec);
        selectionWin->addButton("INCBRIGHTNESS", INCBRIGHTNESS, 1, "++", "", &callBtnBrightnessInc);
        Win->childWin = selectionWin;
        //pthread_mutex_unlock(&mutexNcurses);
        return selectionWin->get_winContainer();
    } else
        {
            // ChildWindow exist for bottom Window, destroy it
            // First save brightness level to config file
            configXML->flushConfig(DISPLAY);
            bottomWin->clean();
            Win->childWin = NULL;
            pthread_mutex_lock(&mutexNcurses);
            redrawwin(mainWin->get_winContainer());
            wrefresh(mainWin->get_winContainer());
            redrawwin(ipWin->get_winContainer());
            wrefresh(ipWin->get_winContainer());
            pthread_mutex_unlock(&mutexNcurses);
            // unfroze windows
            frozeScreen(false);
            return NULL;
        }
}

/***********************************************************************************
*   function frozeScreen
***********************************************************************************/
void frozeScreen(bool froze)
{
    // init window system refresh;
    portWin->setRefresh(!froze);
    ipWin->setRefresh(!froze);
    mainWin->setRefresh(!froze);
}
/***********************************************************************************
*  CallBack function indiport
***********************************************************************************/
void *callIndiport(void *_mutex)
{
    //Initialisation timer
    struct timespec currentclock, lastclockupdate;
    clock_gettime(CLOCK_MONOTONIC, &currentclock);
    lastclockupdate = currentclock;
    bool first = true;

    char libelle[16] = "SERVER: ";
    pthread_mutex_lock(&mutexNcurses);
    wprintw(portWin->get_winContentInfo(), libelle);
    pthread_mutex_unlock(&mutexNcurses);

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &currentclock);
        if (((abs(currentclock.tv_sec - lastclockupdate.tv_sec) >= REFRESHPORTINT) || first) && portWin->getRefresh())
        {
            first = false;
            lastclockupdate = currentclock;

            pthread_mutex_lock(&mutexNcurses);
            wmove(portWin->get_winContentInfo(), 0, strlen(libelle));
            wclrtoeol(portWin->get_winContentInfo());
            pthread_mutex_unlock(&mutexNcurses);
            FILE *fp = popen("netstat -p -n -a 2> /dev/null|grep indiserver", "r");
            if (fp)
            {
                char *p=NULL, *p0=NULL, *e; size_t n;
                while ((getline(&p, &n, fp) > 0) && p)
                {
                    p0 = p;
                    if ((p0 = strstr(p0, "indiserver")))
                    {
                        p0 = p;
                        if ((p0 = strstr(p0, ":")))
                        {
                            ++p0;
                            if ((e = strchr(p0, ' ')))
                            {
                                *e='\0';
                                pthread_mutex_lock(&mutexNcurses);
                                mvwprintw(portWin->get_winContentInfo(), 0, strlen(libelle), "port %s", p0);
                                pthread_mutex_unlock(&mutexNcurses);
                            }
                        }
                    }
                }
                if (strlen(p) == 0)
                {
                    pthread_mutex_lock(&mutexNcurses);
                    mvwprintw(portWin->get_winContentInfo(),  0, strlen(libelle), "%s", "not found");
                    pthread_mutex_unlock(&mutexNcurses);
                }
                pthread_mutex_lock(&mutexNcurses);
                wrefresh(portWin->get_winContentInfo());
                pthread_mutex_unlock(&mutexNcurses);
                pclose(fp);
                // free the memory got by getline()
                free(p);
            }
        }
        usleep(SLEEPTIME);
        if (!portWin->isStopThread()) break;
    }
    pthread_exit(NULL);
}

/***********************************************************************************
*  CallBack function indiserver
***********************************************************************************/
void *callIndiserver(void *_mutex)
{
    // Prepare the command string
    const char* endCommand= "2>&1 &";

    if (configXML->isConfigValid())
    {
        char* drivers = configXML->getDrivers();
        char* params = configXML->getParameters();
        char* commandIndiServer = (char*) malloc(strlen(INDISERVERNAME) + strlen(params) + strlen(drivers) + strlen(endCommand) + sizeof(char));
        strcpy(commandIndiServer, INDISERVERNAME);
        strcat(commandIndiServer, params);
        strcat(commandIndiServer, drivers);
        strcat(commandIndiServer, endCommand);
        free(drivers);
        free(params);

        FILE *fp = popen(commandIndiServer, "r");
        if (fp)
        {
            char *p=NULL;
            size_t n;
            while (1)
            {
                if ((getline(&p, &n, fp) > 0) && p)
                {
                    if (mainWin->getRefresh())
                    {
                        mainWin->writeline(p, 2);
                        pthread_mutex_lock(&mutexNcurses);
                        wrefresh(mainWin->get_winContentInfo());
                        pthread_mutex_unlock(&mutexNcurses);
                    }
                }
                if (!mainWin->isStopThread()) break;
                usleep(SLEEPTIME);
            }
            pclose(fp);
            free(p);
        }
        free(commandIndiServer);
    }
    pthread_exit(NULL);
    return NULL;
}

/***********************************************************************************
*  CallBack function ipaddress
***********************************************************************************/
void *callMyIpaddress(void *_mutex)
{
    //Initialisation timer
    struct timespec currentclock, lastclockupdate;
    clock_gettime(CLOCK_MONOTONIC, &currentclock);
    lastclockupdate = currentclock;
    bool first = true;

    char buf[1024];
    struct ifconf ifc;
    struct ifreq *ifr;
    int sck;
    int nInterfaces;
    int i = 0;

    // Get a socket handle.
    sck = socket(AF_INET, SOCK_DGRAM, 0);
    if(sck < 0)
    {
        perror("socket");
        return NULL;
    }
    // Query available interfaces. */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &currentclock);
        if (((abs(currentclock.tv_sec - lastclockupdate.tv_sec) >= REFRESHIPINT) || first) && ipWin->getRefresh())
        {
            first = false;
            lastclockupdate = currentclock;

            if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
            {
                perror("ioctl(SIOCGIFCONF)");
                return NULL;
            }

            // Iterate through the list of interfaces. */
            ifr         = ifc.ifc_req;
            nInterfaces = ifc.ifc_len / sizeof(struct ifreq);

            pthread_mutex_lock(&mutexNcurses);
            wclear(ipWin->get_winContentInfo());  // if some issue use werase() instead of wclear()
            wrefresh(ipWin->get_winContentInfo());
            pthread_mutex_unlock(&mutexNcurses);
            // Get the interface name
            struct ifreq *item = &ifr[i];

            ipWin->writeline(1, 0, (char*) "IP     :", 2);
            ipWin->writeline(1, 9, inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr), 2);

            // Get the MAC address
            if(ioctl(sck, SIOCGIFHWADDR, item) < 0)
            {
                perror("ioctl(SIOCGIFHWADDR)");
                return NULL;
            }
            else
            {
                unsigned char *hwaddr = (unsigned char *)item->ifr_hwaddr.sa_data;

                pthread_mutex_lock(&mutexNcurses);
                mvwprintw(ipWin->get_winContentInfo(), 0, 0, "%s", item->ifr_name);
                mvwprintw(ipWin->get_winContentInfo(), 0, 7, ":");
                mvwprintw(ipWin->get_winContentInfo(), 0, 9, "%02X:%02X:%02X:%02X:%02X:%02X",
                          hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
                wrefresh(ipWin->get_winContentInfo());
                pthread_mutex_unlock(&mutexNcurses);
            }

            if (i < nInterfaces - 1)
                i++;
                else i = 0;
        }
        usleep(SLEEPTIME);
        if (!ipWin->isStopThread()) break;
    }
    pthread_exit(NULL);
 //   return 0;
}

/***********************************************************************************
*  CallBack function stopIndiserver
***********************************************************************************/
void* callBtnStopIndiserver(void* _a)
{
    portWin->setRefresh(true);
    int procID = getProcIdByName("indiserver");
    if (procID != -1)
        kill((pthread_t)procID, SIGKILL);
    if (getProcIdByName("indiserver") == -1)
    {
        mainWin->stopThread();
        static_cast<buttonWindow*>(_a)->activButton("STOPINDI", notactive);
        static_cast<buttonWindow*>(_a)->activButton("STARTINDI", active);
        static_cast<buttonWindow*>(_a)->activButton("EDITCONF", active);
        static_cast<buttonWindow*>(_a)->activButton("SHOWCONF", notactive);
        pthread_mutex_lock(&mutexNcurses);
        wclear(mainWin->get_winContentInfo());
        pthread_mutex_unlock(&mutexNcurses);
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnstartIndiserver
***********************************************************************************/
 void* callBtnStartIndiserver(void* _a)
{
    portWin->setRefresh(true);
    mainWin->startThread();

    // Give a delay to start all processes
    int counter = 0;
    while (getProcIdByName("indiserver") == -1)
    {
        usleep(5000);
        if (counter++ > 10) break;
    }
    if (getProcIdByName("indiserver") != -1)
    {
        static_cast<buttonWindow*>(_a)->activButton("STOPINDI", active);
        static_cast<buttonWindow*>(_a)->activButton("STARTINDI", notactive);
        static_cast<buttonWindow*>(_a)->activButton("EDITCONF", notactive);
        static_cast<buttonWindow*>(_a)->activButton("SHOWCONF", active);
        callBtnWinIndiServer(_a);
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnshowConfigIndiserver
***********************************************************************************/
void* callBtnShowConfigIndiserver(void* _a)
{
    if (bottomWin->childWin->childWin == NULL)
    {
        char param[32];
        char lineParameter[32];
        lineParameter[0] = '\0';

        bottomWin->childWin->deactiveAllButton();
        buttonWindow* showConfWin;
        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, create the new one
        showConfWin = new buttonWindow(LINES - hgtp1, COLS, hgtp1, 0, NULL, 0, rectanglebottombtn, NULL);
        showConfWin->addButton("EXIT", EXIT, 0, "Exit", "", &callBtnShowConfigIndiserver);

        bottomWin->childWin->childWin = showConfWin;

        // Get drivers lunched by Indiserver command and ruinning

        FILE *fp = popen("ps -eo cmd|grep indiserver 2>&1", "r");
        if (fp)
        {
            char *p=NULL, *p0=NULL, *e; size_t n; int line = 2;
            if ((getline(&p, &n, fp) > 0) && p);
            {
                // Save the begin of the string
                p0 = p;
                pthread_mutex_lock(&mutexNcurses);
                wattron(showConfWin->get_winContentInfo(), A_BOLD | A_UNDERLINE);
                pthread_mutex_unlock(&mutexNcurses);
                showConfWin->writeline(0, (showConfWin->get_winContentInfo()->_maxx / 2) - strlen("Parameters") / 2 + 1, (char*)"Parameters", 2);
                pthread_mutex_lock(&mutexNcurses);
                wattroff(showConfWin->get_winContentInfo(), A_BOLD | A_UNDERLINE);
                pthread_mutex_unlock(&mutexNcurses);

                // no option begin with -
                while ((p0 = strstr(p0, "-")))
                {
                    if ((e = strchr(p0, ' ')))
                    {
                        *e='\0';
                        if (strncmp("-v", p0, 2) != 0)
                        {
                            strcpy(param, p0);
                            strcat(param, " ");
                            *e = ' ';

                            p0 = e + 1;
                            if ((e = strchr(p0, ' ')))
                            {
                                *e='\0';
                                strcat(param, p0);
                                strcat(lineParameter, param);
                                strcat(lineParameter, " ");
                                *e = ' ';
                            }
                        }
                        else
                        {
                            strcat(lineParameter, p0);
                            strcat(lineParameter, " ");
                            *e = ' ';
                        }
                        ++p0;
                    } else break;
                    showConfWin->writeline(line, COLS / 2 - strlen(lineParameter) / 2 - 1, lineParameter, 2);
                    pthread_mutex_lock(&mutexNcurses);
                    wrefresh(showConfWin->get_winContentInfo());
                    pthread_mutex_unlock(&mutexNcurses);

                }  // Exemple indiserver -vv -p 7624 indi_eqmod_telescope indi_lynx_focus

                line = line + 2;
                pthread_mutex_lock(&mutexNcurses);
                wattron(showConfWin->get_winContentInfo(), A_BOLD | A_UNDERLINE);
                pthread_mutex_unlock(&mutexNcurses);
                showConfWin->writeline(line, (showConfWin->get_winContentInfo()->_maxx / 2) - strlen("Drivers loaded") / 2 + 1, (char*) "Drivers loaded", 2);
                pthread_mutex_lock(&mutexNcurses);
                wattroff(showConfWin->get_winContentInfo(), A_BOLD | A_UNDERLINE);
                pthread_mutex_unlock(&mutexNcurses);
                ++line;

                // Restore the pointer to start of the string
                p0 = p;
                while ((p0 = strstr(p0, "indi_")))
                {
                    ++line;
                    if ((e = strchr(p0, ' ')))
                    {
                        *e='\0';
                        showConfWin->writeline(line, COLS / 2 - strlen(p0) / 2 - 1, p0, 2);
                        pthread_mutex_lock(&mutexNcurses);
                        wrefresh(showConfWin->get_winContentInfo());
                        pthread_mutex_unlock(&mutexNcurses);
                        ++line;
                        *e = ' ';
                        ++p0;
                    }
                    else
                    {
                        showConfWin->writeline(line, COLS / 2 - strlen(p0) / 2 - 1, p0, 2);
                        pthread_mutex_lock(&mutexNcurses);
                        wrefresh(showConfWin->get_winContentInfo());
                        pthread_mutex_unlock(&mutexNcurses);
                        break;
                    }
                }
            }
           pclose(fp);
           free(p);
        }
    } else
    {
        // ChildWindow exist for bottom Window, destroy it
        delete bottomWin->childWin->childWin;
        bottomWin->childWin->childWin = NULL;
        delete bottomWin->childWin;
        bottomWin->childWin = NULL;
        mainWin->redrawWindow();
        mainWin->refreshWindow();
        ipWin->redrawWindow();
        ipWin->refreshWindow();
        bottomWin->redrawWindow();
        bottomWin->refreshWindow();
        bottomWin->showLink(0);
        bottomWin->activeAllButton();

        pthread_mutex_lock(&mutexNcurses);
        box(bottomWin->get_winContainer(), 0, 0);
        pthread_mutex_unlock(&mutexNcurses);

        return NULL;
    }

    return showConfWin;
}

/***********************************************************************************
*  CallBack function editConfigIndiserverSave
***********************************************************************************/
void* callBtnEditConfigIndiserverSave(void* _a)
{
    // First Save the drivers configuration
    editConfWin->saveDriversConfig();
    // Then destroy the window
    delete bottomWin->childWin->childWin;
    bottomWin->childWin->childWin = NULL;
    mainWin->redrawWindow();
    mainWin->refreshWindow();
    ipWin->redrawWindow();
    ipWin->refreshWindow();
    bottomWin->redrawWindow();
    bottomWin->refreshWindow();
    if (getProcIdByName("indiserver") != -1)
    {
        bottomWin->childWin->activButton("STOPINDI", active);
        bottomWin->childWin->activButton("SHOWCONF", active);
    }
    else
    {
        bottomWin->childWin->activButton("STARTINDI", active);
        bottomWin->childWin->activButton("EDITCONF", active);
    }
    bottomWin->childWin->showLink(0);
    bottomWin->childWin->showLink(-1);
    bottomWin->childWin->redrawWindow();
    bottomWin->childWin->refreshWindow();
    bottomWin->showLink(1);
    bottomWin->activButton("STOP", active);
    pthread_mutex_lock(&mutexNcurses);
    box(bottomWin->get_winContainer(), 0, 0);
    pthread_mutex_unlock(&mutexNcurses);

    return NULL;
}

/***********************************************************************************
*  CallBack function editConfigServer
***********************************************************************************/
void* callBtnEditConfigServer(void* _a)
{
    if (bottomWin->childWin->childWin == NULL)
    {
        bottomWin->deactiveAllButton();
        bottomWin->childWin->deactiveAllButton();
        bottomWin->childWin->activButton("EDITCONF", active);

        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, create the new one
        buttonWindow* selectionWin = new buttonWindow(5, COLS, bottomWin->childWin->get_winContainer()->_begy -5, 0, NULL, -4, rectanglebtn, NULL);
        bottomWin->childWin->showLink(4);
        selectionWin->addButton("PARAMETER", PARAMETER, 0, "Parameters", "", callBtnEditParamIndiserver);
        selectionWin->addButton("DRIVER", DRIVER, 1, "Drivers", "", callBtnEditConfigIndiserver);
        bottomWin->childWin->childWin = selectionWin;
    } else
    {
        // ChildWindow exist for bottom Window, destroy it
        delete bottomWin->childWin->childWin;
        bottomWin->childWin->childWin = NULL;
        mainWin->redrawWindow();
        mainWin->refreshWindow();
        ipWin->redrawWindow();
        ipWin->refreshWindow();
        bottomWin->redrawWindow();
        bottomWin->refreshWindow();
        if (getProcIdByName("indiserver") != -1)
        {
            bottomWin->childWin->activButton("STOPINDI", active);
            bottomWin->childWin->activButton("SHOWCONF", active);
        }
        else
        {
            bottomWin->childWin->activButton("STARTINDI", active);
            bottomWin->childWin->activButton("EDITCONF", active);
        }
        bottomWin->childWin->redrawWindow();
        bottomWin->childWin->refreshWindow();
        bottomWin->showLink(1);
        bottomWin->activButton("STOP", active);
        pthread_mutex_lock(&mutexNcurses);
        box(bottomWin->get_winContainer(), 0, 0);
        pthread_mutex_unlock(&mutexNcurses);
    }

    return NULL;
}

/***********************************************************************************
*  CallBack function callBtneditConfigIndiserver
***********************************************************************************/
void* callBtnEditConfigIndiserver(void* _a)
{
    if (bottomWin->childWin->childWin->childWin == NULL)
    {
        bottomWin->deactiveAllButton();
        bottomWin->childWin->deactiveAllButton();

        // Froze screen refresh of existing windows
        frozeScreen(true);
        // No child Window from Bottom window, create the new one
        editConfWin = new configIndiWindow(LINES - hgtp1, COLS, hgtp1, 0, NULL, 0, squarebottombtn, NULL);
        editConfWin->addButton("PREVIOUS", PREVIOUS, 0, "", "<-", callBtnEditConfigIndiserverPrevious);
        editConfWin->addButton("SAVE", SAVE, 1, "", "Save", callBtnEditConfigIndiserverSave);
        editConfWin->addButton("EXIT", EXIT, 2, "", "Exit", callBtnEditConfigIndiserver);
        editConfWin->addButton("NEXT", NEXT, 3, "", "->", callBtnEditConfigIndiserverNext);
        editConfWin->activButton("PREVIOUS", notactive);
        bottomWin->childWin->childWin->childWin = editConfWin;
    } else
    {
        // ChildWindow exist for bottom Window, destroy it
        delete bottomWin->childWin->childWin->childWin;
        bottomWin->childWin->childWin->childWin = NULL;
        delete bottomWin->childWin->childWin;
        bottomWin->childWin->childWin = NULL;
        mainWin->redrawWindow();
        mainWin->refreshWindow();
        ipWin->redrawWindow();
        ipWin->refreshWindow();
        bottomWin->redrawWindow();
        bottomWin->refreshWindow();
        if (getProcIdByName("indiserver") != -1)
        {
            bottomWin->childWin->activButton("STOPINDI", active);
            bottomWin->childWin->activButton("SHOWCONF", active);
        }
        else
        {
            bottomWin->childWin->activButton("STARTINDI", active);
            bottomWin->childWin->activButton("EDITCONF", active);
        }
        bottomWin->childWin->showLink(0);
        bottomWin->childWin->showLink(-1);
        bottomWin->childWin->redrawWindow();
        bottomWin->childWin->refreshWindow();
        bottomWin->showLink(1);
        bottomWin->activButton("STOP", active);
        pthread_mutex_lock(&mutexNcurses);
        box(bottomWin->get_winContainer(), 0, 0);
        pthread_mutex_unlock(&mutexNcurses);
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtneditParamIndiserver
***********************************************************************************/
void* callBtnEditParamIndiserver(void* _a)
{
    if (editParamWin == NULL)
    {
        bottomWin->deactiveAllButton();
        bottomWin->childWin->deactiveAllButton();
        bottomWin->childWin->childWin->deactiveAllButton();

        // Froze screen refresh of existing windows
        frozeScreen(true);

        // No child Window from Bottom window, create the new one
        editParamWin = new configIndiParamWindow(LINES - hgtp1, COLS, hgtp1, 0, NULL, 0, rectanglebottombtn, NULL);
        editParamWin->addButton("SAVE", SAVE, 0, "Save", "", callBtnEditParamIndiserverSave);
        editParamWin->addButton("EXIT", EXIT, 1, "Exit", "", callBtnEditParamIndiserver);
        editParamWin->addSmallButton("VERBOSEM", VERBOSEM, 0, " - ", NULL);
        editParamWin->addSmallButton("VERBOSEP", VERBOSEP, 1, " + ", NULL);
        editParamWin->addSmallButton("MEMM", MEMM, 2, " - ", NULL);
        editParamWin->addSmallButton("MEMP", MEMP, 3, " + ", NULL);
        editParamWin->addSmallButton("PORT10000P", PORT10000P, 4, " + ", NULL);
        editParamWin->addSmallButton("PORT10000M", PORT10000M, 5, " - ", NULL);
        editParamWin->addSmallButton("PORT1000P", PORT1000P, 6, " + ", NULL);
        editParamWin->addSmallButton("PORT1000M", PORT1000M, 7, " - ", NULL);
        editParamWin->addSmallButton("PORT100P", PORT100P, 8, " + ", NULL);
        editParamWin->addSmallButton("PORT100M", PORT100M, 9, " - ", NULL);
        editParamWin->addSmallButton("PORT10P", PORT10P, 10, " + ", NULL);
        editParamWin->addSmallButton("PORT10M", PORT10M, 11, " - ", NULL);
        editParamWin->addSmallButton("PORT1P", PORT1P, 12, " + ", NULL);
        editParamWin->addSmallButton("PORT1M", PORT1M, 13, " - ", NULL);
        bottomWin->childWin->childWin->childWin = editParamWin;

        editParamWin->initialValue();
    } else
    {
        delete editParamWin;
        editParamWin = NULL;
        delete bottomWin->childWin->childWin;
        bottomWin->childWin->childWin = NULL;
        mainWin->redrawWindow();
        mainWin->refreshWindow();
        ipWin->redrawWindow();
        ipWin->refreshWindow();
        bottomWin->redrawWindow();
        bottomWin->refreshWindow();
        if (getProcIdByName("indiserver") != -1)
        {
            bottomWin->childWin->activButton("STOPINDI", active);
            bottomWin->childWin->activButton("SHOWCONF", active);
        }
        else
        {
            bottomWin->childWin->activButton("STARTINDI", active);
            bottomWin->childWin->activButton("EDITCONF", active);
        }
        bottomWin->childWin->showLink(0);
        bottomWin->childWin->showLink(-1);
        bottomWin->childWin->redrawWindow();
        bottomWin->childWin->refreshWindow();
        bottomWin->showLink(1);
        bottomWin->activButton("STOP", active);
        pthread_mutex_lock(&mutexNcurses);
        box(bottomWin->get_winContainer(), 0, 0);
        pthread_mutex_unlock(&mutexNcurses);
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnEditParamIndiserverSave
***********************************************************************************/
void* callBtnEditParamIndiserverSave(void* _a)
{
    // First Save the drivers configuration

    editParamWin->saveParamsConfig();
    // Then destroy the window
    delete editParamWin;
    editParamWin = NULL;
    delete bottomWin->childWin->childWin;
    bottomWin->childWin->childWin = NULL;
    mainWin->redrawWindow();
    mainWin->refreshWindow();
    ipWin->redrawWindow();
    ipWin->refreshWindow();
    bottomWin->redrawWindow();
    bottomWin->refreshWindow();
    if (getProcIdByName("indiserver") != -1)
    {
        bottomWin->childWin->activButton("STOPINDI", active);
        bottomWin->childWin->activButton("SHOWCONF", active);
    }
    else
    {
        bottomWin->childWin->activButton("STARTINDI", active);
        bottomWin->childWin->activButton("EDITCONF", active);
    }
    bottomWin->childWin->showLink(0);
    bottomWin->childWin->showLink(-1);
    bottomWin->childWin->redrawWindow();
    bottomWin->childWin->refreshWindow();
    bottomWin->showLink(1);
    bottomWin->activButton("STOP", active);
    pthread_mutex_lock(&mutexNcurses);
    box(bottomWin->get_winContainer(), 0, 0);
    pthread_mutex_unlock(&mutexNcurses);
    return NULL;
}

/***********************************************************************************
*  CallBack function editConfigIndiserverNext
***********************************************************************************/
void* callBtnEditConfigIndiserverNext(void* _a)
{
    // Selection "Next"
    editConfWin->writeDeviceInfoNextPage();
    return NULL;
}

/***********************************************************************************
*  CallBack function editConfigIndiserverPrevious
***********************************************************************************/
void* callBtnEditConfigIndiserverPrevious(void* _a)
{
    // Selection "Previous"
    editConfWin->writeDeviceInfoPreviousPage();
    return NULL;
}

/***********************************************************************************
*  CallBack function increaseBrightness
***********************************************************************************/
void* callBtnBrightnessDec(void* _a)
{
    int value = configXML->getScreenBrithness();
    if (value > 1)
    {
        value--;
        configXML->setScreenBrightness(value);
#ifndef LAPTOP
        pwmWrite(18, value);
#endif // LAPTOP
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function decreaseBrightness
***********************************************************************************/
void* callBtnBrightnessInc(void* _a)
{
    int value = configXML->getScreenBrithness();
    if (value < 10)
    {
        value++;
        configXML->setScreenBrightness(value);
#ifndef LAPTOP
        pwmWrite(18, value);
#endif // LAPTOP
    }
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnConfDisplay
***********************************************************************************/
void* callBtnConfDisplay(void* _a)
{
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnCalTouch
***********************************************************************************/
void* callBtnCalTouch(void* _a)
{
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnActiveBkgnd
***********************************************************************************/
void* callBtnActiveBkgnd(void * _a)
{
    editScreenConfig->setBackgroundActive(true);
    editScreenConfig->activButton("BCKGND", notactive);
    editScreenConfig->activButton("TEXT", active);
    editScreenConfig->drawContent();
    return NULL;
}

/***********************************************************************************
*  CallBack function callBtnActiveTxt
***********************************************************************************/
void* callBtnActiveTxt(void * _a)
{
    editScreenConfig->setBackgroundActive(false);
    editScreenConfig->activButton("BCKGND", active);
    editScreenConfig->activButton("TEXT", notactive);
    editScreenConfig->drawContent();
    return NULL;
}

/***********************************************************************************
*  Function getProcIdByName
***********************************************************************************/
int getProcIdByName(string procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                ifstream cmdFile(cmdPath.c_str());
                string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }
    closedir(dp);

    return pid;
}
