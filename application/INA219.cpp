#include "BattSensor.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#define INA219_SYSFS_PATH "/sys/class/hwmon/hwmon1"
#define INA219_V_PATH INA219_SYSFS_PATH "/in1_input"
#define INA219_I_PATH INA219_SYSFS_PATH "/curr1_input"

struct BattSensor::Impl {
public:
    Impl() : should_stop(false) {
        init();
    }

    ~Impl() {
        should_stop = true;
        thread_->join();
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
    bool should_stop;

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

        while (!should_stop) {

            ifs_i >> str_i;
            ifs_v >> str_v;

            ifs_i.seekg (0, ifs_i.beg);
            ifs_v.seekg (0, ifs_v.beg);

            voltage = std::stoi(str_v) / 1000.0f;
            current = std::stoi(str_i) / 1000.0f;

            std::this_thread::sleep_for (std::chrono::milliseconds(200));
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
