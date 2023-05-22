
/********************************************************************************************************************
* @file		hw_desc_ext_eep.c
* @author	Astri Voufo
* @date		25.01.2023
*********************************************************************************************************************
*
*		This file containt all the function that will be used for eeprom 24LC32A.
*
*********************************************************************************************************************
*@remarks
*
********************************************************************************************************************/


#include "eep_24LCXX.h"
#include "pin_function.h"

/********************************************************************************************************************
 *                                                                                                                  *
 *                                             D E F I N I T I O N                                                  *
 *                                                                                                                  *
 *******************************************************************************************************************/
#define EEPROM_ZERO                          0
#define EEPROM_TIME_OUT                      5
#define EEPROM_DATA_LENGHT                   (uint8_t)(1)
#define EEPROM_CMD_LENGHT                    (uint8_t)(2)
#define EEPROM_PAGE_SIZE                     (uint8_t)(32)
#define EEPROM_ERASE                         (uint8_t)(0xFF)
#define EEPROM_ADDR_MAX                      (uint16_t)(0x0FFF)
#define EEPROM_DATA_SIZE_MAX                 (uint16_t)(4096)
#define EEPROM_HIGH_ADDR_OFFSET              (uint16_t)(0x08)
#define EEPROM_LOW_ADDR_MSK                  (uint16_t)(0x00FF)
#define EEPROM_HIGH_ADDR_MSK                 (uint16_t)(0x000F)
#define EEPROM_LOW_ADDR(addr)                (uint8_t)((addr) & EEPROM_LOW_ADDR_MSK)
#define EEPROM_HIGH_ADDR(addr)               (uint8_t)(((addr) >> EEPROM_HIGH_ADDR_OFFSET) & (EEPROM_HIGH_ADDR_MSK))
#define LED_GREEN_PIN                        (uint8_t)14
#define LED_RED_PIN                          (uint8_t)13
#define LED_BLUE_PIN                         (uint8_t)15
#define LED_RED_ON                           SET_BIT(PORT9->PODR, LED_RED_PIN)

/********************************************************************************************************************
 *                                                                                                                  *
 *                                        E N U M E R A T I O N                                                     *
 *                                                                                                                  *
 *******************************************************************************************************************/

/** eeprom transfert state */
enum EEPROM24XXTransferState
{
  EEPROM_STATE_DRIVER_NOT_INITIALIZED = 0,
  EEPROM_STATE_DRIVER_INITIALIZED     = 1,
  EEPROM_STATE_READ_IN_PROGRESS       = 2,
  EEPROM_STATE_READ_COMPLETED         = 3,
  EEPROM_STATE_WRITE_PAGE_COMPLETED   = 5,
  EEPROM_STATE_WRITE_COMPLETED        = 6,
  EEPROM_STATE_WRITE_PAGE             = 7,
  EEPROM_STATE_TRANSFER_IN_PROGRESS   = 4,
  EEPROM_STATE_TRANSFER_COMPLETED     = 8,
  EEPROM_STATE_READ_ABORTED           = 9,
  EEPROM_STATE_WRITE_ABORTED          = 10,
  EEPROM_STATE_WAIT_WRITE_CYCLE       = 11,

  EEPROM_STATE_MAX
};

typedef enum EEPROM24XXTransferState EEPROM24XXTransferState_t;

/** eeprom driver state */
enum EEPROM24XXDRVState
{
  EEPROM_DRIVER_NOT_INITIALIZED = 0,
  EEPROM_DRIVER_INITIALIZED     = 1,

  EEPROM_DRIVER_STATE_MAX
};

typedef enum EEPROM24XXDRVState EEPROM24XXDRVState_t;

/********************************************************************************************************************
 *                                                                                                                  *
 *                                              S T R U C T U R E                                                   *
 *                                                                                                                  *
 *******************************************************************************************************************/

