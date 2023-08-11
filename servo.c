/*
 * CH32V003J4M6 (SOP-8) Servo
 */

#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"

// Bit definitions for systick regs
#define SYSTICK_SR_CNTIF   (1 << 0)
#define SYSTICK_CTLR_STE   (1 << 0)
#define SYSTICK_CTLR_STIE  (1 << 1)
#define SYSTICK_CTLR_STCLK (1 << 2)
#define SYSTICK_CTLR_STRE  (1 << 3)
#define SYSTICK_CTLR_SWIE  (1 << 31)

// Servo positions
#define SERVO_MIN_US        1000
#define SERVO_MID_US        1500
#define SERVO_MAX_US        2000
#define SERVO_FULL_CYCLE_US 20000

uint32_t high_us = SERVO_MIN_US;

// Pins
uint16_t servo_pin  = GPIOv_from_PORT_PIN(GPIO_port_C, 1);
uint16_t button_pin = GPIOv_from_PORT_PIN(GPIO_port_C, 2);

void systick_init(void)
{
    SysTick->CTLR = 0;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CMP  = (FUNCONF_SYSTEM_CORE_CLOCK / 1000 * 20) - 1;
    SysTick->CNT  = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK / 1000 * 20);
    SysTick->SR = 0;

    GPIO_digitalWrite_high(servo_pin);
    Delay_Us(high_us);
    GPIO_digitalWrite_low(servo_pin);
}

int main()
{
    SystemInit();
    Delay_Ms(100);

    GPIO_port_enable(GPIO_port_C);
    GPIO_pinMode(servo_pin, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_digitalWrite_low(servo_pin);
    GPIO_pinMode(button_pin, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);

    systick_init();

    int16_t  step      = 50;
    uint32_t count     = 0;
    uint32_t max_count = 100;
    int8_t   mode      = -1;
    while (1)
    {
        if (GPIO_digitalRead(button_pin))
        {
            Delay_Ms(300);  // Debounce
            mode++;
            mode %= 8;

            switch (mode)
            {
                case 0:
                    high_us = SERVO_MAX_US;
                    break;
                case 1:
                    high_us = SERVO_MID_US;
                    break;
                case 2:
                    high_us = SERVO_MIN_US;
                    break;
                case 3:
                    step      = 1000;
                    max_count = 50;
                    break;
                case 4:
                    step      = 500;
                    max_count = 50;
                    break;
                case 5:
                    step      = 200;
                    max_count = 30;
                    break;
                case 6:
                    step      = 100;
                    max_count = 10;
                    break;
                case 7:
                    step      = 50;
                    max_count = 5;
                    break;
            }
        }

        if (mode >= 3)
        {
            if (count++ >= max_count)
            {
                count = 0;

                high_us += step;
                if (high_us >= SERVO_MAX_US)
                {
                    high_us = SERVO_MAX_US;
                    step    = -step;
                }
                else if (high_us <= SERVO_MIN_US)
                {
                    high_us = SERVO_MIN_US;
                    step    = -step;
                }
            }
        }

        Delay_Ms(10);
    }
}
