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
//ble notify
void peripheralChar1Notify(uint8_t *pValue, uint16_t len);


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
// �㲥���ݣ�����Ϲ��
static uint8_t advertData[31] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
};

// GAP - SCAN RSP data (max size = 31 bytes)
// �㲥�ظ�����
static uint8_t scanRspData[31] = {
    // complete name
//    11, // length of this data
//    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
//    'S',
//    'u',
//    'p',
//    'e',
//    'r',
//    'C',
//    'l',
//    'o',
//    'c',
//    'k'
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

//���ù㲥���ݣ����õ��ǳ�����Ϣ������Զ�������
void ble_update_adv(uint8_t *p_data,uint16_t len){
    static uint8_t* p= &advertData[3];
    p[0]=len+3;
    p[1]=GAP_ADTYPE_MANUFACTURER_SPECIFIC;  //�����ǳ�����Ϣ
    p[2]=COMPANY_ID_LOW;
    p[3]=COMPANY_ID_HIGH;
    if (len!=0) {
        memcpy(&p[4],p_data,len);
    }
    GAP_UpdateAdvertisingData( 0,TRUE,len+4+3,advertData );
}

// �޸���������
void ble_update_name(uint8_t *p_name,uint16_t len){
    scanRspData[0]=len+1;
    scanRspData[1]=GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&scanRspData[2],p_name,len);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, len+2, scanRspData);

}

void app_init(){

//    tmosTaskID my_id = TMOS_ProcessEventRegister(app_task_callback);
//    tmos_start_task( my_id, HAL_TEST_EVENT, MS1_TO_SYSTEM_TIME( 100 ));


    ble_update_name("BLELite", 7);


    uart0_init();
}


/*
 * ��ʼ��ch571k�� uart0_
 */
void uart0_init()
    {
        //http://www.wch.cn/index.php/bbs/thread-84370-1.html
        //��ʼ������0_
        //����ӳ��
        GPIOPinRemap(1, RB_PIN_UART0 );
        GPIOA_SetBits(bTXD0_);
        GPIOA_ModeCfg(bRXD0_, GPIO_ModeIN_PU);      // RXD-������������
        GPIOA_ModeCfg(bTXD0_, GPIO_ModeOut_PP_5mA); // TXD-�������������ע������IO������ߵ�ƽ
        UART0_DefInit();
        //�յ�һ���ֽھʹ����ж�
        UART0_ByteTrigCfg(UART_1BYTE_TRIG);
        //���������¼�
        UART0_INTCfg(ENABLE, RB_IER_RECV_RDY);
        //�����ж�
        PFIC_EnableIRQ(UART0_IRQn);


    }



void uart0_send(uint8_t *ble_data, uint16_t ble_data_len){
    //���У��λ
    if(ble_data!=ble_send_buffer){
        memcpy(ble_send_buffer,ble_data,ble_data_len);
    }
    ble_send_buffer[ble_data_len]=calc_xor(ble_send_buffer,ble_data_len);
    //����
    UART0_SendString(ble_send_buffer, ble_data_len+1);
}

void timer0_start(){
    ///////////////��ʼ����ʱ��,reset��ʱ��Ҳ�����������
    //������ϵͳʱ�Ӷ����ó�60M,��֪��Ϊɶ,������32m�𣿣���
    //����init �������FREQ_SYS����1�룬����1000����1ms��

    TMR0_TimerInit(FREQ_SYS / 1000);         // ���ö�ʱʱ�� 1ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END); // ������ʱ�������ж�
    PFIC_EnableIRQ(TMR0_IRQn);   // �����ж�
}

void timer0_stop(){
    //��֪����ô�ض�ʱ����ֱ�Ӱ��жϹ��˾����˰�
    TMR0_ITCfg(DISABLE, TMR0_3_IT_CYC_END); // �رն�ʱ�������ж�
    PFIC_DisableIRQ(TMR0_IRQn);   // �ر��ж�
}



