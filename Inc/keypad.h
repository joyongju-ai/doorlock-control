#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"

/**
 * @brief 4x4 키패드를 스캔하여 눌린 키를 반환한다.
 * @return 눌린 키의 문자값, 없으면 0
 */
char Keypad_Scan(void);

#endif /* KEYPAD_H */
