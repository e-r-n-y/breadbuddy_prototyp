#include "sensor_resistance.h"
const char *TAG_RES = "ADC-Resistance";

// FUNKTIONEN FÜR WIDERSTANDSMESSUNG
// =================================
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG_RES, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG_RES, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG_RES, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG_RES, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG_RES, "Invalid arg or no memory");
    }

    return calibrated;
}

void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG_RES, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG_RES, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

void resistance_task(void *pvParameters)
{

    // GPIO Config for sensor power control
    ESP_ERROR_CHECK(gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << RES_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    }));
    gpio_dump_io_configuration(stdout, (1ULL << RES_GPIO));
    ESP_ERROR_CHECK(gpio_set_level(RES_GPIO, 0));

    // ADC1 CONFIG
    // ==============================

    adc_oneshot_chan_cfg_t config = {
        .atten = RES_ADC_ATTEN,
        .bitwidth = RES_ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RES_ADC_CHANNEL, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan3_handle = NULL;
    bool do_calibration1_chan3 = adc_calibration_init(ADC_UNIT_1, RES_ADC_CHANNEL, RES_ADC_ATTEN, &adc1_cali_chan3_handle);

    while (1)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW(TAG_RES, "Stack remaining: %u words", stackRemaining);

            // WIDERSTANDSMESSUNG
            // ==============================
            ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)RES_GPIO, 1));

            vTaskDelay(pdMS_TO_TICKS(5000)); // wait for sensor to stabilize

            int sum = 0;
            for (int i = 0; i < 10; i++)
            {
                ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, RES_ADC_CHANNEL, &adc_res_raw));
                sum += adc_res_raw;
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            adc_res_raw = sum / 10;

            ESP_LOGI(TAG_RES, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, RES_ADC_CHANNEL, adc_res_raw);

            int vin = 3240; // Assuming a 3.3V supply voltage in mV - empirisch ermittelt (gemessen am GPIO 3.24 V)
            // adc_res_voltage = adc_res_raw * 1750 / 4095; // Convert raw to voltage in mV for 12-bit ADC
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_res_raw, &adc_res_voltage));
            adc_res_voltage = (int)(adc_res_voltage * 3300.0 / vin); // adjust for voltage divider
            ESP_LOGI(TAG_RES, "ADC%d Channel[%d] Voltage: %d mV", ADC_UNIT_1 + 1, RES_ADC_CHANNEL, adc_res_voltage);
            // Calculate the resistance of the unknown resistor
            // Using the voltage divider formula: Vout = Vin * (R2 / (R1 + R2))
            // Rearranged to find R2: R2 = R1 * (Vout / (Vin - Vout))
            // vin = 3300;
            int vout = adc_res_voltage;
            int r1 = RESISTOR_VALUE_OHMS; // Known resistor value in ohms
            float r2 = r1 * vout / (vin - vout);
            r2 = r2 + ((r1 - r2) * 0.0055); // Versuch einer Korrektur von Grenzwerten

            if (vout >= 3150) // wenn nichts dran hängt werden 3200 mV gemessen
            {
                ESP_LOGW(TAG_RES, "Measured voltage is greater than or equal to supply voltage. Check connections.");
            }
            else
            {
                if (r2 < 1000)
                {
                    ESP_LOGI(TAG_RES, "Calculated Resistance: %.2f ohms\n", r2);
                }
                else
                {
                    ESP_LOGI(TAG_RES, "Calculated Resistance: %.2f kohms\n", r2 / 1000);
                }
            }
            last_resistance = resistance;
            resistance = r2;

            update_dataset_int(&dataset_resistance, r2);

            // FIXME:
            /*
            time_t now;
            time(&now);
            duration = now - start_time;

            ESP_LOGW("DEBUG", "Resistance: %ld", resistance);
            ESP_LOGW("DEBUG", "Last Resistance: %ld", last_resistance);
            ESP_LOGW("DEBUG", "Duration: %ld", (uint32_t)duration);
            ESP_LOGW("DEBUG", "Knickpunkt erreicht: %s", knickpunkt_erreicht ? "true" : "false");

            // FIXME: erster Versuch einer zeitbasierten Vorhersage
            if (!knickpunkt_erreicht && resistance > (last_resistance + 50) && duration > 60) // normal 2000 ?
            {
                ESP_LOGW("PREDIGT", "Der Knickpunkt wurde erreicht!");
                knickpunkt_erreicht = true;
                time(&knickpunkt);
                time_ready = knickpunkt + time_remain;
            }

            if (knickpunkt_erreicht)
            {
                time_remain = time_ready - now;
            }
            */

            ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)RES_GPIO, 0));
            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay - 5000 - 2000));
        }
    }

    // Deinitialize ADC Calibration
    if (do_calibration1_chan3)
    {
        adc_calibration_deinit(adc1_cali_chan3_handle);
    }

    // Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}