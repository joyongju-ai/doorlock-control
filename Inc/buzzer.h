#ifndef BUZZER_H
#define BUZZER_H

#include "main.h"

/* 알림음 재생 함수. 내부적으로 PWM 듀티를 켜고 끄며 지정 시간만큼 소리를 낸다.
 * 원본과 동일하게 HAL_Delay 기반으로 동작한다. */

void Buzzer_Start(TIM_HandleTypeDef *htim);    // 입력 시작음 (150ms)
void Buzzer_Key(TIM_HandleTypeDef *htim);      // 키 입력음 (100ms)
void Buzzer_Timeout(TIM_HandleTypeDef *htim);  // 타임아웃음 (300ms)
void Buzzer_Success(TIM_HandleTypeDef *htim);  // 성공음 (100ms 3회)
void Buzzer_Fail(TIM_HandleTypeDef *htim);     // 실패음 (500ms)

#endif /* BUZZER_H */
