#include <stdint.h>

#include "Utils/Interrupt.h"

void delay_ms(uint32_t ms)
{
    delay_times = ms;
    while (delay_times != 0);
}
