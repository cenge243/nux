#ifndef __TRAFFIC_H__
#define __TRAFFIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void traffic_init(void);
void traffic_task(void);

void set_uart_light(void);
 

#ifdef __cplusplus
}
#endif
#endif 
