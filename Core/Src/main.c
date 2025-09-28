/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pfeiffer_protocol.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SLAVE_ADDR 1
#define QUERY_ACTION_CHAR "00"	//a c-literal is already null-terminated. we need null-terminated for sprintf in build_telegram()
#define CTRL_ACTION_CHAR "10"
#define QUERY_DATA_LENGTH 2
#define QUERY_DATA "=?"
#define STM_ERROR_LENGTH 15
#define STM_ERROR_DATA "STM32 Rx Error!"
#define ERROR_PARAM 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char rxdata[32]={0}; //for huart1
char rxdata2[32]={0}; //for huart2
size_t count = 0;
uint16_t rxlen;
uint16_t rxcnt=0;
volatile bool receive_flag=false, transmit_ready=false;

//char *assembled_telegram, *received_telegram;
char telegram[32], received_telegram[32];
int param;
int data_length;
char data_to_send[16]; //max size of data that can be sent is 16

Telegram parsed_telegram;
uint8_t rxByte;
uint16_t rxIndex = 0;

//-------below variables for looping messages.
volatile bool next=false, wait = false;
char messages[12][4] = {"326","303","360","361","362","363","364","365","366","367","368","369"};
int message_index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) //for the ReceiveToIdle_IT func.
{
    if (huart->Instance == USART2)  // Change if using a different UART
    {
        rxlen = Size;
        rxcnt++;
        receive_flag = true;
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t*)rxdata2, sizeof(rxdata2));
    }


}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // Transmission complete for huart1
		//master max3485 receiving
		HAL_GPIO_WritePin(GPIOB, master_direction_Pin, GPIO_PIN_RESET);


    }
    if (huart->Instance == USART2) {
		// Transmission complete for huart1

		free_telegram_memory(parsed_telegram);

		//-------below is for looping mechanism
//		if(message_index == 12){
//			next = true;
//			message_index = 0;
//			strcpy(rxdata2, messages[message_index]);
//		}
//		else{
//			wait = true;
//			message_index++;
//			strcpy(rxdata2, messages[message_index]);
//		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rxdata[rxIndex++] = rxByte;

    // Your custom termination logic here
    if (rxByte == '\r' || rxIndex >= sizeof(rxdata)) {
        // Process complete message
    	rxdata[rxIndex] ='\0';  // \r has already been added. so now we end the str with \0
    	transmit_ready = true;
        rxIndex = 0;
    }

    // Restart single byte reception
    HAL_UART_Receive_IT(&huart1, &rxByte, 1);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t*)rxdata2, sizeof(rxdata2));   //comment this line out for the looping method.
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);

  //-------first iteration for the looping method. Uncomment these two lines for the looping method.
//  strcpy(rxdata2, messages[message_index]);
//  receive_flag=true;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(next){					//'next' and 'wait' flags are for the looping method and will not affect anything if its kept there.
		  HAL_Delay(2000);
		  receive_flag=true;
	  }

	  if(wait){
		  HAL_Delay(1500);
		  receive_flag = true;
	  }

	  if(transmit_ready){
		  transmit_ready = false;

		  if(rxdata[0] =='\0'){
				// The new string is a pointer to the non-null character
				for (size_t i = 0; i < sizeof(rxdata) && rxdata[i] == '\0'; i++) {
				  count++;
				}
				char* newString = &rxdata[count];
				strcpy(rxdata, newString);

				parsed_telegram = parse_telegram(rxdata);
			}
			else{
				parsed_telegram = parse_telegram(rxdata);
			}

			if (parsed_telegram.error_code == 0){
				 build_telegram(parsed_telegram.slave_address, parsed_telegram.action_char, parsed_telegram.param_number, parsed_telegram.data_length, parsed_telegram.command_str);
			}
			else{
				build_telegram(parsed_telegram.slave_address, "10\0", parsed_telegram.param_number, parsed_telegram.data_length, parsed_telegram.command_str);
			}

			HAL_UART_Transmit_IT(&huart2, (uint8_t*)received_telegram, strlen(telegram)); //works

			memset(rxdata, 0, sizeof(rxdata));
			count=0;
		}

	  if (receive_flag){
		  wait=false;
		  next = false;
		  receive_flag = false;

		  if(rxlen<=4){
			  sscanf((char*)rxdata2, "%d", &param);
			  build_telegram(SLAVE_ADDR, QUERY_ACTION_CHAR, param, QUERY_DATA_LENGTH, QUERY_DATA);
		  }
		  else{
			  sscanf((char*)rxdata2, "%d,%s", &param, data_to_send);
			  build_telegram(SLAVE_ADDR, CTRL_ACTION_CHAR, param, strlen(data_to_send), data_to_send);
		  }

		HAL_GPIO_WritePin(GPIOB, master_direction_Pin, GPIO_PIN_SET);
		HAL_Delay(200);
	    HAL_UART_Transmit_IT(&huart1, (uint8_t*)telegram, strlen(telegram));//works

	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD2_Pin|master_direction_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin master_direction_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|master_direction_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
