/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *Stabile Uebrtragung bei 15,625 kBits pro Sekunde.
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint8_t txData_const = 0xC0;  // Beispiel: Opcode für GetStatus
uint8_t rxData_const = 0x00;  // Hier wird die Antwort gespeichert
char uart_buf[100];
int uart_buf_len;
uint8_t busy, nss, reset;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

void Erase_Flash(void) {
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SectorError;

	// Unlock the Flash
	HAL_FLASH_Unlock();

	// Configure the erase operation
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = FLASH_SECTOR_0; // Specify sector to erase
	EraseInitStruct.NbSectors = 1;

	// Erase the sector
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
		// Handle error
	}

	// Lock the Flash
	HAL_FLASH_Lock();
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
//#define GND_CONTROL_PIN GPIO_PIN_1
//#define GND_CONTROL_PORT GPIOA
//
//void toggle_gnd(bool state) {
//    if (state) {
//        HAL_GPIO_WritePin(GND_CONTROL_PORT, GND_CONTROL_PIN, GPIO_PIN_SET); // GND einschalten
//    } else {
//        HAL_GPIO_WritePin(GND_CONTROL_PORT, GND_CONTROL_PIN, GPIO_PIN_RESET); // GND ausschalten
//    }
//}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

void ResetChip(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
	HAL_Delay(20);
}

void SelectChip(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state) {
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, state);
	HAL_Delay(2); // Falls notwendig
}
void SPI_TransmitReceiveWithDebug(SPI_HandleTypeDef* hspi, uint8_t* txData, uint8_t* rxData, uint16_t length, const char* debugMessage) {
	HAL_SPI_TransmitReceive(hspi, txData, rxData, length, HAL_MAX_DELAY);
	if (debugMessage) {
		HAL_UART_Transmit(&huart2, (uint8_t*)debugMessage, strlen(debugMessage), 100);
	}
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t ProcessStatusByte(uint8_t* statusByte) {
	// Extrahiere die Bits 7:5 (Circuit Mode) und 4:2 (Command Status)
	uint8_t circuitMode = (*statusByte >> 5) & 0x07; // Maske 0x07 für 3 Bits
	uint8_t commandStatus = (*statusByte >> 2) & 0x07; // Maske 0x07 für 3 Bits

	// Debug-Ausgabe für UART
	char uart_buf[50];
	int uart_buf_len = sprintf(uart_buf, "Circuit Mode: %u, Command Status: %u\r\n", circuitMode, commandStatus);
	HAL_UART_Transmit(&huart2, (uint8_t *)uart_buf, uart_buf_len, 100);

	// Verarbeitung von Circuit Mode
	switch (circuitMode) {
	case 0x2:
		// STDBY_RC
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: STDBY_RC\r\n", 17, 100);
		break;
	case 0x3:
		// STDBY_XOSC
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: STDBY_XOSC\r\n", 19, 100);
		break;
	case 0x4:
		// FS
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: FS\r\n", 10, 100);
		break;
	case 0x5:
		// Rx
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: Rx\r\n", 10, 100);
		break;
	case 0x6:
		// Tx
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: Tx\r\n", 10, 100);
		break;
	default:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Mode: Unknown\r\n", 16, 100);


	}

	// Verarbeitung von Command Status
	switch (commandStatus) {
	case 0x1:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Success\r\n", 26, 100);
		break;
	case 0x2:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Data Available\r\n", 33, 100);
		break;
	case 0x3:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Timeout\r\n", 26, 100);
		break;
	case 0x4:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Error\r\n", 24, 100);
		break;
	case 0x5:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Failure\r\n", 26, 100);
		break;
	case 0x6:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Tx Done\r\n", 26, 100);
		break;
	default:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Command Status: Unknown\r\n", 27, 100);
	}
	return circuitMode;
}



void SPI_WaitUntilReady(SPI_HandleTypeDef *hspi) {
	while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET) {
		busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

		HAL_Delay(5);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
		HAL_Delay(5);
		HAL_Delay(100); // Small delay to ensure stability



	}
	busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));


}
static void SPI1_TRANSCEIVER_Delay(uint8_t* txData, uint8_t* rxData, uint8_t lengh)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
	HAL_Delay(10); // Small delay to ensure stability

	SPI_WaitUntilReady(&hspi1);
	HAL_SPI_TransmitReceive(&hspi1, txData, rxData, lengh, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

}
//void SPI_WaitPause(SPI_HandleTypeDef *hspi) {
//	while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_RESET) {
//		busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
//		HAL_SPI_Receive(&hspi1, &rxData_const, 1, HAL_MAX_DELAY);   // Lese die Antwort
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Select the chip
//		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
//
//	}
//	busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
//	HAL_Delay(10); // Small delay to ensure stability
//}


