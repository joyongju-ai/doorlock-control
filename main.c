/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private define ------------------------------------------------------------*/
#define KEYPAD_ROWS     4
#define KEYPAD_COLS     4
#define DIGIT_ON        GPIO_PIN_RESET  // 공통 애노드: LOW = ON
#define DIGIT_OFF       GPIO_PIN_SET
#define INPUT_TIMEOUT   3000            // 마지막 입력 후 3초 경과 시 자동 초기화

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* 키패드 Row 핀 배열 (출력) */
GPIO_TypeDef* rowPorts[4] = {GPIOA, GPIOC, GPIOB, GPIOC};
uint16_t rowPins[4]       = {GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_0, GPIO_PIN_1};

/* 키패드 Col 핀 배열 (입력) */
GPIO_TypeDef* colPorts[4] = {GPIOA, GPIOA, GPIOA, GPIOB};
uint16_t colPins[4]       = {GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_7};

/* 7-세그먼트 각 세그먼트(A~G, DP)에 연결된 MCU 핀 */
GPIO_TypeDef *ports[] = {GPIOB, GPIOB, GPIOA, GPIOB, GPIOB, GPIOB, GPIOB, GPIOA};
uint16_t pins[]       = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_10, GPIO_PIN_3,
                          GPIO_PIN_5, GPIO_PIN_4, GPIO_PIN_10, GPIO_PIN_8};

/* 4자리 세그먼트 공통 단자 (자리 선택용) */
GPIO_TypeDef *digitPorts[4] = {GPIOC, GPIOC, GPIOB, GPIOA};
uint16_t digitPins[4]       = {GPIO_PIN_7, GPIO_PIN_9, GPIO_PIN_6, GPIO_PIN_0};

/* 숫자별 세그먼트 패턴 (공통 애노드 기준, 비트 = A~DP 순서) */
uint8_t digit_patterns[] = {
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

/* 키패드 문자 배열 */
char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

uint8_t input_idx    = 0;              // 현재 입력된 자릿수
uint8_t digits[4]    = {0, 0, 0, 0};  // 입력된 숫자 저장 배열
uint32_t input_time  = 0;             // 마지막 키 입력 시각 (ms)
uint8_t password_mode = 0;            // 0: 대기, 1: 비밀번호 입력 중
char password[]      = "1234";        // 초기 비밀번호

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_ICACHE_Init(void);
char Keypad_Scan(void);

/**
 * @brief 4x4 키패드를 스캔하여 눌린 키를 반환합니다.
 * @return 눌린 키의 문자값, 없으면 0
 */
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

/**
 * @brief 4자리 세그먼트 디스플레이를 멀티플렉싱 방식으로 새로고침합니다.
 *        TIM3 인터럽트에서 주기적으로 호출됩니다.
 */
void Display_Refresh(void)
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

/**
 * @brief TIM3 주기 인터럽트 콜백 - 세그먼트 새로고침 트리거
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        Display_Refresh();  // 세그먼트 새로고침
    }
}

/**
 * @brief 메인 함수 - 도어락 제어 메인 루프
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_ICACHE_Init();

    HAL_TIM_Base_Start_IT(&htim3);       // TIM3 인터럽트 시작 (세그먼트 새로고침용)
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);  // PWM 출력 시작 (스피커용)

    while (1)
    {
        char key = Keypad_Scan();

        // '*' 또는 '#' 키 입력 시 비밀번호 입력 모드 시작
        if (key == '*' || key == '#') {
            password_mode = 1;
            input_idx = 0;
            for (int i = 0; i < 4; i++) digits[i] = 0;  // 입력 초기화
            input_time = HAL_GetTick();

            // 입력 시작 알림음 (짧게 1번)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 32767);
            HAL_Delay(150);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
            HAL_Delay(100);
        }

        // 비밀번호 입력 모드에서 숫자 키 입력 처리
        if (password_mode && key != 0 && input_idx < 4 && key >= '0' && key <= '9')
        {
            digits[input_idx++] = key - '0';  // 숫자 저장
            input_time = HAL_GetTick();        // 마지막 입력 시간 갱신

            // 키 입력 알림음
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 32767);
            HAL_Delay(100);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
            HAL_Delay(100);
        }

        // 3초 이상 입력 없으면 자동 초기화 (HAL_GetTick 기반 논블로킹 타임아웃)
        if (password_mode && (HAL_GetTick() - input_time > INPUT_TIMEOUT))
        {
            input_idx = 0;
            for (int i = 0; i < 4; i++) digits[i] = 0;
            password_mode = 0;  // 대기 상태로 복귀

            // 타임아웃 알림음 (길게 1번)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 32767);
            HAL_Delay(300);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
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
                // 성공 알림음: 짧게 3번
                for (int i = 0; i < 3; i++)
                {
                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 32767);
                    HAL_Delay(100);
                    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
                    HAL_Delay(100);
                }
            }
            else
            {
                // 실패 알림음: 길게 1번
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 32767);
                HAL_Delay(500);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
                HAL_Delay(100);
            }

            // 입력 초기화
            input_idx = 0;
            for (int i = 0; i < 4; i++) digits[i] = 0;
            input_time = HAL_GetTick();
            password_mode = 0;
        }

        HAL_Delay(1);  // 키패드 폴링 간격
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // HSE 외부 클럭 + PLL 설정
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 2;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();

    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

static void MX_ICACHE_Init(void)
{
    // 명령어 캐시 활성화 (실행 성능 향상)
    if (HAL_ICACHE_Enable() != HAL_OK) Error_Handler();
}

static void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    // TIM3: 세그먼트 새로고침 및 스피커 PWM 겸용
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 65535;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) Error_Handler();

    // PWM 채널 1 설정 (스피커 출력, 초기 듀티: 50%)
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 32767;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

    HAL_TIM_MspPostInit(&htim3);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 출력 핀 초기 레벨 설정
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_3
                            | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7 | GPIO_PIN_9, GPIO_PIN_RESET);

    // PC13: 외부 인터럽트 입력 (Rising Edge)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 키패드 Row 출력 핀 (PC0, PC1) 및 세그먼트 자리 선택 핀 (PC7, PC9)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 키패드 Row 출력 핀 (PA0, PA1) 및 세그먼트 출력 핀 (PA10)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA2: USART2 TX (Alternate Function)
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 키패드 Col 입력 핀 (PA5, PA6, PA7) - Pull-up
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 세그먼트 출력 핀 (PB)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15
                        | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 세그먼트 DP 핀 (PA8) - Pull-up
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 키패드 Col 입력 핀 (PB7) - Pull-up
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PC6: TIM3_CH1 PWM 출력 핀 (스피커)
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    // 에러 발생 시 무한 루프 (디버깅용)
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
