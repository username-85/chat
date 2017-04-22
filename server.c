#include "common.h"
#include "rc.h"
#include "util.h"
#include <mqueue.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define USLEEP_SEC 500000
#define MAX_USERS 10

static char *usr_names[MAX_USERS];
static bool srv_running = true;

static mqd_t create_mq(char const *name);

static int add_to_unames(char const *name);

static void print_arr(char const *arr[], size_t arr_size);
static void free_usr_names(void);
static void send_to_all(char const *from, char const *txt);
static int del_from_unames(char const *name);

void sig_handle(int unused)
{
	(void) unused;
	srv_running = false;
}

int main(void)
{
	int ret = SUCCESS;
	struct sigaction act = {0};
	act.sa_handler = sig_handle;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	mqd_t mq = create_mq(MQ_SRV_NAME); 
	if (mq == (mqd_t) - 1) {
		perror("mq_open");
		ret = ERROR;
		goto end;
	}

	while (srv_running) {
		int bytes = 0;
		while(1) { 
			struct msg buf = {0};
			bytes = mq_receive(mq, (char *)&buf, 
			                   sizeof(buf), NULL);
			if (bytes < 0)
				break;

			printf("%s : %s\n", buf.mq_name, buf.txt);
			
			if (strcmp(buf.txt, "/offline") == 0) {
				del_from_unames(buf.mq_name);
			} else if (add_to_unames(buf.mq_name) != SUCCESS) {
				send_msg(MQ_SRV_NAME, buf.mq_name, 
				         "Too many users online, try later");
				break;
			}

			// send message recieved from client to all clients
			send_to_all(buf.mq_name, buf.txt);
			
			print_arr((char const **)usr_names, MAX_USERS);
		}

		if (bytes < 0 && errno != EAGAIN) {
			perror("mq_receive");
			ret = ERROR;
			goto end;
		}

		usleep(USLEEP_SEC);
	}

end:
	mq_unlink(MQ_SRV_NAME);
	free_usr_names();

	if (ret != SUCCESS) 
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}

static mqd_t create_mq(char const *name)
{
	int mq_flags = O_RDONLY | O_EXCL | O_CREAT | O_NONBLOCK;
	struct mq_attr attr = {0};
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = MQ_MSG_SIZE;

	return mq_open(name, mq_flags, 0644, &attr);
}

static int add_to_unames(char const *name)
{
	int rc = find_in((char const **)usr_names, MAX_USERS, 
	                 name, strlen(name));
	if (rc >= 0)
		return SUCCESS;
	
	rc = add_to_arr(usr_names, MAX_USERS, name, strlen(name));
	return rc;
}

static int del_from_unames(char const *name)
{
	int ind = find_in((char const **)usr_names, MAX_USERS, 
		          name, strlen(name));

	if (ind < 0) //nothing to delete 
		return SUCCESS;
	
	if (usr_names[ind]) {
		free(usr_names[ind]);
		usr_names[ind] = NULL;
	}
	return SUCCESS;
}

static void print_arr(char const *arr[], size_t arr_size)
{
	printf("\n");
	for (size_t i = 0; i < arr_size; i++) {
		if (arr[i])
			printf("usr_names[%zd] = %s\n", i, arr[i]);
	}
}


static void free_usr_names(void) 
{
	for (int i = 0; i < MAX_USERS; i++) {
		if (usr_names[i]) {
			free(usr_names[i]);
			usr_names[i] = NULL;
		}
	}
}

static void send_to_all(char const *from, char const *txt)
{
	for (int i = 0; i < MAX_USERS; i++) {
		if (!usr_names[i]) 
			continue;

		int rc = send_msg(from, usr_names[i], txt);
		if (rc != SUCCESS)
			del_from_unames(usr_names[i]);
	}

}

