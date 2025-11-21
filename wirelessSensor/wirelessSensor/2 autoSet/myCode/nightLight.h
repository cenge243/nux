#ifndef __NIGHT_LIGHT_H
#define __NIGHT_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void night_light_init(void);
void night_light_task(void);
void night_light_task_period(void);

#ifdef __cplusplus
}
#endif

#endif 
