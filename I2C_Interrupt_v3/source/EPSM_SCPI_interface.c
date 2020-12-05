/*
 * EPSM_SCPI_interface.c
 *
 *  Created on: Oct 30, 2020
 *      Author: Mason Loyet
 */
#include <string.h>
#include <stdbool.h>
#include <EPSM_SCPI_interface.h>
#include <i2c.h>
#include <stdlib.h>
#include <stdio.h>

#define Slave_Address 0x48

//char *itoa(int value, char *string, int radix);

void make_SCPI_request(uint8_t *command_buffer, size_t length, char *receive_buffer, size_t receive_length) {

    i2cSetSlaveAdd(i2cREG1, Slave_Address);
    i2cSetMode(i2cREG1, I2C_MASTER);
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    i2cSetCount(i2cREG1, length);

    i2cSetStart(i2cREG1);
    i2cDisableNotification(i2cREG1, I2C_TX_INT);
    i2cSend(i2cREG1, length, command_buffer);
    i2cSetStop(i2cREG1);

    uint32_t delay;
    for(delay=0;delay<200000;++delay);

    uint8_t devNum[1];
    devNum[0] = 0;
    while (devNum[0] == 0) {

        i2cSetSlaveAdd(i2cREG1, Slave_Address);
        i2cSetMode(i2cREG1, I2C_MASTER);
        i2cSetDirection(i2cREG1, I2C_RECEIVER);
        i2cSetCount(i2cREG1, 1);

        i2cSetStop(i2cREG1);

        i2cSetStart(i2cREG1);
        i2cDisableNotification(i2cREG1, I2C_RX_INT);
        i2cReceive(i2cREG1, 1, devNum);
        i2cSetStop(i2cREG1);

        for(delay=0;delay<100000;++delay);
    }

    i2cSetSlaveAdd(i2cREG1, Slave_Address);
    i2cSetMode(i2cREG1, I2C_MASTER);
    i2cSetDirection(i2cREG1, I2C_RECEIVER);

    i2cSetCount(i2cREG1, receive_length);

    i2cSetStart(i2cREG1);

    i2cReceive(i2cREG1, receive_length, receive_buffer);
    i2cSetStop(i2cREG1);
    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);
}

void send_SCPI_command(uint8_t *command_buffer, size_t length) {
    i2cSetSlaveAdd(i2cREG1, Slave_Address);
    i2cSetMode(i2cREG1, I2C_MASTER);
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    i2cSetCount(i2cREG1, length);

    i2cSetStart(i2cREG1);

    i2cSend(i2cREG1, length, command_buffer);
    i2cSetStop(i2cREG1);

    /* Delay before the I2C bus is clear */
    uint32_t delay;
    for(delay=0;delay<1000000;++delay);
}


// ----------- REQUESTS -----------

// @brief gets various voltage, power, and current data for a converter on
//        the EPSM device
EPSM_converter_data_t *EPSM_get_converter_data(EPSM_converter converter) {
    // construct a SCPI string
    int teleDevice = (int)converter;
    // call make_SCPI_request
    char command[25];
    sprintf(command, "EPSM:TEL %d,DATA", teleDevice);
    size_t command_length = strlen(command);

    uint8_t result_buffer[14];

    make_SCPI_request(command, command_length+1, result_buffer, 14);

    // put result into EPSM_converter_data_t
    EPSM_converter_data_t *result = malloc(sizeof(EPSM_converter_data_t));
    result->converter = converter;
    result->voltage_mV = (((uint16_t) result_buffer[0]) << 8) | ((uint16_t) result_buffer[1]);
    result->voltage_limit_mV = (((uint16_t) result_buffer[2]) << 8) | ((uint16_t) result_buffer[3]);
    result->current_mA = (((int16_t) result_buffer[4]) << 8) | ((int16_t) result_buffer[5]);
    result->current_limit_mA = (((int16_t) result_buffer[6]) << 8) | ((int16_t) result_buffer[7]);
    result->rail_status = result_buffer[8];
    result->power_mW = (((int16_t) result_buffer[9]) << 24) | (((int16_t) result_buffer[10]) << 16)
                     | (((int16_t) result_buffer[11]) << 8) | ((int16_t) result_buffer[12]);
    // return pointer
    return result;
}


