#include "lightsensor.h"
#include "system.h"
#include "logger.h"
#include <math.h>

CLightSensor::CLightSensor()
{
    m_matter_update_by_client_clus_illummeas_attr_measureval = false;
}


bool CLightSensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::light_sensor::config_t config_endpoint;
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::light_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }
    return CDevice::matter_init_endpoint();
}

bool CLightSensor::matter_config_attributes()
{
    return true;
}

bool CLightSensor::set_min_measured_value(uint16_t value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::IlluminanceMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get IlluminanceMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::IlluminanceMeasurement::Attributes::MinMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MinMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_uint16(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MinMeasuredValue attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CLightSensor::set_max_measured_value(uint16_t value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::IlluminanceMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get IlluminanceMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::IlluminanceMeasurement::Attributes::MaxMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MaxMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_uint16(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MaxMeasuredValue attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

void CLightSensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::IlluminanceMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_illummeas_attr_measureval) {
                m_matter_update_by_client_clus_illummeas_attr_measureval = false;
            }
        }
    }
}

void CLightSensor::matter_update_all_attribute_values()
{
    matter_update_clus_illummeas_attr_measureval();
}

void CLightSensor::update_measured_value_illuminance(uint16_t value)
{
    m_measured_value_illuminance = (uint16_t)(10000. * log10((double)value) + 1.);
    if (m_measured_value_illuminance != m_measured_value_illuminance_prev) {
        GetLogger(eLogType::Info)->Log("Update measured illuminance value as %u", m_measured_value_illuminance);
        matter_update_clus_illummeas_attr_measureval();
    }
    m_measured_value_illuminance_prev = m_measured_value_illuminance;
}

void CLightSensor::matter_update_clus_illummeas_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_uint16(m_measured_value_illuminance);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::IlluminanceMeasurement::Id,
        chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_illummeas_attr_measureval,
        force_update
    );
}