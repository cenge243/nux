#ifndef __SENSOR_LAYER_H__
#define __SENSOR_LAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void sensor_layer_init(void);
void sensor_layer_task(void);
void sensor_layer_task_period(void);
  

#ifdef __cplusplus
}
#endif
#endif 
