#include "common.h"
#include "util.h"
#include "rc.h"

#include <string.h>
#include <mqueue.h>

int send_msg(char const *mq_from, char const *mq_to, char const *txt)
{
	struct msg buf = {0};
	snprintf(buf.mq_name, sizeof(buf.mq_name), mq_from);
	snprintf(buf.txt, sizeof(buf.txt), txt);

	mqd_t mq = mq_open(mq_to, O_WRONLY);
	if (mq == (mqd_t) - 1) {
		//perror("mq_open");
		return ERROR;
	}

	if (mq_send(mq, (char const *)&buf, sizeof(buf), 0) < 0) {
		//perror("mq_send");
		return ERROR;
	}

	return SUCCESS;
}

int add_to_arr(char *arr[], size_t arr_size, char const *str, size_t str_size)
{
	int free_i = -1;
	for (size_t i = 0; i < arr_size; i++) { 
		if (!arr[i]) {
			free_i = i;
			break;
		}
	}

	if (free_i < 0) 
		return ERROR; 
	
	arr[free_i] = strndup(str, str_size);
	if (arr[free_i])
		return SUCCESS;
	return ERROR;
}

int find_in(char const *arr[], size_t arr_size, 
            char const *str,  size_t str_size)
{
	int si = -1;
	if (!arr) 
		return si;

	for (size_t i = 0; i < arr_size; i++) {
		if (!arr[i])
			continue;
		if (strncmp(arr[i], str, str_size) == 0) {
			si = i;
			break;
		}
	}

	return si;
}

