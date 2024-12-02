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

#include <stdlib.h>
#include <string.h>
#include <uart.h>

#include "board.h"
#include "cqueue.h"
#include "display.h"
#include "encoder.h"
#include "fsl.h"
#include "gpio.h"
#include "hardware.h"
#include "macros.h"
#include "magcard.h"
#include "os.h"
#include "pit.h"
#include "serial.h"
#include "timer.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/* Task Start */
#define TASKSTART_STK_SIZE 		512u
#define TASKSTART_PRIO 			3u

/* Task Gateway */
#define TASKG_STK_SIZE			256u
#define TASKG_STK_SIZE_LIMIT	(TASKG_STK_SIZE / 10u)
#define TASKG_PRIO              3u

/* Task Keep Alive */
#define TASKKA_STK_SIZE			256u
#define TASKKA_STK_SIZE_LIMIT	(TASKKA_STK_SIZE / 10u)
#define TASKKA_PRIO              3u


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void TaskStart(void *p_arg);
static void TaskGateway(void *p_arg);
static void TaskKeepAlive(void *p_arg);

static void TmrEncoder1Callback(void);
static void TmrEncoder2Callback(void);
static void TmrDisplayCallback(void);
static void TmrUARTCallback(void);
static void TmrBlinkyCallback(void);

static void Timeout15sCallback(void);
static void TimeoutKeepAliveCallback(void);

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/* Task Start */
static OS_TCB TaskStartTCB;
static CPU_STK TaskStartStk[TASKSTART_STK_SIZE];

/* Task Gateway */
static OS_TCB TaskGatewayTCB;
static CPU_STK TaskGatewayStk[TASKG_STK_SIZE];

/* Task Keep Alive */
static OS_TCB TaskKeepAliveTCB;
static CPU_STK TaskKeepAliveStk[TASKKA_STK_SIZE];

/* Semaphores */
static OS_SEM semGatewayTx, semGatewayRx, semSerialTx, semSerialRx;
static OS_SEM semTimeout15s, semKeepAlive, connected;

/* Multiple Pend */
static OS_PEND_DATA pend_data_table[2];
static OS_PEND_DATA pend_data_table2[2];

/* Message Queue */
static OS_Q msgQueue;
static queue_id_t queue;

/* Mutex */
static OS_MUTEX gateway_mutex;

/* Timers */
static ticks_t timeout_15s, timeout_keep_alive;

/* Flags */
static OS_FLAG_GRP flags;

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

	PIT_Init(PIT0_ID, TmrBlinkyCallback, 10);
	PIT_Init(PIT0_ID, TmrEncoder1Callback, 200);
	PIT_Init(PIT0_ID, TmrEncoder2Callback, 20);
	PIT_Init(PIT0_ID, TmrDisplayCallback, 7500);
	PIT_Init(PIT0_ID, TmrUARTCallback, 200);

	serialInit();

	/* Create Message Queue */
	OSQCreate(&msgQueue, "Message Queue", 10, &err);
	queue = queueInit();
	setQueue(queue);

	init_fsl(&msgQueue);

	OSSemCreate(&semGatewayRx, "Gateway Semaphore", 0, &err);
	OSSemCreate(&semGatewayTx, "Gateway Tx Semaphore", 0, &err);
	OSSemCreate(&semSerialTx, "Serial Tx Semaphore", 0, &err);
	OSSemCreate(&semSerialRx, "Serial Rx Semaphore", 0, &err);
	OSSemCreate(&semTimeout15s, "Timeout 15s Semaphore", 0, &err);
	OSSemCreate(&semKeepAlive, "Keep Alive Semaphore", 0, &err);

	setQueueSems(&semGatewayRx, &semGatewayTx);
	uartSetSem(&semSerialRx, &semSerialTx);

	OSMutexCreate(&gateway_mutex, "Gateway Mutex", &err);

	OSFlagCreate(&flags, "Flags", (OS_FLAGS)0, &err);
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

	/* Create Gateway Task */
	OSTaskCreate(&TaskGatewayTCB,         // tcb
				 "Task Gateway",          // name
				  TaskGateway,            // func
				  0u,                     // arg
				  TASKG_PRIO,             // prio
				 &TaskGatewayStk[0u],     // stack
				  TASKG_STK_SIZE_LIMIT,   // stack limit
				  TASKG_STK_SIZE,         // stack size
				  0u,
				  0u,
				  0u,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	/* Create Keep Alive Task */
	OSTaskCreate(&TaskKeepAliveTCB,       // tcb
				 "Task Keep Alive",       // name
				  TaskKeepAlive,          // func
				  0u,                     // arg
				  TASKKA_PRIO,            // prio
				 &TaskKeepAliveStk[0u],   // stack
				  TASKKA_STK_SIZE_LIMIT,  // stack limit
				  TASKKA_STK_SIZE,        // stack size
				  0u,
				  0u,
				  0u,
				 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 &os_err);

	gpioMode(PIN_LED_RED, OUTPUT);
	gpioMode(PIN_LED_GREEN, OUTPUT);
	gpioMode(PIN_LED_BLUE, OUTPUT);
	gpioWrite(PIN_LED_RED, HIGH);
	gpioWrite(PIN_LED_GREEN, HIGH);
	gpioWrite(PIN_LED_BLUE, HIGH);

	pend_data_table[0].PendObjPtr = (OS_PEND_OBJ*)&semSerialRx;
	pend_data_table[1].PendObjPtr = (OS_PEND_OBJ*)&semTimeout15s;
	pend_data_table2[0].PendObjPtr = (OS_PEND_OBJ*)&semSerialRx;
	pend_data_table2[1].PendObjPtr = (OS_PEND_OBJ*)&semKeepAlive;

	timeout_15s = timerStart(TIMER_MS2TICKS(15000), Timeout15sCallback);
	timeout_keep_alive = timerStart(TIMER_MS2TICKS(1000), TimeoutKeepAliveCallback);

    while (1) { update_fsl(); }
}

