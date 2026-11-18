
#include "adc_lab.h"

/*
 * Lab 3 - Question 2
 * Name: REPLACE WITH YOUR NAME
 * ID:   REPLACE WITH YOUR STUDENT ID
 *
 * Bare-metal SAMD21 ADC driver.
 * Board clock: 8 MHz
 * Analog input: A3 (PA04 / AIN4)
 */

#define REG8(addr)  (*(volatile uint8_t *)(addr))
#define REG16(addr) (*(volatile uint16_t *)(addr))
#define REG32(addr) (*(volatile uint32_t *)(addr))

#define BIT(n) (1UL << (n))

// Clock and power registers
#define PM_APBCMASK     REG32(0x40000420UL)
#define GCLK_STATUS     REG8(0x40000C01UL)
#define GCLK_CLKCTRL    REG16(0x40000C02UL)

// PORT Group A registers
#define PORTA_PMUX2     REG8(0x41004432UL)
#define PORTA_PINCFG4   REG8(0x41004444UL)

// ADC registers
#define ADC_CTRLA       REG8(0x42004000UL)
#define ADC_REFCTRL     REG8(0x42004001UL)
#define ADC_AVGCTRL     REG8(0x42004002UL)
#define ADC_SAMPCTRL    REG8(0x42004003UL)
#define ADC_CTRLB       REG16(0x42004004UL)
#define ADC_SWTRIG      REG8(0x4200400CUL)
#define ADC_INPUTCTRL   REG32(0x42004010UL)
#define ADC_INTFLAG     REG8(0x42004018UL)
#define ADC_STATUS      REG8(0x42004019UL)
#define ADC_RESULT      REG16(0x4200401AUL)
#define ADC_CALIB       REG16(0x42004028UL)

// ADC bit masks
#define ADC_ENABLE      BIT(1)
#define ADC_RESREADY    BIT(0)
#define ADC_SYNCBUSY    BIT(7)

// ADC state
static bool adc_initialized = false;
static bool conversion_active = false;

// Wait for ADC synchronization to finish
static void adc_wait_sync(void)
{
    while ((ADC_STATUS & ADC_SYNCBUSY) != 0U)
    {
        // Wait for register synchronization
    }
}

// Load factory calibration values
static void adc_load_calibration(void)
{
    const uint32_t word0 = REG32(0x00806020UL);
    const uint32_t word1 = REG32(0x00806024UL);

    uint16_t linearity =
        (uint16_t)((word0 >> 27) & 0x1FU);

    linearity |=
        (uint16_t)((word1 & 0x07U) << 5);

    const uint16_t bias =
        (uint16_t)((word1 >> 3) & 0x07U);

    ADC_CALIB =
        (uint16_t)((bias << 8) | linearity);
}

// ======================================
// 1. INITIALIZE ADC
// ======================================

bool init_adc(void)
{
    // Enable ADC bus clock
    PM_APBCMASK |= BIT(16);

    // Feed ADC from GCLK generator 0
    GCLK_CLKCTRL = (uint16_t)(BIT(14) | 30U);

    while ((GCLK_STATUS & BIT(7)) != 0U)
    {
        // Wait for generic clock synchronization
    }

    // Configure PA04 for ADC peripheral B
    PORTA_PMUX2 =
        (uint8_t)((PORTA_PMUX2 & 0xF0U) | 0x01U);

    PORTA_PINCFG4 = BIT(0);

    // Disable and reset ADC
    ADC_CTRLA = 0U;
    adc_wait_sync();

    ADC_CTRLA = BIT(0);
    adc_wait_sync();

    // Load device factory calibration
    adc_load_calibration();

    // Reference = VDDANA / 2
    ADC_REFCTRL = 0x02U;

    // No hardware averaging
    ADC_AVGCTRL = 0U;

    // Sampling length
    ADC_SAMPCTRL = 5U;

    // 12-bit resolution; prescaler DIV64
    // ADC clock = 8 MHz / 64 = 125 kHz
    ADC_CTRLB = (uint16_t)(4U << 8);
    adc_wait_sync();

    // Gain = DIV2
    // Negative input = GND
    // Positive input = AIN4 (PA04 / A3)
    ADC_INPUTCTRL =
        (0xFUL << 24) |
        (0x18UL << 8) |
        4UL;

    adc_wait_sync();

    // Clear pending conversion flag
    ADC_INTFLAG = ADC_RESREADY;

    // Enable ADC
    ADC_CTRLA = ADC_ENABLE;
    adc_wait_sync();

    adc_initialized = true;
    conversion_active = false;

    return true;
}

// ======================================
// 2. START ADC CONVERSION
// ======================================

bool start_adc(void)
{
    if (!adc_initialized || conversion_active)
    {
        return false;
    }

    // Clear old result-ready flag
    ADC_INTFLAG = ADC_RESREADY;

    // Start conversion
    ADC_SWTRIG = BIT(1);
    adc_wait_sync();

    conversion_active = true;

    return true;
}

// ======================================
// 3. CHECK CONVERSION COMPLETE
// ======================================

bool is_done_adc(void)
{
    if (!adc_initialized || !conversion_active)
    {
        return false;
    }

    return (ADC_INTFLAG & ADC_RESREADY) != 0U;
}

// ======================================
// 4. READ ADC RESULT
// ======================================

bool singleread_adc(uint16_t *result)
{
    if (result == 0 || !is_done_adc())
    {
        return false;
    }

    // Read 12-bit conversion result
    *result = ADC_RESULT & 0x0FFFU;

    conversion_active = false;

    return true;
}

// ======================================
// 5. WRITE ADC CONFIGURATION
// ======================================

bool singlewrite_adc(uint8_t sample_length)
{
    if (!adc_initialized ||
        conversion_active ||
        sample_length > 63U)
    {
        return false;
    }

    // ADC configuration is write-protected
    // while enabled, so disable temporarily.
    ADC_CTRLA = 0U;
    adc_wait_sync();

    // Update sampling length
    ADC_SAMPCTRL = sample_length;

    // Re-enable ADC
    ADC_CTRLA = ADC_ENABLE;
    adc_wait_sync();

    return true;
}

// ======================================
// 6. STOP ADC
// ======================================

bool stop_adc(void)
{
    if (!adc_initialized || conversion_active)
    {
        return false;
    }

    ADC_CTRLA = 0U;
    adc_wait_sync();

    adc_initialized = false;

    return true;
}
