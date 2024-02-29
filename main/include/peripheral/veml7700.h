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

private:
    static CVeml7700Ctrl *_instance;
    CI2CMaster *m_i2c_master;
};

inline CVeml7700Ctrl* GetVeml7700Ctrl() {
    return CVeml7700Ctrl::Instance();
}

#ifdef __cplusplus
}
#endif
#endif