static void TaskGateway(void *p_arg)
{
	(void)p_arg;
	OS_ERR os_err;
	void *msg;
	uchar_t _msg[12];
	OS_MSG_SIZE msg_size;
	CPU_TS ts;
	static bool msg_sent, msg_received;
	uchar_t msg_ok[]	= { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0x81 };
	uchar_t msg_fail[]	= { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0xC1 };

	while (1)
	{
		OSSemPend(&semTimeout15s, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err); // Wait for timeout between messages
		OSSemPend(&semGatewayTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err); // Wait for message to send
		msg_received = msg_sent = false;

		uint8_t i = queueSize(queue);
		if (i <= 12)
			for (uint8_t j=i; j>0; j--)
				_msg[i-j] = queuePop(queue);
		else
			msg_received = true; // Not really (just skips)

		while (!msg_received)
		{
			OSFlagPend(&connected, (OS_FLAGS)1, 0, OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME, &ts, &os_err); // Wait for connection
			OSMutexPend(&gateway_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);

			if (!msg_sent)
			{
				OSSemPend(&semSerialTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err); // Wait for tx buffer to be free
				serialWriteData(_msg, i); // SendData
				timeout_15s = timerStart(TIMER_MS2TICKS(15000), Timeout15sCallback);
				msg_sent = true;
			}

			OSPendMulti(pend_data_table, 2, 0, OS_OPT_PEND_BLOCKING, &os_err);
			if ((pend_data_table[0].RdyObjPtr == (OS_PEND_OBJ*)&semSerialRx) && (serialReadStatusLength() == 6))
			{
				uchar_t len, *data = serialReadData(&len);

				msg_received = true;
				if (len == 6)
					for (uint8_t j=0; j<len; j++)
						msg_received = msg_received && (data[j] == msg_ok[j]); // SendDataOK
			}
			else if (pend_data_table[1].RdyObjPtr == (OS_PEND_OBJ*)&semTimeout15s)
				msg_sent = false; // Resend message

			OSMutexPost(&gateway_mutex, OS_OPT_POST_NONE, &os_err);
		}
	}
}

static void TaskKeepAlive(void *p_arg)
{
	(void)p_arg;
	OS_ERR os_err;
	uchar_t _msg[6]		= { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0x02 };
	uchar_t msg_ok[6]	= { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0x82 };
	uchar_t msg_fail[6]	= { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0xC1 };
	uint8_t fail_count = 0;
	static bool msg_sent, msg_received;

	while (1)
	{
		OSSemPend(&semKeepAlive, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err); // Check every second
		OSFlagPost(&connected, (OS_FLAGS)1, OS_OPT_POST_FLAG_SET, &os_err); // Connected
		msg_received = msg_sent = false;
		fail_count = 0;

		while (!msg_received)
		{
			OSMutexPend(&gateway_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);

			if (!msg_sent)
			{
				OSSemPend(&semSerialTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
				serialWriteData(_msg, 6); // KeepAlive
				timeout_keep_alive = timerStart(TIMER_MS2TICKS(1000), TimeoutKeepAliveCallback);
				msg_sent = true;
			}

			OSPendMulti(pend_data_table2, 2, 0, OS_OPT_PEND_BLOCKING, &os_err);
			if ((pend_data_table2[0].RdyObjPtr == (OS_PEND_OBJ*)&semSerialRx) && (serialReadStatusLength() == 6))
			{
				uchar_t len, *data = serialReadData(&len);

				msg_received = true;
				if (len == 6)
					for (uint8_t j=0; j<len; j++)
						msg_received = msg_received && (data[j] == msg_ok[j]); // KeepAliveOK
			}
			else if (pend_data_table2[1].RdyObjPtr == (OS_PEND_OBJ*)&semKeepAlive)
			{
				if (++fail_count >= 5) // Retry for 5 seconds
				{
					OSFlagPost(&connected, (OS_FLAGS)1, OS_OPT_POST_FLAG_CLR, &os_err); // Disconnected
					fail_count = 5;
					gpioToggle(PIN_LED_RED);
				}
				msg_sent = false;
			}

			OSMutexPost(&gateway_mutex, OS_OPT_POST_NONE, &os_err);
		}
	}
}

static void TmrEncoder1Callback(void)
{
	directionCallback();
}

static void TmrEncoder2Callback(void)
{
	switchCallback();
}

static void TmrDisplayCallback(void)
{
	RefreshDisplay();
}

static void TmrUARTCallback(void)
{
	handler();
}

static void TmrBlinkyCallback(void)
{
	gpioToggle(PIN_LED_RED);
}

static void Timeout15sCallback(void)
{
	OS_ERR os_err;
	OSSemPost(&semTimeout15s, OS_OPT_POST_1, &os_err);
}

static void TimeoutKeepAliveCallback(void)
{
	OS_ERR os_err;
	OSSemPost(&semKeepAlive, OS_OPT_POST_1, &os_err);
}

/******************************************************************************/
