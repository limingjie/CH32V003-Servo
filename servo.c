/*
 * CH32V003J4M6 (SOP-8) Servo
 *
 * Reference:
 *  - https://github.com/cnlohr/ch32v003fun/blob/master/examples/systick_irq/systick_irq.c
 *
 * Aug 2023 by Li Mingjie
 *  - Email:  limingjie@outlook.com
 *  - GitHub: https://github.com/limingjie/
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
#define SERVO_MIN_PULSE_WIDTH_US     1000   //   0 degrees or min depends on servo
#define SERVO_NEUTRAL_PULSE_WIDTH_US 1500   //  90 degrees
#define SERVO_MAX_PULSE_WIDTH_US     2000   // 180 degrees or max depends on servo
#define SERVO_PULSE_PERIOD_US        20000  // 50Hz

uint32_t servo_pulse_width = SERVO_NEUTRAL_PULSE_WIDTH_US;  // Initial position = 90 degrees

// Pins
uint16_t servo_pin  = GPIOv_from_PORT_PIN(GPIO_port_C, 1);
uint16_t button_pin = GPIOv_from_PORT_PIN(GPIO_port_C, 2);

void systick_init(void)
{
    SysTick->CTLR = 0;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CMP  = (FUNCONF_SYSTEM_CORE_CLOCK / 1000 * 20) - 1;  // 20 ms
    SysTick->CNT  = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    // SysTick maintains accurate 20ms period
    SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK / 1000 * 20);  // 20 ms
    SysTick->SR = 0;

    // Send the pulse
    GPIO_digitalWrite_high(servo_pin);
    Delay_Us(servo_pulse_width);
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

    int8_t   mode        = -1;
    int16_t  step_us     = 50;
    uint32_t cycle       = 0;
    uint32_t servo_cycle = 100;
    while (1)
    {
        if (GPIO_digitalRead(button_pin))
        {
            Delay_Ms(300);  // Naive debounce :)
            mode++;
            mode &= 0x07;

            switch (mode)
            {
                case 0:  // min
                    servo_pulse_width = SERVO_MAX_PULSE_WIDTH_US;
                    break;
                case 1:  // neutral
                    servo_pulse_width = SERVO_NEUTRAL_PULSE_WIDTH_US;
                    break;
                case 2:  // max
                    servo_pulse_width = SERVO_MIN_PULSE_WIDTH_US;
                    break;
                case 3:  // min <-> max
                    step_us     = 1000;
                    servo_cycle = 50;
                    break;
                case 4:  // min <-> neutral <-> max
                    step_us     = 500;
                    servo_cycle = 50;
                    break;
                case 5:  // min <-> 5 steps <-> max
                    step_us     = 200;
                    servo_cycle = 30;
                    break;
                case 6:  // min <-> 10 steps <-> max
                    step_us     = 100;
                    servo_cycle = 10;
                    break;
                case 7:  // min <-> 20 steps <-> max
                    step_us     = 50;
                    servo_cycle = 5;
                    break;
            }
        }

        if (mode >= 3)
        {
            // Change servo position every servo_cycle (servo_cycle x 10ms)
            if (cycle++ >= servo_cycle)
            {
                cycle = 0;

                servo_pulse_width += step_us;
                if (servo_pulse_width >= SERVO_MAX_PULSE_WIDTH_US)
                {
                    servo_pulse_width = SERVO_MAX_PULSE_WIDTH_US;
                    step_us           = -step_us;
                }
                else if (servo_pulse_width <= SERVO_MIN_PULSE_WIDTH_US)
                {
                    servo_pulse_width = SERVO_MIN_PULSE_WIDTH_US;
                    step_us           = -step_us;
                }
            }
        }

        Delay_Ms(10);
    }
}
