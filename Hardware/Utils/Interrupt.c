#include "Utils/Interrupt.h"

volatile unsigned int delay_times = 0;

void SysTick_Handler(void)
{
    if (delay_times != 0)
    {
        delay_times--;
    }
}
