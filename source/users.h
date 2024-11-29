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

typedef struct
{
	char *id;
	char *password;
} user_t;

#define MAX_USERS 100

void add_user(char *id, char *password);
void delete_user(char *id, char *password);
void change_password(char *id, char *old_password, char *new_password);
bool access_system(char *id, char *password);

#endif /* _USERS_H_ */