// @brief gets temperature of the requested solar panel, in 0.1 K
//
// (to convert to regular Kelvin, divide by 10)
int16_t EPSM_get_temperature(EPSM_SAI SAI) {
    // construct a SCPI string
    int saiNum;
    switch(SAI){
        case(SAI1):
                saiNum = 25;
                break;
        case(SAI2):
                saiNum = 26;
                break;
        case(SAI3):
                saiNum = 27;
                break;
        case(SAI4):
                saiNum = 28;
                break;
        case(SAI5A):
                saiNum = 29;
                break;
        case(SAI5B):
                saiNum = 30;
                break;
        case(SAI6A):
                saiNum = 31;
                break;
        case(SAI6B):
                saiNum = 32 ;
                break;
    }
    char command[25];
    // call make_SCPI_request
    sprintf(command, "EPSM:TEL? %d,DATA", saiNum); // request for TEMPerature form the correct SAI
    uint8_t result_buffer[2];
    make_SCPI_request((uint8_t *)command, strlen(command)+1, result_buffer, 2);

    // put result into int16_t
    return (((int16_t) result_buffer[0]) << 8) | ((int16_t) result_buffer[1]);
    // value
}


// ----------- COMMANDS -----------

// Turns on display LEDs
void EPSM_turnOnLED(void) {
    // construct a SCPI string
    uint8_t turnOnLEDString[] = "SUP:LED ON\0";
    send_SCPI_command(turnOnLEDString, 11);
    // call send_SCPI_command
}

// Turns power on or off for given converter
void EPSM_set_bus(EPSM_bus bus, bool status) {
    char state[] = "OFF";
    if(status){
        sprintf(state, "ON");
    }
    char busStr[6];
    switch(bus){
        case(BUS3_3V_bus):
                sprintf(busStr, "3V3");
            break;
        case(BUS5V_bus):
                sprintf(busStr, "5V5");
                break;
        case(BUS12V_bus):
                sprintf(busStr, "12V");
                break;
        case(AUX_bus):
                sprintf(busStr, "AUX");
                break;
    }
    char command[25];
    sprintf(command, "EPSM:BUS %s,%s", busStr, state);
    size_t len = strlen(command);
    send_SCPI_command((uint8_t *)command, len+1); //+1 for null terminator

}


void EPSM_set_SAI_state(int saiNum, bool status) {
    char state[] = "OFF";
    if(status){
        sprintf(state, "ON");
    }
    if (saiNum < 1 || saiNum > 6){
        return ; // invalid SAI num ;
    }

    char command[13];
    sprintf(command, "EPSM:SAI %d,%s", saiNum, state);
    size_t len = strlen(command);
    send_SCPI_command((uint8_t *)command, len+1); //+1 for null terminator

}

// @brief Sets charging voltage limit of battery batNum.
//
// batNum is 1 or 2.
// mV: voltage in mV between 6000 and 16800 mV.
// 7-11 corresponding to BAT1, BAT2, 3.3V, 5V, 12V, AUX
void EPSM_set_battery_voltage_limit(int batNum, int mV) {
    // construct a SCPI string
    if(batNum > 2 || batNum < 1){
         return; // invalid batnum
     }
     if(mV > 16800){
         mV = 16800;
     }
     if (mV < 6000){
         mV = 6000;  // invalid MA
     }
     //TODO Make tighter bound
     char batNumCommand[25];
     //<batNum>,CHG_V,<mV>:
     sprintf(batNumCommand, "EPSM:BAT %d,CHG_V,%d",batNum, mV);
     size_t len = strlen(batNumCommand);
     //printf("\n");
     //printf("%d", len);
     send_SCPI_command((uint8_t *)batNumCommand, len+1); //+1 for null terminator
     // call send_SCPI_command
}


// @brief Sets charging voltage and current limits respectively.
//
// batNum is 1 or 2
// mA: charging current limit in mA between 0 and 12000 mA
// Basically the same code, sending the mA needed to the right battery
void EPSM_set_battery_current_limit(int batNum, int mA) {
    // construct a SCPI string
    if(batNum > 2 || batNum < 1){

        return; // invalid batnum
    }
    if(mA > 12000){
        mA = 12000;
    }
    if(mA < 0){
        mA = 0;
    }
    //TODO Make tighter bound
    char batNumCommand[25];
    //<batNum>,CHG_I,<mA>:
    sprintf(batNumCommand, "EPSM:BAT %d,CHG_I,%d",batNum, mA);
    printf("EPSM:BAT %d,CHG_I,%d\0",batNum, mA);
    size_t len = strlen(batNumCommand);
    //printf("\n");
    //printf("%d", len);
    send_SCPI_command((uint8_t *)batNumCommand, len+1); // +1 for null terminator
    // call send_SCPI_command
}


