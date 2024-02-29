#include "veml7700.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

CVeml7700Ctrl* CVeml7700Ctrl::_instance = nullptr;

CVeml7700Ctrl::CVeml7700Ctrl()
{
    m_i2c_master = nullptr;
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

    GetLogger(eLogType::Info)->Log("Initialized");
    return true;
}

bool CVeml7700Ctrl::release()
{
    return true;
}