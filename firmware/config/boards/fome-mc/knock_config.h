/**
 * @file knock_config.h
 * @brief Software knock configuration for fome-mc (STM32H743/H753 ADC3)
 */

#pragma once

// Knock is on ADC3
#define KNOCK_ADC ADCD3

// knock 1 - pin PF4 (ADC3_INP9)
#define KNOCK_ADC_CH1 9
#define KNOCK_PIN_CH1 Gpio::F4

// knock 2 - pin PF5 (ADC3_INP4)
#define KNOCK_HAS_CH2 true
#define KNOCK_ADC_CH2 4
#define KNOCK_PIN_CH2 Gpio::F5

// Sample rate & time - 250 kHz @ 4x hardware oversampling from 40 MHz ADC3 clock
#define H7_KNOCK_OVERSAMPLE 4
#define KNOCK_SAMPLE_TIME ADC_SMPR_SMP_32P5
#define KNOCK_SAMPLE_RATE (STM32_ADC3_CLOCK / (H7_KNOCK_OVERSAMPLE * (32 + 8)))

static_assert(KNOCK_SAMPLE_RATE == 250000);
static_assert(STM32_ADCCLK == 40000000);
static_assert(STM32_ADC3_CLOCK == 40000000);
