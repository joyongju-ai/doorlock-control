#include "keypad.h"

/* 키패드 Row 핀 배열 (출력) */
static GPIO_TypeDef *rowPorts[4] = {GPIOA, GPIOC, GPIOB, GPIOC};
static uint16_t rowPins[4]       = {GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_0, GPIO_PIN_1};

/* 키패드 Col 핀 배열 (입력) */
static GPIO_TypeDef *colPorts[4] = {GPIOA, GPIOA, GPIOA, GPIOB};
static uint16_t colPins[4]       = {GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_7};

/* 키패드 문자 배열 */
static char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char Keypad_Scan(void)
{
    for (int row = 0; row < 4; row++)
    {
        // 모든 Row를 HIGH로 설정
        for (int r = 0; r < 4; r++) {
            HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_SET);
        }
        // 현재 Row만 LOW로 설정하여 해당 행 활성화
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_RESET);
        HAL_Delay(1);  // 신호 안정화 대기

        // 각 Col 입력 확인
        for (int col = 0; col < 4; col++)
        {
            if (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET)
            {
                return keymap[row][col];  // 눌린 키 반환
            }
        }
    }
    return 0;  // 눌린 키 없음
}