/** EEPROM control block structure */
struct EEPROMDrv
{
  EEPROM24XXDRVState_t      eDrvState;                 ///< Allow to now if EEPROM was initialized
  EEPROM24XXTransferState_t eTranferState;             ///< Alllow to now if write or read operation is in progress
  eEEP24LCXXAddress_t       eAdresse;                  ///< EEPROM adress 
  sObjTimer_t               *psTimerInst;              ///< Pointer to an timer object
  I2CObj_t                  *psI2CInst;                ///< Pointer to an I2C object
  I2CTransfer_t             sI2CData;                  ///< data to use by I2C driver 
  cbkFunc_t                 pfvCbkTransmitEnd;         ///< user callback function is called when all data have been written */ 
  cbkFunc_t                 pfvCbkRcv;                 ///< user callback function detect the reception of each byte */
  cbkFunc_t                 pfvCbkError;               ///< user callback function detect the error durung write or read operation */ 
};

typedef struct EEPROMDrv EEPROMDrv_t;

/********************************************************************************************************************
 *                                                                                                                  *
 *                                      P R I V A T E  V A R I A B L E                                              *
 *                                                                                                                  *
 *******************************************************************************************************************/
#define EEPROM_CB_DRV_INIT                        {                                                                    \
                                                    .eDrvState                  = EEPROM_DRIVER_NOT_INITIALIZED,       \
                                                    .eTranferState              = EEPROM_STATE_DRIVER_NOT_INITIALIZED, \
                                                    .eAdresse                   = EEP24LCXX_ADDR_MAX,                  \
                                                    .psTimerInst                = NULL_PTR,                            \
                                                    .sI2CData.u8SlaveAddress    = EEP24LCXX_ADDR_MAX,                  \
                                                    .sI2CData.pu8Data           = NULL_PTR,                            \
                                                    .sI2CData.u16DataLength     = EEPROM_ZERO,                         \
                                                    .sI2CData.u8CmdLength       = EEPROM_ZERO,                         \
                                                    .sI2CData.u8RxIndex         = EEPROM_ZERO,                         \
                                                    .sI2CData.u8TxIndex         = EEPROM_ZERO,                         \
                                                    .sI2CData.pfvCbkTransmitEnd = NULL_PTR,                            \
                                                    .sI2CData.pfvCbkRcv         = NULL_PTR,                            \
                                                    .sI2CData.pfvCbkStop        = NULL_PTR,                            \
                                                    .sI2CData.pfvCbkError       = NULL_PTR,                            \
                                                    .sI2CData.eDirection        = I2_DIR_MAX,                          \
                                                    .sI2CData.pu8Cmd            = {EEPROM_ZERO}                        \
                                                  }

/** EEPROM control block variable */
static EEPROMDrv_t sCb = EEPROM_CB_DRV_INIT;

/********************************************************************************************************************
 *                                                                                                                  *
 *                          P R I V A T E  F U N C T I O N   D E C L A R A T I O N                                  *
 *                                                                                                                  *
 *******************************************************************************************************************/

/** @brief       This function write a collection of data in a page ofthe eeprom
  * @param [IN]  u16PageAddress  : adress of data to write
  * @param [IN]  pu8Data          : data to store
  * @return      true if write operation was don correctly, otherwise false
 **/
static bool bEEP24LC32WritePage(uint16_t u16PageAddress, uint8_t *pu8Data, uint16_t u16DataSize);

/** @brief       This function initialize eeprom
  * @param [IN]  eSlaveAddress   : adress of the eeprom
  * @param [IN]  psI2CInst       : pointer to I2C object
  * @param [IN]  psTimerInst     : pointer to a timer object
  * @param [OUT] none
  * @return      none
 **/
static bool bEEP24LC32Init(eEEP24LCXXAddress_t eSlaveAddress, I2CObj_t  *psI2CInst, sObjTimer_t *psTimerInst);


/** @brief       This function wite data in the eeprom
  * @param [IN]  sEEPData : eeprom data
  * @return      true if write operation was done correctly, otherwise false
 **/
