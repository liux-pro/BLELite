/*
 * uart0.c
 *
 *  Created on: May 7, 2022
 *      Author: Legend
 */
#include <app.h>

#define BLE_RECEIVE_BUFFER (256+1)
#define BLE_SEND_BUFFER (256+1)
void uart0_init();


uint8_t ble_receive_buffer[BLE_RECEIVE_BUFFER]={0};
uint8_t ble_send_buffer[BLE_SEND_BUFFER]={0};

volatile bool ble_busy = false;

volatile bool ble_receiving = false;
volatile uint16_t ble_receive_count = 0;

volatile bool ble_received = false;


uint8_t * ble_get_receive_buffer(){
   return ble_receive_buffer;
}
uint8_t* ble_get_send_buffer(){
    return ble_send_buffer;
 }
// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
uint8_t advertData[31] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
};



//uint16_t app_task_callback(uint8_t task_id, uint16_t events){
//
//    if ( events & HAL_TEST_EVENT )
//    {
//        tmos_start_task( task_id, HAL_TEST_EVENT, MS1_TO_SYSTEM_TIME( 1000 ));
//        advertData[6]--;
//        GAP_UpdateAdvertisingData( 0,TRUE,7,advertData );
//        uart0_send( "a",1 );
//        return events ^ HAL_TEST_EVENT;
//     }
//    return 0;
//
//}

void ble_update_adv(uint8_t *p_data,uint16_t len){
    static uint8_t* p= &advertData[3];
    p[0]=len+3;
    p[1]=0xff;
    p[2]=COMPANY_ID_LOW;
    p[3]=COMPANY_ID_HIGH;
    memcpy(&p[4],p_data,len);
    GAP_UpdateAdvertisingData( 0,TRUE,len+4+3,advertData );
}

void app_init(){

//    tmosTaskID my_id = TMOS_ProcessEventRegister(app_task_callback);
//    tmos_start_task( my_id, HAL_TEST_EVENT, MS1_TO_SYSTEM_TIME( 100 ));



    uart0_init();
}


/*
 * 初始化ch571k的 uart0_
 */
void uart0_init()
    {
        //http://www.wch.cn/index.php/bbs/thread-84370-1.html
        //初始化串口0_
        //引脚映射
        GPIOPinRemap(1, RB_PIN_UART0 );
        GPIOA_SetBits(bTXD0_);
        GPIOA_ModeCfg(bRXD0_, GPIO_ModeIN_PU);      // RXD-配置上拉输入
        GPIOA_ModeCfg(bTXD0_, GPIO_ModeOut_PP_5mA); // TXD-配置推挽输出，注意先让IO口输出高电平
        UART0_DefInit();
        //收到一个字节就触发中断
        UART0_ByteTrigCfg(UART_1BYTE_TRIG);
        //开启接收事件
        UART0_INTCfg(ENABLE, RB_IER_RECV_RDY);
        //开启中断
        PFIC_EnableIRQ(UART0_IRQn);


    }



void uart0_send(uint8_t *ble_data, uint16_t ble_data_len){
    //添加校验位
    if(ble_data!=ble_send_buffer){
        memcpy(ble_send_buffer,ble_data,ble_data_len);
    }
    ble_send_buffer[ble_data_len]=calc_xor(ble_send_buffer,ble_data_len);
    //发送
    UART0_SendString(ble_send_buffer, ble_data_len+1);
}

void timer0_start(){
    ///////////////初始化计时器,reset计时器也用这个函数。
    //看例程系统时钟都设置成60M,不知道为啥,晶振不是32m吗？？？
    //下面init 参数设成FREQ_SYS就是1秒，除以1000就是1ms了

    TMR0_TimerInit(FREQ_SYS / 1000);         // 设置定时时间 1ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END); // 开启定时器结束中断
    PFIC_EnableIRQ(TMR0_IRQn);   // 开启中断
}

void timer0_stop(){
    //不知道怎么关定时器，直接把中断关了就行了吧
    TMR0_ITCfg(DISABLE, TMR0_3_IT_CYC_END); // 关闭定时器结束中断
    PFIC_DisableIRQ(TMR0_IRQn);   // 关闭中断
}



