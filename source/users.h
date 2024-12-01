/*
 * users.h
 *
 *  Created on: 2 sept 2024
 *      Author: asolari
 */

#ifndef _USERS_H_
#define _USERS_H_

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t floor_t;
typedef uint8_t room_t;

typedef struct {
	char *id;
	char *password;
	floor_t floor;
	room_t room;
} user_t;

#define MAX_FLOORS 4 // Ground floor is not available
#define MAX_ROOMS 10
#define MAX_USERS ((MAX_FLOORS) * (MAX_ROOMS))

void add_user(char *id, char *password, floor_t floor, room_t room);
void delete_user(char *id, char *password);
void change_password(char *id, char *old_password, char *new_password);
bool access_system(char *id, char *password);
int return_password_length (char* id);
floor_t return_user_floor(char* id);
room_t return_user_room(char* id);

#endif /* _USERS_H_ */
