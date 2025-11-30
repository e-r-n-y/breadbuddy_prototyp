#pragma once

#define ETH_ADC_ATTEN ADC_ATTEN_DB_12
#define ETH_ADC_CHANNEL ADC_CHANNEL_7 // ist Pin 8
#define ETH_ADC_WIDTH ADC_BITWIDTH_DEFAULT
extern const char *TAG_ETH;
extern int adc_eth_raw;
extern float adc_eth_voltage;
extern uint32_t adc_eth_ppm;