// /* USER CODE BEGIN Header */
// /**
//   ******************************************************************************
//   * @file           : main.c
//   * @brief          : Main program body
//   ******************************************************************************
//   * @attention
//   *
//   * Copyright (c) 2026 STMicroelectronics.
//   * All rights reserved.
//   *
//   * This software is licensed under terms that can be found in the LICENSE file
//   * in the root directory of this software component.
//   * If no LICENSE file comes with this software, it is provided AS-IS.
//   *
//   ******************************************************************************
//   */
// /* USER CODE END Header */

// /* Includes ------------------------------------------------------------------*/
// #include "main.h"
// #include "fatfs.h"

// /* Private includes ----------------------------------------------------------*/
// /* USER CODE BEGIN Includes */
// #include "ov7670.h"

// #include <stdio.h>
// #include <string.h>

// #include "mnist_784_8_10.h"

// //---------------------------------------------------------------------------------------------------------------------------
// // mode select
// #define CAMERA_UART_AND_OLED   // Change as needed

// #if defined(CAMERA_OLED_DISPLAY) || defined(CAMERA_UART_AND_OLED)
//     #define OLED_ENABLED
// #endif

// #ifdef OLED_ENABLED
// #include "ssd1306.h"
// #endif
// /* USER CODE END Includes */

// /* Private typedef -----------------------------------------------------------*/
// /* USER CODE BEGIN PTD */
// /* USER CODE END PTD */

// /* Private define ------------------------------------------------------------*/
// /* USER CODE BEGIN PD */
// /* USER CODE END PD */

// /* Private macro -------------------------------------------------------------*/
// /* USER CODE BEGIN PM */
// /* USER CODE END PM */

// /* Private variables ---------------------------------------------------------*/

// DCMI_HandleTypeDef hdcmi;
// DMA_HandleTypeDef hdma_dcmi;

// I2C_HandleTypeDef hi2c1;
// I2C_HandleTypeDef hi2c4;

// SD_HandleTypeDef hsd1;

// UART_HandleTypeDef huart3;

// /* USER CODE BEGIN PV */
// uint8_t frame_buf[160 * 120 * 2];
// uint8_t  gray_buf[160 * 120];
// uint8_t  small_buf[28 * 28];

// #ifdef OLED_ENABLED
// uint8_t oled_addr = 0;
// #endif
// /* USER CODE END PV */

// /* Private function prototypes -----------------------------------------------*/
// void SystemClock_Config(void);
// static void MPU_Config(void);
// static void MX_GPIO_Init(void);
// static void MX_DMA_Init(void);
// static void MX_DCMI_Init(void);
// static void MX_I2C1_Init(void);
// static void MX_USART3_UART_Init(void);
// static void MX_I2C4_Init(void);
// static void MX_SDMMC1_SD_Init(void);
// static void MX_SDMMC1_SD_Init(void);

// /* USER CODE BEGIN PFP */
// static void extract_gray(uint8_t *src, uint8_t *dst);
// static void downsample_28x28(uint8_t *src, uint8_t *dst);

// void SD_Card_Test(void);
// FRESULT SD_Save_Camera_Frame(uint8_t *buffer, uint32_t size, const char *filename);
// FRESULT SD_Save_28x28(uint8_t *small_buf, const char *filename);
// FRESULT SD_Log_Prediction(int digit);
// /* USER CODE END PFP */

// /* Private user code ---------------------------------------------------------*/
// /* USER CODE BEGIN 0 */

// /* Redirect printf to UART3 */
// int __io_putchar(int ch)
// {
//     HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
//     return ch;
// }

// /* USER CODE END 0 */

// /**
//   * @brief  The application entry point.
//   * @retval int
//   */
// int main(void)
// {
//   /* USER CODE BEGIN 1 */
//   /* USER CODE END 1 */

//   MPU_Config();
//   HAL_Init();
//   SystemClock_Config();

//   /* Initialize all configured peripherals */
//   MX_GPIO_Init();
//   MX_DMA_Init();
//   MX_DCMI_Init();
//   MX_I2C1_Init();
//   MX_USART3_UART_Init();
//   MX_I2C4_Init();
//   MX_SDMMC1_SD_Init();
//   MX_FATFS_Init();

