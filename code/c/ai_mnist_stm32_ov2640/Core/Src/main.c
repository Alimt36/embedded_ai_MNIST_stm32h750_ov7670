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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ov7670.h"

#include <stdio.h>
#include <string.h>

#include "mnist_784_8_10.h"

//---------------------------------------------------------------------------------------------------------------------------
// mode select : pick exactly ONE, comment out the others
//   ---> CAMERA_UART_DEBUG   : full raw frame + 28x28 buffer streamed over UART, no OLED
//                              touched at all, I2C4/PD12-PD13 never initialized
//                              (original working debug/dev pipeline)
//   ---> CAMERA_OLED_DISPLAY : predicted digit + 28x28 preview shown on the SSD1306,
//                              UART only used for the DIGIT: result line, no raw frame dump
//                              (supervisor-facing hardware demo mode)
//   ---> CAMERA_UART_AND_OLED: both active together, full debug UART stream AND live OLED
//                              display each loop (root cause of the earlier "UART dies"
//                              bug was a CubeMX regression that silently remapped USART3
//                              off PB10/PB11 onto PD8/PD9, colliding with I2C4's PD12/PD13 -
//                              not a real pin/hardware conflict - so this combined mode is
//                              safe now that USART3 is confirmed back on PB10/PB11)
//---------------------------------------------------------------------------------------------------------------------------
// #define CAMERA_UART_DEBUG
// #define CAMERA_OLED_DISPLAY
#define CAMERA_UART_AND_OLED

#if (defined(CAMERA_UART_DEBUG) + defined(CAMERA_OLED_DISPLAY) + defined(CAMERA_UART_AND_OLED)) > 1
    #error "pick only ONE mode, not multiple"
#endif
#if !defined(CAMERA_UART_DEBUG) && !defined(CAMERA_OLED_DISPLAY) && !defined(CAMERA_UART_AND_OLED)
    #error "pick exactly one mode"
#endif

// ---> OLED code/handle needed whenever OLED is involved, in either OLED-only or combined mode
#if defined(CAMERA_OLED_DISPLAY) || defined(CAMERA_UART_AND_OLED)
    #define OLED_ENABLED
#endif

#ifdef OLED_ENABLED
#include "ssd1306.h"
#endif
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

DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi;

I2C_HandleTypeDef hi2c1;
#ifdef OLED_ENABLED
I2C_HandleTypeDef hi2c4;
#endif

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t frame_buf[160 * 120 * 2];   // QQVGA YUV422 → 2 bytes per pixel

uint8_t  gray_buf[160 * 120];      // Y channel extracted from YUV422
uint8_t  small_buf[28 * 28];       // final 28x28 output

#ifdef OLED_ENABLED
uint8_t oled_addr = 0;
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DCMI_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
#ifdef OLED_ENABLED
static void MX_I2C4_Init(void);
#endif
/* USER CODE BEGIN PFP */
static void extract_gray(uint8_t *src, uint8_t *dst);
static void downsample_28x28(uint8_t *src, uint8_t *dst);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
#ifdef OLED_ENABLED
  MX_I2C4_Init();   // ---> only touched in OLED mode, so debug mode never risks the I2C4 hang
#endif
  /* USER CODE BEGIN 2 */

// ---> init RST and PWDN pins FIRST
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
HAL_Delay(200);                                        // ---> wait for camera to boot

// ---> I2C scan (camera bus, both modes)
uint8_t scan_result;
for(uint8_t addr = 1; addr < 128; addr++)
{
    if(HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
    {
        scan_result = addr;
        HAL_UART_Transmit(&huart3, (uint8_t*)"FOUND:", 6, 100);
        HAL_UART_Transmit(&huart3, &scan_result, 1, 100);
        HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
    }
}

// ---> init OV7670
if(OV7670_Init(&hi2c1) != HAL_OK)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_ERROR\r\n", 11, 100);
    Error_Handler();
}

HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_OK\r\n", 8, 100);

#if defined(CAMERA_UART_DEBUG) || defined(CAMERA_UART_AND_OLED)
// ---> one-shot capture + full diagnostic dump, exactly the original debug flow
HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
HAL_Delay(2000);

uint32_t dcmi_sr = DCMI->SR;
uint8_t sr_buf[30];
sprintf((char*)sr_buf, "DCMI_SR:%lu\r\n", dcmi_sr);
HAL_UART_Transmit(&huart3, sr_buf, strlen((char*)sr_buf), 100);

HAL_DCMI_Stop(&hdcmi);

uint32_t nonzero = 0;
for(int i = 0; i < sizeof(frame_buf); i++)
{
    if(frame_buf[i] != 0) nonzero++;
}
uint8_t dbg[30];
sprintf((char*)dbg, "NONZERO:%lu\r\n", nonzero);
HAL_UART_Transmit(&huart3, dbg, strlen((char*)dbg), 100);

HAL_UART_Transmit(&huart3, (uint8_t*)"FRAME_START\r\n", 13, 100);
HAL_UART_Transmit(&huart3, frame_buf, 160 * 120 * 2, 5000);
HAL_UART_Transmit(&huart3, (uint8_t*)"FRAME_END\r\n", 11, 100);
#endif

