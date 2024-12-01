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
#include "os.h"
#include <uart.h>

#include "board.h"
#include "hardware.h"
#include "macros.h"
#include "serial.h"
#include "timer.h"
#include "encoder.h"
#include "display.h"
#include "magcard.h"
#include "fsl.h"
#include "cqueue.h"

#include "gpio.h"
#include "board.h"
#include "pit.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/* Task Start */
#define TASKSTART_STK_SIZE 		512u
#define TASKSTART_PRIO 			3u
static OS_TCB TaskStartTCB;
static CPU_STK TaskStartStk[TASKSTART_STK_SIZE];

/* Task Encoder */
// #define TASKE_STK_SIZE			256u
// #define TASKE_STK_SIZE_LIMIT	(TASKE_STK_SIZE / 10u)
// #define TASKE_PRIO              3u
// static OS_TCB TaskEncoderTCB;
// static CPU_STK TaskEncoderStk[TASKE_STK_SIZE];

/* Task MagCard */
// #define TASKMC_STK_SIZE			256u
// #define TASKMC_STK_SIZE_LIMIT	(TASKMC_STK_SIZE / 10u)
// #define TASKMC_PRIO              3u
// static OS_TCB TaskMagCardTCB;
// static CPU_STK TaskMagCardStk[TASKMC_STK_SIZE];

/* Task Gateway */
#define TASKG_STK_SIZE			256u
#define TASKG_STK_SIZE_LIMIT	(TASKG_STK_SIZE / 10u)
#define TASKG_PRIO              3u
static OS_TCB TaskGatewayTCB;
static CPU_STK TaskGatewayStk[TASKG_STK_SIZE];

/* Task Keep Alive */
#define TASKKA_STK_SIZE			256u
#define TASKKA_STK_SIZE_LIMIT	(TASKKA_STK_SIZE / 10u)
#define TASKKA_PRIO              3u
static OS_TCB TaskKeepAliveTCB;
static CPU_STK TaskKeepAliveStk[TASKKA_STK_SIZE];

/* Semaphores */
static OS_SEM semGatewayTx, semGatewayRx, semSerialTx, semSerialRx;
static OS_SEM semTimeout15s, semKeepAlive;

/* Message Queue */
static OS_Q msgQueue;
static queue_id_t queue;

static uint64_t card = '0';
static ticks_t timeout_15s, timeout_keep_alive;
static OS_PEND_DATA pend_data_table[2];
static OS_PEND_DATA pend_data_table2[2];

/* Mutex */
static OS_MUTEX gateway_mutex;

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void TaskStart(void *p_arg);
static void TaskKeepAlive(void *p_arg);
// static void TASK_Encoder(void *p_arg);
// static void TASK_MagCard(void *p_arg);
static void TASK_Gateway(void *p_arg);
static void TmrEncoder1Callback(void);
static void TmrEncoder2Callback(void);
// static void TmrMagCardCallback(OS_TMR *p_tmr, void *p_arg);
static void TmrGatewayCallback(void);
static void TmrDisplayCallback(void);
static void TmrUARTCallback(void);
// static void TmrBlinkyCallback(OS_TMR *p_tmr, void *p_arg);
static void TmrBlinkyCallback(void);
static void Timeout15sCallback(void);
static void TimeoutKeepAliveCallback(void);

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

	// serialInit();
	// timerInit();
	PIT_Init(PIT0_ID, TmrBlinkyCallback, 10);
	PIT_Init(PIT0_ID, TmrEncoder1Callback, 1000);
	PIT_Init(PIT0_ID, TmrEncoder2Callback, 20);