//   /* USER CODE BEGIN 2 */

//   // ====================== SD CARD MOUNT TEST ======================
//   SCB_CleanDCache();
//   SCB_InvalidateDCache();
  
//   SD_Card_Test();
//   // ====================== END SD TEST ======================

//   // Camera & OLED initialization (your original code)
//   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
//   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
//   HAL_Delay(200);

//   // I2C scan...
//   uint8_t scan_result;
//   for(uint8_t addr = 1; addr < 128; addr++)
//   {
//       if(HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
//       {
//           scan_result = addr;
//           HAL_UART_Transmit(&huart3, (uint8_t*)"FOUND:", 6, 100);
//           HAL_UART_Transmit(&huart3, &scan_result, 1, 100);
//           HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
//       }
//   }

//   if(OV7670_Init(&hi2c1) != HAL_OK)
//   {
//       HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_ERROR\r\n", 11, 100);
//       Error_Handler();
//   }
//   HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_OK\r\n", 8, 100);

//   // ... rest of your original initialization code (DCMI, OLED, etc.)
// #if defined(CAMERA_UART_DEBUG) || defined(CAMERA_UART_AND_OLED)
//   HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
//   HAL_Delay(2000);
//   // ... (your existing debug code)
// #endif

// #ifdef OLED_ENABLED
//   oled_addr = 0x3C;
//   if(SSD1306_Init(&hi2c4, oled_addr << 1) == HAL_OK)
//   {
//       HAL_UART_Transmit(&huart3, (uint8_t*)"OLED_OK\r\n", 9, 100);
//   }
//   else
//   {
//       oled_addr = 0;
//       HAL_UART_Transmit(&huart3, (uint8_t*)"OLED_INIT_FAIL\r\n", 16, 100);
//   }
// #endif

//   /* USER CODE END 2 */

//   while (1)
//   {
//     // // Your existing main loop (camera capture, inference, OLED...)
//     // HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
//     // HAL_Delay(500);
//     // HAL_DCMI_Stop(&hdcmi);

//     // extract_gray(frame_buf, gray_buf);
//     // downsample_28x28(gray_buf, small_buf);

//     // float features[784];
//     // for(int i = 0; i < 784; i++)
//     // {
//     //     features[i] = (float)small_buf[i] / 255.0f;
//     // }

//     // int predicted = mnist_model_predict(features, 784);

//     // uint8_t result_buf[20];
//     // sprintf((char*)result_buf, "DIGIT:%d\r\n", predicted);
//     // HAL_UART_Transmit(&huart3, result_buf, strlen((char*)result_buf), 100);

//     // // ... rest of your loop (debug print, OLED update, etc.)

//     // HAL_Delay(100);


//     // Capture frame
//         HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
//         HAL_Delay(300);           // adjust as needed
//         HAL_DCMI_Stop(&hdcmi);

//         extract_gray(frame_buf, gray_buf);
//         downsample_28x28(gray_buf, small_buf);

//         float features[784];
//         for(int i = 0; i < 784; i++) {
//             features[i] = (float)small_buf[i] / 255.0f;
//         }

//         int predicted = mnist_model_predict(features, 784);

//         // === SD CARD OPERATIONS ===
//         SD_Log_Prediction(predicted);                    // Log digit
//         SD_Save_28x28(small_buf, "0:/frame_28x28.bin"); // Save small image
//         // SD_Save_Camera_Frame(frame_buf, sizeof(frame_buf), "0:/raw_frame.bin"); // Full raw frame (big!)

//         // Print result
//         printf("DIGIT:%d\r\n", predicted);

//     #ifdef OLED_ENABLED
//         if(oled_addr != 0) {
//             SSD1306_Clear();
//             SSD1306_DrawGrayBuffer(small_buf, 0, 16, 1);
//             SSD1306_DrawDigit(predicted, 90, 24, 2);
//             SSD1306_UpdateScreen();
//         }
//     #endif

//         HAL_Delay(800);   // Adjust rate
//   }
// }

