/***************************************************************************//**
  @file     timer.c
  @brief    Timer driver. Simple implementation, support multiple timers
  @author   Group 4: - Oms, Mariano
                     - Solari Raigoso, Agustín
                     - Wickham, Tomás
                     - Vieira, Valentin Ulises
  @note     Based on the work of Nicolás Magliola
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "timer.h"
//#include "pisr.h"
#include "pit.h"
#include <stdlib.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DEVELOPMENT_MODE		1

#define TIMER_FREQUENCY_HZ		(1000000 / TIMER_TICK_US)

#define TIMER_MAX_TIMERS		20

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Periodic service
 */
static void timer_isr(void);

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static volatile ticks_t timer_main_counter, timer_mark;

typedef struct {
	callback_t callback;
	ticks_t timeout;
} timer_t;

static timer_t timers[TIMER_MAX_TIMERS];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void timerInit(void)
{
    static bool yaInit = false;
    if (yaInit)
        return;

    // pisrRegister(timer_isr, PISR_FREQUENCY_HZ / TIMER_FREQUENCY_HZ); // init peripheral
	PIT_Init(PIT0_ID, timer_isr, TIMER_FREQUENCY_HZ);
    
    yaInit = true;
}

ticks_t timerStart(ticks_t ticks, callback_t callback)
{
    ticks_t now_copy;
    
    if (ticks < 0)
        ticks = 0; // truncate min wait time
    
    //disable_interrupts();
    now_copy = timer_main_counter; // esta copia debe ser atomic!!
    //enable_interrupts();

    now_copy += ticks;

	for (uint8_t i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (timers[i].callback == NULL)
		{
			timers[i].callback = callback;
			timers[i].timeout = now_copy;
			break;
		}
	}

    return now_copy;
}

bool timerExpired(ticks_t timeout)
{
    ticks_t now_copy;

    //disable_interrupts();
    now_copy = timer_main_counter; // esta copia debe ser atomic!!
    //enable_interrupts();

    now_copy -= timeout;
    return (now_copy >= 0);
}

void timerStop(callback_t callback)
{
	for (uint8_t i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (timers[i].callback == callback)
		{
			timers[i].callback = NULL;
			break;
		}
	}
}

void timerDelay(ticks_t ticks)
{
    ticks_t tim;
    
    tim = timerStart(ticks, NULL);
    while (!timerExpired(tim))
    {
        // wait...
    }
}

ticks_t timerCounter(void)
{
	ticks_t diff = timer_main_counter-timer_mark;
	if (diff > 2)
		timer_mark = diff; // Dummy action to be able to set a breakpoint
	timer_mark = timer_main_counter;
	return diff * TIMER_TICK_US;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void timer_isr(void)
{
    ++timer_main_counter; // update main counter

	for (uint8_t i = 0; i < TIMER_MAX_TIMERS; i++)
	{
		if (timers[i].callback != NULL && (timer_main_counter >= timers[i].timeout))
		{
			timers[i].callback();
			timers[i].callback = NULL;
		}
	}
}

/******************************************************************************/
