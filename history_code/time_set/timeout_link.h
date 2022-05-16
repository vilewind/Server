#ifndef __TIMEOUT_LINK__
#define __TIMEOUT_LINK__
#include "general_header.h"

#include <time.h>

const static int BUFFER_SIZE = 64;

class util_timer;

struct client_data 
{
	sockaddr_in addr;
	int sock_fd;
	char buffer[BUFFER_SIZE];
	util_timer *timer;
};

/**
 * @brief 
 * 
 */
class util_timer 
{
public: 
	util_timer():expire(0), user_data(nullptr),prev(nullptr), next(nullptr){}
public:
/*< 任务超时时间，使用绝对时间*/
	time_t expire;
/*< 任务回调函数*/
	void (*cb_func)(client_data *);
/*< 回调函数处理客户数据，由定时器的执行者传递给回调函数*/
	client_data *user_data;
	util_timer *prev;
	util_timer *next;
};

class sort_timer_list 
{
public:
	sort_timer_list():head(nullptr), tail(nullptr){}
	~sort_timer_list();
/*< 将目标定时器timer添加到链表中*/
	void add_timer(util_timer *timer);
/*< 当某个定时任务发生变化时，调整对应定时器在链表中的位置，此函数只考虑被调整的定时器超时时间延长的情况*/
	void adjust_timer(util_timer *timer);
/*< 将目标定时器timer从链表中删除*/
	void del_timer(util_timer *timer);
/*< SIGALARM信号每次被处罚就在其信号处理函数中执行一次tick函数，处理链表上到期的任务（若采用统一事件源，则该信号处理函数就是主函数）*/
	void tick();

private:
/*< 重载的辅助函数，用于保护头节点时对链表进行修改*/
	void add_timer(util_timer *timer, util_timer *lst_head);
private:
	util_timer *head;
	util_timer *tail;
};
#endif