void SPI_WaitOpcod(SPI_HandleTypeDef *hspi) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Select the chip
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
	while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET) {
		busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

		HAL_Delay(5);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
		HAL_Delay(100); // Small delay to ensure stability


		//SPI1_TRANSCEIVER_Delay(&tx, &rx, 1);
	}
	busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));

}

void SPI_WaitTransmit(SPI_HandleTypeDef *hspi) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Select the chip
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
	while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) != GPIO_PIN_SET) {
		busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

		HAL_Delay(5); // Small delay to ensure stability
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Select the chip
		nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));



	}
	busy=uint8_t(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
	HAL_Delay(50); // Small delay to ensure stability

}
uint8_t GetStatus() {
	uint8_t command = 0xC0;  // Nur Low Byte wird verwendet
	uint8_t response = 0;

	// Sende 16-Bit-Datenrahmen (High Byte wird ignoriert)
	HAL_SPI_TransmitReceive(&hspi1, &command, &response, 1, HAL_MAX_DELAY);

	// Extrahiere nur das Low Byte aus der Antwort
	return ProcessStatusByte(&response);

}

void SetStandby() {
	uint8_t command[2] = {0x80, 0x01};
	//uint8_t response[2]= {0x00, 0x00};
	// Sende und empfange 16-Bit-Datenrahmen
	// Zuerst Daten senden
	HAL_SPI_Transmit(&hspi1, (uint8_t*)command, 2, HAL_MAX_DELAY);

	// Dann die Antwort empfangen
	//HAL_SPI_Receive(&hspi1, (uint8_t*)&response, 2, HAL_MAX_DELAY);
}

// Funktion zum Aktivieren des LoRa Modus
void setLoRaMode() {
	uint8_t tx[2] = {0x8A, 0x01}; // LoRa Mode aktivieren
	HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t *)"Set LoRa mode\r\n", 15, 100);
	HAL_Delay(10);

}
void getPacketType(SPI_HandleTypeDef* hspi) {
	//uint8_t txData;
	//*(uint32_t*)txData = 0x03 | 0x00 << 16; // LoRa mode
	uint8_t txData[3] = {0x03, 0x00, 0x00};  // Opcode = 0x03, NOP, NOP
	uint8_t rxData[3] = {0x00, 0x00, 0x00};

	// SPI-Übertragung und Empfang
	HAL_SPI_Transmit(&hspi1,txData, 3, HAL_MAX_DELAY);   // Lese die Antwort
	HAL_SPI_Receive(&hspi1, rxData, 3, HAL_MAX_DELAY);   // Lese die Antwort

	// SPI_TransmitReceiveWithDebug(hspi, txData[0], rxData, 3, "GetPacketType: ");
	// Der Pakettyp befindet sistatusch im dritten Byte (rxData[2])
	uint8_t packetType = rxData[2];

	// Debug-Ausgabe
	switch (packetType) {
	case 0x00:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Packet Type: GFSK (0x00)\r\n", 30, 100);
		break;
	case 0x01:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Packet Type: LoRa (0x01)\r\n", 30, 100);

		break;
	case 0x02:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Packet Type: Ranging (0x02)\r\n", 30, 100);

		break;
	case 0x03:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Packet Type: FLRC (0x03)\r\n", 30, 100);

		break;
	case 0x04:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Packet Type: BLE (0x04)\r\n", 30, 100);

		break;
	default:
		HAL_UART_Transmit(&huart2, (uint8_t *)"Unknown Packet Type:", 20, 100);
		HAL_UART_Transmit(&huart2, &packetType, 1, 100);
		HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 4, 100);

		break;
	}
}


// Funktion zum Setzen der Frequenz
void setFrequency() {
	uint8_t tx[4] = {0x86, 0xB8, 0x9D, 0x89}; // Frequenzdaten
	uint8_t rx[4] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 4, "Set Frequency\r\n");  // SPI mit Debugger
	HAL_Delay(10);

}

