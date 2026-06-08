#ifndef DISPLAY_H
#define DISPLAY_H

#include "main.h"

/**
 * @brief 4자리 세그먼트 디스플레이를 멀티플렉싱 방식으로 새로고침한다.
 *        TIM3 인터럽트에서 주기적으로 호출된다.
 * @param digits 표시할 4자리 숫자 배열 (각 0~9)
 */
void Display_Refresh(const uint8_t *digits);

#endif /* DISPLAY_H */
