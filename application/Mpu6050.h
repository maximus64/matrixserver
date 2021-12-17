#ifndef MATRIXSERVER_MPU6050_H
#define MATRIXSERVER_MPU6050_H

#include <memory>
#include <thread>
#include <Eigen/Dense>
#include <Eigen/StdVector>

class Mpu6050 {
public:
    Mpu6050();
    ~Mpu6050();
    void init();
    Eigen::Vector3i getCubeAccIntersect();
    Eigen::Vector3f getAcceleration();
private:
    bool should_stop_;
    void startRefreshThread();
    void internalLoop();
    std::unique_ptr<std::thread> thread_;
    Eigen::Vector3f acceleration_;
};


#endif //MATRIXSERVER_MPU6050_H
