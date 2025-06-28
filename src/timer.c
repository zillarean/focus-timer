#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <stdlib.h>
#include "timer.h"

void timer_update(Timer *timer, int elapsed_seconds){
    if (timer->paused) return;
    timer->seconds += elapsed_seconds;
    while (timer->seconds >= 60)
    {
        timer->minutes++;
        timer->seconds -= 60;
    }
    while (timer->minutes >= 60)
    {
        timer->hours++;
        timer->minutes -= 60;
    }
}
