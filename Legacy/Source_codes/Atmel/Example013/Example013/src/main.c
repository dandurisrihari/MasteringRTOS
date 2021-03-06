/*
    FreeRTOS V6.0.5 - Copyright (C) 2009 Real Time Engineers Ltd.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/
/* Library includes. */
#include <stdio.h>
#include <stdlib.h>

#include <asf.h>
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/* The tasks to be created. */
static void vManagerTask( void *pvParameters );
static void vEmployeeTask( void *pvParameters );

/*-----------------------------------------------------------*/

/* Declare a variable of type xSemaphoreHandle.  This is used to reference the
semaphore that is used to synchronize bothe manager and employee task */
xSemaphoreHandle xBinarySemaphore;

/* this is the queue manager uses to put the work ticket id */
xQueueHandle xWorkQueue;

/*-----------------------------------------------------------*/
/**
 *  Configure UART console.
 */
// [main_console_configure]
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
		.charlength = CONF_UART_CHAR_LENGTH,
#endif
		.paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
		.stopbits = CONF_UART_STOP_BITS,
#endif
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

int main( void )
{
	/* This function initializes the MCU clock  */
	sysclk_init();

	/* Board initialization */
	board_init();
	
	/* Initialize the serial I/O(console ) */
	configure_console();

    /* Before a semaphore is used it must be explicitly created.  In this example a binary semaphore is created. */
    vSemaphoreCreateBinary( xBinarySemaphore );
	
	/* The queue is created to hold a maximum of 1 Element. */
    xWorkQueue = xQueueCreate( 1, sizeof( unsigned int ) );
	
	/* The tasks are going to use a pseudo random delay, seed the random number generator. */
	srand( 567 );

    /* Check the semaphore and queue was created successfully. */
    if( (xBinarySemaphore != NULL) && (xWorkQueue != NULL) )
    {

		/* Create the 'Manager' task.  This is the task that will be synchronized with the Employee task.  The Manager task is created with a high priority  */
        xTaskCreate( vManagerTask, "Manager", 240, NULL, 3, NULL );

        /* Create a employee task with less priority than manager */
        xTaskCreate( vEmployeeTask, "Employee", 240, NULL, 1, NULL );

        /* Start the scheduler so the created tasks start executing. */
        vTaskStartScheduler();
    }

    /* If all is well we will never reach here as the scheduler will now be
    running the tasks.  If we do reach here then it is likely that there was
    insufficient heap memory available for a resource to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void vManagerTask( void *pvParameters )
{
unsigned int xWorkTicketId;
portBASE_TYPE xStatus;
    /* The semaphore is created in the 'empty' state, meaning the semaphore must
	 first be given using the xSemaphoreGive() API function before it
	 can subsequently be taken (obtained) */
    xSemaphoreGive( xBinarySemaphore);

    for( ;; )
    {	
        /* get a work ticket id */
        xWorkTicketId = ( rand() & 0x1FF );
		
		/* Sends work ticket id to the work queue */
		xStatus = xQueueSend( xWorkQueue, &xWorkTicketId , portMAX_DELAY );
		
		if( xStatus != pdPASS )
		{
			printf( "Could not send to the queue.\n" );
			
		}else
		{
			/* Manager notifying the employee by "Giving" semaphore */
			xSemaphoreGive( xBinarySemaphore);
			/* after assigning the work , just yield the processor because nothing to do */
			taskYIELD();
			
		}
    }
}
/*-----------------------------------------------------------*/

void EmployeeDoWork(unsigned char TicketId)
{
	
	/* implement the work according to TickedID */
	printf ( " Employee : Working on Ticked id : %d\n",TicketId);
	
}

static void vEmployeeTask( void *pvParameters )
{
unsigned char xWorkTicketId;
portBASE_TYPE xStatus;
    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
		/* First Employee tries to take the semaphore, if it is available that means there is a task assigned by manager, otherwise employee task will be blocked */
		xSemaphoreTake( xBinarySemaphore, 0 );
		
		/*if we are here means, Semaphore take successfull. So, get the ticket id from the work queue */
		xStatus = xQueueReceive( xWorkQueue, &xWorkTicketId, 0 );

		if( xStatus == pdPASS )
		{
		  /* employe may decode the xWorkTicketId in thish function to do the work*/
			EmployeeDoWork(xWorkTicketId);
		}
		else
		{
			/* We did not receive anything from the queue.  This must be an error as this task should only run when the manager assigns at least one work. */
			printf( "Error getting the xWorkTicketId from queue\n" );
		}
    }
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* This function will only be called if an API call to create a task, queue
	or semaphore fails because there is too little heap RAM remaining. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
	/* This function will only be called if a task overflows its stack.  Note
	that stack overflow checking does slow down the context switch
	implementation. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* This example does not use the idle hook to perform any processing. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This example does not use the tick hook to perform any processing. */
}