// /* SD Card Test Function */
// void SD_Card_Test(void)
// {
//     printf("\r\n=== SD Card Mount Test (STM32H750) ===\r\n");

//     FATFS fs;
//     FRESULT fr;

//     fr = f_mount(&fs, "0:", 1);
//     if (fr != FR_OK)
//     {
//         printf("Mount FAILED! Error: %d\r\n", fr);
//         return;
//     }

//     printf("Mount SUCCESS!\r\n");

//     DWORD free_clust;
//     FATFS *pfs;
//     if (f_getfree("0:", &free_clust, &pfs) == FR_OK)
//     {
//         printf("Free space: ~%lu KB\r\n", ((free_clust * pfs->csize) * 512UL) / 1024UL);
//     }

//     FIL fil;
//     UINT bw;
//     if (f_open(&fil, "0:/test_sd.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
//     {
//         f_write(&fil, "SD Card works on DevEBOX STM32H750!\r\n", 38, &bw);
//         f_close(&fil);
//         printf("Created test file: test_sd.txt\r\n");
//     }
//     else
//     {
//         printf("Failed to create test file\r\n");
//     }

//     printf("SD Card Test Finished\r\n\r\n");
// }
// /* ==================== SD CARD HELPERS ==================== */

// FRESULT SD_Save_Camera_Frame(uint8_t *buffer, uint32_t size, const char *filename)
// {
//     FIL fil;
//     UINT bw;
//     FRESULT fr;

//     fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
//     if (fr != FR_OK) {
//         printf("Open failed: %d\r\n", fr);
//         return fr;
//     }

//     fr = f_write(&fil, buffer, size, &bw);
//     f_close(&fil);

//     printf("Saved %u bytes to %s\r\n", bw, filename);
//     return fr;
// }

// FRESULT SD_Save_28x28(uint8_t *small_buf, const char *filename)
// {
//     FIL fil;
//     UINT bw;
//     FRESULT fr;

//     fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
//     if (fr != FR_OK) {
//         printf("Open failed: %d\r\n", fr);
//         return fr;
//     }

//     char header[32];
//     sprintf(header, "28x28\r\n");
//     f_write(&fil, header, strlen(header), &bw);

//     fr = f_write(&fil, small_buf, 28*28, &bw);
//     f_close(&fil);

//     printf("Saved 28x28 image to %s\r\n", filename);
//     return fr;
// }

// FRESULT SD_Log_Prediction(int digit)
// {
//     FIL fil;
//     UINT bw;
//     FRESULT fr;

//     fr = f_open(&fil, "0:/predictions.csv", FA_OPEN_APPEND | FA_WRITE);
//     if (fr == FR_NO_FILE) {
//         fr = f_open(&fil, "0:/predictions.csv", FA_CREATE_ALWAYS | FA_WRITE);
//         if (fr == FR_OK) {
//             f_write(&fil, "Tick,Digit\r\n", 12, &bw);
//         }
//     }
//     if (fr != FR_OK) return fr;

//     char line[32];
//     sprintf(line, "%lu,%d\r\n", HAL_GetTick(), digit);
//     f_write(&fil, line, strlen(line), &bw);
//     f_close(&fil);

//     return FR_OK;
// }






// /**
//   * @brief System Clock Configuration
//   * @retval None
//   */
// void SystemClock_Config(void)
// {
//   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

//   /** Supply configuration update enable
//   */
//   HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

//   /** Configure the main internal regulator output voltage
//   */
//   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

//   while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

//   /** Initializes the RCC Oscillators according to the specified parameters
//   * in the RCC_OscInitTypeDef structure.
//   */
//   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//   RCC_OscInitStruct.PLL.PLLM = 5;
//   RCC_OscInitStruct.PLL.PLLN = 192;
//   RCC_OscInitStruct.PLL.PLLP = 2;
//   RCC_OscInitStruct.PLL.PLLQ = 64;
//   RCC_OscInitStruct.PLL.PLLR = 2;
//   RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
//   RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
//   RCC_OscInitStruct.PLL.PLLFRACN = 0;
//   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Initializes the CPU, AHB and APB buses clocks
//   */
//   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                               |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
//                               |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
//   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//   RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
//   RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
//   RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
//   RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
//   RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
//   RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