/*********************************************************************
 * @fn      UART0_IRQHandler
 *
 * @brief   UART0�жϺ���
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
//        case UART_II_LINE_STAT: // ��·״̬����
//        {
//            UART0_GetLinSTA();
//            break;
//        }

        case UART_II_RECV_RDY: // ���ݴﵽ���ô�����

            if(ble_receiving){    //���ڽ��գ���ǰ�Ѿ��յ���������
                //�ܵ�������ռ���Ӧ�ô���0
                //���ղ��ܳ�����������
                if(ble_receive_count>0 &&
                        ble_receive_count<BLE_RECEIVE_BUFFER){
                    ble_receive_buffer[ble_receive_count++]=UART0_RecvByte();
                    //���ö�ʱ��
                    timer0_start();
                }else{
                    //����Ӧ�ò����ߵ�����
                    //���Ƿ������ݳ���������buffer
                    ble_receiving=false;
                    ble_receive_count=0;
                    timer0_stop();
                }
            }else{       //�յ���һ������
                //������ݽ��տ�ʼ��
                ble_receiving=true;
                //������ʱ��
                timer0_start();
                //��ʼ�����ռ�����
                ble_receive_count=0;
                //�������һ�����ݵ�������
                ble_receive_buffer[ble_receive_count++]=UART0_RecvByte();
            }



            break;

//        case UART_II_RECV_TOUT: // ���ճ�ʱ����ʱһ֡���ݽ������
//            i = UART0_RecvString(buffer);
//            UART0_SendString(buffer, i);
//            break;
//
//        case UART_II_THR_EMPTY: // ���ͻ������գ��ɼ�������
//            break;
//
//        case UART_II_MODEM_CHG: // ֻ֧�ִ���0
//            break;

        default:
            break;
    }
}

//��ѭ����ѯ��ȡ���ݡ�
bool uart0_get_data(uint8_t **buffer,uint16_t *len) {
    if (ble_received) {
        //����ȡ���Զ������־
        ble_received = false;
        *buffer = ble_receive_buffer;
        //��ȥУ��λ��
        *len = ble_receive_count -1;
        return true;
    };
    return false;
}



/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0�жϺ���
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void TMR0_IRQHandler(void) // TMR0 ��ʱ�ж�
{
    if(TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END); // ����жϱ�־
        timer0_stop();   //�رն�ʱ��
        //��ǽ������
        ble_receiving = false;
        //�������У�顣
        //����ʱ��������������������ƴ���������
        //У���ʱ����������ݺ�У��ֵһ����򣬽��Ӧ����0
        if(calc_xor(ble_receive_buffer,ble_receive_count)==0){
            ble_received=true;


            uint8_t *p_data;
            uint16_t len;
//            ��ȡ���ν��յ���������
            uart0_get_data(&p_data,&len);
            /*
            * �յ����޸Ĺ㲥
            */
           if((len>=7) && (memcmp(p_data,"AT+RUOK",7)==0)){
               uart0_send("AT+OK", 5);
               return;
           }
           if((len>=7) && (memcmp(p_data,"AT+SADV",7)==0)){
               ble_update_adv(p_data+7,len-7);
               uart0_send("AT+OK", 5);
               return;
           }
           if((len>=8) && (memcmp(p_data,"AT+SNAME",8)==0)){
               ble_update_name(p_data+8, len-8);
               uart0_send("AT+OK", 5);
               return;
           }
            peripheralChar1Notify(p_data,len);
        }
    }
}
//���У��
uint8_t calc_xor(uint8_t* data,uint16_t len){
    uint8_t result = data[0];
    for(uint16_t i=1;i<len;i++){
        result^=data[i];
    }
    return result;
}
/*
 * �ӹ㲥��������ȡ��ʵ��Ҫ��������
 */
void grep_data(uint8_t *p_raw, uint16_t raw_len,
               uint8_t *p_data, uint16_t *p_len) {
    *p_len=0;
    uint16_t index = 0;
    while (index < raw_len) {
        //��ǰ���ݿ��С����һ����Ϊ����Ϊ�������Լ�
        uint8_t block_len = p_raw[index]+1;
        if ((p_raw[index + 1]) == GAP_ADTYPE_MANUFACTURER_SPECIFIC  //�㲥���ݿ�����Ϊ�������ݡ�
            && p_raw[index + 2] == COMPANY_ID_LOW    //�Լ����峧��idΪ0xBABE
            && p_raw[index + 3] == COMPANY_ID_HIGH
                ) {
            //ȥ��ǰ4���ֽڣ�������Ϣ���㲥���ͣ�����id
            *p_len = block_len - 4;
            uint8_t * p = &p_raw[index+ 4];   //
            memcpy(p_data, p, *p_len);
            break;
        }
        index = index + block_len;
    }
}
