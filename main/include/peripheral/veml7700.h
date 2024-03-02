#pragma once
#ifndef _VEML7700_H_
#define _VEML7700_H_

#include "I2CMaster.h"

#ifdef __cplusplus
extern "C" {
#endif

class CVeml7700Ctrl
{
public:
    CVeml7700Ctrl();
    virtual ~CVeml7700Ctrl();
    static CVeml7700Ctrl* Instance();

public:
    bool initialize(CI2CMaster *i2c_master);
    bool release();

    // read measurement
    bool read_measurement(float *result);

    // related to configuration register
    bool power_on();
    bool shutdown();
    bool is_running(bool *running, bool read_register = false);
    bool set_enable_interrupt(bool enabled);
    bool is_interrupt_enabled(bool *enabled, bool read_register = false);
    bool set_als_persistence(uint8_t value);
    bool get_als_persistence(uint8_t *value, bool read_register = false);
    bool set_als_integration_time(uint8_t value);
    bool get_als_integration_time(uint8_t *value, bool read_register = false);
    bool set_gain(uint8_t value);
    bool get_gain(uint8_t *value, bool read_register = false);

    // related to power saving register
    bool set_enable_power_saving(bool enable);
    bool is_power_saving_enabled(bool *enable, bool read_register = false);
    bool set_power_saving_mode(uint8_t value);
    bool get_power_saving_mode(uint8_t *value, bool read_register = false);

private:
    static CVeml7700Ctrl *_instance;
    CI2CMaster *m_i2c_master;

    uint16_t m_config_reg_val;
    uint16_t m_pwr_save_reg_val;

    float convert_raw_to_lux(uint16_t raw, bool correction);

    bool read_register_common(uint8_t code, uint16_t *value);
    bool write_register_common(uint8_t code, uint16_t value);

    bool read_configure_register(uint16_t *value);
    bool write_configure_register(uint16_t value);
    bool read_power_saving_register(uint16_t *value);
    bool write_power_saving_register(uint16_t value);

    bool read_low_threshold_window_setting(uint16_t *value);
    bool write_low_threshold_window_setting(uint16_t value);
    bool read_high_threshold_window_setting(uint16_t *value);
    bool write_high_threshold_window_setting(uint16_t value);

    void wait_for_read_measurement();
    bool read_als_high_resolution_output_data(uint16_t *value, bool wait = true);
    bool read_white_channel_output_data(uint16_t *value, bool wait = true);

    bool read_device_id(uint16_t *value);
};

inline CVeml7700Ctrl* GetVeml7700Ctrl() {
    return CVeml7700Ctrl::Instance();
}

#ifdef __cplusplus
}
#endif
#endif