#include <stdio.h>
#include <string.h>
#include "fsl.h"
#include "pit.h"
#include "timer.h"
#include "cqueue.h"

static volatile char key_pressed = 0;
static volatile int key_flag = 0;

static uint8_t state = ADD_USER;
static bool return_flag = false, magcard_ready = false;

static OS_SEM encoder_sem, magcard_sem, error_sem;
static OS_SEM *sems[3] = { &encoder_sem, &magcard_sem, &error_sem };
static OS_PEND_DATA pend_data_table[3];
static OS_OBJ_QTY pended_count;
static OS_SEM *pended_sem;
static OS_ERR err;
static ticks_t timer_access, timer_error, timer_gateway;
static OS_Q *msgQueue;
static bool floors[MAX_FLOORS][MAX_ROOMS];
static room_t floor_count[MAX_FLOORS];
// static uint8_t floors[MAX_FLOORS];
static queue_id_t queue;
OS_SEM *qSems[2];

void change_brightness_call(void);
void add_user_call(void);
void delete_user_call(void);
void change_password_call(void);
void access_system_call(void);
void print_menu(enum states_fsl state);
void print_code_on_display(char *id);
void manage_access(void);
void manage_error(void);
void read_password(char *password);
void read_id(char *id);

void init_fsl(OS_Q *_msgQueue)
{
	encoder_Init(&encoder_sem);
	DisplayInit();
	MagCardInit(&magcard_sem);
	LEDS_Init();
	timerInit();

	state = ADD_USER;
	print_menu(state);
	DisplaySetBrightness(100);

	gpioMode(PIN_TP_PER, OUTPUT);
	gpioMode(PIN_TP_DED, OUTPUT);

	OSSemCreate(&encoder_sem, "Encoder Semaphore", 0, &err);
	OSSemCreate(&magcard_sem, "MagCard Semaphore", 0, &err);
	// OSSemCreate(&error_sem, "Error Semaphore", 0, &err);

	pend_data_table[0].PendObjPtr = (OS_PEND_OBJ*)&encoder_sem;
	pend_data_table[1].PendObjPtr = (OS_PEND_OBJ*)&magcard_sem;
	// pend_data_table[2].PendObjPtr = (OS_PEND_OBJ*)&error_sem;

	PIT_Init(PIT0_ID, manage_access, 10);

	msgQueue = _msgQueue;

	add_user("00000001", "1111", 1, 0);
	add_user("00000002", "2222", 1, 1);
	add_user("00000003", "3333", 2, 0);
	add_user("00000004", "4444", 2, 1);
	add_user("00000005", "5555", 3, 0);
	add_user("03192206", "1234", 3, 1);
//	floor_count[3] = floor_count[2] = floor_count[1] = 2;
}

