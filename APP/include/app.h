/*
 * uart0.h
 *
 *  Created on: May 7, 2022
 *      Author: Legend
 */

#ifndef INCLUDE_APP_H_
#define INCLUDE_APP_H_
#include "stdbool.h"
#include "HAL.h"

#define COMPANY_ID_HIGH 0xBA
#define COMPANY_ID_LOW 0xBE

void app_init();
bool uart0_get_data();
void uart0_send(uint8_t *ble_data, uint16_t ble_data_len);
void grep_data(uint8_t *p_raw, uint16_t raw_len,
               uint8_t *p_data, uint16_t *p_len);
uint8_t * ble_get_receive_buffer();
uint8_t * ble_get_send_buffer();
uint8_t calc_xor(uint8_t* data,uint16_t len);

//设置广播数据，设置的是厂商信息后面的自定义数据
void ble_update_adv(uint8_t *p_data,uint16_t len);
// 修改蓝牙名字
void ble_update_name(uint8_t *p_name,uint16_t len);

#endif /* INCLUDE_APP_H_ */