// Funktion zum Setzen der Basisadresse des Buffers
void setBufferBaseAddress() {
	uint8_t tx[3] = {0x8F, 0x80, 0x00}; // Basisadresse
	uint8_t rx[3] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 3, "Set Buffer Base Address\r\n");  // SPI mit Debugger
	HAL_Delay(10);

}

// Funktion zum Setzen der Modulationsparameter
void setModulationParams() {
	uint8_t tx[4] = {0x8B, 0x70, 0x18, 0x01}; // Modulationsparameter
	uint8_t rx[4] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 4, "Set Modulation Params\r\n");  // SPI mit Debugger
	HAL_Delay(10);
}

// Funktion zum Setzen der Paketparameter
void setPacketParams() {
	uint8_t tx[8] = {0x8C, 0x0C, 0x80, 0x08, 0x20, 0x40, 0x00, 0x00}; // Paketparameter
	uint8_t rx[8] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 8, "Set Packet Params\r\n");  // SPI mit Debugger
	HAL_Delay(10);
}

// Funktion zum Setzen der IRQ-Parameter (DIO1, DIO2, DIO3)
void setDioIrqParams() {
	uint8_t tx[9] = {0x8D, 0x40, 0x23, 0x00, 0x01, 0x00, 0x02, 0x40, 0x20}; // IRQ Masken
	uint8_t rx[9] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 9, "Set DIO IRQ Params\r\n");  // SPI mit Debugger
	HAL_Delay(10);
}

// Funktion zum Setzen der RX-Periode (Empfangsparameter)
void setRxPeriod() {
	uint8_t tx[4] = {0x82, 0x00, 0x00, 0x00}; // RX Periode
	uint8_t rx[4] = {0};
	SPI_TransmitReceiveWithDebug(&hspi1, tx, rx, 4, "Set RX Period\r\n");  // SPI mit Debugger
	HAL_Delay(10);
}

void DerTakt()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   // NSS auf HIGH
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
	HAL_Delay(2);
	SPI_WaitUntilReady(&hspi1);
	HAL_Delay(2);
	SPI_WaitTransmit(&hspi1);

}
void RX_BitRateTest(SPI_HandleTypeDef* hspi, UART_HandleTypeDef* huart) {
	uint32_t time_temp = HAL_GetTick();
	uint8_t received_arr[100] = {0}; // Puffer für empfangene Pakete
	int pre_get = 0;                // Letzter empfangener Wert
	uint8_t sum = 0;                // Anzahl empfangener Pakete
	float bps;                      // Bitrate
	uint8_t uart_buf[100];          // UART-Puffer
	int uart_buf_len;

	uart_buf_len = sprintf((char*)uart_buf, "SX1280 RX Bit Rate Test\r\n");
	HAL_UART_Transmit(huart, uart_buf, uart_buf_len, HAL_MAX_DELAY);

	while (1) {
		uint8_t tx[11] = {0}; // TX-Puffer für SPI-Befehle
		uint8_t rx[11] = {0}; // RX-Puffer für SPI-Antworten

		// WriteBuffer(offset, *data)
		tx[0] = 0x1A;
		DerTakt();
		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 8, HAL_MAX_DELAY);
		// SetRx(periodBase, periodBaseCount[15:8], periodBaseCount[7:0])
		tx[0] = 0x82;
		DerTakt();
		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, HAL_MAX_DELAY);

		// Warten auf IRQ
		while (1) {
			tx[0] = 0x15;
			DerTakt();
			HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, HAL_MAX_DELAY);
			if (rx[3] & 0x02) break; // IRQ gesetzt
		}

		// GetPacketStatus()
		tx[0] = 0x1D;
		DerTakt();

		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 7, HAL_MAX_DELAY);

		// ClrIrqStatus(irqMask)
		tx[0] = 0x97;
		tx[1] = 0xFF;
		tx[2] = 0xFF;
		DerTakt();

		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, HAL_MAX_DELAY);
		// GetRxBufferStatus()
		tx[0] = 0x17;
		DerTakt();

		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, HAL_MAX_DELAY);
		// ReadBuffer(offset, payloadLengthRx)
		tx[0] = 0x1B;
		DerTakt();

		HAL_SPI_TransmitReceive(&hspi1, tx, rx, 11, HAL_MAX_DELAY);

		// Empfangenes Paket auslesen
		int received = rx[3];
		uart_buf_len = sprintf((char*)uart_buf, "RX: %d %d\r\n", rx[3], rx[7]);
		HAL_UART_Transmit(huart, uart_buf, uart_buf_len, HAL_MAX_DELAY);

		// Bitrate berechnen
		if (received < 100 && received >= 0) {
			if (pre_get > received) {
				uint32_t elapsed_time = HAL_GetTick() - time_temp;
				sum = 0;

				for (int i = 0; i < 100; i++) {
					if (received_arr[i] == 1) {
						sum++;
					}
				}

				bps = (float)(sum * 8 * 8 * 1000) / elapsed_time;
				uart_buf_len = sprintf((char*)uart_buf, "Bit Rate: %15.5f bps, sum: %03d\r\n", bps, sum);
				HAL_UART_Transmit(huart, uart_buf, uart_buf_len, HAL_MAX_DELAY);

				// Puffer zurücksetzen
				memset(received_arr, 0, sizeof(received_arr));
				time_temp = HAL_GetTick();
			}

			pre_get = received;
			received_arr[received] = 1; // Paket markieren
		}

		HAL_Delay(500); // Wartezeit für Stabilität
	}
}