static bool bEEP24LC32WriteData(EEP24LCXXData_t *sEEPData);


/** @brief       This function read data in the eeprom
  * @param [IN]  sEEPData : eeprom data
  * @return      true if read operation was done correctly, otherwise false
 **/
static bool bEEP24LC32ReadData(EEP24LCXXData_t *sEEPData);


/** @brief       This function is call when error occur during transmission 
  * @return      none
 **/
static void vEEP24LC32ErrorHandler(void);


/** @brief       This function is call for each byte transmited by the eeprom 
  * @return      none
 **/
static void vEEP24LC32ReceiveHandler(void);


/** @brief       This function is call when transfer is completed
  * @return      none
 **/
static void vEEP24LC32TransmitHandler(void);


/** @brief       This function is list all callback function of eeprom
  * @return      none
 **/
static void vEEP24LC32Handler(void);

/********************************************************************************************************************
 *                                                                                                                  *
 *                           P R I V A T E  F U N C T I O N  D E F I N I T I O N                                    *
 *                                                                                                                  *
 *******************************************************************************************************************/

/** @brief       This function initialize eeprom
  * @param [IN]  eSlaveAddress   : adress of the eeprom
  * @param [IN]  psI2CInst       : pointer to I2C object
  * @param [IN]  psTimerInst     : pointer to a timer object
  * @param [OUT] none
  * @return      none
 **/
static bool bEEP24LC32Init(eEEP24LCXXAddress_t eSlaveAddress, I2CObj_t  *psI2CInst, sObjTimer_t *psTimerInst)
{   
   /* check if I2C driver and Timer was initialized */
   if ((psI2CInst != NULL_PTR) && (psTimerInst != NULL_PTR))
   {
      sCb.psTimerInst   = psTimerInst; 
      sCb.psI2CInst     = psI2CInst;
      sCb.eAdresse      = eSlaveAddress;
      sCb.eDrvState     = EEPROM_DRIVER_INITIALIZED;
      sCb.eTranferState = EEPROM_STATE_DRIVER_INITIALIZED;
   }

   return (EEPROM_DRIVER_INITIALIZED == sCb.eDrvState);
}


/** @brief       This function write a collection of data in a page of the eeprom
  * @param [IN]  u16PageAddress  : adress of data to write
  * @param [IN]  pu8Data         : data to store
  * @return      true if write operation was done correctly, otherwise false
 **/
static bool bEEP24LC32WritePage(uint16_t u16PageAddress, uint8_t *pu8Data, uint16_t u16DataSize)
{
  bool bRet = false;

  if (sCb.eDrvState == EEPROM_DRIVER_INITIALIZED)
  {
    if (((u16DataSize > EEPROM_ZERO) && (u16DataSize <= EEPROM_PAGE_SIZE)) && (u16PageAddress <= EEPROM_ADDR_MAX) && (pu8Data != NULL_PTR))
    {
      sCb.sI2CData.u8SlaveAddress    = (uint8_t)sCb.eAdresse;
      sCb.sI2CData.pu8Data           = &pu8Data[0];
      sCb.sI2CData.u16DataLength     = u16DataSize;
      sCb.sI2CData.pu8Cmd[0]         = (uint8_t)EEPROM_HIGH_ADDR(u16PageAddress);
      sCb.sI2CData.pu8Cmd[1]         = (uint8_t)EEPROM_LOW_ADDR(u16PageAddress);
      sCb.sI2CData.u8CmdLength       = EEPROM_CMD_LENGHT;
      sCb.sI2CData.pfvCbkTransmitEnd = vEEP24LC32Handler; 
      sCb.sI2CData.pfvCbkRcv         = vEEP24LC32Handler; 
      sCb.sI2CData.pfvCbkStop        = vEEP24LC32Handler; 
      sCb.sI2CData.pfvCbkError       = vEEP24LC32Handler; 
      sCb.sI2CData.eDirection        = I2C_DIR_WRITE;

      bRet = sCb.psI2CInst->pfbMasterStartTransmit(&sCb.sI2CData);
    }
  }

  return bRet;
}


