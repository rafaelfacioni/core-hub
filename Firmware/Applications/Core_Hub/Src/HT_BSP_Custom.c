/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "HT_BSP_Custom.h"
#include "slpman_qcx212.h"
#include "plat_config.h"
#include "debug_log.h"
#include "unilog_qcx212.h"
#include "HT_usart_unilog.h"

void GPR_SetUartClk(void) {
    GPR_ClockDisable(GPR_UART0FuncClk);
    GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
    GPR_ClockEnable(GPR_UART0FuncClk);
    GPR_SWReset(GPR_ResetUART0Func);
}

#if LOW_POWER_AT_TEST
slpManSlpState_t CheckUsrdefSlpStatus(void) {
    slpManSlpState_t status = SLP_HIB_STATE;
    if((slpManGetWakeupPinValue() & (0x1<<1)) == 0)         // pad1 value is low
        status = SLP_IDLE_STATE;
    else
        status = SLP_HIB_STATE;

    return status;
}

#endif

void BSP_CustomInit(void) {
    extern void mpu_init(void);
    mpu_init();

    GPR_SetUartClk();

#if LOW_POWER_AT_TEST
    slpManRegisterUsrSlpDepthCb(CheckUsrdefSlpStatus);
#endif

    plat_config_raw_flash_t* rawFlashPlatConfig;

    BSP_LoadPlatConfigFromRawFlash();
    rawFlashPlatConfig = BSP_GetRawFlashPlatConfig();

    if(rawFlashPlatConfig && (rawFlashPlatConfig->logControl != 0 ))
    {
        SetUnilogUart(PORT_USART_0, rawFlashPlatConfig->uartBaudRate, false);
        uniLogInitStart(UART_0_FOR_UNILOG);
        HT_STRING(UNILOG_PLA_STRING, build_info, P_SIG, "%s", getBuildInfo());
    }

	slpManGetPMUSettings();				// get pmu setting from flash, we can overwrite after that
}

void Pad0_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup0_IRQn)==false)
        return;
}

void Pad1_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup1_IRQn)==false)
        return;
}

void Pad2_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup2_IRQn)==false)
        return;
}

void Pad3_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup3_IRQn)==false)
        return;
}

void Pad4_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup4_IRQn)==false)
        return;
}

void Pad5_WakeupIntHandler(void) {
    if(slpManExtIntPreProcess(PadWakeup5_IRQn)==false)
        return;
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
