#pragma once
#ifndef _ILLUMINANCE_SENSOR_H_
#define _ILLUMINANCE_SENSOR_H_

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

class CIlluminanceSensor : public CDevice
{
public:
    CIlluminanceSensor();

    bool matter_init_endpoint() override;
    bool matter_config_attributes() override;
    void matter_on_change_attribute_value(
        esp_matter::attribute::callback_type_t type,
        uint32_t cluster_id,
        uint32_t attribute_id,
        esp_matter_attr_val_t *value
    ) override;
    void matter_update_all_attribute_values() override;

public:
    bool set_min_measured_value(uint16_t value); // unit: lux
    bool set_max_measured_value(uint16_t value); // unit: lux

    void update_measured_value_illuminance(uint16_t value) override; // unit: lux

private:
    bool m_matter_update_by_client_clus_illummeas_attr_measureval;

    void matter_update_clus_illummeas_attr_measureval(bool force_update = false);
};

#ifdef __cplusplus
};
#endif
#endif