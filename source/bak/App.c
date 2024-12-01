/***************************************************************************//**
  @file     App.c
  @brief    Application functions
  @author   Group 4: - Oms, Mariano
                     - Solari Raigoso, Agustín
                     - Wickham, Tomás
                     - Vieira, Valentin Ulises
  @note     Based on the work of Nicolás Magliola
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <gpio.h>
#include <string.h>
#include <stdlib.h>
#include <os.h>

#include "board.h"
#include "hardware.h"
#include "macros.h"
//#include "serial.h"
//#include "timer.h"
#include "encoder.h"
#include "display.h"
#include "magcard.h"
#include "fsl.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/* Task Start */
#define TASKSTART_STK_SIZE 		512u
#define TASKSTART_PRIO 			2u
static OS_TCB TaskStartTCB;
static CPU_STK TaskStartStk[TASKSTART_STK_SIZE];

/* Task Encoder */
// #define TASKE_STK_SIZE			256u
// #define TASKE_STK_SIZE_LIMIT	(TASKE_STK_SIZE / 10u)
// #define TASKE_PRIO              3u
// static OS_TCB TaskEncoderTCB;
// static CPU_STK TaskEncoderStk[TASKE_STK_SIZE];

/* Task MagCard */
#define TASKMC_STK_SIZE			256u
#define TASKMC_STK_SIZE_LIMIT	(TASKMC_STK_SIZE / 10u)
#define TASKMC_PRIO              3u
static OS_TCB TaskMagCardTCB;
static CPU_STK TaskMagCardStk[TASKMC_STK_SIZE];

/* Task Gateway */
#define TASKG_STK_SIZE			256u
#define TASKG_STK_SIZE_LIMIT	(TASKG_STK_SIZE / 10u)
#define TASKG_PRIO              3u
static OS_TCB TaskGatewayTCB;
static CPU_STK TaskGatewayStk[TASKG_STK_SIZE];

/* Semaphores */
static OS_SEM semEncoder, semMagCard, semGateway;
static uint64_t card;

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void TaskStart(void *p_arg);
static void TASK_MagCard(void *p_arg);
static void TmrEncoder1Callback(OS_TMR *p_tmr, void *p_arg);
static void TmrEncoder2Callback(OS_TMR *p_tmr, void *p_arg);
static void TmrMagCardCallback(OS_TMR *p_tmr, void *p_arg);
static void TmrGatewayCallback(OS_TMR *p_tmr, void *p_arg);
static void TmrDisplayCallback(OS_TMR *p_tmr, void *p_arg);

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

//static ticks_t timeout_1ms, timeout_10ms, timeout_50ms, timeout_1s, timeout_2s;

//typedef enum {
//	MS_1 = TIMER_MS2TICKS(1),
//	MS_10 = TIMER_MS2TICKS(10),
//	MS_50 = TIMER_MS2TICKS(50),
//	S_1 = TIMER_MS2TICKS(1000),
//	S_2 = TIMER_MS2TICKS(2000)
//} ms_t;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/**
 * @brief Initialize the application
 * @note This function is called once at the beginning of the main program
 */
void App_Init (void)
{
	OS_ERR err;
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR cpu_err;
#endif

    OSInit(&err);
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	/* Enable task round robin. */
	OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &err);
#endif

	OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);
    OSTaskCreate(&TaskStartTCB,
                 "App Task Start",
                  TaskStart,
                  0u,
                  TASKSTART_PRIO,
                 &TaskStartStk[0u],
                 (TASKSTART_STK_SIZE / 10u),
                  TASKSTART_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &err);

	init_fsl();
	// serialInit();
	// timerInit();
}

/**
 * @brief Run the application
 * @note This function is called constantly in an infinite loop
 */
