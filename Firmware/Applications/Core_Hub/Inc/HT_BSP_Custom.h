/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutores S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_BSP_Custom.h
 * \brief Custom settings for iMCP HTNB32L-XXX. 
  * \author HT Micron Advanced R&D,
 *         Hêndrick Bataglin Gonçalves, Christian Roberto Lehmen,  Matheus da Silva Zorzeto, Felipe Kalinski Ferreira,
 *         Leandro Borges, Mauricio Carlotto Ribeiro, Henrique Kuhn, Cleber Haack, Eduardo Mendel
 *         Gleiser Alvarez Arrojo
 * 
 * \link https://github.com/htmicron
 * \version 0.1
 * \date February 23, 2023
 */

#ifndef  __HT_BSP_CUSTOM_H__
#define  __HT_BSP_CUSTOM_H__

#include "bsp.h"

/* Functions ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn void BSP_CustomInit(void)
 * \brief Function responsible for startup configurations, such as 
 * UART0 init, MPU init, wakeup pins and retrive platform configuration 
 * from raw flash
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void BSP_CustomInit(void);

#endif /* __HT_BSP_CUSTOM_H__ */

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
