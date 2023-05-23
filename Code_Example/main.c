

/********************************************************************************************************************
* @file		main.c
* @author	VOUFO BOGNING ULRICH ASTRI
* @date		20.05.2023
*********************************************************************************************************************
* @brief	Main of project.
*
*		This file is the main file of project.
*
*********************************************************************************************************************
*@remarks
*
********************************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "hw_desc_timer.h"
#include "hw_desc_i2c.h"
#include "eep_24LCXX.h"


#define LED_GREEN_PIN                        (uint8_t)14
#define LED_RED_PIN                          (uint8_t)13
#define LED_BLUE_PIN                         (uint8_t)15
#define LED_GREEN_TOGGLE                     TOGGLE_BIT(PORT9->PODR, LED_GREEN_PIN)
#define LED_RED_ON                           TOGGLE_BIT(PORT9->PODR, LED_RED_PIN)
#define LED_BLUE_TOGGLE                      TOGGLE_BIT(PORT9->PODR, LED_BLUE_PIN)

#define TIME_OUT                             50
#define TX_BUFFER_SIZE                       254
#define RX_BUFFER_SIZE                       100
#define DATA_LENGHT                          10
#define EEPROM_ADDR_MAX                      (uint16_t)(0x0FFF)
#define EEPROM_ZERO                          0


uint32_t u32CurrentTime = 0;
uint8_t  u8Index = 0;

void vToggLedGreen(void)
{
  LED_GREEN_TOGGLE;
}

void vToggleLedBlue(void)
{
  LED_BLUE_TOGGLE;
}

void vLedRedOn(void)
{
  LED_RED_ON;
}

//***************************************************************************
//main: main function
//***************************************************************************

int main()
{
  vSystemInit();

  /* Buffer for I2C transfer */
  uint8_t u8TxBuffer[TX_BUFFER_SIZE] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF};
  uint8_t u8RxBuffer[RX_BUFFER_SIZE] = {0};

  /* Creation of timer instance */
  sObjTimer_t sTimerInst;

  /* Initialization of timer */
  vInitTimerInst(&sTimerInst, TIMER_ID0);

  /* Start timer */
  sTimerInst.pfvStart();

  /* initialization of receive data structure */
  EEP24LCXXData_t sRData =
  {
    .u16StartAddress   = 0x00,
    .pu8Data           = (uint8_t*)&u8RxBuffer[0],
    .u16DataSize       = DATA_LENGHT,
    .pfvCbkError       = vLedRedOn,
    .pfvCbkRcv         = vToggleLedBlue,
    .pfvCbkTransmitEnd = vToggLedGreen
  };
  
  /* Initialization of tx data structure */
   EEP24LCXXData_t sWData =
  {
    .u16StartAddress   = 0x00,
    .pu8Data           = (uint8_t*)&u8TxBuffer[0],
    .u16DataSize       = DATA_LENGHT,
    .pfvCbkError       = vLedRedOn,
    .pfvCbkRcv         = vToggleLedBlue,
    .pfvCbkTransmitEnd = vToggLedGreen
  };

  /* Creation of I2C instance */
  I2CObj_t sI2CInst = 
  {
    .eI2CId         = I2C_ID0,
    .eI2CFreq       = I2C_FREQ_400_KHZ,
    .eSCLPin        = I2C_PIN_SCL_P100,
    .eSDAPin        = I2C_PIN_SDA_P101,
    .eMode          = I2C_MASTER_MODE
  };

  /* Initialization of I2C driver  */
  vI2CInitInst(&sI2CInst);

  /* Creation of eeprom instance */
  EEP24LCXXObj_t sEEPInst =
  {
    .eEEPSlaveAddress = EEP24LCXX_ADDR0,
    .psI2CInst        = &sI2CInst,
    .psTimerInst      = &sTimerInst 
  };

  /* Initialization of EEPROM */
  bEEP24LCXXInitInst(&sEEPInst);

  /* Initialization of tx buffer */
  for(u8Index = EEPROM_ZERO; u8Index <= TX_BUFFER_SIZE; u8Index++)
  {
    u8TxBuffer[u8Index] = (u8Index + 1);
  } 

  /* Initialization of index */
  u8Index = EEPROM_ZERO;

  while(1)
  {
    /* write Data */
    if(sEEPInst.pfbEEPWriteData(&sWData))
    {   
      /* if write transaction was done correctly, read the data */ 
      sEEPInst.pfbEEPReadData(&sRData);

      /* go to the next address */
      sRData.u16StartAddress += DATA_LENGHT;
      sWData.u16StartAddress = sRData.u16StartAddress;
      u8Index += DATA_LENGHT; 
      
      /* check that the index and the address do not exceed the limit  of the eeprom*/
      if(sRData.u16StartAddress > EEPROM_ADDR_MAX)
      {
        sRData.u16StartAddress = EEPROM_ZERO;
      }

    }

    /* wait 50 milliseconds between each reading and blink the led */
    u32CurrentTime = (uint32_t)~(sTimerInst.pfu32GetTickMs()) + 1;
    while((sTimerInst.pfu32GetTickMs() + u32CurrentTime) <= TIME_OUT);

    /* write 32 byte on specified address  */
    sWData.pu8Data     = &u8TxBuffer[u8Index % ((TX_BUFFER_SIZE/DATA_LENGHT)*DATA_LENGHT)];
    sWData.u16DataSize = DATA_LENGHT;

    sRData.pu8Data     = &u8RxBuffer[u8Index % ((RX_BUFFER_SIZE/DATA_LENGHT)*DATA_LENGHT)];
    sRData.u16DataSize = DATA_LENGHT;
  }  
} 

/********************************************************************************************************************
 *                                                                                                                  *
 *                                          E N D   OF  M O D U L E                                                 *
 *                                                                                                                  *
 *******************************************************************************************************************/
