#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

// send message to message queue
int send_msg(char const *mq_from, char const *mq_to, char const *txt);

// add string to array of strings at free place. 
// alloctes memory for new string, that should be freed after
int add_to_arr(char *arr[], size_t arr_size, char const *str, size_t str_size);

// search for string in array of strings
int find_in(char const *arr[], size_t arr_size, 
            char const *str,  size_t str_size);

#endif 