/* USER CODE END 0 */
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */
	//	Erase_Flash();
	int i=0;
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
	MX_SPI1_Init();
	/* USER CODE BEGIN 2 */
	HAL_Delay(2000);


	// Reset Chip
	ResetChip(GPIOC, GPIO_PIN_7);
	HAL_Delay(5);

	/*=======================================================================*/
	//NSSNSSNSSNSSNSSNSSNSSNSSNSSNSSNSSNSSNSSNSS
	/*=======================================================================*/

	// NSS Test
	SelectChip(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

	HAL_Delay(5);

	SelectChip(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

	HAL_Delay(5);

	SelectChip(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	nss = uint8_t(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));

	//HAL_Delay(5);

	//	SPI_WaitUntilReady(&hspi1);
	//	SPI_WaitOpcod(&hspi1);
	SPI_WaitTransmit(&hspi1);

	//HAL_Delay(2);

	//    uint8_t txBuffer[2] = {0x01, 0x00}; // Beispiel-Daten
	//    uint8_t rxBuffer[2];
	//    SPI_TransmitReceiveWithDebug(&hspi1, txBuffer, rxBuffer, 2, "SPI Test Completed\r\n");
	SetStandby();
	HAL_Delay(5);

	DerTakt();



	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{

		//SPI_SetStandby();
		//SPI_WaitTransmit(&hspi1);
		while(i!=3){
			GetStatus();
//			HAL_SPI_Transmit(&hspi1, &txData_const, 1, HAL_MAX_DELAY);  // Sende den Befehl
//			HAL_SPI_Receive(&hspi1, &rxData_const, 1, HAL_MAX_DELAY);   // Lese die Antwort
//			ProcessStatusByte(&rxData_const);
			DerTakt();
			//SPI_WaitPause(&hspi1);
			setLoRaMode();
			// LoRa Modus setzen
			DerTakt();


			getPacketType(&hspi1);

			DerTakt();

			setFrequency();
			// Frequenz setzen
			DerTakt();


			setBufferBaseAddress(); // Basisadresse setzen

			DerTakt();


			setModulationParams();  // Modulationsparameter setzen

			DerTakt();


			setPacketParams();

			DerTakt();

			// Paketparameter setzen
			setDioIrqParams();      // IRQ Parameter setzen

			DerTakt();

			setRxPeriod();          // RX Periode setzen

			DerTakt();



			//SPI_SetStandby();
			//SPI_WaitTransmit(&hspi1);

			HAL_SPI_Transmit(&hspi1, &txData_const, 1, HAL_MAX_DELAY);  // Sende den Befehl
			HAL_SPI_Receive(&hspi1, &rxData_const, 1, HAL_MAX_DELAY);   // Lese die Antwort
			ProcessStatusByte(&rxData_const);
			DerTakt();
			HAL_Delay(500);
			i++;
		}
		//DerTakt();

		RX_BitRateTest(&hspi1, &huart2);
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

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 80;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
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
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

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
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PA8 */
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PA9 */
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PB6 */
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // CS auf High

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
