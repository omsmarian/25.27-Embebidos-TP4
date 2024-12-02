/*
 * fsl.h
 *
 *  Created on: 2 sept 2024
 *      Author: asolari
 */

#ifndef FSL_H_
#define FSL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "os.h"
//#include "fsl_debug_console.h"
#include "users.h"
#include "encoder.h"
#include "display.h"
#include "macros.h"
#include "magcard.h"
#include "LEDs.h"
//#include "timer.h"
#include "gpio.h"
#include "board.h"
#include "cqueue.h"

static bool access_flag = false;
static bool error_flag = false;

//static ticks_t timer_access;
//static ticks_t timer_error;

void read_console(void);
int read_key(void);
void update_fsl();
void update_menu();
void clear_terminal();
void init_fsl(OS_Q *msgQueue);
bool read_from_encoder(char *id);
void access_system_call(void);

void setQueue(queue_id_t q);
void setQueueSems(OS_SEM *semRx, OS_SEM *semTx);

enum states_fsl {ADD_USER, DELETE_USER, CHANGE_PASSWORD, ACCESS_SYSTEM, BRIGHTNESS};

#endif /* FSL_H_ */
