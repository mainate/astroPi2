#include "screensaver.h"

#define LAPTOP

#ifndef LAPTOP
    #include <wiringPi.h>
#endif // LAPTOP

static bool isSaverScreenActive = false;
static int saveDelay = 10;
static int gpioPin = 18;
static struct timespec currentclock, lastclockupdate;
static bool isThreadRunning = false;
//mutex
pthread_mutex_t mutexScreenSaver;

void* screenSaverTimer(void *)
{
    do
    {
        clock_gettime(CLOCK_MONOTONIC, &currentclock);
        pthread_mutex_lock(&mutexScreenSaver);
        if ((abs(currentclock.tv_sec - lastclockupdate.tv_sec) >= saveDelay) && !isSaverScreenActive)
        {
#ifndef LAPTOP
            // Set PWM to 0
            pwmWrite(gpioPin, 0);
#endif // LAPTOP
            // Set stet of sleepScreen
            isSaverScreenActive = true;
        }
        pthread_mutex_unlock(&mutexScreenSaver);
        sleep(1);
    } while (isThreadRunning);
    pthread_exit(NULL);
};


/***********************************************************************************
*   Class screenSaver
***********************************************************************************/
screenSaver_c::screenSaver_c(int _delay, int _gpio)
{
    saveDelay = _delay;
    gpioPin = _gpio;
    isSaverScreenActive = false;
    isThreadRunning = false;
    clock_gettime(CLOCK_MONOTONIC, &lastclockupdate);
    //mutex
    if (pthread_mutex_init(&mutexScreenSaver, NULL) != 0)
    {
        printf("\n mutex 'mutexScreenSaver' init failed\n");
        exit(EXIT_FAILURE);
    }
}

screenSaver_c::~screenSaver_c()
{
    pthread_mutex_destroy(&mutexScreenSaver);
}

bool screenSaver_c::isStopThread()
{
    return !isThreadRunning;
}

void screenSaver_c::startThread()
{
    isThreadRunning = true;
    pthread_create(&threadsTimer, NULL, screenSaverTimer, NULL);
}

void screenSaver_c::stopThread()
{
    isThreadRunning = false;
    pthread_join(threadsTimer, NULL);
}

void screenSaver_c::updateDelay(int _delay, int _brithnesslevel)
{
    pthread_mutex_lock(&mutexScreenSaver);
    saveDelay = _delay;
    pthread_mutex_unlock(&mutexScreenSaver);
    resetTimer(_brithnesslevel);
}

void screenSaver_c::resetTimer(int _brithnesslevel)
{
    pthread_mutex_lock(&mutexScreenSaver);
    clock_gettime(CLOCK_MONOTONIC, &currentclock);
    lastclockupdate = currentclock;
    if (isSaverScreenActive)
    {
#ifndef LAPTOP
        // Set PWM to default or set value
        pwmWrite(gpioPin, _brithnesslevel);
#endif // LAPTOP
        isSaverScreenActive = false;
    }
    pthread_mutex_unlock(&mutexScreenSaver);
}

bool screenSaver_c::isSaverActive()
{
    pthread_mutex_lock(&mutexScreenSaver);
    bool resp = isSaverScreenActive;
    pthread_mutex_unlock(&mutexScreenSaver);
    return resp;
}