void update_fsl()
{
// 	OSPendMulti(pend_data_table, 3, 0, OS_OPT_PEND_BLOCKING, &err);
// 	update_menu(); // Update the menu
// 	manage_access(); //Manage access
// }

// void update_menu()
// {
	OSSemPend(&encoder_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

	int key = read_key();

	if (key == 1) // LEFT
	{
		if (state == ADD_USER)	{ state = BRIGHTNESS; }
		else					{ state--; }
	}
	else if (key == 3) // RIGHT
	{
		if (state == BRIGHTNESS)	{ state = ADD_USER; }
		else						{ state++; }
	}
	else if (key == 4) // ENTER
	{
		switch (state)
		{
			case ADD_USER:			add_user_call();			break;
			case DELETE_USER:		delete_user_call();			break;
			case CHANGE_PASSWORD:	change_password_call();		break;
			case ACCESS_SYSTEM:		access_system_call();		break;
			case BRIGHTNESS:		change_brightness_call();	break;
		}
	}

	if ((key == 1) || (key == 3))
		print_menu(state);
}

void print_menu(enum states_fsl state)
{
	switch (state)
	{
		case ADD_USER:			DisplayWriteChar("ADD USER");	break;
		case DELETE_USER:		DisplayWriteChar("DEL ");		break;
		case CHANGE_PASSWORD:	DisplayWriteChar("CHNG");		break;
		case ACCESS_SYSTEM:		DisplayWriteChar("ACCS");		break;
		case BRIGHTNESS:		DisplayWriteChar("BRIG");		break;
	}
}


int read_key(void)
{
	action_t key = encoderRead();

	ResetEncoder();
	switch (key)
	{
		case NONE:			return 0;
		case LEFT:			return 1;
		case LONG_CLICK:	return 2;
		case RIGHT:			return 3;
		case CLICK:			return 4;
		case DOUBLE_CLICK:	return 5;
		default:			return 0;
	}
}

char* intToString(int num)
{
	static char* str;

	sprintf(str, "%d", num);

	return str;
}

void print_code_on_display(char *id)
{
	int id_length = strlen(id); // Obtener la longitud del ID
	DisplayClear(); // Limpiar el display

	// Comenzar desde el cuarto último dígito, o desde el inicio si el ID tiene menos de 4 dígitos
	int start = id_length > 4 ? id_length - 4 : 0;

	for (int i = start; i < id_length; i++)
	{
		// Convertir el carácter del ID a un número y imprimirlo en el display
		uint8_t number = id[i] - '0';
		DisplaySetDigitNum(i - start, number);
	}
}

bool read_from_encoder(char *id)
{
	static int digit_index = 0;
	static int current_digit = 0;

	//static char* ptrKey = "    ";

	int key = read_key();
	if (key == 0)
	{ // No key pressed
		return false;
	}
	print_code_on_display(id);

	if (key == 1) { //LEFT
		current_digit = (current_digit == 0) ? 9 : current_digit - 1;
		id[digit_index] = '0' + current_digit;
		id[digit_index + 1] = '\0'; // Null-terminate the ID string

		DisplayClear();
		print_code_on_display(id);

	} else if (key == 3) { //RIGHT
		current_digit = (current_digit == 9) ? 0 : current_digit + 1;
		id[digit_index] = '0' + current_digit;
		id[digit_index + 1] = '\0'; // Null-terminate the ID string

		DisplayClear();
		print_code_on_display(id);

	} else if (key == 4) { //ENTER
		id[digit_index] = '0' + current_digit;


		if (digit_index + 1  == 8) {
			id[digit_index + 1] = '\0'; // Null-terminate the ID string
			digit_index = 0; // Reset for next ID read
			return true;
		}
		else
		{
			id[digit_index + 1] = '0'; // Null-terminate the ID string
			id[digit_index + 2] = '\0'; // Null-terminate the ID string

			digit_index++;
			current_digit = 0;

			DisplayClear();
			print_code_on_display(id);
		}
	}

	else if(key == 2) //GO BACK IN MENU
	{
		return_flag = true;
		// OSSemPost(&error_sem, OS_OPT_POST_1, &err);
		digit_index = 0;
		current_digit = 0;
		return false;
	}

	else if (key == 5)
	{ //DOUBLE_CLICK
			if (digit_index > 0) {
				digit_index--;
				current_digit = 0;
				id[digit_index] = '0'; // Null-terminate the ID string
				id[digit_index + 1] = '\0'; // Null-terminate the ID string
				DisplayClear();
				print_code_on_display(id);
			}

	}
	return false;
}

bool read_from_card(char *id)
{
	char data_str[17]; // Buffer para almacenar la representación de cadena de data
	uint64_t data;

	data = MagCardGetCardNumber();
	MagCardClearData();

	for (int i = 0; i < 8; i++) {
		id[7 - i] = '0' + (data % 10);
		data /= 10;
	}
	id[8] = '\0';

	// sprintf(data_str, "%016llu", data); // Convertir data a cadena, asegurándose de que tenga 16 dígitos
	// strncpy(id, &data_str[8], 8); // Copiar los últimos 8 dígitos a id

	return true;
}

int8_t change_brightness(void)
{
	int success = 0, key;
	int8_t brightness = 0;

	while (success != 2)
	{
		OSSemPend(&encoder_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		key = read_key();
		if (key == 1) { //LEFT
			brightness = (uint8_t)DisplaySetBrightness(-1);

			uint8_t cent = brightness / 100; // Para obtener las centenas
			uint8_t dec = (brightness % 100) / 10; // Para obtener las decenas
			uint8_t uni = brightness % 10; // Para obtener las unidades

			DisplaySetDigitNum(0, 0);
			DisplaySetDigitNum(1, cent);
			DisplaySetDigitNum(2, dec);
			DisplaySetDigitNum(3, uni);

			success = 1;
		} else if (key == 3) { //RIGHT
			brightness = (uint8_t)DisplaySetBrightness(1);

			uint8_t cent = brightness / 100; // Para obtener las centenas
			uint8_t dec = (brightness % 100) / 10; // Para obtener las decenas
			uint8_t uni = brightness % 10; // Para obtener las unidades

			DisplaySetDigitNum(0, 0);
			DisplaySetDigitNum(1, cent);
			DisplaySetDigitNum(2, dec);
			DisplaySetDigitNum(3, uni);

			success = 1;
		} else if (key == 4) { //ENTER
			success = 2;
			return brightness;
		}
		else if (key == 2) { //Go back
			brightness = (uint8_t)DisplaySetBrightness(100);
			return -1;
		}
	}

	return brightness;
}

void change_brightness_call(void)
{
	int8_t brightness = change_brightness();

	if (brightness == -1) {
		state = ADD_USER;
		return_flag = false;
		print_menu(state);
	}
	else
	{
		state = ADD_USER;
		print_menu(state);
	}
}

void read_id(char *id)
{
	int read_successful = 0;

	MagCardClearData();
	while (!read_successful)
	{
		// manage_access();

		OSPendMulti(pend_data_table, 2, 0, OS_OPT_PEND_BLOCKING, &err);

		if (error_flag)
			break;
		else if (pend_data_table[0].RdyObjPtr == (OS_PEND_OBJ*)&encoder_sem)
			read_successful = read_from_encoder(id);
		else if (pend_data_table[1].RdyObjPtr == (OS_PEND_OBJ*)&magcard_sem)
			read_successful = read_from_card(id);

		// if (!strcmp(pend_data_table[2].RdyObjPtr->NamePtr, "Error Semaphore"))
		// 	break;
		// else if (!strcmp(pend_data_table[0].RdyObjPtr->NamePtr, "Encoder Semaphore"))
		// 	read_successful = read_from_encoder(id);
		// else if (!strcmp(pend_data_table[1].RdyObjPtr->NamePtr, "MagCard Semaphore"))
		// 	read_successful = read_from_card(id);
	}
}

int type_of_pasword(void)
{
	char* str = " ";
	int digit = 4;

	while(1)
	{
		// manage_access();

		OSSemPend(&encoder_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		int key = read_key();
		if(key == 1 || key == 3) //LEFT or RIGHT
		{
			digit = (digit == 4) ? 5 : 4;
			str = (digit == 4) ? "PL 4" : "PL 5";
			DisplayWriteChar(str);
		}
		else if (key == 4) //ENTER
		{
			DisplayWriteChar("PASS");
			return digit;
		}
		else if (key == 2) //Go back
		{
			return_flag = true;
			// OSSemPost(&error_sem, OS_OPT_POST_1, &err);
			return 0;
		}
	}
}

void read_password_without_length(char * id, char *password)
{
	int digit_index = 0;
	int current_digit = 0;
	int digits = return_password_length(id);

	if (return_flag || digits == 0)
	{
		state = ADD_USER;
		print_menu(state);
		return;
	}

	while (digit_index < digits)
	{
		// manage_access();

		OSSemPend(&encoder_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		int key = read_key();
		if (key == 1) { //LEFT
			current_digit = (current_digit == 0) ? 9 : current_digit - 1;
			password[digit_index] = '0' + current_digit;
			password[digit_index + 1] = '\0'; // Null-terminate the password string

			//DisplayWriteChar(password);
			DisplayWritePassword(digit_index, current_digit);
		} else if (key == 3) { //RIGHT
			current_digit = (current_digit == 9) ? 0 : current_digit + 1;
			password[digit_index] = '0' + current_digit;
			password[digit_index + 1] = '\0'; // Null-terminate the password string

			//DisplayWriteChar(password);
			DisplayWritePassword(digit_index, current_digit);
		} else if (key == 4) { //ENTER
			password[digit_index] = '0' + current_digit;

			if (digit_index + 1  == digits)
			{
				password[digit_index+1] = '\0'; // Null-terminate the ID string
				digit_index = 0; // Reset for next ID read
				current_digit = 0;
				return;
			}
			else
			{
				password[digit_index + 1] = '0'; // Null-terminate the ID string
				password[digit_index + 2] = '\0'; // Null-terminate the ID string

				digit_index++;
				current_digit = 0;

				DisplayWritePassword(digit_index, current_digit);
			}
		}
		else if (key == 2) //Go back
		{
				return_flag = true;
				// OSSemPost(&error_sem, OS_OPT_POST_1, &err);
				digit_index = 0;
				current_digit = 0;
				break;
		}
		else if (key == 5)
		{
			if (digit_index > 0)
			{
				digit_index--;
				current_digit = 0;
				password[digit_index] = '0'; // Null-terminate the ID string
				password[digit_index + 1] = '\0'; // Null-terminate the ID string
				DisplayWritePassword(digit_index, current_digit);
			}
		}
	}
}

void read_password(char *password)
{
	int digit_index = 0, current_digit = 0, digits = type_of_pasword(), key;

	if (return_flag || digits == 0)
	{
		state = ADD_USER;
		print_menu(state);
		return;
	}

	while (digit_index < digits)
	{
		// manage_access();

		OSSemPend(&encoder_sem, 0, OS_OPT_PEND_BLOCKING, NULL, &err);

		key = read_key();
		if (key == 1) { //LEFT
			current_digit = (current_digit == 0) ? 9 : current_digit - 1;
			password[digit_index] = '0' + current_digit;
			print_code_on_display(password);
		} else if (key == 3) { //RIGHT
			current_digit = (current_digit == 9) ? 0 : current_digit + 1;
			password[digit_index] = '0' + current_digit;
			password[digit_index + 1] = '\0'; // Null-terminate the password string
			print_code_on_display(password);
		} else if (key == 4) { //ENTER
			password[digit_index] = '0' + current_digit;

			if (digit_index + 1  == digits) {
				password[digit_index+1] = '\0'; // Null-terminate the ID string
				digit_index = 0; // Reset for next ID read
				current_digit = 0;
				return;
			}
			else
			{
				password[digit_index + 1] = '0'; // Null-terminate the ID string
				password[digit_index + 2] = '\0'; // Null-terminate the ID string

				digit_index++;
				current_digit = 0;

				print_code_on_display(password);
			}
		}
		else if (key == 2) //Go back
		{
				return_flag = true;
				// OSSemPost(&error_sem, OS_OPT_POST_1, &err);
				digit_index = 0;
				current_digit = 0;
				break;
		}
		else if (key == 5)
		{
			if (digit_index > 0)
			{
				digit_index--;
				current_digit = 0;
				password[digit_index] = '0'; // Null-terminate the ID string
				password[digit_index + 1] = '\0'; // Null-terminate the ID string
				print_code_on_display(password);
			}
		}

	}
}

void add_user_call()
{
	char id[20] = "";
	char password[20] = "";

	DisplayWriteChar("ID  ");
	read_id(id);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	DisplayWriteChar("LENG");
	read_password(password);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	add_user(id, password, 0, 0);

	print_menu(state);
}

void delete_user_call()
{
	char id[20] = "";
	char password[20] = "";

	DisplayWriteChar("ID  ");
	read_id(id);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	DisplayWriteChar("PASS");
	read_password_without_length(id,password);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}


	delete_user(id, password);
	floor_t floor = return_user_floor(id);
	room_t room = return_user_room(id);
	if (floors[floor][room])
		floor_count[floor]--;
	floors[floor][room] = false;

	print_menu(state);
}

void change_password_call()
{
	char id[20] = "";
	char old_password[20] = "";

	DisplayWriteChar("ID  ");
	read_id(id);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	DisplayWriteChar("OLD ");
	read_password_without_length(id, old_password);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	char new_password[20];
	DisplayWriteChar("NEW ");
	read_password(new_password);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	change_password(id, old_password, new_password);
	print_menu(state);
}

void access_system_call(void)
{
	char id[20] = "";
	char password[20] = "";

	DisplayWriteChar("ID  ");
	read_id(id);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	DisplayWriteChar("PASS");
	read_password_without_length(id, password);

	if (return_flag)
	{
			state = ADD_USER;
			return_flag = false;
			print_menu(state);
			return;
	}

	uint8_t access = access_system(id, password);

	if (access)
	{
		DisplayWriteChar("ACCE");
		if(access_flag == false)
		{
			access_flag = true;
			// timer_access = timerStart(TIMER_MS2TICKS(5000));
//			LEDS_Set(ALL);

			floor_t floor = return_user_floor(id);
			room_t room = return_user_room(id);
			if (floors[floor][room])
				floor_count[floor]--;
			else
				floor_count[floor]++;
			floors[floor][room] = !floors[floor][room];

			// char floor_count_str[(MAX_FLOORS - 1) * 2 + 1] = {0};
			char message[(MAX_FLOORS - 1) * 2 + 6] = { 0xAA, 0x55, 0xC3, 0x3C, 0x00, 0x01,
													   floor_count[1]%10, floor_count[1]/10,
													   floor_count[2]%10, floor_count[2]/10,
													   floor_count[3]%10, floor_count[3]/10 };
//			for (int i = 0; i < MAX_FLOORS - 1; i++)
//			{
//				// char count_str[3];
//				int count = floor_count[i+1];
//				message[i + 0 + 6] = (count / 10); //+ '0';
//				message[i + 1 + 6] = (count % 10); //+ '0';
//				// count_str[i + 2 - 1] = '\0';
//				// strcat(floor_count_str, count_str);
//			}
			message[4] = (MAX_FLOORS - 1) * 2 + 1;
//			OSQFlush(msgQueue, &err);
//			OSQPost(msgQueue, &floor_count_str[0], (MAX_FLOORS - 1) * 2, OS_OPT_POST_FIFO, &err);
			queueClear(queue);
			for (int i = 0; i < (MAX_FLOORS - 1) * 2 + 6; i++)
				queuePush(queue, message[i]);
			OSSemPost(qSems[1], OS_OPT_POST_1, &err);
			OS_ERR aux = err;
		}
		else
		{
			// timer_access = timerStart(TIMER_MS2TICKS(5000));
//			LEDS_Set(ONLY_CENTER);
		}

		timer_access = timerStart(TIMER_MS2TICKS(5000), NULL);
//		OSTmrCreate(&timer_access, "Access Timer", 0, 5000, OS_OPT_TMR_ONE_SHOT, NULL, NULL, &err);
//		OSTmrStart(&timer_access, &err);

		LEDS_Set(0b001);
		LEDS_Set(0b010);
		LEDS_Set(0b100);
	}
	else
	{
		DisplayWriteChar("ERR ");
		if (error_flag == false)
		{
			error_flag = true;
			// timer_error = timerStart(TIMER_MS2TICKS(5000));
		}
		else
		{
			// timer_error = timerStart(TIMER_MS2TICKS(5000));
		}

		timer_error = timerStart(TIMER_MS2TICKS(5000), NULL);
//		OSTmrCreate(&timer_error, "Error Timer", 0, 5000, OS_OPT_TMR_ONE_SHOT, NULL, NULL, &err);
//		OSTmrStart(&timer_error, &err);

		LEDS_Set(0b010);
	}

//	print_menu(state);
}

void manage_access(void)
{
	bool timer_access_expired = timerExpired(timer_access);
	bool timer_error_expired = timerExpired(timer_error);
//	bool timer_gateways_expired = timerExpired(timer_gateway);

 	if (timer_access_expired)
// 	if (OSTmrRemainGet(&timer_access, &err))
 	{
 		access_flag = false;
 		// LEDS_Set(NOTHING);
		gpioWrite(PIN_LED_GREEN, HIGH);
 	}
 	else
 	{
 		// LEDS_Set(0b001);
 		// LEDS_Set(0b010);
 		// LEDS_Set(0b100);
		gpioWrite(PIN_LED_GREEN, LOW);
 	}

 	if (timer_error_expired)
// 	if (OSTmrRemainGet(&timer_error, &err))
 	{
 		error_flag = false;
 		// LEDS_Set(NOTHING);
		gpioWrite(PIN_LED_RED, HIGH);
 	}
 	else
 	{
 		// LEDS_Set(0b010);
		gpioWrite(PIN_LED_RED, LOW);
 	}

	if (timer_access_expired && timer_error_expired)
		gpioWrite(PIN_LED_BLUE, LOW);
	else
		gpioWrite(PIN_LED_BLUE, HIGH);

//	if (timer_gateways_expired)
//	{
//		OSSemPost(qSems[0], OS_OPT_POST_1, &err);
//	}
}

//void manage_access(void)
//{
//	access_flag = false;
//	LEDS_Set(NOTHING);
//}
//
//void manage_error(void)
//{
//	error_flag = false;
//	LEDS_Set(NOTHING);
//}

void setQueue(queue_id_t q)
{
	queue = q;
}

void setQueueSems(OS_SEM *semRx, OS_SEM *semTx)
{
	qSems[0] = semRx;
	qSems[1] = semTx;
}

void setGatewayTimer(ticks_t timer)
{
	timer_gateway = timer;
}
