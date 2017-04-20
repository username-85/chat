#include "common.h"
#include "util.h"
#include "rc.h"

#include <errno.h>
#include <ncurses.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

// chat history window
static WINDOW *chat_win;
// window for message input. And for some info messages output
static WINDOW *msg_win;
// initscr successfully called
bool ncurses_on;

static void fail_exit(char const *msg);
static void term_init(void);
static void term_deinit(void);

static mqd_t create_mq(char const *name);

int main(void)
{
	term_init();

	char mq_usr_name[MQ_NAME_MAXLEN] = {0};
	snprintf(mq_usr_name, sizeof(mq_usr_name), 
	         MQ_USR_PREFIX"%d", getpid());
	mqd_t mq_usr = create_mq(mq_usr_name);
	if (mq_usr == (mqd_t) - 1)
		fail_exit("create_mq failed");
	
	wprintw(chat_win, "/*************************************/\n");
	wprintw(chat_win, "/*          <F10> - quit             */\n");
	wprintw(chat_win, "/*     other keys - start message    */\n");
	wprintw(chat_win, "/*************************************/\n");

	int rc = send_msg(mq_usr_name, MQ_SRV_NAME, "/online");
	if (rc != SUCCESS) 
		fail_exit("Couldn't connect to server.");

	while (1) {
		
		int bytes = 0;
		while(1) { // reading messages
			struct msg buf = {0};
			bytes = mq_receive(mq_usr, (char *)&buf, 
			                   sizeof(buf), NULL);
			if (bytes < 0)
				break;
			wprintw(chat_win, "%s : %s\n", buf.mq_name, buf.txt);
		}

		if (bytes < 0 && errno != EAGAIN)
			fail_exit("mq_receive");
		else if (bytes > 0) 
			wrefresh(chat_win);

		noecho();
		int ch = wgetch(chat_win);

		if (ch == KEY_F(10)) {
			break;
		} else if (ch != ERR) {
			curs_set(1);
			echo();
			wprintw(msg_win, ">> ");
			wrefresh(msg_win);

			char buf[MQ_TXT_MSG_SIZE] = {0};
			wgetnstr(msg_win, buf, MQ_TXT_MSG_SIZE - 1);

			werase(msg_win);
			wrefresh(msg_win);
			curs_set(0);
			
			if (strnlen(buf, MQ_TXT_MSG_SIZE))
				send_msg(mq_usr_name, MQ_SRV_NAME, buf);
		}
	}

	send_msg(mq_usr_name, MQ_SRV_NAME, "/offline");

	mq_unlink(mq_usr_name);
	term_deinit();
	exit(EXIT_SUCCESS);
}

static void term_init(void)
{
	setlocale(LC_ALL, "");

	if (initscr() == NULL)
		fail_exit("Failed to initialize ncurses");

	ncurses_on = true;

	cbreak();
	noecho();
	curs_set(0);

	if (LINES >= 2) {
		chat_win = newwin(LINES - 1, COLS, 0, 0);
		msg_win = newwin(LINES -1, COLS, LINES - 1, 0);
	} else {
		fail_exit("Window is too small");
	}

	if (chat_win == NULL || msg_win == NULL)
		fail_exit("Failed ctreate windows");

	keypad(chat_win, TRUE);
	wtimeout(chat_win, 100);
	scrollok(chat_win,TRUE);
}

static void fail_exit(char const *msg)
{
	//? !isendwin()
	if (ncurses_on)
		endwin();
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

static void term_deinit(void)
{
	delwin(msg_win);
	delwin(chat_win);
	if (ncurses_on) {
		endwin();
		ncurses_on = false;
	}
}

static mqd_t create_mq(char const *name)
{
	int mq_flags = O_RDONLY | O_CREAT | O_NONBLOCK;
	struct mq_attr attr = {0};
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = MQ_MSG_SIZE;

	return mq_open(name, mq_flags, 0644, &attr);
}

