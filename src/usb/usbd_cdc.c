/**
  ******************************************************************************
  * @file    usbd_cdc.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the high layer firmware functions to manage the 
  *          following functionalities of the USB CDC Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *           
  *  @verbatim
  *      
  *          ===================================================================      
  *                                CDC Class Driver Description
  *          =================================================================== 
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus 
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class
  * 
  *           These aspects may be enriched or modified for a specific user application.
  *          
  *            This driver doesn't implement the following aspects of the specification 
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *      
  *  @endverbatim
  *                                  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_CDC_Private_Defines
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_CDC_Private_Macros
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */

#include <cdc_descriptor.h>


static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx);

static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx);

static uint8_t  USBD_CDC_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req);

static uint8_t  USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, 
                                 uint8_t epnum);

static uint8_t  USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, 
                                 uint8_t epnum);

static uint8_t  USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev);

static uint8_t  *USBD_CDC_GetFSCfgDesc (uint16_t *length);

static uint8_t  *USBD_CDC_GetHSCfgDesc (uint16_t *length);

static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc (uint16_t *length);

uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length);

void *ctxPointers[NUM_CDC_INSTANCES];

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Variables
  * @{
  */ 


/* CDC interface class callbacks structure */
USBD_ClassTypeDef  USBD_CDC = 
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_EP0_RxReady,
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  NULL,
  NULL,
  NULL,     
  USBD_CDC_GetHSCfgDesc,  
  USBD_CDC_GetFSCfgDesc,    
  USBD_CDC_GetOtherSpeedCfgDesc, 
  USBD_CDC_GetDeviceQualifierDescriptor,
};

/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */ 

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev, 
                               uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_CDC_HandleTypeDef   *hcdc;
  
  if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
  {  
    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   CDC_IN_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_IN_PACKET_SIZE);
    
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   CDC_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_OUT_PACKET_SIZE);
    
    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   CDC2_IN_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_IN_PACKET_SIZE);
    
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   CDC2_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_HS_OUT_PACKET_SIZE);
  }
  else
  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   CDC_IN_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_IN_PACKET_SIZE);
    
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   CDC_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_OUT_PACKET_SIZE);
    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   CDC2_IN_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_IN_PACKET_SIZE);
    
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   CDC2_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   CDC_DATA_FS_OUT_PACKET_SIZE);
  }
  /* Open Command IN EP */
  USBD_LL_OpenEP(pdev,
                 CDC_CMD_EP,
                 USBD_EP_TYPE_INTR,
                 CDC_CMD_PACKET_SIZE);
  /* Open Command IN EP */
  USBD_LL_OpenEP(pdev,
                 CDC2_CMD_EP,
                 USBD_EP_TYPE_INTR,
                 CDC_CMD_PACKET_SIZE);
  
    
  pdev->pClassData = USBD_malloc(sizeof (USBD_CDC_HandleTypeDef));
  
  if(pdev->pClassData == NULL)
  {
    ret = 1; 
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
    
    /* Init  physical Interface components */
    for (int i = 0; i < NUM_CDC_INSTANCES; i++) {
	    /* Init Xfer states */
	    hcdc->TxState[i] = 0;
	    hcdc->RxState[i] = 0;

	    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init(i, ctxPointers + i);

    }
       
    if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
    {      
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
			     CDC_OUT_EP,
			     hcdc->RxBuffer[0],
			     CDC_DATA_HS_OUT_PACKET_SIZE);
      USBD_LL_PrepareReceive(pdev,
			     CDC2_OUT_EP,
			     hcdc->RxBuffer[1],
			     CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
			     CDC_OUT_EP,
			     hcdc->RxBuffer[0],
			     CDC_DATA_FS_OUT_PACKET_SIZE);
      USBD_LL_PrepareReceive(pdev,
			     CDC2_OUT_EP,
			     hcdc->RxBuffer[1],
			     CDC_DATA_FS_OUT_PACKET_SIZE);
    }
    
    
  }
  return ret;
}

