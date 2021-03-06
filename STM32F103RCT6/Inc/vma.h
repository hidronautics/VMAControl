#ifndef VMA_H
#define VMA_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

#define DRIVER_VEL_MAX			127
typedef struct
{
	enum {
		FORWARD = 0,
		BACKWARD = 1,
		VMA = 2,
		ADD = 3,
		VMA_PWM_DELTA = 10,	// 5
		VMA_PWM_NULL_DELTA = 5
	};
	enum {
		STATE_DOWN = 0,
		STATE_NULL1 = 1,
		STATE_REG = 2,
		STATE_NULL2 = 3,
		MAX_BACKWARD_COUNTER_NULL1 = 10,//40//10//7
		MAX_FORWARD_COUNTER_NULL = 10,//30//7
		MAX_BACKWARD_COUNTER = 12//10//7
	};
	
	uint16_t current;
	uint8_t velocity;
	uint16_t PWMDesire;
	uint16_t PWMCurrent;
	uint8_t ErrorCounter;
	uint8_t BackwardCounter;
	uint8_t ForwardCounter;
	uint8_t TypeOfDriver;
	bool flagStarted;
	bool flagACPOver; 
	bool flagDirection;
	bool flagEnablePWM;
	bool flagBrake;
	uint8_t BackwardState;
} DRIVER;

void DRIVER_Init(DRIVER* driver_handle, uint8_t type);
void DRIVER_Start(DRIVER* driver_handle);
void DRIVER_Stop(DRIVER* driver_handle);

void DRIVER_SetVelocity(DRIVER* driver_handle, uint8_t setvalue);
void VMA_HallSensorShift();

void DRIVER_SetPwm(DRIVER* driver_handle);
void DRIVER_SetPwmNull(DRIVER* driver_handle);
void DRIVER_SetPwmLow(DRIVER* driver_handle);
void DRIVER_PwmEnable(DRIVER* driver_handle);
void DRIVER_PwmDisable(DRIVER* driver_handle);

#endif