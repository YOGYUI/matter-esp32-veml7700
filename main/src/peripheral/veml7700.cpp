#include "veml7700.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

#define VEML7700_I2CADDR_DEFAULT    0x10    /**< I2C address */

#define VEML7700_ALS_CONFIG         0x00    /**< Light configuration register */
#define VEML7700_ALS_THREHOLD_HIGH  0x01    /**< Light high threshold for irq */
#define VEML7700_ALS_THREHOLD_LOW   0x02    /**< Light low threshold for irq */
#define VEML7700_ALS_POWER_SAVE     0x03    /**< Power save regiester */
#define VEML7700_ALS_DATA           0x04    /**< The light data output */
#define VEML7700_WHITE_DATA         0x05    /**< The white light data output */
#define VEML7700_INTERRUPTSTATUS    0x06    /**< What IRQ (if any) */
#define VEML7700_DEVICE_ID          0x07    /**< Device ID */

#define VEML7700_INTERRUPT_HIGH     0x4000  /**< Interrupt status for high threshold */
#define VEML7700_INTERRUPT_LOW      0x8000  /**< Interrupt status for low threshold */

#define VEML7700_GAIN_1             0x00    /**< ALS gain 1x */
#define VEML7700_GAIN_2             0x01    /**< ALS gain 2x */
#define VEML7700_GAIN_1_8           0x02    /**< ALS gain 1/8x */
#define VEML7700_GAIN_1_4           0x03    /**< ALS gain 1/4x */

#define VEML7700_IT_100MS           0x00    /**< ALS intetgration time 100ms */
#define VEML7700_IT_200MS           0x01    /**< ALS intetgration time 200ms */
#define VEML7700_IT_400MS           0x02    /**< ALS intetgration time 400ms */
#define VEML7700_IT_800MS           0x03    /**< ALS intetgration time 800ms */
#define VEML7700_IT_50MS            0x08    /**< ALS intetgration time 50ms */
#define VEML7700_IT_25MS            0x0C    /**< ALS intetgration time 25ms */

#define VEML7700_PERS_1             0x00    /**< ALS irq persistence 1 sample */
#define VEML7700_PERS_2             0x01    /**< ALS irq persistence 2 samples */
#define VEML7700_PERS_4             0x02    /**< ALS irq persistence 4 samples */
#define VEML7700_PERS_8             0x03    /**< ALS irq persistence 8 samples */

#define VEML7700_POWERSAVE_MODE1    0x00    /**< Power saving mode 1 */
#define VEML7700_POWERSAVE_MODE2    0x01    /**< Power saving mode 2 */
#define VEML7700_POWERSAVE_MODE3    0x02    /**< Power saving mode 3 */
#define VEML7700_POWERSAVE_MODE4    0x03    /**< Power saving mode 4 */

CVeml7700Ctrl* CVeml7700Ctrl::_instance = nullptr;

CVeml7700Ctrl::CVeml7700Ctrl()
{
    m_i2c_master = nullptr;
    m_config_reg_val = 0;
    m_pwr_save_reg_val = 0;
}

CVeml7700Ctrl::~CVeml7700Ctrl()
{
}

CVeml7700Ctrl* CVeml7700Ctrl::Instance()
{
    if (!_instance) {
        _instance = new CVeml7700Ctrl();
    }

    return _instance;
}

bool CVeml7700Ctrl::initialize(CI2CMaster *i2c_master)
{
    m_i2c_master = i2c_master;

    uint16_t dev_id_value = 0;
    if (read_device_id(&dev_id_value)) {
        GetLogger(eLogType::Info)->Log("Slave address option code: 0x%02X, Device ID Code: 0x%02X", (uint8_t)(dev_id_value >> 8), (uint8_t)(dev_id_value & 0xFF));
    } else {
        return false;
    }

    // initialize register value to member variables
    read_configure_register(&m_config_reg_val);
    read_power_saving_register(&m_pwr_save_reg_val);

    // initialize IC
    shutdown();
    set_enable_interrupt(false);
    set_als_persistence(VEML7700_PERS_1);
    set_gain(VEML7700_GAIN_1_8);
    set_als_integration_time(VEML7700_IT_100MS);
    set_power_saving_mode(false);
    power_on();

    GetLogger(eLogType::Info)->Log("Initialized");
    return true;
}

bool CVeml7700Ctrl::release()
{
    shutdown();
    return true;
}

