#include "BattSensor.h"

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <byteswap.h>
#include <thread>

#define INA219_SYSFS_PATH "/sys/class/hwmon/hwmon1"
#define INA219_V_PATH INA219_SYSFS_PATH "/in1_input"
#define INA219_I_PATH INA219_SYSFS_PATH "/curr1_input"

struct BattSensor::Impl {
public:
    Impl() {
        init();
    }

    void init()
    {
        startRefreshThread();
    }

    float getVoltage() {
        return voltage;
    }

    float getCurrent() {
        return current;
    }

private:
    void startRefreshThread()
    {
        thread_ = std::unique_ptr<std::thread> (
            new std::thread(&Impl::internalLoop, this)
        );
    }

    void internalLoop() {
        std::string str_i, str_v;
        std::ifstream ifs_i (INA219_I_PATH, std::ifstream::in);
        std::ifstream ifs_v (INA219_V_PATH, std::ifstream::in);

        while (1 ){

            ifs_i >> str_i;
            ifs_v >> str_v;

            ifs_i.seekg (0, ifs_i.beg);
            ifs_v.seekg (0, ifs_v.beg);

            voltage = std::stoi(str_v) / 1000.0f;
            current = std::stoi(str_i) / 1000.0f;

            usleep(100000); //10fps
        }
    }

    std::unique_ptr<std::thread> thread_;

    float voltage, current;
};

BattSensor::BattSensor()
    : pimpl(new Impl())
{
}

BattSensor::~BattSensor()
{
}

float BattSensor::getVoltage() {
    return pimpl->getVoltage();
}

float BattSensor::getCurrent() {
    return pimpl->getCurrent();
}