void App_Run (void)
{
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void TaskStart(void *p_arg)
{
    (void)p_arg;
    OS_ERR os_err;

    /* Initialize the uC/CPU Services. */
    CPU_Init();

#if OS_CFG_STAT_TASK_EN > 0u
    /* (optional) Compute CPU capacity with no task running */
    OSStatTaskCPUUsageInit(&os_err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    /* Create semaphores */
	OSSemCreate(&semEncoder, "Encoder Semaphore", 0, &os_err);
	OSSemCreate(&semMagCard, "MagCard Semaphore", 0, &os_err);
	OSSemCreate(&semGateway, "Gateway Semaphore", 0, &os_err);

	/* Create Encoder Timer */
	OS_TMR TmrEncoder1;
	OSTmrCreate(&TmrEncoder1,
				"Encoder1 Timer",
				 0u,
				 100u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrEncoder1Callback,
				 0x0,
				&os_err);
	OS_TMR TmrEncoder2;
	OSTmrCreate(&TmrEncoder2,
				"Encoder2 Timer",
				 0u,
				 100u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrEncoder1Callback,
				 0x0,
				&os_err);

	/* Create MagCard Timer */
	OS_TMR TmrMagCard;
	OSTmrCreate(&TmrMagCard,
				"MagCard Timer",
				 0u,
				 1000u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrMagCardCallback,
				 0x0,
				&os_err);

	/* Create Gateway Timer */
	OS_TMR TmrGateway;
	OSTmrCreate(&TmrGateway,
				"Gateway Timer",
				 0u,
				 1000u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrGatewayCallback,
				 0x0,
				&os_err);

	/* Create Display Timer */
	OS_TMR TmrDisplay;
	OSTmrCreate(&TmrDisplay,
				"Display Timer",
				 0u,
				 100u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrDisplayCallback,
				 0x0,
				&os_err);

    /* Create Encoder Task */
    // OSTaskCreate(&TaskEncoderTCB, 		//tcb
    //              "Task Encoder",			//name
    //               TaskEncoder,			//func
    //               0u,					//arg
    //               TASKE_PRIO,			//prio
    //              &TaskEncoderStk[0u],	//stack
    //               TASKE_STK_SIZE_LIMIT,	//stack limit
    //               TASKE_STK_SIZE,		//stack size
    //               0u,
    //               0u,
    //               0u,
    //              (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
    //              &os_err);

	/* Create MagCard Task */
	// OSTaskCreate(&TaskMagCardTCB,         // tcb
	// 			 "Task MagCard",          // name
	// 			  TASK_MagCard,           // func
	// 			  0u,                     // arg
	// 			  TASKMC_PRIO,            // prio
	// 			 &TaskMagCardStk[0u],     // stack
	// 			  TASKMC_STK_SIZE_LIMIT,  // stack limit
	// 			  TASKMC_STK_SIZE,        // stack size
	// 			  0u,
	// 			  0u,
	// 			  0u,
	// 			 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
	// 			 &os_err);

	/* Create Gateway Task */
	// OSTaskCreate(&TaskGatewayTCB,         // tcb
	// 			 "Task Gateway",          // name
	// 			  TASK_Gateway,           // func
	// 			  0u,                     // arg
	// 			  TASKG_PRIO,             // prio
	// 			 &TaskGatewayStk[0u],     // stack
	// 			  TASKG_STK_SIZE_LIMIT,   // stack limit
	// 			  TASKG_STK_SIZE,         // stack size
	// 			  0u,
	// 			  0u,
	// 			  0u,
	// 			 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
	// 			 &os_err);

	/* Start Timers */
	OSTmrStart(&TmrEncoder1, &os_err);
	OSTmrStart(&TmrEncoder2, &os_err);
	OSTmrStart(&TmrMagCard, &os_err);
	OSTmrStart(&TmrGateway, &os_err);
	OSTmrStart(&TmrDisplay, &os_err);

    while (1)
	{
		OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &os_err);
		update_fsl();
	}
}

static void TASK_Encoder(void *p_arg)
{
    (void)p_arg;
    OS_ERR os_err;

    while (1)
	{
		OSSemPend(&semEncoder, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
		// encoderRead();
		OSSemPost(&semMagCard, OS_OPT_POST_1, &os_err);
    }
}

static void TASK_MagCard(void *p_arg)
{
    (void)p_arg;
    OS_ERR os_err;

    while (1)
	{
		OSSemPend(&semMagCard, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
		card = MagCardGetCardNumber();
		OSSemPost(&semGateway, OS_OPT_POST_1, &os_err);
    }
}

static void TASK_Gateway(void *p_arg)
{
	(void)p_arg;
	OS_ERR os_err;

	while (1)
	{
		OSSemPend(&semGateway, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
	}
}

static void TmrEncoder1Callback(OS_TMR *p_tmr, void *p_arg)
{
	directionCallback();
}

static void TmrEncoder2Callback(OS_TMR *p_tmr, void *p_arg)
{
	switchCallback();
}

static void TmrMagCardCallback(OS_TMR *p_tmr, void *p_arg)
{
	MagCardUpdate();
}

static void TmrGatewayCallback(OS_TMR *p_tmr, void *p_arg)
{
	// gateway();
}

static void TmrDisplayCallback(OS_TMR *p_tmr, void *p_arg)
{
	RefreshDisplay();
}

/******************************************************************************/