#ifdef OLED_ENABLED
// ---> OLED bring-up, hardcoded address (scan loop was the known hang point on this
//      board's I2C4/PD12-PD13 setup, so it's skipped here rather than reintroduced)
oled_addr = 0x3C;
if(SSD1306_Init(&hi2c4, oled_addr << 1) == HAL_OK)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)"OLED_OK\r\n", 9, 100);
}
else
{
    oled_addr = 0;
    HAL_UART_Transmit(&huart3, (uint8_t*)"OLED_INIT_FAIL\r\n", 16, 100);
}
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // ---> capture
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
    HAL_Delay(500);
    HAL_DCMI_Stop(&hdcmi);

    // ---> preprocess → 28x28
    extract_gray(frame_buf, gray_buf);
    downsample_28x28(gray_buf, small_buf);

    // ---> normalize to float for emlearn
    float features[784];
    for(int i = 0; i < 784; i++)
    {
        features[i] = (float)small_buf[i] / 255.0f;
    }

    // ---> run inference
    int predicted = mnist_model_predict(features, 784);

    // ---> result string, sent in every mode
    uint8_t result_buf[20];
    sprintf((char*)result_buf, "DIGIT:%d\r\n", predicted);
    HAL_UART_Transmit(&huart3, result_buf, strlen((char*)result_buf), 100);

#if defined(CAMERA_UART_DEBUG) || defined(CAMERA_UART_AND_OLED)
    // ---> full debug stream : 28x28 buffer + raw frame, every loop
    HAL_UART_Transmit(&huart3, (uint8_t*)"FRAME_START\r\n", 13, 100);
    HAL_UART_Transmit(&huart3, small_buf, 784, 100);

    HAL_UART_Transmit(&huart3, (uint8_t*)"RAW_START\r\n", 11, 100);
    HAL_UART_Transmit(&huart3, frame_buf, 160 * 120 * 2, 2000);
#endif

#ifdef OLED_ENABLED
    // ---> hardware display : 28x28 preview + predicted digit on the OLED
    if(oled_addr != 0)
    {
        SSD1306_Clear();
        SSD1306_DrawGrayBuffer(small_buf, 0, 0, 2);
        SSD1306_DrawDigit(predicted, 90, 20, 3);
        SSD1306_UpdateScreen();
    }
#endif

    HAL_Delay(100);

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

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x307075B1;
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

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

#ifdef OLED_ENABLED
/**
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x307075B1;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 1000000;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
#ifdef OLED_ENABLED
  // ---> I2C4 pins, only configured in OLED mode, so debug mode's GPIO state is
  //      identical to the original known-working build
  GPIO_InitTypeDef GPIO_InitStruct_I2C4 = {0};
  GPIO_InitStruct_I2C4.Pin       = GPIO_PIN_12 | GPIO_PIN_13;   // ---> PD12 = SCL, PD13 = SDA
  GPIO_InitStruct_I2C4.Mode      = GPIO_MODE_AF_OD;
  GPIO_InitStruct_I2C4.Pull      = GPIO_PULLUP;
  GPIO_InitStruct_I2C4.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct_I2C4.Alternate = GPIO_AF4_I2C4;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct_I2C4);
#endif
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//---------------------------------------------------------------------------------------------------------------------------
// extract_gray :
//   ---> extracts Y channel from YUV422 frame buffer
//   ---> YUV422 format : Y0 U0 Y1 V0 Y2 U2 Y3 V2 ...
//   ---> inputs  : src  ---> YUV422 buffer (160*120*2 bytes)
//   ---> outputs : dst  ---> grayscale buffer (160*120 bytes)
//---------------------------------------------------------------------------------------------------------------------------
static void extract_gray(uint8_t *src, uint8_t *dst)
{
    for(int i = 0; i < 160 * 120; i++)
    {
        dst[i] = src[i * 2 + 1];     // ---> Y is at the odd byte index for this camera
    }
}
//---------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------
// downsample_28x28 :
//   ---> center crops 90x120 from 160x120 then downsamples to 28x28
//   ---> applies adaptive threshold + inversion to match MNIST style
//   ---> inputs  : src ---> grayscale buffer (160*120 bytes)
//   ---> outputs : dst ---> 28x28 buffer (784 bytes)
//---------------------------------------------------------------------------------------------------------------------------
static void downsample_28x28(uint8_t *src, uint8_t *dst)
{
    int x_start = 35;
    int y_start = 0;
    int width   = 90;
    int height  = 120;

    // ---> compute adaptive threshold from center region
    uint32_t total = 0;
    for(int y = y_start + 5; y < y_start + height - 5; y++)
        for(int x = x_start + 5; x < x_start + width - 5; x++)
            total += src[y * 160 + x];
    uint8_t threshold = (uint8_t)(total / (100 * 100));

    // ---> downsample to 28x28
    for(int row = 0; row < 28; row++)
    {
        for(int col = 0; col < 28; col++)
        {
            uint32_t sum = 0;
            int count = 0;

            for(int dy = 0; dy < 4; dy++)
            {
                for(int dx = 0; dx < 4; dx++)
                {
                    int y = y_start + (row * height / 28) + dy;
                    int x = x_start + (col * width  / 28) + dx;
                    if(y < 120 && x < 160)
                    {
                        sum += src[y * 160 + x];
                        count++;
                    }
                }
            }

            uint8_t avg = count > 0 ? (uint8_t)(sum / count) : 128;

            // ---> stretch contrast instead of hard threshold, invert so dark digit
            //      on white paper becomes bright on dark, like MNIST
            dst[row * 28 + col] = (avg < threshold) ? (uint8_t)((threshold - avg) * 255 / threshold) : 0;
        }
    }

    // ---> only force left and right borders
    for(int i = 0; i < 28; i++)
    {
        dst[i * 28 + 0]  = 0;
        dst[i * 28 + 1]  = 0;
        dst[i * 28 + 2]  = 0;
        dst[i * 28 + 27] = 0;
        dst[i * 28 + 26] = 0;
        dst[i * 28 + 25] = 0;
    }
}
//---------------------------------------------------------------------------------------------------------------------------

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
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
#endif /* USE_FULL_ASSERT */