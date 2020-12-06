/*
 * EPSM_SCPI_interface.h
 *
 *  Created on: Oct 20, 2020
 *      Author: Mason Loyet
 *      email:  mloyet@andrew.cmu.edu
 *
 *  This file describes the interface between the EPSM and
 *  the peripheral computer.
 *
 *  The functions in this file should be able to be
 *  used with the real pumpkin EPSM, but as of now are
 *  to be used with the EPSM emulator.
 */
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#ifndef INCLUDE_EPSM_SCPI_INTERFACE_H_
#define INCLUDE_EPSM_SCPI_INTERFACE_H_


typedef enum EPSM_converters{
    SAI1_converter = 0,
    SAI2_converter = 1,
    SAI3_converter = 2,
    SAI4_converter = 3,
    SAI5_converter = 4,
    SAI6_converter = 5,
    BAT1_converter = 6,
    BAT2_converter = 7,
    BUS3_3V_converter = 8,
    BUS5V_converter = 9,
    BUS12V_converter = 10 ,
    AUX_converter = 11
} EPSM_converter;

typedef enum EPSM_SAIs{
    SAI1,
    SAI2,
    SAI3,
    SAI4,
    SAI5A,
    SAI5B,
    SAI6A,
    SAI6B
} EPSM_SAI;

typedef enum EPSM_bus{
    BUS3_3V_bus,
    BUS5V_bus,
    BUS12V_bus,
    AUX_bus
} EPSM_bus;


typedef struct EPSM_converter_data {
    EPSM_converter converter;
    uint16_t voltage_mV;
    uint16_t voltage_limit_mV;
    int16_t current_mA;
    int16_t current_limit_mA;
    uint8_t rail_status;
    int32_t power_mW;
} EPSM_converter_data_t;


// ----------- REQUESTS -----------

// @brief gets various voltage, power, and current data for a converter on
//        the EPSM device
EPSM_converter_data_t *EPSM_get_converter_data(EPSM_converter converter);


// @brief gets temperature of the requested solar panel, in 0.1 K
//
// (to convert to regular Kelvin, divide by 10)
int16_t EPSM_get_temperature(EPSM_SAI SAI);


// ----------- COMMANDS -----------

// Turns on display LEDs
void EPSM_turnOnLED(void);

// Turns power on or off for given converter
void EPSM_set_bus(EPSM_bus bus, bool status);

// Turns power on or off for given battery
void EPSM_set_battery(int batNum, bool status);

// Turns power on or off for given SAI converter
void EPSM_set_SAI(int saiNum, bool status);


// @brief Sets charging voltage limit of battery batNum.
//
// batNum is 1 or 2.
// mV: voltage in mV between 6000 and 16800 mV.
void EPSM_set_battery_voltage_limit(int batNum, int mV);


// @brief Sets charging voltage and current limits respectively.
//
// batNum is 1 or 2
// mA: charging current limit in mA between 0 and 12000 mA
void EPSM_set_battery_current_limit(int batNum, int mA);


#endif /* INCLUDE_EPSM_SCPI_INTERFACE_H_ */