//   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
// }

// /**
//   * @brief DCMI Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_DCMI_Init(void)
// {

//   /* USER CODE BEGIN DCMI_Init 0 */

//   /* USER CODE END DCMI_Init 0 */

//   /* USER CODE BEGIN DCMI_Init 1 */

//   /* USER CODE END DCMI_Init 1 */
//   hdcmi.Instance = DCMI;
//   hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
//   hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
//   hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;
//   hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
//   hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
//   hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
//   hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
//   hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
//   hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
//   hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
//   hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
//   if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN DCMI_Init 2 */

//   /* USER CODE END DCMI_Init 2 */

// }

// /**
//   * @brief I2C1 Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_I2C1_Init(void)
// {

//   /* USER CODE BEGIN I2C1_Init 0 */

//   /* USER CODE END I2C1_Init 0 */

//   /* USER CODE BEGIN I2C1_Init 1 */

//   /* USER CODE END I2C1_Init 1 */
//   hi2c1.Instance = I2C1;
//   hi2c1.Init.Timing = 0x307075B1;
//   hi2c1.Init.OwnAddress1 = 0;
//   hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//   hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//   hi2c1.Init.OwnAddress2 = 0;
//   hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
//   hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//   hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//   if (HAL_I2C_Init(&hi2c1) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Configure Analogue filter
//   */
//   if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Configure Digital filter
//   */
//   if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN I2C1_Init 2 */

//   /* USER CODE END I2C1_Init 2 */

// }

// /**
//   * @brief I2C4 Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_I2C4_Init(void)
// {

//   /* USER CODE BEGIN I2C4_Init 0 */

//   /* USER CODE END I2C4_Init 0 */

//   /* USER CODE BEGIN I2C4_Init 1 */

//   /* USER CODE END I2C4_Init 1 */
//   hi2c4.Instance = I2C4;
//   hi2c4.Init.Timing = 0x307075B1;
//   hi2c4.Init.OwnAddress1 = 0;
//   hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//   hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//   hi2c4.Init.OwnAddress2 = 0;
//   hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
//   hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//   hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//   if (HAL_I2C_Init(&hi2c4) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Configure Analogue filter
//   */
//   if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
//   {
//     Error_Handler();
//   }

//   /** Configure Digital filter
//   */
//   if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN I2C4_Init 2 */

//   /* USER CODE END I2C4_Init 2 */

// }

// /**
//   * @brief SDMMC1 Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_SDMMC1_SD_Init(void)
// {

//   /* USER CODE BEGIN SDMMC1_Init 0 */

//   /* USER CODE END SDMMC1_Init 0 */

//   /* USER CODE BEGIN SDMMC1_Init 1 */

//   /* USER CODE END SDMMC1_Init 1 */
//   hsd1.Instance = SDMMC1;
//   hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
//   hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
//   hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
//   hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
//   hsd1.Init.ClockDiv = 0;
//   if (HAL_SD_Init(&hsd1) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN SDMMC1_Init 2 */

//   /* USER CODE END SDMMC1_Init 2 */

// }

// /**
//   * @brief USART3 Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_USART3_UART_Init(void)
// {

//   /* USER CODE BEGIN USART3_Init 0 */

//   /* USER CODE END USART3_Init 0 */

//   /* USER CODE BEGIN USART3_Init 1 */

//   /* USER CODE END USART3_Init 1 */
//   huart3.Instance = USART3;
//   huart3.Init.BaudRate = 1000000;
//   huart3.Init.WordLength = UART_WORDLENGTH_8B;
//   huart3.Init.StopBits = UART_STOPBITS_1;
//   huart3.Init.Parity = UART_PARITY_NONE;
//   huart3.Init.Mode = UART_MODE_TX_RX;
//   huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//   huart3.Init.OverSampling = UART_OVERSAMPLING_16;
//   huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
//   huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
//   huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//   if (HAL_UART_Init(&huart3) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
//   {
//     Error_Handler();
//   }
//   /* USER CODE BEGIN USART3_Init 2 */

