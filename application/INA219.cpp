#include "BattSensor.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#define INA219_SYSFS_PATH "/sys/class/hwmon/hwmon1"
#define INA219_V_PATH INA219_SYSFS_PATH "/in1_input"
#define INA219_I_PATH INA219_SYSFS_PATH "/curr1_input"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

namespace {

struct poc_volt {
    int poc;
    float volt;
};
/* percentage of charge vs. voltage curve for 4S Li-ion batt*/
static const struct poc_volt poc_curve[] = {
    {0, 12.0},
    {10, 13.6},
    {20, 14.0},
    {40, 14.28},
    {60, 14.6},
    {80, 15.0},
    {100, 16.4},
};

}

struct BattSensor::Impl {
public:
    Impl() :
        should_stop_(false),
        voltage_(0),
        current_(0),
        init_(false),
        charging_(false),
        percent_(0) {
        init();
    }

    ~Impl() {
        should_stop_ = true;
        if (thread_->joinable())
            thread_->join();
    }

    void init()
    {
        internalLoop();
        startRefreshThread();
    }

    float getVoltage() {
        return voltage_;
    }

    float getCurrent() {
        return current_;
    }

    int getPercentage() {
        return percent_;
    }

    bool isCharging() {
        return charging_;
    }

private:
    bool should_stop_;

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

        if (!ifs_i.good() || !ifs_v.good()) {
            voltage_ = 0.0f;
            current_ = 0.0f;
            std::cerr << "Cannot open INA219 sysfs at: " << INA219_SYSFS_PATH << std::endl;
            return;
        }

        if (!init_) {
            ifs_i >> str_i;
            ifs_v >> str_v;

            ifs_i.seekg (0, ifs_i.beg);
            ifs_v.seekg (0, ifs_v.beg);

            voltage_ = std::stoi(str_v) / 1000.0f;
            current_ = std::stoi(str_i) / 1000.0f;

            if (current_ < -0.5f)
                charging_ = true;

            init_ = true;
            return;
        }

        while (!should_stop_) {
            int i;

            ifs_i >> str_i;
            ifs_v >> str_v;

            ifs_i.seekg (0, ifs_i.beg);
            ifs_v.seekg (0, ifs_v.beg);

            float volt = std::stoi(str_v) / 1000.0f;
            float curr = std::stoi(str_i) / 1000.0f;

            constexpr float smooth = 0.8;

            voltage_ = voltage_ * smooth + (1.0 - smooth) * volt;
            current_ = current_ * smooth + (1.0 - smooth) * curr;

            /* schmitt trigger for charging status */
            if (current_ < -0.5f)
                charging_ = true;
            if (current_ > -0.1f)
                charging_ = false;

            /* calculate POC */
            float curr_voltage = getVoltage();

            for(i = 0; i < ARRAY_SIZE(poc_curve); i++) {
                if (curr_voltage <= poc_curve[i].volt)
                    break;
            }

            int idxa = i - 1;
            int idxb = i;
            if (idxa < 0)
                idxa = 0;

            float range = poc_curve[idxb].volt - poc_curve[idxa].volt;
            float t = (curr_voltage - poc_curve[idxa].volt) / range;
            /* clamp t value 0.0 .. 1.0 */
            if (t < 0.0)
                t = 0.0;
            if (t > 1.0)
                t = 1.0;

            percent_ = (int)(poc_curve[idxa].poc + t * (poc_curve[idxb].poc - poc_curve[idxa].poc));

            std::this_thread::sleep_for (std::chrono::milliseconds(500));
        }
    }

    std::unique_ptr<std::thread> thread_;

    float voltage_, current_;
    bool init_, charging_;
    int percent_;
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

int BattSensor::getPercentage() {
    return pimpl->getPercentage();
}

bool BattSensor::isCharging() {
    return pimpl->isCharging();
}
