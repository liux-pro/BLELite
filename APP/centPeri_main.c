/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2019/11/05
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <app.h>
#include "CONFIG.h"
#include "HAL.h"
#include "peripheral.h"
#include "central.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__attribute__((section(".highcode")))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
    }
}
uint16_t cb(uint8_t task_id, uint16_t events);
/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    //所有debug都用的串口1，而ch571k上没有串口1，所以都没用了，不管他们了。

    app_init();

//    PRINT("%s\n", VER_LIB);

    //蓝牙初始化，主从一体
    CH57X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    GAPRole_CentralInit();
    Peripheral_Init();
    Central_Init();
//    {
//        //广播模式设置为 不可连接，可获取回复包。因为连接和通讯代码还没看懂，暂时不让他连接，姑且使用广播通讯。
//        uint8 initial_adv_event_type = GAP_ADTYPE_ADV_SCAN_IND;
//        GAPRole_SetParameter( GAPROLE_ADV_EVENT_TYPE, sizeof( uint8 ), &initial_adv_event_type );
//    }

    Main_Circulation();
}


/******************************** endfile @ main ******************************/