bool CVeml7700Ctrl::read_measurement(float *result)
{
    /* Automatically adjust gain and integration time to obtain goot result */
    const uint8_t gain_word_array[] = {VEML7700_GAIN_1_8, VEML7700_GAIN_1_4, VEML7700_GAIN_1, VEML7700_GAIN_2};
    const uint8_t integ_time_word_array[] = {VEML7700_IT_25MS, VEML7700_IT_50MS, VEML7700_IT_100MS, VEML7700_IT_200MS, VEML7700_IT_400MS, VEML7700_IT_800MS};

    int gain_idx = 0;
    int integ_idx = 2;
    uint16_t als_value = 0;
    bool correction = false;

    set_gain(gain_word_array[gain_idx]);
    set_als_integration_time(integ_time_word_array[integ_idx]);
    read_als_high_resolution_output_data(&als_value);

    if (als_value <= 100) {
        while ((als_value <= 100) && !((gain_idx == 3) && (integ_idx == 5))) {
            if (gain_idx < 3) {
                set_gain(gain_word_array[++gain_idx]);
            } else if (integ_idx < 5) {
                set_als_integration_time(integ_time_word_array[++integ_idx]);
            }
            read_als_high_resolution_output_data(&als_value);
        }
    } else {
        correction = true;
        while ((als_value > 10000) && (integ_idx > 0)) {
            set_als_integration_time(integ_time_word_array[--integ_idx]);
            read_als_high_resolution_output_data(&als_value);
        }
    }

    // calculation 
    if (result)
        *result = convert_raw_to_lux(als_value, correction);

    return true;
}

static float real_integration_time(uint8_t val)
{
    float real_value;

    switch(val) {
    case VEML7700_IT_25MS:
        real_value = 25.f;
        break;
    case VEML7700_IT_50MS:
        real_value = 50.f;
        break;
    case VEML7700_IT_100MS:
        real_value = 100.f;
        break;
    case VEML7700_IT_200MS:
        real_value = 200.f;
        break;
    case VEML7700_IT_400MS:
        real_value = 400.f;
        break;
    case VEML7700_IT_800MS:
        real_value = 800.f;
        break;
    default:
        real_value = -1.f;
        break;
    }

    return real_value;
}

static float real_gain(uint8_t val)
{
    float real_value;

    switch (val) {
    case VEML7700_GAIN_1_8:
        real_value = 0.125f;
        break;
    case VEML7700_GAIN_1_4:
        real_value = 0.25f;
        break;
    case VEML7700_GAIN_1:
        real_value = 1.f;
        break;
    case VEML7700_GAIN_2:
        real_value = 2.f;
        break;
    default:
        real_value = -1.f;
        break;
    }

    return real_value;
}

float CVeml7700Ctrl::convert_raw_to_lux(uint16_t raw, bool correction)
{
    uint8_t integ_time_raw, gain_raw;
    
    get_als_integration_time(&integ_time_raw);
    get_gain(&gain_raw);
    
    float integ_time_val = real_integration_time(integ_time_raw);
    float gain_val = real_gain(gain_raw);

    float resolution = 0.0036f * (800.f / integ_time_val) * (2.f / gain_val);
    float calc = (float)raw * resolution;
    if (correction) {
        calc = (((6.0135e-13 * calc - 9.3924e-9) * calc + 8.1488e-5) * calc + 1.0023) * calc;
    }

    return calc;
}

bool CVeml7700Ctrl::power_on()
{
    m_config_reg_val &= 0xFFFE;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::shutdown()
{
    m_config_reg_val |= 0x0001;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::is_running(bool *running, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_configure_register(&m_config_reg_val))
            return false;
    }

    if (running) {
        *running = !(bool)(m_config_reg_val & 0x0001);
    }

    return true;
}

bool CVeml7700Ctrl::set_enable_interrupt(bool enable)
{
    if (enable)
        m_config_reg_val |= 0x0002;
    else
        m_config_reg_val &= 0xFFFD;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::is_interrupt_enabled(bool *enabled, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_configure_register(&m_config_reg_val))
            return false;
    }

    if (enabled) {
        *enabled = !(bool)(m_config_reg_val & 0x0002);
    }

    return true;
}

bool CVeml7700Ctrl::set_als_persistence(uint8_t value)
{
    m_config_reg_val &= 0xFFCF;
    if (value >= 4) {
        GetLogger(eLogType::Info)->Log("Exceeded value range");
        return false;
    }
    m_config_reg_val |= (uint16_t)value << 4;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::get_als_persistence(uint8_t *value, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_configure_register(&m_config_reg_val))
            return false;
    }

    if (value) {
        *value = (uint8_t)((m_config_reg_val & 0x0030) >> 4);
    }

    return true;
}

bool CVeml7700Ctrl::set_als_integration_time(uint8_t value)
{
    m_config_reg_val &= 0xFC3F;
    m_config_reg_val |= (uint16_t)value << 6;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::get_als_integration_time(uint8_t *value, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_configure_register(&m_config_reg_val))
            return false;
    }

    if (value) {
        *value = (uint8_t)((m_config_reg_val & 0x03C0) >> 6);
    }

    return true;
}

bool CVeml7700Ctrl::set_gain(uint8_t value)
{
    m_config_reg_val &= 0xE7FF;
    if (value >= 4) {
        GetLogger(eLogType::Info)->Log("Exceeded value range");
        return false;
    }
    m_config_reg_val |= (uint16_t)value << 11;
    return write_configure_register(m_config_reg_val);
}

