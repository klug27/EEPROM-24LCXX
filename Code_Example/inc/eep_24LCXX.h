

/********************************************************************************************************************
* @file		hw_desc_ext_eep.h
* @author	Astri Voufo
* @date		25.01.2023
*********************************************************************************************************************
*
*		This file containt all the function that will be used for eeprom 24LC32A.
*
*********************************************************************************************************************
* @remarks
*
********************************************************************************************************************/

#ifndef EXT_EEP_H
#define EXT_EEP_H

#include <stdbool.h>
#include "R7FA2E1A9.h"
#include "hw_desc_timer.h"
#include "hw_desc_i2c.h"


/********************************************************************************************************************
 *                                                                                                                  *
 *                                               D E F I N I T I O N                                                *
 *                                                                                                                  *
 *******************************************************************************************************************/
#define EEPROM_CRTL_CODE                  (uint8_t)(0x0A)
#define EEPROM_ADDR_OFFSET                (uint8_t)(0x03)
#define EEPROM_CS_ADDR0                   (uint8_t)(0)             
#define EEPROM_CS_ADDR1                   (uint8_t)(1)             
#define EEPROM_CS_ADDR2                   (uint8_t)(2)            
#define EEPROM_CS_ADDR3                   (uint8_t)(3)             
#define EEPROM_CS_ADDR4                   (uint8_t)(4)             
#define EEPROM_CS_ADDR5                   (uint8_t)(5)             
#define EEPROM_CS_ADDR6                   (uint8_t)(6)             
#define EEPROM_CS_ADDR7                   (uint8_t)(7)   
#define EEPROM_ADDRESS(CS_ADDR)           (uint8_t)((EEPROM_CRTL_CODE << EEPROM_ADDR_OFFSET)|(CS_ADDR))   

/********************************************************************************************************************
 *                                                                                                                  *
 *                                              E N U M E R A T I O N                                               *
 *                                                                                                                  *
 *******************************************************************************************************************/

/*
* For using 24LC32A eeprom we muss set the address of the chip. So choose one of this 
* following address to handle this device with I2C communication protocol
*/
enum eEEP24LCXXAddress
{
   EEP24LCXX_ADDR0 = EEPROM_ADDRESS(EEPROM_CS_ADDR0),
   EEP24LCXX_ADDR1 = EEPROM_ADDRESS(EEPROM_CS_ADDR1),
   EEP24LCXX_ADDR2 = EEPROM_ADDRESS(EEPROM_CS_ADDR2),
   EEP24LCXX_ADDR3 = EEPROM_ADDRESS(EEPROM_CS_ADDR3),
   EEP24LCXX_ADDR4 = EEPROM_ADDRESS(EEPROM_CS_ADDR4),
   EEP24LCXX_ADDR5 = EEPROM_ADDRESS(EEPROM_CS_ADDR5),
   EEP24LCXX_ADDR6 = EEPROM_ADDRESS(EEPROM_CS_ADDR6),
   EEP24LCXX_ADDR7 = EEPROM_ADDRESS(EEPROM_CS_ADDR7),

   EEP24LCXX_ADDR_MAX
};

typedef enum eEEP24LCXXAddress eEEP24LCXXAddress_t; 

/********************************************************************************************************************
 *                                                                                                                  *
 *                                              S T R U C T U R E                                                   *
 *                                                                                                                  *
 *******************************************************************************************************************/

/*
 * eeprom data structure
 */
struct EEP24LCXXData
{
  uint16_t   u16StartAddress;        /**< Address of the memory case to write or to read */
  uint8_t    *pu8Data;               /**< in read operation : buffer who data will be stored */ 
                                     /**< in write operatio : pointer to the data to write */
  uint16_t   u16DataSize;            /**< lenght of data to read or to write */
  cbkFunc_t  pfvCbkTransmitEnd;      /**< user callback function is called when all data have been written */ 
  cbkFunc_t  pfvCbkRcv;              /**< user callback function detect the reception of each byte */
  cbkFunc_t  pfvCbkError;            /**< user callback function detect the error durung write or read operation */ 
};

typedef struct EEP24LCXXData EEP24LCXXData_t;

typedef bool (*EEPCbkFunc_t)(EEP24LCXXData_t *sEEPData);   

/*
 * eeprom object
 */
struct EEP24LCXXObj
{
  eEEP24LCXXAddress_t   eEEPSlaveAddress;  /**< eeprom slave address */
  sI2CObj_t             *psI2CInst;        /**< pointer to I2C object */
  sTimerObj_t           *psTimerInst;      /**< pointer to timer object */
  EEPCbkFunc_t          pfbEEPWriteData;   /**< This function write a collection of data in the eeprom */
  EEPCbkFunc_t          pfbEEPReadData;    /**< This function read data in the eeprom */
 };

typedef struct EEP24LCXXObj EEP24LCXXObj_t;

/********************************************************************************************************************
 *                                                                                                                  *
 *                                    P U B L I C  F U N C T I O N                                                  *
 *                                                                                                                  *
 *******************************************************************************************************************/


/** @brief       This function initialize eeprom 24LC32A
  * @param [IN]  sEEPObj : pointer to the eeprom object
  * @return      true if instance was initialized succesfully, otherwise false
 **/
bool bEEP24LCXXInitInst(EEP24LCXXObj_t *sEEPObj);


#endif

/********************************************************************************************************************
 *                                                                                                                  *
 *                                        E N D   OF  M O D U L E                                                   *
 *                                                                                                                  *
 *******************************************************************************************************************/