/** @brief       This function write data in the eeprom
  * @param [IN]  sEEPData : eeprom data
  * @return      true if write operation was done correctly, otherwise false
 **/
static bool bEEP24LC32WriteData(EEP24LCXXData_t *sEEPData)
{
   static bool     bRet              = false;
   static uint16_t u16DataSize_m      = EEPROM_ZERO;
   static uint16_t u16StartAddress_m  = EEPROM_ZERO;
   static uint16_t u16PageStartAddr   = EEPROM_ZERO;
   static uint16_t u16PageEndAddr     = EEPROM_ZERO;
   static uint8_t  u8PageSize         = EEPROM_ZERO;
   static uint8_t  u8Index            = EEPROM_ZERO;
   static uint32_t u32WriteTimeOut    = EEPROM_ZERO;
   
   if (((sEEPData->u16DataSize > EEPROM_ZERO) && (sEEPData->u16DataSize <= EEPROM_DATA_SIZE_MAX)) && (sEEPData->u16StartAddress <= EEPROM_ADDR_MAX) && (sEEPData->pu8Data != NULL_PTR))
   {
      switch(sCb.eTranferState)
      {
        case EEPROM_STATE_DRIVER_INITIALIZED : 
        case EEPROM_STATE_READ_COMPLETED     :
        case EEPROM_STATE_WRITE_COMPLETED    :
        case EEPROM_STATE_WRITE_ABORTED      :
        {
          /* Initialization of local variables */
          bRet             = false;
          u8Index           = EEPROM_ZERO;
          u16DataSize_m     = sEEPData->u16DataSize;
          u16StartAddress_m = sEEPData->u16StartAddress;
          
          /* Compute the start address of the first page */
          u16PageStartAddr = (sEEPData->u16StartAddress / EEPROM_PAGE_SIZE) * (EEPROM_PAGE_SIZE);

          /* Compute the number of data to be write in the first page */
          u8PageSize       = (EEPROM_PAGE_SIZE) - (uint8_t)(sEEPData->u16StartAddress - u16PageStartAddr);

          /* Compute the end address of the first page */
          u16PageEndAddr   = (sEEPData->u16StartAddress) + (uint16_t)(u8PageSize - 1);

          /* if DataSize < 32 set PageSize to data size */
          if (u8PageSize > sEEPData->u16DataSize)
          {
            u8PageSize = (uint8_t)sEEPData->u16DataSize; 
          }

          /* set state */
          sCb.eTranferState = EEPROM_STATE_WRITE_PAGE;

          break;
        }

        case EEPROM_STATE_WRITE_PAGE:
        {
          /* storage of user callback functions */
          sCb.pfvCbkError        = sEEPData->pfvCbkError;
          sCb.pfvCbkRcv          = sEEPData->pfvCbkRcv;
          sCb.pfvCbkTransmitEnd  = sEEPData->pfvCbkTransmitEnd;
          
          /* set state */
          sCb.eTranferState = EEPROM_STATE_TRANSFER_IN_PROGRESS;

          /* Write data on the page */
          if ((u16StartAddress_m + u8PageSize) <= EEPROM_ADDR_MAX)
          {
            bRet = bEEP24LC32WritePage(u16StartAddress_m, &sEEPData->pu8Data[u8Index], u8PageSize);
          }

          if (bRet == false)
          {
            /* set state */
            sCb.eTranferState = EEPROM_STATE_WRITE_ABORTED;
          }

          break; 
        }

        case EEPROM_STATE_TRANSFER_IN_PROGRESS:
          /* wait until the transfer is completed */
          break;

        case EEPROM_STATE_TRANSFER_COMPLETED:
        {
          /* set state */
          sCb.eTranferState = EEPROM_STATE_WAIT_WRITE_CYCLE;

          /* set TimeOut */
          u32WriteTimeOut  = (uint32_t)~sCb.psTimerInst->pfu32GetTickMs() + 1;

          break;
        }

        case EEPROM_STATE_WAIT_WRITE_CYCLE:
        {
          /* wait 5 milliseconds until the chip completed the internal write cycle */
          if ((sCb.psTimerInst->pfu32GetTickMs() + u32WriteTimeOut) > (uint32_t)(EEPROM_TIME_OUT))
          {
            /* set state */
            sCb.eTranferState = EEPROM_STATE_WRITE_PAGE_COMPLETED;    
          }

          break;
        }

        case EEPROM_STATE_WRITE_PAGE_COMPLETED:
        {
          /* Sustract to data size the data which has previously writed */
          u16DataSize_m    -= u8PageSize; 

          /* Compute the new start address of the bytes to write */
          u16StartAddress_m = u16PageEndAddr + 1;

          /* Add previous index to the new index */
          u8Index          += u8PageSize;

          /* Compute the new page size */
          if (u16DataSize_m >= EEPROM_PAGE_SIZE)
          {
            u8PageSize = EEPROM_PAGE_SIZE;
          }
          else
          {
            u8PageSize = (uint8_t)u16DataSize_m; 
          }
          
          /* Compute the new page end address */
          u16PageEndAddr  = u16StartAddress_m + (uint16_t)(u8PageSize - 1);  

          if (u16DataSize_m > EEPROM_ZERO)
          {
            /* set state */
            sCb.eTranferState = EEPROM_STATE_WRITE_PAGE;
          }
          else
          {
            /* set state */
            sCb.eTranferState = EEPROM_STATE_WRITE_COMPLETED;           
          }
        
          break;
        }

        default:
          break;
      }
          
   }
   
   return (EEPROM_STATE_WRITE_COMPLETED == sCb.eTranferState);
}


