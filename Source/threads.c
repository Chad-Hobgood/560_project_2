#include <stdint.h>
#include <stdio.h>
#include <cmsis_os2.h>
#include <MKL25Z4.h>
#include "misc.h"

#include "LEDs.h"
#include "delay.h"

#include "LCD.h"
#include "touchscreen.h"
#include "LCD_driver.h"
#include "ST7789.h"

#include "font.h"
#include "threads.h"
#include "GPIO_defs.h"
#include "debug.h"
#include "control.h"
#include "UI.h"

#include "ADC.h"
#include "MMA8451.h"
#include "fault.h"
#include "config.h"

void Thread_Read_Touchscreen(void * arg); // 
void Thread_Draw_Waveforms(void * arg);
void Thread_Draw_UI_Controls(void * arg);

#if !SCOPE_SYNC_WITH_RTOS
extern volatile char buf_state; 
#endif

//creation of OS event flag as a global variable
#if SCOPE_SYNC_WITH_RTOS
osEventFlagsId_t volatile scope_flags = NULL;
#endif

osThreadId_t t_DW, t_DUC;
const osThreadAttr_t Draw_Waveforms_attr = {
	.priority = osPriorityAboveNormal, 
	.stack_size = 256
};
const osThreadAttr_t Update_UI_Controls_attr = {
	.priority = osPriorityNormal, 
	.stack_size = 512 
};


void Thread_Update_Setpoint(void * arg);
void Thread_Read_Accelerometer(void * arg); 

osThreadId_t t_Read_TS, t_USP, t_Read_Accelerometer;

// Basic thread priority options: osPriority[RealTime|High|AboveNormal|Normal|BelowNormal|Low|Idle]
// Each can have 1-7 appended for finer resolution

const osThreadAttr_t Read_Touchscreen_attr = {
  .priority = osPriorityNormal,
	.stack_size = 384
};

const osThreadAttr_t Update_Setpoint_attr = {
  .priority = osPriorityHigh,
	.stack_size = 128
};
const osThreadAttr_t Read_Accelerometer_attr = {
  .priority = osPriorityAboveNormal,      
	.stack_size = READ_ACCEL_STK_SZ
};

void Create_OS_Objects(void) {
	LCD_Create_OS_Objects();
	
	t_Read_TS = osThreadNew(Thread_Read_Touchscreen, NULL, &Read_Touchscreen_attr);  
	t_DW = osThreadNew(Thread_Draw_Waveforms, NULL, &Draw_Waveforms_attr);
	t_DUC = osThreadNew(Thread_Draw_UI_Controls, NULL, &Update_UI_Controls_attr);
	t_USP = osThreadNew(Thread_Update_Setpoint, NULL, &Update_Setpoint_attr);
	t_Read_Accelerometer = osThreadNew(Thread_Read_Accelerometer, NULL, &Read_Accelerometer_attr);
}




void Thread_Read_Touchscreen(void * arg) {
	PT_T p;
	uint32_t tick;
 
  tick = osKernelGetTickCount();        // retrieve the number of system ticks	
	while (1) {
		DEBUG_START(DBG_TREADTS_POS);
		if (LCD_TS_Read(&p)) { 
			UI_Process_Touch(&p);
		}
		DEBUG_STOP(DBG_TREADTS_POS);
		tick += THREAD_READ_TS_PERIOD_TICKS;
		osDelayUntil(tick);
	}
}