/*********************************************************************
 * @fn      UART0_IRQHandler
 *
 * @brief   UART0中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART0_IRQHandler(void)
{
    volatile uint8_t i;
    uint8_t buffer[20];

    switch(UART0_GetITFlag())
    {
//        case UART_II_LINE_STAT: // 线路状态错误
//        {
//            UART0_GetLinSTA();
//            break;
//        }

        case UART_II_RECV_RDY: // 数据达到设置触发点

            if(ble_receiving){    //正在接收，以前已经收到过数据了
                //能到这里接收计数应该大于0
                //接收不能超过缓冲区。
                if(ble_receive_count>0 &&
                        ble_receive_count<BLE_RECEIVE_BUFFER){
                    ble_receive_buffer[ble_receive_count++]=UART0_RecvByte();
                    //重置定时器
                    timer0_start();
                }else{
                    //正常应该不会走到这里
                    //除非发的数据超长，挤爆buffer
                    ble_receiving=false;
                    ble_receive_count=0;
                    timer0_stop();
                }
            }else{       //收到第一个数据
                //标记数据接收开始。
                ble_receiving=true;
                //开启定时器
                timer0_start();
                //初始化接收计数器
                ble_receive_count=0;
                //保存这第一个数据到缓冲区
                ble_receive_buffer[ble_receive_count++]=UART0_RecvByte();
            }



            break;

//        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
//            i = UART0_RecvString(buffer);
//            UART0_SendString(buffer, i);
//            break;
//
//        case UART_II_THR_EMPTY: // 发送缓存区空，可继续发送
//            break;
//
//        case UART_II_MODEM_CHG: // 只支持串口0
//            break;

        default:
            break;
    }
}

//主循环轮询获取数据。
bool uart0_get_data(uint8_t **buffer,uint16_t *len) {
    if (ble_received) {
        //被读取后自动清除标志
        ble_received = false;
        *buffer = ble_receive_buffer;
        //减去校验位。
        *len = ble_receive_count -1;
        return true;
    };
    return false;
}



/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void TMR0_IRQHandler(void) // TMR0 定时中断
{
    if(TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
        timer0_stop();   //关闭定时器
        //标记接收完成
        ble_receiving = false;
        //进行异或校验。
        //发的时候把所有数据异或运算结果拼在数据最后
        //校验的时候把所有数据和校验值一起异或，结果应该是0
        if(calc_xor(ble_receive_buffer,ble_receive_count)==0){
            ble_received=true;

            /*
             * 收到后发修改广播
             */
            uint8_t *p_data;
            uint16_t len;
            uart0_get_data(&p_data,&len);
            ble_update_adv(p_data,len);
        }
    }
}
//异或校验
uint8_t calc_xor(uint8_t* data,uint16_t len){
    uint8_t result = data[0];
    for(uint16_t i=1;i<len;i++){
        result^=data[i];
    }
    return result;
}
/*
 * 从广播数据中提取，实际要发的数据
 */
void grep_data(uint8_t *p_raw, uint16_t raw_len,
               uint8_t *p_data, uint16_t *p_len) {
    *p_len=0;
    uint16_t index = 0;
    while (index < raw_len) {
        //当前数据块大小，加一是因为长度为不包含自己
        uint8_t block_len = p_raw[index]+1;
        if ((p_raw[index + 1]) == GAP_ADTYPE_MANUFACTURER_SPECIFIC  //广播数据块类型为厂商数据。
            && p_raw[index + 2] == COMPANY_ID_LOW    //自己定义厂商id为0xBABE
            && p_raw[index + 3] == COMPANY_ID_HIGH
                ) {
            //去掉前4个字节：长度信息，广播类型，厂商id
            *p_len = block_len - 4;
            uint8_t * p = &p_raw[index+ 4];   //
            memcpy(p_data, p, *p_len);
            break;
        }
        index = index + block_len;
    }
}