//	 PIT_Init(PIT3_ID, TmrMagCardCallback, 1);
//	 PIT_Init(PIT3_ID, TmrGatewayCallback, 1);
	PIT_Init(PIT0_ID, TmrDisplayCallback, 7500);
	PIT_Init(PIT0_ID, TmrUARTCallback, 200);

	serialInit();

	/* Create Message Queue */
	OSQCreate(&msgQueue, "Message Queue", 10, &err);
	queue = queueInit();
	init_fsl(&msgQueue);
	OSSemCreate(&semGatewayRx, "Gateway Semaphore", 0, &err);
	OSSemCreate(&semGatewayTx, "Gateway Tx Semaphore", 0, &err);
	setQueue(queue);
	setQueueSems(&semGatewayRx, &semGatewayTx);
	OSSemCreate(&semSerialTx, "Serial Tx Semaphore", 0, &err);
	OSSemCreate(&semSerialRx, "Serial Rx Semaphore", 0, &err);
	OSSemCreate(&semTimeout15s, "Timeout 15s Semaphore", 0, &err);
	OSSemCreate(&semKeepAlive, "Keep Alive Semaphore", 0, &err);
	uartSetSem(&semSerialRx, &semSerialTx);
	OSMutexCreate(&gateway_mutex, "Gateway Mutex", &err);
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
	// OSSemCreate(&semEncoder, "Encoder Semaphore", 0, &os_err);
	// OSSemCreate(&semMagCard, "MagCard Semaphore", 0, &os_err);
	// OSSemCreate(&semGateway, "Gateway Semaphore", 0, &os_err);

	/* Create Encoder Timer */
	OS_TMR TmrEncoder1;
	OSTmrCreate(&TmrEncoder1,
				"Encoder1 Timer",
				 0u,
				 1u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrEncoder1Callback,
				 0x0,
				&os_err);
	OS_TMR TmrEncoder2;
	OSTmrCreate(&TmrEncoder2,
				"Encoder2 Timer",
				 0u,
				 1u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrEncoder2Callback,
				 0x0,
				&os_err);

	/* Create MagCard Timer */
	// OS_TMR TmrMagCard;
	// OSTmrCreate(&TmrMagCard,
	// 			"MagCard Timer",
	// 			 0u,
	// 			 1000u,
	// 			 OS_OPT_TMR_PERIODIC,
	// 			(OS_TMR_CALLBACK_PTR)TmrMagCardCallback,
	// 			 0x0,
	// 			&os_err);

	/* Create Gateway Timer */
	OS_TMR TmrGateway;
	OSTmrCreate(&TmrGateway,
				"Gateway Timer",
				 0u,
				 10000u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrGatewayCallback,
				 0x0,
				&os_err);

	// /* Create Keep Alive Timer */
	// OS_TMR TmrKeepAlive;
	// OSTmrCreate(&TmrKeepAlive,
	// 			"Keep Alive Timer",
	// 			 0u,
	// 			 10000u,
	// 			 OS_OPT_TMR_PERIODIC,
	// 			(OS_TMR_CALLBACK_PTR)TimeoutKeepAliveCallback,
	// 			 0x0,
	// 			&os_err);

	/* Create Display Timer */
	OS_TMR TmrDisplay;
	OSTmrCreate(&TmrDisplay,
				"Display Timer",
				 0u,
				 1u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrDisplayCallback,
				 0x0,
				&os_err);

	/* Create UART Timer */
	OS_TMR TmrUART;
	OSTmrCreate(&TmrUART,
				"UART Timer",
				 0u,
				 1u,
				 OS_OPT_TMR_PERIODIC,
				(OS_TMR_CALLBACK_PTR)TmrUARTCallback,
				 0x0,
				&os_err);

	/* Create a Timer for Blinky */
	// OS_TMR TmrBlinky;
	// OSTmrCreate(&TmrBlinky,
	// 			"Blinky Timer",
	// 			 0u,
	// 			 333u,
	// 			 OS_OPT_TMR_PERIODIC,
	// 			(OS_TMR_CALLBACK_PTR)TmrBlinkyCallback,
	// 			 0x0,
	// 			&os_err);

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
	OSTaskCreate(&TaskGatewayTCB,         // tcb
				 "Task Gateway",          // name
				  TASK_Gateway,           // func
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
	OSTaskCreate(&TaskKeepAliveTCB,         // tcb
				 "Task Keep Alive",          // name
				  TaskKeepAlive,           // func
				  0u,                     // arg
				  TASKKA_PRIO,             // prio
				 &TaskKeepAliveStk[0u],     // stack
				  TASKKA_STK_SIZE_LIMIT,   // stack limit
				  TASKKA_STK_SIZE,         // stack size
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

	/* Start Timers */
//	OSTmrStart(&TmrEncoder1,	&os_err);
//	OSTmrStart(&TmrEncoder2,	&os_err);
//	OSTmrStart(&TmrMagCard,		&os_err);
//	OSTmrStart(&TmrGateway,		&os_err);
//	OSTmrStart(&TmrDisplay,		&os_err);
//	OSTmrStart(&TmrUART,		&os_err);
//	OSTmrStart(&TmrBlinky,		&os_err);

    while (1)
	{
		// OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &os_err);
		update_fsl();
	}
}

// static void TASK_Encoder(void *p_arg)
// {
//     (void)p_arg;
//     OS_ERR os_err;

//     while (1)
// 	{
// 		OSSemPend(&semEncoder, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
// 		// encoderRead();
// 		OSSemPost(&semMagCard, OS_OPT_POST_1, &os_err);
//     }
// }

// static void TASK_MagCard(void *p_arg)
// {
//     (void)p_arg;
//     OS_ERR os_err;

//     while (1)
// 	{
// 		OSSemPend(&semMagCard, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
// 		card = MagCardGetCardNumber();
// 		OSSemPost(&semGateway, OS_OPT_POST_1, &os_err);
//     }
// }

static void TASK_Gateway(void *p_arg)
{
	(void)p_arg;
	OS_ERR os_err;
	void *msg;
	uchar_t _msg[12];
	OS_MSG_SIZE msg_size;
	CPU_TS ts;
	static bool msg_sent, msg_received;

	while (1)
	{
		OSSemPend(&semTimeout15s, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
		OSSemPend(&semGatewayTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
		msg_received = msg_sent = false;
		// msg = OSQPend(&msgQueue,
		// 			   0,
		// 			   OS_OPT_PEND_BLOCKING,
		// 			  &msg_size,
		// 			  &ts,
		// 			  &os_err);
		uint8_t i = queueSize(queue);
		// for (uint8_t i=0; i<6; i++)
			// _msg[i] = *(char *)(msg + i);
		if (i <= 12)
			for (uint8_t j=i; j>0; j--)
				_msg[i-j] = queuePop(queue);
		else
			msg_received = true; // Not really (just skips)

		while (!msg_received)
		{
			OSMutexPend(&gateway_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);

			// gpioToggle(PIN_LED_RED);
	//		if (serialReadStatus())
	//		{
	//			uchar_t len;
	//			uchar_t *data = serialReadData(&len);
	//			card = *data;
	//		}
			if (!msg_sent)
			{
				OSSemPend(&semSerialTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
				// if (serialWriteStatus())
				{
					// serialWriteData((uchar_t *)msg, msg_size);
					serialWriteData(_msg, i);
					// if (++card > 'Z') card = NUM2ASCII(0);
				}
				timeout_15s = timerStart(TIMER_MS2TICKS(15000), Timeout15sCallback);
				msg_sent = true;
			}
			OSPendMulti(pend_data_table, 2, 0, OS_OPT_PEND_BLOCKING, &os_err);

			if ((pend_data_table[0].RdyObjPtr == (OS_PEND_OBJ*)&semSerialRx) && (serialReadStatusLength() == 6))
			{
//				timerStop(Timeout15sCallback);
				// msg_sent = check_msg(_msg, i);
				// OSSemPend(&semSerialRx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
				// if (serialReadStatus())
				{
					// timerDelay(TIMER_MS2TICKS(10));
					uchar_t len;
					uchar_t *data = serialReadData(&len);
//					for (uint8_t j=0; j<len; j++)
//						_msg[j] = data[j];
					uchar_t msg_ok[] = { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0x81 };
					uchar_t msg_fail[] = { 0xAA, 0x55, 0xC3, 0x3C, 0x01, 0xC1 };
					// if (!strcmp((char *)data, msg_ok))
					// 	msg_sent = true;
					// else if (!strcmp((char *)data, msg_fail))
					// 	msg_sent = false;
//					msg_sent = !strcmp((char *)data, (char *)msg_ok);
					msg_received = true;
					if (len == 6)
						for (uint8_t j=0; j<len; j++)
							msg_received = msg_received && (data[j] == msg_ok[j]);
				}
			}
			else if (pend_data_table[1].RdyObjPtr == (OS_PEND_OBJ*)&semTimeout15s)
			{
				msg_sent = false;
			}

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
		OSSemPend(&semKeepAlive, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
		msg_received = msg_sent = false;
		fail_count = 0;

		while (!msg_received)
		{
			OSMutexPend(&gateway_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);

			if (!msg_sent)
			{
				OSSemPend(&semSerialTx, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
				serialWriteData(_msg, 6);
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
						msg_received = msg_received && (data[j] == msg_ok[j]);
			}
			else if (pend_data_table2[1].RdyObjPtr == (OS_PEND_OBJ*)&semKeepAlive)
			{
				if (++fail_count >= 5)
				{
					fail_count = 5;
					gpioToggle(PIN_LED_RED);
				}
				msg_sent = false;
			}

			OSMutexPost(&gateway_mutex, OS_OPT_POST_NONE, &os_err);
		}
	}
}

//static void TmrEncoder1Callback(OS_TMR *p_tmr, void *p_arg)
static void TmrEncoder1Callback(void)
{
	directionCallback();
}

//static void TmrEncoder2Callback(OS_TMR *p_tmr, void *p_arg)
static void TmrEncoder2Callback(void)
{
	switchCallback();
}

// static void TmrMagCardCallback(OS_TMR *p_tmr, void *p_arg)
// {
// 	MagCardUpdate();
// }

//static void TmrGatewayCallback(OS_TMR *p_tmr, void *p_arg)
static void TmrGatewayCallback(void)
{
	// gateway();
}

//static void TmrDisplayCallback(OS_TMR *p_tmr, void *p_arg)
static void TmrDisplayCallback(void)
{
	RefreshDisplay();
}

//static void TmrUARTCallback(OS_TMR *p_tmr, void *p_arg)
static void TmrUARTCallback(void)
{
	handler();
}

// static void BlinkyCallback(OS_TMR *p_tmr, void *p_arg)
static void TmrBlinkyCallback(void)
{
	gpioToggle(PIN_LED_RED);
	// OS_ERR os_err;
	// OSSemPost(&semGateway, OS_OPT_POST_1, &os_err);
}

static void Timeout15sCallback(void)
{
	OS_ERR os_err;
	OSSemPost(&semTimeout15s, OS_OPT_POST_1, &os_err);
//	msg_sent = false;
}

static void TimeoutKeepAliveCallback(void)
{
	OS_ERR os_err;
	OSSemPost(&semKeepAlive, OS_OPT_POST_1, &os_err);
}

/******************************************************************************/