//   /* USER CODE END USART3_Init 2 */

// }

// /**
//   * Enable DMA controller clock
//   */
// static void MX_DMA_Init(void)
// {

//   /* DMA controller clock enable */
//   __HAL_RCC_DMA1_CLK_ENABLE();

//   /* DMA interrupt init */
//   /* DMA1_Stream0_IRQn interrupt configuration */
//   HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
//   HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

// }

// /**
//   * @brief GPIO Initialization Function
//   * @param None
//   * @retval None
//   */
// static void MX_GPIO_Init(void)
// {
//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   /* USER CODE BEGIN MX_GPIO_Init_1 */

//   /* USER CODE END MX_GPIO_Init_1 */

//   /* GPIO Ports Clock Enable */
//   __HAL_RCC_GPIOE_CLK_ENABLE();
//   __HAL_RCC_GPIOC_CLK_ENABLE();
//   __HAL_RCC_GPIOH_CLK_ENABLE();
//   __HAL_RCC_GPIOA_CLK_ENABLE();
//   __HAL_RCC_GPIOB_CLK_ENABLE();
//   __HAL_RCC_GPIOD_CLK_ENABLE();

//   /*Configure GPIO pin Output Level */
//   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

//   /*Configure GPIO pin Output Level */
//   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

//   /*Configure GPIO pin : PA7 */
//   GPIO_InitStruct.Pin = GPIO_PIN_7;
//   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//   GPIO_InitStruct.Pull = GPIO_NOPULL;
//   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//   /*Configure GPIO pin : PC4 */
//   GPIO_InitStruct.Pin = GPIO_PIN_4;
//   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//   GPIO_InitStruct.Pull = GPIO_NOPULL;
//   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//   /*Configure GPIO pin : PD9 */
//   GPIO_InitStruct.Pin = GPIO_PIN_9;
//   GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
//   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

//   /*Configure GPIO pin : PA8 */
//   GPIO_InitStruct.Pin = GPIO_PIN_8;
//   GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//   GPIO_InitStruct.Pull = GPIO_NOPULL;
//   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//   GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
//   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//   /* USER CODE BEGIN MX_GPIO_Init_2 */
// #ifdef OLED_ENABLED
//   // ---> I2C4 pins, only configured in OLED mode, so debug mode's GPIO state is
//   //      identical to the original known-working build
//   GPIO_InitTypeDef GPIO_InitStruct_I2C4 = {0};
//   GPIO_InitStruct_I2C4.Pin       = GPIO_PIN_12 | GPIO_PIN_13;   // ---> PD12 = SCL, PD13 = SDA
//   GPIO_InitStruct_I2C4.Mode      = GPIO_MODE_AF_OD;
//   GPIO_InitStruct_I2C4.Pull      = GPIO_PULLUP;
//   GPIO_InitStruct_I2C4.Speed     = GPIO_SPEED_FREQ_LOW;
//   GPIO_InitStruct_I2C4.Alternate = GPIO_AF4_I2C4;
//   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct_I2C4);
// #endif
//   /* USER CODE END MX_GPIO_Init_2 */
// }

// /* USER CODE BEGIN 4 */

// //---------------------------------------------------------------------------------------------------------------------------
// // extract_gray :
// //   ---> extracts Y channel from YUV422 frame buffer
// //   ---> YUV422 format : Y0 U0 Y1 V0 Y2 U2 Y3 V2 ...
// //   ---> inputs  : src  ---> YUV422 buffer (160*120*2 bytes)
// //   ---> outputs : dst  ---> grayscale buffer (160*120 bytes)
// //---------------------------------------------------------------------------------------------------------------------------
// static void extract_gray(uint8_t *src, uint8_t *dst)
// {
//     for(int i = 0; i < 160 * 120; i++)
//     {
//         dst[i] = src[i * 2 + 1];     // ---> Y is at the odd byte index for this camera
//     }
// }
// //---------------------------------------------------------------------------------------------------------------------------

