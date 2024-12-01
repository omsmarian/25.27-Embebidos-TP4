/*
 * users.c
 *
 *  Created on: 2 sept 2024
 *      Author: asolari
 */

#include "users.h"

static user_t users[MAX_USERS];
static int current_user_count = 0;
static bool floors[MAX_FLOORS][MAX_ROOMS];

char* my_strdup(const char *src)
{
    char *str;
    char *p;
    int len = 0;

    while (src[len])
        len++;
    str = malloc(len + 1);
    p = str;
    while (*src)
        *p++ = *src++;
    *p = '\0';
    return str;
}

void add_user(char *id, char *password, floor_t floor, room_t room)
{
    if ((current_user_count >= MAX_USERS) || (floor >= MAX_FLOORS) || (room >= MAX_ROOMS) || (floors[floor][room]))
        return;

    users[current_user_count].id = my_strdup(id);
    users[current_user_count].password = my_strdup(password);
	if (!floor && !room)
	{
		for (floor = 1; floor < MAX_FLOORS; floor++)
			for (room = 0; room < MAX_ROOMS; room++)
				if (!floors[floor][room])
					break;
		// for (room = 0; room < MAX_ROOMS; room++)
		// 	if (!floors[floor][room])
		// 		break;
		// while (floors[floor][MAX_USERS_PER_FLOOR])
		// 	floor++;
	}
	users[current_user_count].floor = floor;
	users[current_user_count].room = room;
	floors[floor][room] = true;

    current_user_count++;
}

void delete_user(char *id, char *password)
{
    for (int i = 0; i < current_user_count; i++)
    {
        if (strcmp(users[i].id, id) == 0 && strcmp(users[i].password, password) == 0)
        {
            // User found, free memory and shift all following users one position back
            free(users[i].id);
            free(users[i].password);

            for (int j = i; j < current_user_count - 1; j++)
                users[j] = users[j + 1];

			floors[users[i].floor][users[i].room] = false;

            current_user_count--;

            return;
        }
    }
}

void change_password(char *id, char *old_password, char *new_password)
{
    for (int i = 0; i < current_user_count; i++)
    {
        if (strcmp(users[i].id, id) == 0 && strcmp(users[i].password, old_password) == 0)
        {
            // User found and old password matches, free old password memory and change password
            free(users[i].password);
            users[i].password = my_strdup(new_password);

            return;
        }
    }
}

bool access_system(char *id, char *password)
{
	for (int i = 0; i < current_user_count; i++)
	{
		if (strcmp(users[i].id, id) == 0
				&& strcmp(users[i].password, password) == 0) {
			// User found and password matches, grant access

			return true;
		}
	}

	return false;
}

int return_password_length (char* id)
{
	for (int i = 0; i < current_user_count; i++)
	{
		if (strcmp(users[i].id, id) == 0)
			return strlen(users[i].password);
	}

	return 0;
}

floor_t return_user_floor(char* id)
{
	for (int i = 0; i < current_user_count; i++)
		if (strcmp(users[i].id, id) == 0)
			return users[i].floor;
}

room_t return_user_room(char* id)
{
	for (int i = 0; i < current_user_count; i++)
		if (strcmp(users[i].id, id) == 0)
			return users[i].room;
}
