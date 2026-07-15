/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "string.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_lcd.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

#define BTN_UP_PORT     GPIOC
#define BTN_UP_PIN      GPIO_PIN_0

#define BTN_DOWN_PORT   GPIOC
#define BTN_DOWN_PIN    GPIO_PIN_3

#define BTN_OK_PORT     GPIOF
#define BTN_OK_PIN      GPIO_PIN_3

#define BTN_RIGHT_PORT  GPIOF
#define BTN_RIGHT_PIN   GPIO_PIN_5

#define BTN_LEFT_PORT   GPIOF
#define BTN_LEFT_PIN    GPIO_PIN_10
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2004c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2004c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2004c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2004c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */

#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t app_state = 0;
char entered_pin[5] = {0};
uint8_t pin_index = 0;
char secret_pin[] = "1234";

typedef struct {
	uint8_t state;
	uint8_t last_tick;
} Button_TypeDef;

Button_TypeDef btn_up = {0, 0};
Button_TypeDef btn_down = {0, 0};
Button_TypeDef btn_left = {0, 0};
Button_TypeDef btn_right = {0, 0};
Button_TypeDef btn_ok = {0, 0};

uint8_t cur_row = 0;
uint8_t cur_col = 0;

// Таблица соответствий ASCII -> USB HID Scancode (US QWERTY)
const uint8_t asciiMap[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0-7
    0x2A, 0x2B, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, // 8-15 (Backspace, Tab, Enter)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 16-23
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 24-31
    0x2C,          // 32: Пробел
    0x1E | 0x80,   // 33: ! (Shift + 1)
    0x34 | 0x80,   // 34: " (Shift + ')
    0x20 | 0x80,   // 35: # (Shift + 3)
    0x21 | 0x80,   // 36: $ (Shift + 4)
    0x22 | 0x80,   // 37: % (Shift + 5)
    0x24 | 0x80,   // 38: & (Shift + 7)
    0x34,          // 39: '
    0x26 | 0x80,   // 40: ( (Shift + 9)
    0x27 | 0x80,   // 41: ) (Shift + 0)
    0x25 | 0x80,   // 42: * (Shift + 8)
    0x2E | 0x80,   // 43: + (Shift + =)
    0x36,          // 44: ,
    0x2D,          // 45: -
    0x37,          // 46: .
    0x38,          // 47: /
    0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, // 48-57: Цифры 0-9
    0x33 | 0x80,   // 58: : (Shift + ;)
    0x33,          // 59: ;
    0x36 | 0x80,   // 60: < (Shift + ,)
    0x2E,          // 61: =
    0x37 | 0x80,   // 62: > (Shift + .)
    0x38 | 0x80,   // 63: ? (Shift + /)
    0x1F | 0x80,   // 64: @ (Shift + 2)
    0x04|0x80, 0x05|0x80, 0x06|0x80, 0x07|0x80, 0x08|0x80, 0x09|0x80, 0x0A|0x80, 0x0B|0x80, // 65-72: A-H
    0x0C|0x80, 0x0D|0x80, 0x0E|0x80, 0x0F|0x80, 0x10|0x80, 0x11|0x80, 0x12|0x80, 0x13|0x80, // 73-80: I-P
    0x14|0x80, 0x15|0x80, 0x16|0x80, 0x17|0x80, 0x18|0x80, 0x19|0x80, 0x1A|0x80, 0x1B|0x80, // 81-88: Q-X
    0x1C|0x80, 0x1D|0x80, // 89-90: Y-Z
    0x2F,          // 91: [
    0x31,          // 92: Обратный слэш
    0x30,          // 93: ]
    0x23 | 0x80,   // 94: ^ (Shift + 6)
    0x2D | 0x80,   // 95: _ (Shift + -)
    0x35,          // 96: `
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, // 97-104: a-h
    0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, // 105-112: i-p
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, // 113-120: q-x
    0x1C, 0x1D,    // 121-122: y-z
    0x2F | 0x80,   // 123: {
    0x31 | 0x80,   // 124: |
    0x30 | 0x80,   // 125: }
    0x35 | 0x80,   // 126: ~
    0x00           // 127: DEL
};

void Send_Char(char c) {
	if (c < 0 || c > 127)
		return;

	uint8_t keycode = asciiMap[(uint8_t)c];
	if (keycode == 0)
		return;

	uint8_t report[8] = {0};

	// Если есть флаг Shift
	if (keycode & 0x80) {
		report[0] = 0x2;
		report[2] = keycode & 0x7F;
	}

	else
		report [2] = keycode;

	USBD_HID_SendReport(&hUsbDeviceFS, report, 8);
	HAL_Delay(25);

	uint8_t empty[8] = {0};
	USBD_HID_SendReport(&hUsbDeviceFS, empty, 8);
	HAL_Delay(25);
}

