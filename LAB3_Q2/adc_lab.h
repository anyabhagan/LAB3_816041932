
#ifndef ADC_LAB_H
#define ADC_LAB_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Lab 3 - Question 2
 * Name: Anya Bhagan
 * ID:   816041932
 * Peripheral: ATSAMD21G18A ADC
 * Input: MKR Zero A3 / PA04 / AIN4
 */

/* Initialize ADC hardware */
bool init_adc(void);

/* Start a single ADC conversion */
bool start_adc(void);

/* Check whether conversion has finished */
bool is_done_adc(void);

/* Read the most recent conversion result */
bool singleread_adc(uint16_t *result);

/* Write the ADC sample length configuration */
bool singlewrite_adc(uint8_t sample_length);

/* Disable ADC operation */
bool stop_adc(void);

#endif
