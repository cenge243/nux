#ifndef __BEEP_H
#define __BEEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  POLICE_MUSIC = 1,
  AMBULANCE_MUSIC,
  TWO_TIGER_MUSIC,
  PEPPA_PIG_MUSIC,
  SUPER_MARIO_MUSIC,
  FIGHT_LANDLORD_MUSCI,
  DIDA_MUSIC,
}MUSIC_INDEX;


void enable_beep_music(void);
void test_beep(void);
void beep_piano(void);
void beep_play_music(uint8_t volume_level,MUSIC_INDEX index);
void beep_bgm_stop(void);
void beep_play_bgm_period(void);
void beep_play_bgm(uint8_t volume_level,MUSIC_INDEX index,uint8_t cycle);  
void beep_play_music_test(void);

#ifdef __cplusplus
}
#endif

#endif 