bool CVeml7700Ctrl::get_gain(uint8_t *value, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_configure_register(&m_config_reg_val))
            return false;
    }

    if (value) {
        *value = (uint8_t)((m_config_reg_val & 0x1800) >> 11);
    }

    return true;
}

bool CVeml7700Ctrl::set_enable_power_saving(bool enable)
{
    if (enable)
        m_pwr_save_reg_val |= 0x0001;
    else
        m_pwr_save_reg_val &= 0xFFFE;
    return write_power_saving_register(m_pwr_save_reg_val);
}

bool CVeml7700Ctrl::is_power_saving_enabled(bool *enable, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_power_saving_register(&m_pwr_save_reg_val))
            return false;
    }

    if (enable) {
        *enable = (bool)(m_pwr_save_reg_val & 0x0001);
    }

    return true;
}

bool CVeml7700Ctrl::set_power_saving_mode(uint8_t value)
{
    m_pwr_save_reg_val &= 0xFFF9;
    m_pwr_save_reg_val |= (uint16_t)value << 1;
    return write_power_saving_register(m_pwr_save_reg_val);
}

bool CVeml7700Ctrl::get_power_saving_mode(uint8_t *value, bool read_register/*=false*/)
{
    if (read_register) {
        if (!read_power_saving_register(&m_pwr_save_reg_val))
            return false;
    }

    if (value) {
        *value = (uint8_t)((m_pwr_save_reg_val & 0x0006) >> 1);
    }

    return true;
}

bool CVeml7700Ctrl::read_register_common(uint8_t code, uint16_t *value)
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[1] = {code};
    uint8_t data_read[2] = {0, };
    if (!m_i2c_master->write_and_read_bytes(VEML7700_I2CADDR_DEFAULT, data_write, sizeof(data_write), data_read, sizeof(data_read)))
        return false;
    
    if (value) {
        *value = ((uint16_t)data_read[1] << 8) | (uint16_t)data_read[0];
    }

    return true;
}

bool CVeml7700Ctrl::write_register_common(uint8_t code, uint16_t value)
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[3] = {
        code,
        (uint8_t)(value & 0xFF),
        (uint8_t)(value >> 8)
    };
    if (!m_i2c_master->write_bytes(VEML7700_I2CADDR_DEFAULT, data_write, sizeof(data_write)))
        return false;
    
    return true;
}

bool CVeml7700Ctrl::read_configure_register(uint16_t *value)
{
    return read_register_common(VEML7700_ALS_CONFIG, value);
}

bool CVeml7700Ctrl::write_configure_register(uint16_t value)
{
    return write_register_common(VEML7700_ALS_CONFIG, value);
}

bool CVeml7700Ctrl::read_power_saving_register(uint16_t *value)
{
    return read_register_common(VEML7700_ALS_POWER_SAVE, value);
}

bool CVeml7700Ctrl::write_power_saving_register(uint16_t value)
{
    return write_register_common(VEML7700_ALS_POWER_SAVE, value);
}

bool CVeml7700Ctrl::read_low_threshold_window_setting(uint16_t *value)
{
    return read_register_common(VEML7700_ALS_THREHOLD_LOW, value);
}

bool CVeml7700Ctrl::write_low_threshold_window_setting(uint16_t value)
{
    return write_register_common(VEML7700_ALS_THREHOLD_LOW, value);
}

bool CVeml7700Ctrl::read_high_threshold_window_setting(uint16_t *value)
{
    return read_register_common(VEML7700_ALS_THREHOLD_HIGH, value);
}

bool CVeml7700Ctrl::write_high_threshold_window_setting(uint16_t value)
{
    return write_register_common(VEML7700_ALS_THREHOLD_HIGH, value);
}

void CVeml7700Ctrl::wait_for_read_measurement()
{
    uint8_t integ_time_raw = 0;
    get_als_integration_time(&integ_time_raw);
    float integ_time_val = real_integration_time(integ_time_raw);
    if (integ_time_val > 0)
        vTaskDelay((uint32_t)(integ_time_val * 2) / portTICK_PERIOD_MS);
}

bool CVeml7700Ctrl::read_als_high_resolution_output_data(uint16_t *value, bool wait/*=true*/)
{
    if (wait)
        wait_for_read_measurement();
    return !read_register_common(VEML7700_ALS_DATA, value);
}

bool CVeml7700Ctrl::read_white_channel_output_data(uint16_t *value, bool wait/*=true*/)
{
    if (wait)
        wait_for_read_measurement();
    return read_register_common(VEML7700_WHITE_DATA, value);
}

bool CVeml7700Ctrl::read_device_id(uint16_t *value)
{
    return read_register_common(VEML7700_DEVICE_ID, value);
}