/** @brief       This function read data in the eeprom
  * @param [IN]  sEEPData : eeprom data
  * @return      true if read operation was done correctly, otherwise false
 **/
static bool bEEP24LC32ReadData(EEP24LCXXData_t *sEEPData)
{
  if ((sEEPData->u16DataSize <= EEPROM_DATA_SIZE_MAX) && (sEEPData->u16StartAddress <= EEPROM_ADDR_MAX) && (sEEPData->pu8Data != NULL_PTR))
  {
    switch(sCb.eTranferState)
    {
      case EEPROM_STATE_DRIVER_INITIALIZED : 
      case EEPROM_STATE_READ_COMPLETED     :
      case EEPROM_STATE_WRITE_COMPLETED    :
      {
        /* storage of user callback functions */
        sCb.pfvCbkError                = sEEPData->pfvCbkError;
        sCb.pfvCbkRcv                  = sEEPData->pfvCbkRcv;
        sCb.pfvCbkTransmitEnd          = sEEPData->pfvCbkTransmitEnd;

        /* storage of i2c datas */
        sCb.sI2CData.u8SlaveAddress    = (uint8_t)sCb.eAdresse;
        sCb.sI2CData.pu8Data           = sEEPData->pu8Data;
        sCb.sI2CData.u16DataLength     = sEEPData->u16DataSize;
        sCb.sI2CData.pu8Cmd[0]         = (uint8_t)EEPROM_HIGH_ADDR(sEEPData->u16StartAddress);
        sCb.sI2CData.pu8Cmd[1]         = (uint8_t)EEPROM_LOW_ADDR(sEEPData->u16StartAddress);
        sCb.sI2CData.u8CmdLength       = EEPROM_CMD_LENGHT;
        sCb.sI2CData.pfvCbkTransmitEnd = vEEP24LC32Handler; 
        sCb.sI2CData.pfvCbkRcv         = vEEP24LC32Handler; 
        sCb.sI2CData.pfvCbkStop        = vEEP24LC32Handler; 
        sCb.sI2CData.pfvCbkError       = vEEP24LC32Handler; 
        sCb.sI2CData.eDirection        = I2C_DIR_WRITE_READ;

        /* set transfer state to tranfer in progress */
        sCb.eTranferState = EEPROM_STATE_READ_IN_PROGRESS;
        
        /* start of data reception */
        sCb.psI2CInst->pfbMasterStartTransmit(&sCb.sI2CData);
        break;
      }

      case EEPROM_STATE_READ_IN_PROGRESS :
        /* we wait until all data are received */
        break;

      default:
        break;

    }

  }

  return (EEPROM_STATE_READ_COMPLETED == sCb.eTranferState);
}


