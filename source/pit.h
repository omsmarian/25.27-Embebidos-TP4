/***************************************************************************//**
  @file     pit.h
  @brief    Periodic Interrupt Timer (PIT) driver for K64F
  @author   Group 4: - Oms, Mariano
                     - Solari Raigoso, Agustín
                     - Wickham, Tomás
                     - Vieira, Valentin Ulises
  @note     Based on the work of Daniel Jacoby
 ******************************************************************************/

#ifndef _PIT_H_
#define _PIT_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "hardware.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define PIT_FREQUENCY_HZ	10000U
#define PIT_HZ2TICKS(f)		((PIT_FREQUENCY_HZ) / (f))
#define PIT_MAX_CALLBACKS	20 // Per channel

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef void (*callback_t)(void);

typedef enum {
	PIT0_ID,
	PIT1_ID,
	PIT2_ID,
	PIT3_ID,

	PIT_CANT_IDS
} pit_id_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialize PIT driver and register callback
 * @param id PIT channel to be used
 * @param fun Function to be called periodically
 * @param freq Callback frequency in Hz
 * @return Initialization succeed
 * @note If the callback is NULL, the PIT channel is only initialized
 */
bool PIT_Init(pit_id_t id, callback_t fun, uint32_t freq);

/*******************************************************************************
 ******************************************************************************/

#endif // _PIT_H_