// //---------------------------------------------------------------------------------------------------------------------------
// // downsample_28x28 :
// //   ---> center crops 90x120 from 160x120 then downsamples to 28x28
// //   ---> applies adaptive threshold + inversion to match MNIST style
// //   ---> inputs  : src ---> grayscale buffer (160*120 bytes)
// //   ---> outputs : dst ---> 28x28 buffer (784 bytes)
// //---------------------------------------------------------------------------------------------------------------------------
// static void downsample_28x28(uint8_t *src, uint8_t *dst)
// {
//     int x_start = 35;
//     int y_start = 0;
//     int width   = 90;
//     int height  = 120;

//     // ---> compute adaptive threshold from center region
//     uint32_t total = 0;
//     for(int y = y_start + 5; y < y_start + height - 5; y++)
//         for(int x = x_start + 5; x < x_start + width - 5; x++)
//             total += src[y * 160 + x];
//     uint8_t threshold = (uint8_t)(total / (100 * 100));

//     // ---> downsample to 28x28
//     for(int row = 0; row < 28; row++)
//     {
//         for(int col = 0; col < 28; col++)
//         {
//             uint32_t sum = 0;
//             int count = 0;

//             for(int dy = 0; dy < 4; dy++)
//             {
//                 for(int dx = 0; dx < 4; dx++)
//                 {
//                     int y = y_start + (row * height / 28) + dy;
//                     int x = x_start + (col * width  / 28) + dx;
//                     if(y < 120 && x < 160)
//                     {
//                         sum += src[y * 160 + x];
//                         count++;
//                     }
//                 }
//             }

//             uint8_t avg = count > 0 ? (uint8_t)(sum / count) : 128;

//             // ---> stretch contrast instead of hard threshold, invert so dark digit
//             //      on white paper becomes bright on dark, like MNIST
//             dst[row * 28 + col] = (avg < threshold) ? (uint8_t)((threshold - avg) * 255 / threshold) : 0;
//         }
//     }

//     // ---> only force left and right borders
//     for(int i = 0; i < 28; i++)
//     {
//         dst[i * 28 + 0]  = 0;
//         dst[i * 28 + 1]  = 0;
//         dst[i * 28 + 2]  = 0;
//         dst[i * 28 + 27] = 0;
//         dst[i * 28 + 26] = 0;
//         dst[i * 28 + 25] = 0;
//     }
// }







// //---------------------------------------------------------------------------------------------------------------------------

// /* USER CODE END 4 */

//  /* MPU Configuration */

// void MPU_Config(void)
// {
//   MPU_Region_InitTypeDef MPU_InitStruct = {0};

//   /* Disables the MPU */
//   HAL_MPU_Disable();

//   /** Initializes and configures the Region and the memory to be protected
//   */
//   MPU_InitStruct.Enable = MPU_REGION_ENABLE;
//   MPU_InitStruct.Number = MPU_REGION_NUMBER0;
//   MPU_InitStruct.BaseAddress = 0x0;
//   MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
//   MPU_InitStruct.SubRegionDisable = 0x87;
//   MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
//   MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
//   MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
//   MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
//   MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
//   MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

//   HAL_MPU_ConfigRegion(&MPU_InitStruct);
//   /* Enables the MPU */
//   HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

// }

// /**
//   * @brief  This function is executed in case of error occurrence.
//   * @retval None
//   */
// void Error_Handler(void)
// {
//   /* USER CODE BEGIN Error_Handler_Debug */
//   /* User can add his own implementation to report the HAL error return state */
//   __disable_irq();
//   while (1)
//   {
//   }
//   /* USER CODE END Error_Handler_Debug */
// }
// #ifdef USE_FULL_ASSERT
// /**
//   * @brief  Reports the name of the source file and the source line number
//   *         where the assert_param error has occurred.
//   * @param  file: pointer to the source file name
//   * @param  line: assert_param error line source number
//   * @retval None
//   */
// void assert_failed(uint8_t *file, uint32_t line)
// {
//   /* USER CODE BEGIN 6 */
//   /* User can add his own implementation to report the file name and line number,
//      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//   /* USER CODE END 6 */
// }
// #endif /* USE_FULL_ASSERT */
























































































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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ov7670.h"

