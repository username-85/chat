#ifndef COMMON_H
#define COMMON_H

#define MQ_SRV_NAME "/chat-srv"
#define MQ_USR_PREFIX "/usr"
#define MQ_MSG_SIZE 512
#define MQ_NAME_MAXLEN 20
#define MQ_TXT_MSG_SIZE ((MQ_MSG_SIZE) - (MQ_NAME_MAXLEN))

struct msg {
	char mq_name[MQ_NAME_MAXLEN];
	char txt[MQ_TXT_MSG_SIZE];
};

#endif