void Thread_Draw_Waveforms(void * arg) {
	uint32_t tick;
	
	// Initialization
	
	
	//only try to get the muxet and write to the LCD if we have something even in the init case
	
#if !SCOPE_SYNC_WITH_RTOS 
	if((buf_state == BUF_DRAW) || (buf_state == BUF_FULL)){
		osMutexAcquire(LCD_mutex, osWaitForever); // get LCD permission
		UI_Draw_Waveforms();
		osMutexRelease(LCD_mutex);		// relinquish LCD permission
	}
#endif
	
#if SCOPE_SYNC_WITH_RTOS 
	
		osMutexAcquire(LCD_mutex, osWaitForever); // get LCD permission
		UI_Draw_Waveforms();
		osMutexRelease(LCD_mutex);		// relinquish LCD permission
	
	//this needs to make the RTOS event flags
	scope_flags = osEventFlagsNew(NULL); //this makes a new event flag
	if(scope_flags == NULL){ //if it didn't make right
			osThreadExit(); //send the thread to the shadow relam
	}
	
	//if there is no error, have it set the flag to be that the interrupt can do things
	if (osEventFlagsSet(scope_flags, SCOPE_FLAG_REARM) & osFlagsError) {
        osThreadExit();
    }
	
	while (1){	
		//make a new local flag
		  uint32_t flags = osEventFlagsWait(
            scope_flags, SCOPE_FLAG_FULL, osFlagsWaitAny, osWaitForever);
				
			//if there is a error in the flag begone thread
        if (flags & osFlagsError) {
            osThreadExit();
        }
				
				//OS debug section start
        DEBUG_START(DBG_T_DRAW_WVFMS_POS);
		
				//copy and paste the conditional compile from the source we were given
				#if USE_LCD_MUTEX_LEVEL == 1
								DEBUG_START(DBG_BLOCKING_LCD_POS);
								osMutexAcquire(LCD_mutex, osWaitForever);
								DEBUG_STOP(DBG_BLOCKING_LCD_POS);
				#endif

								UI_Draw_Waveforms(); //we have the mutex and the right flag just go from there

				#if USE_LCD_MUTEX_LEVEL == 1
								osMutexRelease(LCD_mutex);
				#endif
				
				//end the debvug flag on the os
				 DEBUG_STOP(DBG_T_DRAW_WVFMS_POS);

				//pass the flag back to theinterrup[t to say run again with the flag set to zero from the macro
					if (osEventFlagsSet(scope_flags, SCOPE_FLAG_REARM) & osFlagsError) {
											osThreadExit();
									}
	
#endif	
	
		
		
	        // retrieve the number of system ticks
	
	
	#if !SCOPE_SYNC_WITH_RTOS 
		tick = osKernelGetTickCount();
		// Regular operation
	while (1) {
		tick += THREAD_DRAW_WAVEFORM_PERIOD_TICKS;
		osDelayUntil(tick);
		
		//if the buffer can even be accessed to show that things are acceptable for deawing
		
// State machine section ====================		

		if((buf_state == BUF_DRAW) || (buf_state == BUF_FULL)){

		DEBUG_START(DBG_T_DRAW_WVFMS_POS);  // Show thread's work has started
#if USE_LCD_MUTEX_LEVEL==1
			DEBUG_START(DBG_BLOCKING_LCD_POS);
			osMutexAcquire(LCD_mutex, osWaitForever); // get LCD permission
			DEBUG_STOP(DBG_BLOCKING_LCD_POS);
#endif
			
			UI_Draw_Waveforms(); // Update scope part of screen with waveforms
		
#if USE_LCD_MUTEX_LEVEL==1
			osMutexRelease(LCD_mutex);	// relinquish LCD permission
#endif
		DEBUG_STOP(DBG_T_DRAW_WVFMS_POS); // Show thread's work is done
			buf_state = BUF_IDLE; //say the thread can be done
	}
		
	#endif /// state mahcine end ===============
	
		}
}

void Thread_Draw_UI_Controls(void * arg) {
	uint32_t tick;
	
	// Initialization
	osMutexAcquire(LCD_mutex, osWaitForever); // get LCD permission
	UI_Update_Controls(1);
	osMutexRelease(LCD_mutex);		// relinquish LCD permission
  tick = osKernelGetTickCount();        // retrieve the number of system ticks

	// Regular operation
	while (1) {
		tick += THREAD_DRAW_UI_CONTROLS_PERIOD_TICKS;
		osDelayUntil(tick);
		DEBUG_START(DBG_T_DRAW_UI_CTLS_POS); // Show thread's work has started
#if USE_LCD_MUTEX_LEVEL==1
		DEBUG_START(DBG_BLOCKING_LCD_POS);
		osMutexAcquire(LCD_mutex, osWaitForever);	// get LCD permission
		DEBUG_STOP(DBG_BLOCKING_LCD_POS);
#endif

		UI_Update_Controls(0); // Update user interface part of screen

#if USE_LCD_MUTEX_LEVEL==1
		osMutexRelease(LCD_mutex);		// relinquish LCD permission
#endif
		DEBUG_STOP(DBG_T_DRAW_UI_CTLS_POS);  // Show thread's work is done
	}
}

 void Thread_Update_Setpoint(void * arg) {
	uint32_t tick;
 
  tick = osKernelGetTickCount();        // retrieve the number of system ticks
	while (1) {
		tick += THREAD_UPDATE_SETPOINT_PERIOD_TICKS;
		osDelayUntil(tick); 
		DEBUG_START(DBG_TUSP_POS);
		Update_Set_Current();
		DEBUG_STOP(DBG_TUSP_POS);
	}
 }
 
void Thread_Read_Accelerometer(void * arg) {
	uint32_t tick = osKernelGetTickCount();
	int period;

#if MMA_USE_INTERRUPTS
	mma_set_active(0);
	enable_mma_interrupt_generation(0x01);
#endif
	mma_set_active(1);
	
	while (1) {
		tick += THREAD_READ_ACCELEROMETER_PERIOD_TICKS;
		osDelayUntil(tick);
		DEBUG_START(DBG_TREADACC_POS);
		uint8_t s = read_status();
		if ((s & 0x08)) { 
			read_full_xyz();
			convert_xyz_to_roll_pitch();
			period = 90 + (int) roll;
			period = MAX(1, period);
			g_flash_period = MIN(period, 180);
			g_flash_duration = MAX(1, g_flash_period/3);
		}
		DEBUG_STOP(DBG_TREADACC_POS);
	}
}

