#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <stdlib.h>
#include "timer.h"

/*
int main (int argc, char *argv[]){

    Timer timer = {
    .hours = 0,
    .minutes = 0,
    .seconds = 0,
    .paused = false,
};
    printf("Elapsed time: %02d:%02d:%02d", timer.hours, timer.minutes, timer.seconds);
    Sleep(1000);
    while (1)
    {
        update(&timer);
        Sleep(1000);
    }
}
*/
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
