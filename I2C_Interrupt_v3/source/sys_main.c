/** @file sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
#include "i2c.h"
#include "sci.h"
#include "sys_vim.h"
#include "EPSM_SCPI_interface.h"
/* USER CODE END */

/* Include Files */

#include "sys_common.h"

/* USER CODE BEGIN (1) */
#define Own_Address 0x01
#define Slave_Address 0x48
#define I2C_Data_Count 8
#define  UART_Data_Count 2
#define Num_Of_Wheels 4

uint8_t temp[] = "SUP:LED ON";
uint8_t ledON[] = {0x53, 0x55, 0x50, 0x3a, 0x4c, 0x45, 0x44, 0x20, 0x4f, 0x4e, 0x00}; // "SUP:LED ON\0"
uint8_t ledOFF[] = {0x53, 0x55, 0x50, 0x3a, 0x4c, 0x45, 0x44, 0x20, 0x4f, 0x46, 0x46, 0x00}; // "SUP:LED OFF\0"
uint8_t telemetryRequestSAI1[16] = {0x45, 0x50, 0x53, 0x4d, 0x3a, 0x54, 0x45, 0x4c, 0x20, 0x30, 0x2c, 0x44, 0x41, 0x54, 0x41, 0x00}; // "EPSM:TEL 0,DATA\0"
uint8_t telemetryRequestBAT1[16] = {0x45, 0x50, 0x53, 0x4d, 0x3a, 0x54, 0x45, 0x4c, 0x20, 0x37, 0x2c, 0x44, 0x41, 0x54, 0x41, 0x00}; // "EPSM:TEL 0,DATA\0"
uint8_t tempRequestSAI1[17] = "EPSM:TEL 25,DATA";
uint8_t setBAT1Current[22] = "EPSM:BAT 1,CHG_I,8000";
uint8_t response[14] = {0};


uint8_t I2C_TX_Buffer[I2C_Data_Count] = {1,2,3,4,5,6,7,8};
uint8_t I2C_RX_Buffer[I2C_Data_Count] = {1,2,3,4,5,6,7,8,9,10};
uint8_t UART_TX_Buffer[UART_Data_Count] = {};
uint8_t UART_RX_Buffer[UART_Data_Count] = {};
uint16_t RPM_Values_RM[Num_Of_Wheels] = {};
uint16_t RPM_Values_MSP[Num_Of_Wheels] = {};
float Speed_Values_GPU[Num_Of_Wheels] = {};
float Speed_Values_MSP[Num_Of_Wheels] = {};

uint16_t Max_RPM = 0;
float Wheel_Dia = 20.0;

float Encoding_Factor = 32.0;
float Encoding_Diff = 4.0;



uint8_t I2C_Command_Length = 1;
uint8_t sendRPM = 5;
uint8_t requestRPM = 2;

uint32_t delay = 0;

typedef enum I2C_ModeEnum{
    RX_WAIT,
    IDLE,
} I2C_Mode;

I2C_Mode status = IDLE;


/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
//void CopyArray() {
//
//}
//
void SpeedToRPM() {
	uint8_t index = 0;

	for (index=0;index<Num_Of_Wheels;++index) {
		RPM_Values_RM[index] = (uint16_t)(Speed_Values_GPU[index]*60.0)/(Wheel_Dia*3.141593);
	}
}

void RPMToSpeed() {
	uint8_t index = 0;

	for (index=0;index<UART_Data_Count;++index) {
		Speed_Values_MSP[index] = (float)RPM_Values_MSP[index]*(Wheel_Dia*3.141593)/60.0;
	}


}

void GetSpeed() {
	uint8_t index = 0;

	for (index=0;index<UART_Data_Count;++index) {
		Speed_Values_GPU[index] = (float)UART_RX_Buffer[index]/Encoding_Factor - Encoding_Diff;
	}
}

void SendSpeed() {
	uint8_t index = 0;

	for (index=0;index<Num_Of_Wheels;++index) {
		UART_TX_Buffer[index] = (uint8_t)(Speed_Values_MSP[index]+Encoding_Diff)/(Encoding_Factor);
	}
}

void I2C_RPM_RX() {
	uint8_t index = 0;

	for (index=0;index<Num_Of_Wheels;++index) {
		RPM_Values_MSP[index] = (uint16_t)I2C_RX_Buffer[2*index] << 8 | I2C_RX_Buffer[2*index+1];
	}
}

