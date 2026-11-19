#include "chuanganqi.h"

void ChuanGanQi_ChuShiHua(void)
{
    P5->SEL0 &= ~0xFF;
    P5->SEL1 &= ~0xFF;
    P5->DIR  |=  0x08;
    P5->OUT  &= ~0x08;

    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;
    P7->DIR  &= ~0xFF;
}

uint8_t ChuanGanQi_JianCe(uint8_t time)
{
    uint8_t result;

    P5->OUT  |=  0x08;
    P7->DIR  |=  0xFF;
    P7->OUT  |=  0xFF;
    delay_us(10);
    P7->DIR  &= ~0xFF;
    delay_us(time);
    result =P7->IN;
    P5->OUT  &= ~0x08;

    return result;
}