/**
  * @brief  USBD_CDC_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, 
                                 uint8_t cfgidx)
{
  uint8_t ret = 0;
  
  USBD_LL_CloseEP(pdev,
              CDC_IN_EP);
  USBD_LL_CloseEP(pdev,
              CDC_OUT_EP);
  USBD_LL_CloseEP(pdev,
              CDC_CMD_EP);
  USBD_LL_CloseEP(pdev,
              CDC2_IN_EP);
  USBD_LL_CloseEP(pdev,
              CDC2_OUT_EP);
  USBD_LL_CloseEP(pdev,
              CDC2_CMD_EP);
  
  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit(ctxPointers[0]);
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit(ctxPointers[1]);
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }
  
  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_CDC_Setup (USBD_HandleTypeDef *pdev, 
                                USBD_SetupReqTypedef *req)
{
  int instance = 0;

  if ((req->wIndex == USB_CDC_CIF_NUM1) ||
		  (req->wIndex == USB_CDC_DIF_NUM1)) {
	  instance = 1;
  }

  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  hcdc->ctrlInst = instance;

  static uint8_t ifalt = 0;
    
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    if (req->wLength)
    {
      if (req->bmRequest & 0x80)
      {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(ctxPointers[instance],
			req->bRequest, (uint8_t *)hcdc->data, req->wLength);
          USBD_CtlSendData (pdev, 
                            (uint8_t *)hcdc->data,
                            req->wLength);
      }
      else
      {
        hcdc->CmdOpCode = req->bRequest;
        hcdc->CmdLength = req->wLength;
        
        USBD_CtlPrepareRx (pdev, 
                           (uint8_t *)hcdc->data,
                           req->wLength);
      }
      
    }
    else
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(ctxPointers[instance],
	                                                req->bRequest,
                                                        (uint8_t*)req,
                                                        0);
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        &ifalt,
                        1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      break;
    }
 
  default: 
    break;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  int instance = 0;
  if (epnum & CDC2_EP_MASK) {
	  instance = 1;
  }
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  
  if(pdev->pClassData != NULL)
  {
      
    hcdc->TxState[instance] = 0;

      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->TxComplete(ctxPointers[instance]);

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  int instance = 0;
  if (epnum & CDC2_EP_MASK) {
	  instance = 1;
  }

  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  
  /* Get the received data length */
  hcdc->RxLength[instance] = USBD_LL_GetRxDataSize (pdev, epnum);
  
  /* USB data will be immediately processed, this allow next USB traffic being 
  NAKed till the end of the application Xfer */
  if(pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Receive(ctxPointers[instance], hcdc->RxBuffer[instance], &hcdc->RxLength[instance]);

    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}



/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  int instance = hcdc->ctrlInst;
  
  if((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFF))
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(ctxPointers[instance],
	    					hcdc->CmdOpCode,
                                                      (uint8_t *)hcdc->data,
                                                      hcdc->CmdLength);
      hcdc->CmdOpCode = 0xFF; 
      
  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetFSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetFSCfgDesc (uint16_t *length)
{
  *length = sizeof (IADCDCTwoDescriptor);
  return (uint8_t *) IADCDCTwoDescriptor;
}

/**
  * @brief  USBD_CDC_GetHSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetHSCfgDesc (uint16_t *length)
{
  *length = 0;
  return NULL;

  //*length = sizeof (USBD_CDC_CfgHSDesc);
  //return USBD_CDC_CfgHSDesc;
}

/**
  * @brief  USBD_CDC_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_CDC_GetOtherSpeedCfgDesc (uint16_t *length)
{
  *length = sizeof (IADCDCTwoDescriptor);
  return (uint8_t *) IADCDCTwoDescriptor;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_CDC_DeviceQualifierDesc);
  return USBD_CDC_DeviceQualifierDesc;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_CDC_RegisterInterface  (USBD_HandleTypeDef   *pdev, 
                                      USBD_CDC_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;
  
  if(fops != NULL)
  {
    pdev->pUserData= fops;
    ret = USBD_OK;    
  }
  
  return ret;
}

/**
  * @brief  USBD_CDC_SetTxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Tx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetTxBuffer  (USBD_HandleTypeDef   *pdev,
				int instance,
                                const uint8_t  *pbuff,
                                uint16_t length)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  
  hcdc->TxBuffer[instance] = pbuff;
  hcdc->TxLength[instance] = length;  
  
  return USBD_OK;  
}


/**
  * @brief  USBD_CDC_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
uint8_t  USBD_CDC_SetRxBuffer  (USBD_HandleTypeDef   *pdev,
                                int instance,
                                uint8_t  *pbuff)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  
  hcdc->RxBuffer[instance] = pbuff;
  
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
uint8_t  USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, int instance)
{      
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
 
  int ep = CDC_IN_EP;

  if (instance) {
	  ep = CDC2_IN_EP;
  }

  if(pdev->pClassData != NULL)
  {
    if(hcdc->TxState[instance] == 0)
    {
      /* Tx Transfer in progress */
      hcdc->TxState[instance] = 1;
      
      /* Transmit next packet */
      USBD_LL_Transmit(pdev,
                       ep,
                       hcdc->TxBuffer[instance],
                       hcdc->TxLength[instance]);

      return USBD_OK;
    }
    else
    {
      return USBD_BUSY;
    }
  }
  else
  {
    return USBD_FAIL;
  }
}


/**
  * @brief  USBD_CDC_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev, int instance)
{      
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  int ep = CDC_OUT_EP;

  if (instance) {
	  ep = CDC2_OUT_EP;
  }
  
  /* Suspend or Resume USB Out process */
  if(pdev->pClassData != NULL)
  {
    if(pdev->dev_speed == USBD_SPEED_HIGH  ) 
    {      
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
                             ep,
                             hcdc->RxBuffer[instance],
                             CDC_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev,
                             ep,
                             hcdc->RxBuffer[instance],
                             CDC_DATA_FS_OUT_PACKET_SIZE);
    }
    return USBD_OK;
  }
  else
  {
    return USBD_FAIL;
  }
}
/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