void I2C_RPM_TX() {
	uint8_t index = 0;

	for (index=0;index<Num_Of_Wheels;++index) {
		I2C_TX_Buffer[2*index] = (uint8_t)RPM_Values_RM[index]>>8;
		I2C_TX_Buffer[2*index+1] = (uint8_t)RPM_Values_RM[index];
	}
}

void I2C_sendRPM() {

    /* Sending RPM Values */

	/* Transmitting command for MSP to storing RPM Values */

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2cREG1, Slave_Address);

    /* Set mode as Master */
    i2cSetMode(i2cREG1, I2C_MASTER);

    /* Set direction to transmitter */
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    /*Enabling Data Transmit Interrupt */
    i2cEnableNotification (i2cREG1, I2C_TX_INT);

    /* Configure Data count for command */
    i2cSetCount(i2cREG1, I2C_Command_Length);

    /* Set Stop after programmed Count */
    i2cSetStop(i2cREG1);

    /* Transmit Start Condition */
    i2cSetStart(i2cREG1);

    /* Transmit the command */
    i2cSendByte(i2cREG1, sendRPM);

    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);

}

void I2C_sendSCPI() {


    i2cSetSlaveAdd(i2cREG1, Slave_Address);
    i2cSetMode(i2cREG1, I2C_MASTER);
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    i2cSetCount(i2cREG1, 22);

    i2cSetStart(i2cREG1);

    i2cSend(i2cREG1, 22, setBAT1Current);
    i2cSetStop(i2cREG1);

    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);
    for(delay=0;delay<1000000;++delay);

}

void I2C_send_and_receiveSCPI() {


    i2cSetSlaveAdd(i2cREG1, Slave_Address);
    i2cSetMode(i2cREG1, I2C_MASTER);
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    i2cSetCount(i2cREG1, 16);

    i2cSetStart(i2cREG1);
    i2cDisableNotification(i2cREG1, I2C_TX_INT);
    i2cSend(i2cREG1, 16, telemetryRequestBAT1);
    i2cSetStop(i2cREG1);

    for(delay=0;delay<200000;++delay);
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

    i2cSetCount(i2cREG1, 14);

    i2cSetStart(i2cREG1);

    i2cReceive(i2cREG1, 14, response);
    i2cSetStop(i2cREG1);
    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);
    for(delay=0;delay<1000000;++delay);

//    i2cSetSlaveAdd(i2cREG1,Slave_Address);
//    /* Set direction to Transmitter */
//    /* Note: Optional - It is done in Init */
//    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
//    /* Configure Data count */
//    i2cSetCount(i2cREG1, 16);
//    /* Set mode as Master */
//    i2cSetMode(i2cREG1, I2C_MASTER);
//    /* Set Stop after programmed Count */
//    i2cSetStop(i2cREG1);
//    /* Transmit Start Condition */
//    i2cSetStart(i2cREG1);
//    /* Send the Register Address */
//    i2cSend(i2cREG1, 16, telemetryRequestSAI1);
//    /* Wait until Bus Busy is cleared */
//    while(i2cIsBusBusy(i2cREG1) == true);
//    /* Wait until Stop is detected */
//    while(i2cIsStopDetected(i2cREG1) == 0);
//    /* Clear the Stop condition */
//    i2cClearSCD(i2cREG1);
//    /*****************************************/
//    /*****************************************/
//    /* wait until MST bit gets cleared, this takes
//    * few cycles after Bus Busy is cleared */
//    while(i2cIsMasterReady(i2cREG1) != true);
//    /* Configure address of Slave to talk to */
//    i2cSetSlaveAdd(i2cREG1, Slave_Address);
//    /* Set direction to receiver */
//    i2cSetDirection(i2cREG1, I2C_RECEIVER);
//    /* Configure Data count */
//    /* Note: Optional - It is done in Init, unless user want to change */
//    i2cSetCount(i2cREG1, 14);
//    /* Set mode as Master */
//    i2cSetMode(i2cREG1, I2C_MASTER);
//    /* Set Stop after programmed Count */
//    i2cSetStop(i2cREG1);
//    /* Transmit Start Condition */
//    i2cSetStart(i2cREG1);
//    /* Tranmit DATA_COUNT number of data in Polling mode */
//    i2cReceive(i2cREG1, 14, response);
//    /* Wait until Bus Busy is cleared */
//    while(i2cIsBusBusy(i2cREG1) == true);
//    /* Wait until Stop is detected */
//    while(i2cIsStopDetected(i2cREG1) == 0);
//    /* Clear the Stop condition */
//    i2cClearSCD(i2cREG1);


    for(delay=0;delay<10000000;++delay);

}

