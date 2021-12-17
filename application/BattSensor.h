#ifndef MATRIXSERVER_BATT_SENSOR_H
#define MATRIXSERVER_BATT_SENSOR_H

#include <memory>

class BattSensor {
    public:
        BattSensor();
        ~BattSensor();
        float getVoltage();
        float getCurrent();
        int getPercentage();
        bool isCharging();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl;
};

#endif /* BATT_SENSOR_H */
