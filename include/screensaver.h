#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include <pthread.h>
#include <time.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>

/***********************************************************************************
*   Class screenSaver
***********************************************************************************/
class screenSaver_c
{
public:

    void (*ptrFunction)(void*) = NULL;

    screenSaver_c(int _delay, int _gpio);
    ~screenSaver_c();
    bool isStopThread();
    void startThread();
    void stopThread();
    void updateDelay(int _delay, int _brithnesslevel);
    void resetTimer(int _brithnesslevel);
    bool isSaverActive();

protected:

private:
    pthread_t threadsTimer;
};


#endif // SCREENSAVER_H
