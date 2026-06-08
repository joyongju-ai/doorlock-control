#ifndef DOORLOCK_H
#define DOORLOCK_H

#include "main.h"

/**
 * @brief 도어락 상태를 초기화한다. (대기 상태, 입력 비움)
 */
void Doorlock_Init(void);

/**
 * @brief 키 입력 한 건을 처리한다. 메인 루프에서 매 주기 호출한다.
 *        비밀번호 입력 시작, 숫자 입력, 타임아웃, 4자리 완료 시 검증을
 *        모두 이 함수가 담당한다. 알림음은 htim의 PWM으로 출력한다.
 * @param htim 부저 PWM에 사용할 타이머 핸들
 */
void Doorlock_Process(TIM_HandleTypeDef *htim);

/**
 * @brief 디스플레이에 표시할 현재 입력 숫자 배열을 반환한다.
 *        세그먼트 새로고침(Display_Refresh)에서 사용한다.
 * @return 4자리 숫자 배열 포인터
 */
const uint8_t *Doorlock_GetDigits(void);

#endif /* DOORLOCK_H */
