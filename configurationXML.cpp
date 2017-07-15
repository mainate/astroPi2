#include "astrocurses.h"

extern objWindow* portWin;
extern textWindow* mainWin;

/***********************************************************************************
*   Class configurationXML
***********************************************************************************/
configurationXML::configurationXML(const char* _fileName)
{
    // Get local path of the running program and add the extention for config filename
    char uncFileName[ PATH_MAX + strlen(".xml")];
    ssize_t count = readlink( "/proc/self/exe", uncFileName, PATH_MAX);
    uncFileName[count] = '\0';
    strcat(uncFileName, ".xml");
    strcpy(configFileName, uncFileName);

    // Open the config xml file to iterate through
    XMLDocument xmlconf;
    if (xmlconf.LoadFile(uncFileName) != XML_SUCCESS)
    {
        XMLDocument xmlconf;
    }

    {
        // Get the root of the XML tree
        XMLElement *pListRoot = xmlconf.FirstChildElement("astroconfig");
        if (pListRoot == NULL)
        {
            pListRoot = xmlconf.NewElement("astroconfig");
            xmlconf.InsertFirstChild(pListRoot);
            xmlconf.SaveFile(uncFileName);
        }

        //read paramters to set the var form the class

        // Read the screen configuration
        XMLElement *pListScreen = pListRoot->FirstChildElement("screen");
        if (pListScreen == NULL)
        {
            pListScreen = xmlconf.NewElement("screen");
            pListRoot->InsertFirstChild(pListScreen);
            xmlconf.SaveFile(uncFileName);
        }

        XMLElement *pListBrightness = pListScreen->FirstChildElement("brightness");
        if (pListBrightness == NULL)
        {
            XMLElement *pParam;
            pListBrightness = xmlconf.NewElement("brightness");
            pListRoot->FirstChildElement("screen")->InsertEndChild(pListBrightness);
            pParam = xmlconf.NewElement("param");
            pParam->SetText("4");
            pListBrightness->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        pListBrightness->FirstChildElement("param")->QueryIntText(&screenSetting.brightness);


        XMLElement *pscreensaver = pListScreen->FirstChildElement("screensaver");
        if (pscreensaver == NULL)
        {
            XMLElement *pParam;
            pscreensaver = xmlconf.NewElement("screensaver");
            pListRoot->FirstChildElement("screen")->InsertEndChild(pscreensaver);
            pParam = xmlconf.NewElement("param");
            pParam->SetText("120");
            pscreensaver->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        pscreensaver->FirstChildElement("param")->QueryIntText(&screenSetting.screensaver);



        XMLElement *pListBkgndColor = pListScreen->FirstChildElement("colorbkgnd");
        if (pListBkgndColor == NULL)
        {
            XMLElement *pParam;
            pListBkgndColor = xmlconf.NewElement("colorbkgnd");
            pListRoot->FirstChildElement("screen")->InsertEndChild(pListBkgndColor);
            pParam = xmlconf.NewElement("red");
            pParam->SetText("4");
            pListBkgndColor->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("green");
            pParam->SetText("0");
            pListBkgndColor->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("blue");
            pParam->SetText("0");
            pListBkgndColor->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        pListBkgndColor->FirstChildElement("red")->QueryIntText(&screenSetting.colorbkgnd.red);
        pListBkgndColor->FirstChildElement("green")->QueryIntText(&screenSetting.colorbkgnd.green);
        pListBkgndColor->FirstChildElement("blue")->QueryIntText(&screenSetting.colorbkgnd.blue);

        XMLElement *pListColor = pListScreen->FirstChildElement("colortext");
        if (pListColor == NULL)
        {
            XMLElement *pParam;
            pListColor = xmlconf.NewElement("colortext");
            pListRoot->FirstChildElement("screen")->InsertEndChild(pListColor);
            pParam = xmlconf.NewElement("red");
            pParam->SetText("5");
            pListColor->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("green");
            pParam->SetText("5");
            pListColor->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("blue");
            pParam->SetText("5");
            pListColor->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        pListColor->FirstChildElement("red")->QueryIntText(&screenSetting.colortext.red);
        pListColor->FirstChildElement("green")->QueryIntText(&screenSetting.colortext.green);
        pListColor->FirstChildElement("blue")->QueryIntText(&screenSetting.colortext.blue);

        XMLElement *pListOffset = pListScreen->FirstChildElement("offsetscreen");
        if (pListOffset == NULL)
        {
            XMLElement *pParam;
            pListOffset = xmlconf.NewElement("offsetscreen");
            pListRoot->FirstChildElement("screen")->InsertEndChild(pListOffset);
            pParam = xmlconf.NewElement("offset_x");
            pParam->SetText("0");
            pListOffset->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("offset_y");
            pParam->SetText("0");
            pListOffset->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        pListOffset->FirstChildElement("offset_x")->QueryIntText(&screenSetting.offsetscreen.x);
        pListOffset->FirstChildElement("offset_y")->QueryIntText(&screenSetting.offsetscreen.y);

        // Get the parameters for indiserver
        XMLElement *pListParameters = pListRoot->FirstChildElement("parameters");
        if (pListParameters == NULL)
        {
            pListParameters = xmlconf.NewElement("parameters");
            pListRoot->InsertFirstChild(pListParameters);
            xmlconf.SaveFile(uncFileName);
        }

        XMLElement *pListVerbose = pListParameters->FirstChildElement("verbose");
        if (pListVerbose == NULL)
        {
            XMLElement *pParam;
            pListVerbose = xmlconf.NewElement("verbose");
            pListRoot->FirstChildElement("parameters")->InsertEndChild(pListVerbose);
            pParam = xmlconf.NewElement("param");
            pParam->SetText("vv");
            pListVerbose->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        strcpy(parameters.verbose, pListVerbose->FirstChildElement("param")->GetText());

        XMLElement *pListClientMem = pListParameters->FirstChildElement("clientmem");
        if (pListClientMem == NULL)
        {
            XMLElement *pParam;
            pListClientMem = xmlconf.NewElement("clientmem");
            pListRoot->FirstChildElement("parameters")->InsertEndChild(pListClientMem);
            pParam = xmlconf.NewElement("param");
            pParam->SetText("64");
            pListClientMem->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        strcpy(parameters.maxMemClient, pListClientMem->FirstChildElement("param")->GetText());

        XMLElement *pListParam = pListParameters->FirstChildElement("ipport");
        if (pListParam == NULL)
        {
            XMLElement *pParam;
            pListParam = xmlconf.NewElement("ipport");
            pListRoot->FirstChildElement("parameters")->InsertEndChild(pListParam);
            pParam = xmlconf.NewElement("param");
            pParam->SetText("7624");
            pListParam->InsertEndChild(pParam);
            xmlconf.SaveFile(uncFileName);
        }
        strcpy(parameters.ipPort, pListParam->FirstChildElement("param")->GetText());

        // Get the drivers to be started by indi
        XMLElement *pListDrivers = pListRoot->FirstChildElement("drivers");
        if (pListDrivers == NULL)
        {
            pListDrivers = xmlconf.NewElement("drivers");
            pListRoot->InsertFirstChild(pListDrivers);
            xmlconf.SaveFile(uncFileName);
        }

        drivers = new driversIndiConfig_t;
        driversIndiConfig_t* tempo = drivers;

        XMLElement *pListDevice = pListDrivers->FirstChildElement("device");
        if (pListDevice == NULL)
        {
            XMLElement *pParam;
            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_telescope");
            pParam->SetAttribute("name", "Telescope Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_focus");
            pParam->SetAttribute("name", "Focuser Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_ccd");
            pParam->SetAttribute("name", "CCD Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_wheel");
            pParam->SetAttribute("name", "Filter Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_gps");
            pParam->SetAttribute("name", "GPS Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            pListDevice = xmlconf.NewElement("device");
            pListRoot->FirstChildElement("drivers")->InsertEndChild(pListDevice);
            pParam = xmlconf.NewElement("driver");
            pParam->SetText("indi_simulator_dome");
            pParam->SetAttribute("name", "Dome Simulator");
            pListDevice->InsertEndChild(pParam);
            pParam = xmlconf.NewElement("version");
            pParam->SetText("1.0");
            pListDevice->InsertEndChild(pParam);

            xmlconf.SaveFile(uncFileName);

            pListDevice = pListDrivers->FirstChildElement("device");
        }
        //Loop through configured driver
        while  (pListDevice != NULL)
        {
            strcpy(tempo->driverFile, pListDevice->FirstChildElement("driver")->GetText());
            strcpy(tempo->driverName, pListDevice->FirstChildElement("driver")->Attribute("name"));
            strcpy(tempo->driverVersion, pListDevice->FirstChildElement("version")->GetText());
            pListDevice = pListDevice->NextSiblingElement("device");
            if (pListDevice != NULL)
            {
                tempo->nextDriver = new driversIndiConfig_t;
                tempo = tempo->nextDriver;
            }
        }

        ConfigValid = true;
    }
}

configurationXML::~configurationXML()
{
    if (ConfigValid || (drivers != NULL))
    {
        driversIndiConfig_t* tempo = drivers->nextDriver;
        delete drivers;
        while (tempo != NULL)
        {
            drivers = tempo;
            tempo = drivers->nextDriver;
            delete drivers;
        }
    }
}

color_t configurationXML::getScreenBackgroundColor()
{
    return screenSetting.colorbkgnd;
}

bool configurationXML::setScreenBackgroundColor(color_t _color)
{
    screenSetting.colorbkgnd = _color;
    return true;
}

color_t configurationXML::getScreenTextColor()
{
    return screenSetting.colortext;
}

bool configurationXML::setScreenTextColor(color_t _color)
{
    screenSetting.colortext = _color;
    return true;
}

char* configurationXML::getParameters()
{
    char* _params;
    // Allocate space for space char + \0
    _params = (char*) malloc(2 * sizeof(char));
    strcpy(_params, " ");
    if (strlen(parameters.verbose) != 0)
    {
        _params = (char*) realloc(_params, 2 *sizeof(char) + strlen(_params) + strlen(parameters.verbose));
        strcat(_params, "-");
        strcat(_params, parameters.verbose);
    }
    if (parameters.maxMemClient[0] != '\0')
    {
        _params = (char*) realloc(_params, 5 * sizeof(char) + strlen(_params) + strlen(parameters.maxMemClient));
        strcat(_params, " -m ");
        strcat(_params, parameters.maxMemClient);
    }
    if (parameters.ipPort[0] != '\0')
    {
        _params = (char*) realloc(_params, 5 * sizeof(char) + strlen(_params) + strlen(parameters.ipPort));
        strcat(_params, " -p ");
        strcat(_params, parameters.ipPort);
    }
    _params = (char*) realloc(_params, 2 * sizeof(char) + strlen(_params));
    if (strlen(_params) == 0)
        strcpy(_params, " ");
    else
        strcat(_params, " ");
    return strdup(_params);
}

int configurationXML::getScreenSaveDelay()
{
    return screenSetting.screensaver;
}

void configurationXML::setScreenSaveDelay(int _delay)
{
    screenSetting.screensaver = _delay;
}

parametersIndiConfig_t configurationXML::getFullParameters()
{
    return parameters;
}

bool configurationXML::setParameters(const char* _verbose, int _maxMemClient, int _ipPort)
{
    return true;
}

const driversIndiConfig_t* configurationXML::getFullDrivers()
{
    return drivers;
}

char* configurationXML::getDrivers()
{
    char* _stringDrivers;
    if (drivers != NULL)
    {
        _stringDrivers = (char*) malloc(2 * sizeof(char) + sizeof(drivers->driverFile));
        driversIndiConfig_t* tempo = drivers;
        strcpy(_stringDrivers, drivers->driverFile);
        strcat(_stringDrivers, " ");
        drivers = drivers->nextDriver;
        while (drivers != NULL)
        {
            _stringDrivers = (char*) realloc(_stringDrivers, 2 * sizeof(char) + strlen(drivers->driverFile) + strlen(_stringDrivers));
            strcat(_stringDrivers, drivers->driverFile);
            strcat(_stringDrivers, " ");
            drivers = drivers->nextDriver;
        }
        drivers = tempo;
    }
    else
    {
        _stringDrivers = (char*) malloc(sizeof (char));
        _stringDrivers[0] = '\0';
    }
  //  return _stringDrivers;
  return strdup(_stringDrivers);
}

bool configurationXML::flushConfig(flush_e selection)
{
    // Open the config xml file to iterate through
    XMLDocument xmlconf;
    if (xmlconf.LoadFile(configFileName) == XML_SUCCESS)
    {
        // Get the root of the XML tree
        XMLNode *pRoot = xmlconf.FirstChildElement("astroconfig");


        if ((selection == ALL) || (selection == DRIVERS))
        {
            XMLElement *pDrivers = NULL, *pDevice = NULL, *pDriver = NULL, *pVersion = NULL;

            //clean the drivers tree
            pRoot->DeleteChild(pRoot->FirstChildElement("drivers"));

            // if level drivers not exist, create it
            pDrivers = pRoot->FirstChildElement("drivers");
            if (pDrivers == NULL)
            {
                pDrivers = xmlconf.NewElement("drivers");
                pRoot->InsertFirstChild(pDrivers);
            }

            // iterate throught the pagesList data to find configured driver
            // and introduce it in configuration
            driversIndiConfig_t* tempo = drivers;
            while (tempo != NULL)
            {
                // add the new device in the tree
                pDevice = xmlconf.NewElement("device");
                pRoot->FirstChildElement("drivers")->InsertEndChild(pDevice);

                pDriver = xmlconf.NewElement("driver");
                pDriver->SetText(tempo->driverFile);
                pDriver->SetAttribute("name", tempo->driverName);

                pVersion = xmlconf.NewElement("version");
                pVersion->SetText(tempo->driverVersion);

                pDevice->InsertEndChild(pDriver);
                pDevice->InsertEndChild(pVersion);

                tempo = tempo->nextDriver;
            }
        }
        if ((selection == ALL) || (selection == PARAMETERS))
        {
            XMLElement *pParameters = NULL, *pVerbose = NULL,  *pClientMem = NULL, *pPort = NULL, *pParam = NULL;
            //clean the parameters tree
            pRoot->DeleteChild(pRoot->FirstChildElement("parameters"));
            // if level parameters not exist, create it
            pParameters = pRoot->FirstChildElement("parameters");
            if (pParameters == NULL)
            {
                pParameters = xmlconf.NewElement("parameters");
                pRoot->InsertFirstChild(pParameters);
            }

            pVerbose = xmlconf.NewElement("verbose");
            pRoot->FirstChildElement("parameters")->InsertEndChild(pVerbose);

            pParam = xmlconf.NewElement("param");
            pParam->SetText(parameters.verbose);
            pVerbose->InsertEndChild(pParam);

            pClientMem = xmlconf.NewElement("clientmem");
            pRoot->FirstChildElement("parameters")->InsertEndChild(pClientMem);

            pParam = xmlconf.NewElement("param");
            pParam->SetText(parameters.maxMemClient);
            pClientMem->InsertEndChild(pParam);

            pPort = xmlconf.NewElement("ipport");
            pRoot->FirstChildElement("parameters")->InsertEndChild(pPort);

            pParam = xmlconf.NewElement("param");
            pParam->SetText(parameters.ipPort);
            pPort->InsertEndChild(pParam);
        }
        if ((selection == ALL) || (selection == DISPLAY))
        {
            XMLElement *pScreen = NULL, *pColor = NULL, *pBrightness = NULL, *pSaver = NULL, *pOffset = NULL, *pParam = NULL;
            //clean the screen tree
            pRoot->DeleteChild(pRoot->FirstChildElement("screen"));
            // if level screen not exist, create it
            pScreen = pRoot->FirstChildElement("screen");
            if (pScreen == NULL)
            {
                pScreen = xmlconf.NewElement("screen");
                pRoot->InsertFirstChild(pScreen);
            }

            pBrightness = xmlconf.NewElement("brightness");
            pRoot->FirstChildElement("screen")->InsertEndChild(pBrightness);

            pParam = xmlconf.NewElement("param");
            pParam->SetText(screenSetting.brightness);
            pBrightness->InsertEndChild(pParam);

            pColor = xmlconf.NewElement("colorbkgnd");
            pRoot->FirstChildElement("screen")->InsertEndChild(pColor);

            pParam = xmlconf.NewElement("red");
            pParam->SetText(screenSetting.colorbkgnd.red);
            pColor->InsertEndChild(pParam);

            pParam = xmlconf.NewElement("green");
            pParam->SetText(screenSetting.colorbkgnd.green);
            pColor->InsertEndChild(pParam);

            pParam = xmlconf.NewElement("blue");
            pParam->SetText(screenSetting.colorbkgnd.blue);
            pColor->InsertEndChild(pParam);

            pColor = xmlconf.NewElement("colortext");
            pRoot->FirstChildElement("screen")->InsertEndChild(pColor);

            pParam = xmlconf.NewElement("red");
            pParam->SetText(screenSetting.colortext.red);
            pColor->InsertEndChild(pParam);

            pParam = xmlconf.NewElement("green");
            pParam->SetText(screenSetting.colortext.green);
            pColor->InsertEndChild(pParam);

            pParam = xmlconf.NewElement("blue");
            pParam->SetText(screenSetting.colortext.blue);
            pColor->InsertEndChild(pParam);

            pOffset = xmlconf.NewElement("offsetscreen");
            pRoot->FirstChildElement("screen")->InsertEndChild(pOffset);

            pParam = xmlconf.NewElement("offset_x");
            pParam->SetText(screenSetting.offsetscreen.x);
            pOffset->InsertEndChild(pParam);

            pParam = xmlconf.NewElement("offset_y");
            pParam->SetText(screenSetting.offsetscreen.y);
            pOffset->InsertEndChild(pParam);

            pSaver = xmlconf.NewElement("screensaver");
            pRoot->FirstChildElement("screen")->InsertEndChild(pSaver);

            pParam = xmlconf.NewElement("param");
            pParam->SetText(screenSetting.screensaver);
            pSaver->InsertEndChild(pParam);
        }
    }
    else return false;

    if (xmlconf.SaveFile(configFileName) != XML_SUCCESS)
        return false;
        else return true;

}

bool configurationXML::refreshConfig()
{
    return true;
}

bool configurationXML::isConfigValid()
{
    return ConfigValid;
}

bool configurationXML::isConfigured(const char *name)
{
    driversIndiConfig_t* tempo = drivers;

    //Loop through configured driver
    while (tempo != NULL)
    {
        bool resp = strcmp(tempo->driverFile, name);
        if (!resp) return true;
        tempo = tempo->nextDriver;
    }
    return false;
}

void configurationXML::cleanDrivers()
{
    if (ConfigValid || (drivers != NULL))
    {
        driversIndiConfig_t* tempo = drivers->nextDriver;
        delete drivers;
        while (tempo != NULL)
        {
            drivers = tempo;
            tempo = drivers->nextDriver;
            delete drivers;
        }
        drivers = NULL;
    }
}

bool configurationXML::addDrivers(char* _drvFile, char* _drvName, char* _drvVer)
{
    if (drivers == NULL)
    {
        drivers = new driversIndiConfig_t;
        strcpy(drivers->driverFile, _drvFile);
        strcpy(drivers->driverName, _drvName);
        strcpy(drivers->driverVersion, _drvVer);
    } else
    {
        driversIndiConfig_t* tempo = drivers;
        // Go to the last drivers
        while (tempo->nextDriver != NULL) tempo = tempo->nextDriver;
        tempo->nextDriver = new driversIndiConfig_t;
        tempo = tempo->nextDriver;
        strcpy(tempo->driverFile, _drvFile);
        strcpy(tempo->driverName, _drvName);
        strcpy(tempo->driverVersion, _drvVer);
    }
    return true;
}

void configurationXML::cleanParams()
{
    parameters.verbose[0] = '\0';
    parameters.maxMemClient[0] = '\0';
    parameters.ipPort[0] = '\0';
}

void configurationXML::addParams(char* _verbose, char* _clientmem, char* _portip)
{
    strcpy(parameters.verbose, _verbose);
    strcpy(parameters.maxMemClient, _clientmem);
    strcpy(parameters.ipPort, _portip);
}

screenConfig_t configurationXML::getScreenConfig()
{
    return screenSetting;
}

void configurationXML::setScreenBrightness(int _brightness)
{
    screenSetting.brightness = _brightness;
}

int configurationXML::getScreenBrithness()
 {
    return screenSetting.brightness;
 }

void configurationXML::cleanScreen()
{
    screenSetting.brightness = 0;
    screenSetting.colorbkgnd.red = 4; screenSetting.colorbkgnd.green = 0; screenSetting.colorbkgnd.blue = 0;
    screenSetting.colortext.red = 5; screenSetting.colortext.green = 5; screenSetting.colortext.blue = 5;
    screenSetting.brightness = 5;
}
