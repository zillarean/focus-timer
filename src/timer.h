#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

typedef struct {
    int hours;
    int minutes;
    int seconds;
    bool paused;
} Timer;

void timer_update(Timer *timer, int elapsed_seconds);

#ifdef __cplusplus
}
#endif

#endif // TIMER_H_