void Send_Str(const char* str) {
	while(*str)
		Send_Char(*str++);
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
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_DEVICE_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(100);

  lcd_init();
  lcd_put_cur(0, 0);
  lcd_send_string("  Password Manager");

  lcd_put_cur(2, 2);
  lcd_send_string("Press OK to enter");
  lcd_put_cur(3, 0);
  lcd_send_string("      PIN-code");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	if (HAL_GPIO_ReadPin(BTN_OK_PORT, BTN_OK_PIN) == GPIO_PIN_RESET) {
		if (app_state == 0) {
			app_state = 1;
			pin_index = 0;
			memset(entered_pin, 0, sizeof(entered_pin));

		lcd_clear();
		lcd_put_cur(0, 6);
		lcd_send_string("1  2  3");
		lcd_put_cur(1, 6);
		lcd_send_string("4  5  6");
		lcd_put_cur(2, 6);
		lcd_send_string("7  8  9");
		lcd_put_cur(3, 9);
		lcd_send_string("0");

		lcd_send_cmd(0x0F);
		cur_row = 0;
		cur_col = 6;
		lcd_put_cur(cur_row, cur_col);
		HAL_Delay(300);
		} else if (app_state == 1) {
			char selected_digit = 0;

		    if (cur_row == 0 && cur_col == 6)  selected_digit = '1';
		    else if (cur_row == 0 && cur_col == 9)  selected_digit = '2';
		    else if (cur_row == 0 && cur_col == 12) selected_digit = '3';
		    else if (cur_row == 1 && cur_col == 6)  selected_digit = '4';
		    else if (cur_row == 1 && cur_col == 9)  selected_digit = '5';
		    else if (cur_row == 1 && cur_col == 12) selected_digit = '6';
		    else if (cur_row == 2 && cur_col == 6)  selected_digit = '7';
		    else if (cur_row == 2 && cur_col == 9)  selected_digit = '8';
		    else if (cur_row == 2 && cur_col == 12) selected_digit = '9';
		    else if (cur_row == 3 && cur_col == 9)  selected_digit = '0';

		    if (selected_digit != 0) {
		    	entered_pin[pin_index] = selected_digit;
		    	lcd_send_cmd(0x0C);
		    	lcd_put_cur(3, pin_index);
		    	lcd_send_string("*");
		    	lcd_send_cmd(0x0F);
		    	lcd_put_cur(cur_row, cur_col);

		    	pin_index++;

		    	if (pin_index == 4) {
		    		lcd_send_cmd(0x0C);
		    		lcd_clear();

		    		if (strcmp(entered_pin, secret_pin) == 0) {
		    			lcd_put_cur(1, 3);
		    		    lcd_send_string("ACCESS GRANTED");
		    		    HAL_Delay(2000);
		    		    app_state = 2;
		    		} else {
		    			lcd_put_cur(1, 3);
		    		    lcd_send_string("ACCESS DENIED");
		    		    HAL_Delay(2000);

		    		    app_state = 0;
		    		    lcd_clear();
		    		    lcd_put_cur(1, 1);
		    		    lcd_send_string("Password Manager");
		    		    lcd_put_cur(2, 1);
		    		    lcd_send_string("Press OK to start");
		    		}
		    	}
		    }
		}

		HAL_Delay(200);
	}

	if (HAL_GPIO_ReadPin(BTN_UP_PORT, BTN_UP_PIN) == GPIO_PIN_RESET && cur_row > 0) {
		cur_row--;
		lcd_put_cur(cur_row, cur_col);
		HAL_Delay(200);
	} else if (HAL_GPIO_ReadPin(BTN_DOWN_PORT, BTN_DOWN_PIN) == GPIO_PIN_RESET && cur_row < 3) {
		cur_row++;
		if (cur_row == 3)
			cur_col = 9;
		lcd_put_cur(cur_row, cur_col);
		HAL_Delay(200);
	} else if (HAL_GPIO_ReadPin(BTN_RIGHT_PORT, BTN_RIGHT_PIN) == GPIO_PIN_RESET && cur_col < 12) {
		cur_col+= 3;
		lcd_put_cur(cur_row, cur_col);
		HAL_Delay(200);
	} else if (HAL_GPIO_ReadPin(BTN_LEFT_PORT, BTN_LEFT_PIN) == GPIO_PIN_RESET && cur_col > 6) {
		cur_col-= 3;
		lcd_put_cur(cur_row, cur_col);
		HAL_Delay(200);
	}


	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) {
		Send_Str("Admin123!\n");
		HAL_Delay(500);
	}
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
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
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00808CD2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PF3 PF5 PF10 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
