#include "doorlock.h"
#include "keypad.h"
#include "buzzer.h"

#define INPUT_TIMEOUT   3000  // 마지막 입력 후 3초 경과 시 자동 초기화

static uint8_t input_idx     = 0;             // 현재 입력된 자릿수
static uint8_t digits[4]     = {0, 0, 0, 0};  // 입력된 숫자 저장 배열
static uint32_t input_time   = 0;             // 마지막 키 입력 시각 (ms)
static uint8_t password_mode = 0;             // 0: 대기, 1: 비밀번호 입력 중
static char password[]       = "1234";        // 초기 비밀번호

void Doorlock_Init(void)
{
    input_idx = 0;
    for (int i = 0; i < 4; i++) digits[i] = 0;
    input_time = 0;
    password_mode = 0;
}

const uint8_t *Doorlock_GetDigits(void)
{
    return digits;
}

void Doorlock_Process(TIM_HandleTypeDef *htim)
{
    char key = Keypad_Scan();

    // '*' 또는 '#' 키 입력 시 비밀번호 입력 모드 시작
    if (key == '*' || key == '#') {
        password_mode = 1;
        input_idx = 0;
        for (int i = 0; i < 4; i++) digits[i] = 0;  // 입력 초기화
        input_time = HAL_GetTick();

        Buzzer_Start(htim);
    }

    // 비밀번호 입력 모드에서 숫자 키 입력 처리
    if (password_mode && key != 0 && input_idx < 4 && key >= '0' && key <= '9')
    {
        digits[input_idx++] = key - '0';  // 숫자 저장
        input_time = HAL_GetTick();        // 마지막 입력 시간 갱신

        Buzzer_Key(htim);
    }

    // 3초 이상 입력 없으면 자동 초기화 (HAL_GetTick 기반 논블로킹 타임아웃)
    if (password_mode && (HAL_GetTick() - input_time > INPUT_TIMEOUT))
    {
        input_idx = 0;
        for (int i = 0; i < 4; i++) digits[i] = 0;
        password_mode = 0;  // 대기 상태로 복귀

        Buzzer_Timeout(htim);
    }

    // 4자리 입력 완료 시 비밀번호 검증
    if (input_idx >= 4)
    {
        uint8_t correct = 1;
        for (int i = 0; i < 4; i++)
        {
            if (digits[i] != (password[i] - '0'))
            {
                correct = 0;
                break;
            }
        }

        if (correct)
        {
            Buzzer_Success(htim);
        }
        else
        {
            Buzzer_Fail(htim);
        }

        // 입력 초기화
        input_idx = 0;
        for (int i = 0; i < 4; i++) digits[i] = 0;
        input_time = HAL_GetTick();
        password_mode = 0;
    }
}
