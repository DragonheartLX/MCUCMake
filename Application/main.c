#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();

    while (true)
    {
        DL_GPIO_setPins(GPIO_USER_LED_PORT, GPIO_USER_LED_PIN_LED_PIN);
    }
}