void I2C_blinkSCPI() {



    while (1) {
        i2cSetSlaveAdd(i2cREG1, Slave_Address);
        i2cSetMode(i2cREG1, I2C_MASTER);
        i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

        i2cSetCount(i2cREG1, 11);

        i2cSetStart(i2cREG1);

        i2cSend(i2cREG1, 11, ledON);
        i2cSetStop(i2cREG1);


        /* Delay before the I2C bus is clear */
        for(delay=0;delay<10000000;++delay);

        i2cSetSlaveAdd(i2cREG1, Slave_Address);
        i2cSetMode(i2cREG1, I2C_MASTER);
        i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

        i2cSetCount(i2cREG1, 12);

        i2cSetStart(i2cREG1);

        i2cSend(i2cREG1, 12, ledOFF);
        i2cSetStop(i2cREG1);

        /* Delay before the I2C bus is clear */
        for(delay=0;delay<10000000;++delay);
    }
}

void I2C_getRPM() {

    /* Requesting RPM Values */

    /* Transmitting command for MSP to requesting RPM Values */

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2cREG1, Slave_Address);

    /* Set mode as Master */
    i2cSetMode(i2cREG1, I2C_MASTER);

    /* Set direction to transmitter */
    i2cSetDirection(i2cREG1, I2C_TRANSMITTER);

    /* Enabling Data Transmit Interrupt */
    i2cEnableNotification (i2cREG1, I2C_TX_INT);

    /* Configure Data count for command */
    i2cSetCount(i2cREG1, I2C_Command_Length);

    /* Set Stop after programmed Count */
    i2cSetStop(i2cREG1);

    /* Transmit Start Condition */
    i2cSetStart(i2cREG1);

    /* Transmit the command */
    i2cSendByte(i2cREG1, requestRPM);

    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);


    /* Receiving RPM Values */

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2cREG1, Slave_Address);

    /* Set mode as Master */
    i2cSetMode(i2cREG1, I2C_MASTER);

    /* Set direction to receiver */
    i2cSetDirection(i2cREG1, I2C_RECEIVER);

    /* Enabling Data Receive Interrupt */
    i2cEnableNotification (i2cREG1, I2C_RX_INT);

    /* Configure Data count for RPM Values */
    i2cSetCount(i2cREG1, I2C_Data_Count);

    /* Set Stop after programmed Count */
    i2cSetStop(i2cREG1);

    /* Transmit Start Condition */
    i2cSetStart(i2cREG1);

    /* Receive RPM Values */
    i2cReceive(i2cREG1, I2C_Data_Count, I2C_RX_Buffer);

    /* Delay before the I2C bus is clear */
    for(delay=0;delay<1000000;++delay);

}
/* USER CODE END */


int main(void)
{

	_enable_IRQ();

    sciInit();

    sciSend(scilinREG, 16, (unsigned char *)"DeMon Started!\r\n");

    i2cInit();

    i2cSetOwnAdd(i2cREG1, Own_Address);

    EPSM_set_battery_voltage_limit(1, 8000);
    EPSM_set_battery_current_limit(1, 8000);

    EPSM_converter_data_t *data;

    data = EPSM_get_converter_data(BAT1_converter);

//    I2C_getRPM();

    while(1);

}

/* USER CODE BEGIN (4) */
void i2cNotification (i2cBASE_t *i2c, uint32 flags) {

//	sciSend(scilinREG, 11, (unsigned char *)"Interrupt\r\n");

	switch (flags) {
	case I2C_AL_INT:
		sciSend(scilinREG, 6, (unsigned char *)"AL\r\n");
		break;
	case I2C_NACK_INT:
		sciSend(scilinREG, 6, (unsigned char *)"NA\r\n");
		break;
	case I2C_ARDY_INT:
		sciSend(scilinREG, 6, (unsigned char *)"AR\r\n");
		break;
	case I2C_RX_INT:
		sciSend(scilinREG, 6, (unsigned char *)"RX\r\n");
		break;
	case I2C_TX_INT:
	    i2cSetStop(i2c);
		break;
	case I2C_SCD_INT:
		sciSend(scilinREG, 6, (unsigned char *)"SC\r\n");
		break;
	case I2C_AAS_INT:
		sciSend(scilinREG, 6, (unsigned char *)"AA\r\n");
		break;
	}
}
///* USER CODE END */