#include <stdio.h>
#include <string.h>

#include "mnist_784_8_10.h"

//---------------------------------------------------------------------------------------------------------------------------
// mode select
#define CAMERA_UART_AND_OLED   // Change as needed

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
I2C_HandleTypeDef hi2c4;

SD_HandleTypeDef hsd1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t frame_buf[160 * 120 * 2];
uint8_t  gray_buf[160 * 120];
uint8_t  small_buf[28 * 28];

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
static void MX_I2C4_Init(void);
static void MX_SDMMC1_SD_Init(void);

/* USER CODE BEGIN PFP */
static void extract_gray(uint8_t *src, uint8_t *dst);
static void downsample_28x28(uint8_t *src, uint8_t *dst);
void SD_Card_Test(void);   // SD test function
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Redirect printf to UART3 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
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

  MPU_Config();
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_I2C4_Init();
  MX_SDMMC1_SD_Init();
  MX_FATFS_Init();

  /* USER CODE BEGIN 2 */

  // ====================== SD CARD MOUNT TEST ======================
  SD_Card_Test();
  // ====================== END SD TEST ======================

  // Camera & OLED initialization (your original code)
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(200);

  // I2C scan...
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

  if(OV7670_Init(&hi2c1) != HAL_OK)
  {
      HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_ERROR\r\n", 11, 100);
      Error_Handler();
  }
  HAL_UART_Transmit(&huart3, (uint8_t*)"CAM_OK\r\n", 8, 100);

  // ... rest of your original initialization code (DCMI, OLED, etc.)
#if defined(CAMERA_UART_DEBUG) || defined(CAMERA_UART_AND_OLED)
  HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
  HAL_Delay(2000);
  // ... (your existing debug code)
#endif

#ifdef OLED_ENABLED
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

  while (1)
  {
    // Your existing main loop (camera capture, inference, OLED...)
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)frame_buf, sizeof(frame_buf)/4);
    HAL_Delay(500);
    HAL_DCMI_Stop(&hdcmi);

    extract_gray(frame_buf, gray_buf);
    downsample_28x28(gray_buf, small_buf);

    float features[784];
    for(int i = 0; i < 784; i++)
    {
        features[i] = (float)small_buf[i] / 255.0f;
    }

    int predicted = mnist_model_predict(features, 784);

    uint8_t result_buf[20];
    sprintf((char*)result_buf, "DIGIT:%d\r\n", predicted);
    HAL_UART_Transmit(&huart3, result_buf, strlen((char*)result_buf), 100);

    // ... rest of your loop (debug print, OLED update, etc.)

    HAL_Delay(100);
  }
}

/* SD Card Test Function */
void SD_Card_Test(void)
{
    printf("\r\n=== SD Card Mount Test (STM32H750) ===\r\n");

    FATFS fs;
    FRESULT fr;

    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK)
    {
        printf("Mount FAILED! Error: %d\r\n", fr);
        return;
    }

    printf("Mount SUCCESS!\r\n");

    DWORD free_clust;
    FATFS *pfs;
    if (f_getfree("0:", &free_clust, &pfs) == FR_OK)
    {
        printf("Free space: ~%lu KB\r\n", ((free_clust * pfs->csize) * 512UL) / 1024UL);
    }

    FIL fil;
    UINT bw;
    if (f_open(&fil, "0:/test_sd.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    {
        f_write(&fil, "SD Card works on DevEBOX STM32H750!\r\n", 38, &bw);
        f_close(&fil);
        printf("Created test file: test_sd.txt\r\n");
    }
    else
    {
        printf("Failed to create test file\r\n");
    }

    printf("SD Card Test Finished\r\n\r\n");
}

/* Rest of your functions (SystemClock_Config, MX_xxx_Init, extract_gray, downsample_28x28, MPU_Config, Error_Handler...) */
/* ... keep everything else exactly as you had it ... */









/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 64;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
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
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

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
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
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

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  /* USER CODE END I2C4_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

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
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
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

  /*Configure GPIO pin : PD9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
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
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
