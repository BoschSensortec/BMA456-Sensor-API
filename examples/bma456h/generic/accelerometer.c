/**\
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdio.h>
#include "bma456h.h"
#include "bma4_common.h"

/******************************************************************************/
/*!                Macro definition                                           */

/*! Earth's gravity in m/s^2 */
#define GRAVITY_EARTH      (9.80665f)

/*! Macro that holds the total number of accel x,y and z axes sample counts to be printed */
#define ACCEL_SAMPLE_COUNT UINT8_C(100)

/******************************************************************************/
/*!           Static Function Declaration                                     */

/*! @brief This internal API converts raw sensor values(LSB) to meters per seconds square.
 *
 *  @param[in] val       : Raw sensor value.
 *  @param[in] g_range   : Accel Range selected (2G, 4G, 8G, 16G).
 *  @param[in] bit_width : Resolution of the sensor.
 *
 *  @return Accel values in meters per second square.
 *
 */
static float lsb_to_ms2(int16_t val, float g_range, uint8_t bit_width);

/******************************************************************************/
/*!            Functions                                                      */

/* This function starts the execution of program. */
int main(void)
{
    /* Variable to store the status of API */
    int8_t rslt;

    /* Sensor initialization configuration */
    struct bma4_dev bma = { 0 };

    /* Variable to store accel data ready interrupt status */
    uint16_t int_status = 0;

    /* Variable that holds the accelerometer sample count */
    uint8_t n_data = ACCEL_SAMPLE_COUNT;

    struct bma4_accel sens_data = { 0 };
    float x = 0, y = 0, z = 0;
    struct bma4_accel_config accel_conf = { 0 };

    /* Function to select interface between SPI and I2C, according to that the device structure gets updated */
    rslt = bma4_interface_selection(&bma);
    bma4_error_codes_print_result("bma4_interface_selection status", rslt);

    /* Sensor initialization */
    rslt = bma456h_init(&bma);
    bma4_error_codes_print_result("bma456h_init status", rslt);

    /* Upload the configuration file to enable the features of the sensor. */
    rslt = bma456h_write_config_file(&bma);
    bma4_error_codes_print_result("bma456h_write_config status", rslt);

    /* Enable the accelerometer */
    rslt = bma4_set_accel_enable(BMA4_ENABLE, &bma);
    bma4_error_codes_print_result("bma4_set_accel_enable status", rslt);

    /* Mapping data ready interrupt with interrupt pin 1 to get interrupt status once getting new accel data */
    rslt = bma456h_map_interrupt(BMA4_INTR1_MAP, BMA4_DATA_RDY_INT, BMA4_ENABLE, &bma);
    bma4_error_codes_print_result("bma456h_map_interrupt status", rslt);

    /* Accelerometer configuration settings */
    /* Output data Rate */
    accel_conf.odr = BMA4_OUTPUT_DATA_RATE_50HZ;

    /* Gravity range of the sensor (+/- 2G, 4G, 8G, 16G) */
    accel_conf.range = BMA4_ACCEL_RANGE_2G;

    /* The bandwidth parameter is used to configure the number of sensor samples that are averaged
     * if it is set to 2, then 2^(bandwidth parameter) samples
     * are averaged, resulting in 4 averaged samples
     * Note1 : For more information, refer the datasheet.
     * Note2 : A higher number of averaged samples will result in a less noisier signal, but
     * this has an adverse effect on the power consumed.
     */
    accel_conf.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /* Enable the filter performance mode where averaging of samples
     * will be done based on above set bandwidth and ODR.
     * There are two modes
     *  0 -> Averaging samples (Default)
     *  1 -> No averaging
     * For more info on No Averaging mode refer datasheet.
     */
    accel_conf.perf_mode = BMA4_CIC_AVG_MODE;

    /* Set the accel configurations */
    rslt = bma4_set_accel_config(&accel_conf, &bma);
    bma4_error_codes_print_result("bma4_set_accel_config status", rslt);

    printf("Ax[m/s2], Ay[m/s2], Az[m/s2]\r\n");

    while (1)
    {
        /* Read interrupt status */
        rslt = bma456h_read_int_status(&int_status, &bma);
        bma4_error_codes_print_result("bma456h_read_int_status", rslt);

        /* Filtering only the accel data ready interrupt */
        if ((rslt == BMA4_OK) && (int_status & BMA4_ACCEL_DATA_RDY_INT))
        {
            /* Read the accel x, y, z data */
            rslt = bma4_read_accel_xyz(&sens_data, &bma);
            bma4_error_codes_print_result("bma4_read_accel_xyz status", rslt);

            if (rslt == BMA4_OK)
            {
                /* Converting lsb to meter per second squared for 16 bit resolution at 2G range */
                x = lsb_to_ms2(sens_data.x, 2, bma.resolution);
                y = lsb_to_ms2(sens_data.y, 2, bma.resolution);
                z = lsb_to_ms2(sens_data.z, 2, bma.resolution);

                /* Print the data in m/s2 */
                printf("\n%4.2f, %4.2f, %4.2f", x, y, z);
            }

            /* Decrement the count that determines the number of samples to be printed */
            n_data--;

            /* When the count reaches 0, break and exit the loop */
            if (n_data == 0)
            {
                break;
            }
        }

        /* Pause for 20ms, 50Hz output data rate */
        bma.delay_us(BMA4_MS_TO_US(20), bma.intf_ptr);
    }

    return rslt;
}

/*!
 *  @brief This internal API converts raw sensor values(LSB) to meters per seconds square.
 */
static float lsb_to_ms2(int16_t val, float g_range, uint8_t bit_width)
{
    float half_scale = ((float)(1 << bit_width) / 2.0f);

    return (GRAVITY_EARTH * val * g_range) / half_scale;
}
