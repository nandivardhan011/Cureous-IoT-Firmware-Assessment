#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;

typedef struct
{
    uint32_t button_id;
    uint32_t timestamp;
} ButtonEvent_t;

osMessageQueueId_t ButtonEventQueueHandle;
osThreadId_t LoggerTaskHandle;

const osThreadAttr_t LoggerTask_attributes = {
    .name = "LoggerTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal
};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartLoggerTask(void *argument);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    ButtonEvent_t event;

    event.timestamp = osKernelGetTickCount();

    if (GPIO_Pin == GPIO_PIN_13)
    {
        event.button_id = 1;
    }
    else if (GPIO_Pin == GPIO_PIN_0)
    {
        event.button_id = 2;
    }
    else
    {
        return;
    }

    osMessageQueuePut(ButtonEventQueueHandle, &event, 0, 0);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();

    osKernelInitialize();

    ButtonEventQueueHandle = osMessageQueueNew(
        10,
        sizeof(ButtonEvent_t),
        NULL
    );

    if (ButtonEventQueueHandle == NULL)
    {
        Error_Handler();
    }

    LoggerTaskHandle = osThreadNew(
        StartLoggerTask,
        NULL,
        &LoggerTask_attributes
    );

    if (LoggerTaskHandle == NULL)
    {
        Error_Handler();
    }

    osKernelStart();

    while (1)
    {
    }
}

void StartLoggerTask(void *argument)
{
    ButtonEvent_t event;
    char json[80];

    (void)argument;

    for (;;)
    {
        if (osMessageQueueGet(
                ButtonEventQueueHandle,
                &event,
                NULL,
                osWaitForever) == osOK)
        {
            int length = snprintf(
                json,
                sizeof(json),
                "{\"button_id\":%lu,\"timestamp\":%lu}\r\n",
                (unsigned long)event.button_id,
                (unsigned long)event.timestamp
            );

            if (length > 0 && length < sizeof(json))
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)json,
                    (uint16_t)length,
                    100
                );
            }
        }
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}

#endif
