#include "display.h"

/* 7-세그먼트 각 세그먼트(A~G, DP)에 연결된 MCU 핀 */
static GPIO_TypeDef *ports[] = {GPIOB, GPIOB, GPIOA, GPIOB, GPIOB, GPIOB, GPIOB, GPIOA};
static uint16_t pins[]       = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_10, GPIO_PIN_3,
                                GPIO_PIN_5, GPIO_PIN_4, GPIO_PIN_10, GPIO_PIN_8};

/* 4자리 세그먼트 공통 단자 (자리 선택용) */
static GPIO_TypeDef *digitPorts[4] = {GPIOC, GPIOC, GPIOB, GPIOA};
static uint16_t digitPins[4]       = {GPIO_PIN_7, GPIO_PIN_9, GPIO_PIN_6, GPIO_PIN_0};

/* 숫자별 세그먼트 패턴 (공통 애노드 기준, 비트 = A~DP 순서) */
static uint8_t digit_patterns[] = {
    0xEF, // 0
    0x28, // 1
    0xB3, // 2
    0xBA, // 3
    0x7C, // 4
    0xDA, // 5
    0xDB, // 6
    0xE8, // 7
    0xFF, // 8
    0xFA  // 9
};

void Display_Refresh(const uint8_t *digits)
{
    static int digit_idx = 0;

    // 모든 자리 OFF
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(digitPorts[i], digitPins[i], GPIO_PIN_SET);
    }

    // 현재 자리의 숫자 패턴 가져오기
    uint8_t pattern = (digits[3 - digit_idx] <= 9) ? digit_patterns[digits[3 - digit_idx]] : 0;

    // 세그먼트 A~DP 출력
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(ports[i], pins[i],
                          (pattern & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // 현재 자리 ON
    HAL_GPIO_WritePin(digitPorts[digit_idx], digitPins[digit_idx], GPIO_PIN_RESET);
    digit_idx = (digit_idx + 1) % 4;  // 다음 자리로 이동
}
