/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ICM_20948.h"

#include <chrono>
#include <thread>

ICM_20948_I2C imu;

void setup() {
    // Initialize the IMU
    Wire.begin();
    imu.begin(Wire, 1);
    if (imu.status != ICM_20948_Stat_Ok) {
        fprintf(stderr, "IMU Initialization Failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the DMP
    if (imu.initializeDMP() != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::initializeDMP(...) Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::enableDMPSensor(...) Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::setDMPODRrate(...) Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.enableFIFO() != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::enableFIFO() Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.enableDMP() != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::enableDMP() Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.resetDMP() != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::resetDMP(...) Failed\n");
        exit(EXIT_FAILURE);
    }

    if (imu.resetFIFO() != ICM_20948_Stat_Ok) {
        fprintf(stderr, "ICM_20948::resetFIFO(...) Failed\n");
        exit(EXIT_FAILURE);
    }
}

void loop() {
    icm_20948_DMP_data_t data;
    imu.readDMPdataFromFIFO(&data);

    if ((imu.status == ICM_20948_Stat_Ok)
            || (imu.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
        if (data.header & DMP_header_bitmap_Quat9) {
            // Convert fixed point to floating point (divide by 2^30)
            double q1 = (double) data.Quat9.Data.Q1 / 1073741824.0;
            double q2 = (double) data.Quat9.Data.Q2 / 1073741824.0;
            double q3 = (double) data.Quat9.Data.Q3 / 1073741824.0;
            double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));

            printf("{\"orientation\": {\"w\":%.3lf, \"x\":%.3lf, \"y\":%.3lf,"
                " \"z\":%.3lf}, \"accuracy\":%u}\n",
                q0, q1, q2, q3, data.Quat9.Data.Accuracy);
        }
    } else {
        // TODO: is there a way to avoid polling the sensor?
        // DMP updates at 55 Hz (~18.2 ms), what is a sensible polling rate?
        auto delay = std::chrono::milliseconds(10);
        std::this_thread::sleep_for(delay);
    }
}