/** @brief       This function is call when transfer is completed
  * @return      none
 **/
static void vEEP24LC32TransmitHandler(void)
{
  /* we count the number of transmited byte */
  if(sCb.sI2CData.u8TxIndex == sCb.sI2CData.u16DataLength)
  {
    /* set the flag when all data were transmitted */
    sCb.eTranferState = EEPROM_STATE_TRANSFER_COMPLETED;

    /* call of transmit callback function */
    if (sCb.pfvCbkTransmitEnd != NULL_PTR)
    {
      sCb.pfvCbkTransmitEnd();  
    }
  }
}


/** @brief       This function is call for each byte transmited by the eeprom 
  * @return      none
 **/
static void vEEP24LC32ReceiveHandler(void)
{
  /* we count the number of received byte during the tranfer */
  if (sCb.sI2CData.u8RxIndex == sCb.sI2CData.u16DataLength)
  {
    /* all datas have been received */
    sCb.eTranferState = EEPROM_STATE_READ_COMPLETED;
    
    /* call of received callback function */
    if (sCb.pfvCbkRcv != NULL_PTR)
    {
      sCb.pfvCbkRcv();
    }
  }
}


/** @brief       This function is call when error occur during transmission 
  * @return      none
 **/
static void vEEP24LC32ErrorHandler(void)
{
  /* set state */
  sCb.eTranferState = EEPROM_STATE_WRITE_ABORTED;

  /* call of error callback function */
  if (sCb.pfvCbkError != NULL_PTR)
  {
    sCb.pfvCbkError();
  }
}


/** @brief       This function list all callback function of eeprom
  * @return      none
 **/
static void vEEP24LC32Handler(void)
{
  switch(sCb.psI2CInst->pfeGetTransferState())
  {
    case I2C_STATE_TRANSFER_COMPLETED:
      vEEP24LC32TransmitHandler();
      break;

    case I2C_STATE_RECEIVE_CONDITION:
      vEEP24LC32ReceiveHandler();
      break;

    case I2C_STATE_NACK_DETECTION:
      vEEP24LC32ErrorHandler();

    default:
      break;
  }
}

/********************************************************************************************************************
 *                                                                                                                  *
 *                                    P U B L I C  F U N C T I O N                                                  *
 *                                                                                                                  *
 *******************************************************************************************************************/


bool bEEP24LCXXInitInst(EEP24LCXXObj_t *sEEPObj)
{
  bool bRet = false;

  bRet = bEEP24LC32Init(sEEPObj->eEEPSlaveAddress, sEEPObj->psI2CInst, sEEPObj->psTimerInst);

  if (bRet == true)
  {
    sEEPObj->pfbEEPWriteData = bEEP24LC32WriteData;
    sEEPObj->pfbEEPReadData  = bEEP24LC32ReadData;
  }
  else
  {
    sEEPObj->pfbEEPWriteData = NULL_PTR;
    sEEPObj->pfbEEPReadData  = NULL_PTR;
  }

  return bRet;
}


/********************************************************************************************************************
 *                                                                                                                  *
 *                                          E N D   OF  M O D U L E                                                 *
 *                                                                                                                  *
 *******************************************************************************************************************/









