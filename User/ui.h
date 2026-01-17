#ifndef _UI_H_

#define _UI_H_

#include "u8g2.h"
void IIC_Init(void);
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                            void *arg_ptr);
void u8g2Init(u8g2_t *u8g2);
void my_draw(u8g2_t *u8g2, uint8_t code);

#endif