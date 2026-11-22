#ifndef CONFIG_H
#define CONFIG_H

#include "GPIO_defs.h"

// Platform
// Select one compiler
// #define USING_AC5
#define USING_AC6

#define USING_RTOS

// Application Program
#define USE_ADC_SERVER 				(1)
#define USE_LCD_MUTEX_LEVEL  	(1) // Change to 2, 3 as you try out different mutex levels within the threads



// LCD and Graphics Optimizations
#define LCD_BUS_DEFAULTS_TO_DATA 1 
#define DRAW_LINE_RUNS_AS_RECTANGLES 1 
#define USE_TEXT_BITMAP_RUNS 1 

// I2C Configuration
#define READ_FULL_XYZ 1 
#define I2C_ICR_VALUE 0x20 

// SYNCING ============================================

#define SCOPE_SYNC_WITH_RTOS 0 	//this is the directive we ill need to have it
																//if 1 our code will use the RTOS, if 0 then its just state machine

//STATES ===============================================

#define WAIT 'w' //do nothing
#define BUF_BUSY 'b' //is actiuvaley taking readings
#define BUF_FULL  'f' //is full
#define BUF_DRAW  'd'//buffer is being accessed for the drawing
#define BUF_IDLE  'i'//the buffer is waiting for a trigger condition

#endif // CONFIG_H
