#include "Mpu6050.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#define PI 3.14159265

#define MPU_SYSFS_PATH "/sys/bus/iio/devices/iio:device0"
#define MPU_ACCEL_X_PATH MPU_SYSFS_PATH "/in_accel_x_raw"
#define MPU_ACCEL_Y_PATH MPU_SYSFS_PATH "/in_accel_y_raw"
#define MPU_ACCEL_Z_PATH MPU_SYSFS_PATH "/in_accel_z_raw"

using namespace Eigen;

namespace {

static Vector2f RotateVector2d(Vector2f input, double degrees)
{
    Vector2f result;
    double radians = degrees * PI/180;
    result[0] = input[0] * cos(radians) - input[1] * sin(radians);
    result[1] = input[0] * sin(radians) + input[1] * cos(radians);
    return result;
}
}

Mpu6050::Mpu6050() : should_stop_(false) {
    init();
}

Mpu6050::~Mpu6050() {
    should_stop_ = true;
    if (thread_->joinable())
        thread_->join();
}

Vector3i Mpu6050::getCubeAccIntersect(){
    int maxC;
    auto scaler = acceleration_.cwiseAbs().maxCoeff(&maxC);
    auto scaledAcc = acceleration_ * (1.0f/scaler);

    Vector3i accCubePos = (scaledAcc * 33).template cast<int>() + Vector3i(33,33,33);

    return accCubePos;
}

void Mpu6050::init()
{
   startRefreshThread();
}

void Mpu6050::startRefreshThread()
{
    thread_ = std::unique_ptr<std::thread> (
        new std::thread(&Mpu6050::internalLoop, this)
    );
}

void Mpu6050::internalLoop(){
    std::ifstream ifs_ax (MPU_ACCEL_X_PATH, std::ifstream::in);
    std::ifstream ifs_ay (MPU_ACCEL_Y_PATH, std::ifstream::in);
    std::ifstream ifs_az (MPU_ACCEL_Z_PATH, std::ifstream::in);

    if (!ifs_ax.good() || !ifs_ay.good() || !ifs_az.good()) {
        acceleration_ = {0.0f, 0.0f, 0.0f};
        std::cerr << "Cannot open sysfs interface for MPU6050 at: " << MPU_SYSFS_PATH << std::endl;
        return;
    }

    while (!should_stop_) {
        float ax,ay,az;

        std::string str_ax, str_ay, str_az;
        ifs_ax >> str_ax;
        ifs_ay >> str_ay;
        ifs_az >> str_az;

        ifs_ax.seekg (0, ifs_ax.beg);
        ifs_ay.seekg (0, ifs_ay.beg);
        ifs_az.seekg (0, ifs_az.beg);

        ax = std::stoi(str_ax) / 16384.0f;
        ay = std::stoi(str_ay) / 16384.0f;
        az = std::stoi(str_az) / 16384.0f;

        Vector2f temp;
        temp[0] = ay;
        temp[1] = -ax;
        auto rotated = RotateVector2d(temp, -4.9f);

        acceleration_[0] = rotated[0];
        acceleration_[1] = az;
        acceleration_[2] = rotated[1];

        std::this_thread::sleep_for (std::chrono::milliseconds(20));
    }
}

Vector3f Mpu6050::getAcceleration(){
    return acceleration_;
}
