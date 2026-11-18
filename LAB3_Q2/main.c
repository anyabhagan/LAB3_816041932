
#include <stdbool.h>
#include <stdint.h>

#include "adc_lab.h"
#include "board.h"

/*
 * Lab 3 - Question 2
 * Name: REPLACE WITH YOUR NAME
 * ID:   REPLACE WITH YOUR STUDENT ID
 *
 * Unit test for SAMD21 ADC routines.
 */

// Print an unsigned integer through UART
static void uart_print_number(uint32_t value)
{
    char digits[11];
    char *p = &digits[10];

    *p = '\0';

    do
    {
        *--p = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (value != 0U);

    board_uart_write(p);
}

// Small pause between readings
// Not used for precise timing measurement
static void test_pause(void)
{
    for (volatile uint32_t i = 0; i < 300000UL; i++)
    {
        __asm volatile ("nop");
    }
}

int main(void)
{
    uint16_t adc_result = 0U;
    uint32_t reading_count = 0U;
    uint8_t sample_length = 5U;

    // Initialize board clock, GPIO and UART
    board_init();

    board_uart_write("\r\n");
    board_uart_write("========================\r\n");
    board_uart_write("LAB 3 - ADC UNIT TEST\r\n");
    board_uart_write("Input: MKR Zero A3\r\n");
    board_uart_write("========================\r\n");

    // Initialize ADC
    if (!init_adc())
    {
        board_uart_write("ADC initialization FAILED\r\n");
        for (;;) {}
    }

    board_uart_write("ADC initialized successfully\r\n");

    for (;;)
    {
        // Change sample length every five readings
        if ((reading_count % 5U) == 0U)
        {
            sample_length =
                (sample_length == 5U) ? 10U : 5U;

            if (singlewrite_adc(sample_length))
            {
                board_uart_write("ADC configuration written: ");
                uart_print_number(sample_length);
                board_uart_write("\r\n");
            }
            else
            {
                board_uart_write("ADC configuration write FAILED\r\n");
            }
        }

        // Start an ADC conversion
        if (start_adc())
        {
            // Wait for conversion completion
            while (!is_done_adc())
            {
                // Wait for RESRDY flag
            }

            // Read ADC result
            if (singleread_adc(&adc_result))
            {
                board_uart_write("Reading ");
                uart_print_number(reading_count + 1U);

                board_uart_write(" | ADC = ");
                uart_print_number(adc_result);

                board_uart_write(" / 4095\r\n");
            }
            else
            {
                board_uart_write("ADC read FAILED\r\n");
            }
        }
        else
        {
            board_uart_write("ADC start FAILED\r\n");
        }

        reading_count++;

        // Test stop and reinitialization
        if ((reading_count % 20U) == 0U)
        {
            if (stop_adc())
            {
                board_uart_write("ADC stopped successfully\r\n");
            }

            if (init_adc())
            {
                board_uart_write("ADC restarted successfully\r\n");
            }
        }

        test_pause();
    }
}
