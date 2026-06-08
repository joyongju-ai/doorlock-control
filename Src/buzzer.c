#include "buzzer.h"

#define BUZZER_DUTY  32767  // PWM 듀티 (소리 ON)

/* 지정 시간(ms)만큼 소리를 한 번 내고 끈다. */
static void beep(TIM_HandleTypeDef *htim, uint32_t on_ms)
{
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, BUZZER_DUTY);
    HAL_Delay(on_ms);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0);
}

void Buzzer_Start(TIM_HandleTypeDef *htim)
{
    // 입력 시작 알림음 (짧게 1번)
    beep(htim, 150);
    HAL_Delay(100);
}

void Buzzer_Key(TIM_HandleTypeDef *htim)
{
    // 키 입력 알림음
    beep(htim, 100);
    HAL_Delay(100);
}

void Buzzer_Timeout(TIM_HandleTypeDef *htim)
{
    // 타임아웃 알림음 (길게 1번)
    beep(htim, 300);
}

void Buzzer_Success(TIM_HandleTypeDef *htim)
{
    // 성공 알림음: 짧게 3번
    for (int i = 0; i < 3; i++)
    {
        beep(htim, 100);
        HAL_Delay(100);
    }
}

void Buzzer_Fail(TIM_HandleTypeDef *htim)
{
    // 실패 알림음: 길게 1번
    beep(htim, 500);
    HAL_Delay(100);
}
