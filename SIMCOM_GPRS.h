/**
 ****************************************************************************
 * @Warning :Without permission from the author,Not for commercial use
 * @File    :
 * @Author  :Seblee
 * @date    :2018-10-08 16:58:00
 * @version : V1.0.0
 *************************************************
 * @brief :
 ****************************************************************************
 * @Last Modified by: Seblee
 * @Last Modified time: 2018-10-09 10:08:12
 ****************************************************************************
**/
#ifndef __SIMCOM_GPRS_H_
#define __SIMCOM_GPRS_H_

/* Private include -----------------------------------------------------------*/
#include "SIMCOM.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
bool SIMCOM_CCHSTART(SIMCOM_HANDLE *pHandle);

bool SIMCOM_CCH(SIMCOM_HANDLE *pHandle, const char *host, int port);

bool SIMCOM_IPADDR(SIMCOM_HANDLE *pHandle);

bool SIMCOM_CIPNETWORK(SIMCOM_HANDLE *pHandle, bool Transparent_mode, const char *host, int port);

/*----------------------------------------------------------------------------*/

#endif /* SIMCOM_GPRS*/
