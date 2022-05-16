/**
 * @file time_wheel.h
 * @author vilewind 
 * @brief 固定的频率调用tick，并以此检测到期的定时器，并执行对应的回调函数
 * @version 0.1
 * @date 2022-05-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __TIME_WHEEL_H__
#define __TIME_WHEEL_H__

#include "../general_header.h"

const static int BUFFER_SIZE = 64;

class tw_timer;

struct client_data
{
	sockaddr_in addr;
	int fd;
	char buf[BUFFER_SIZE];
	tw_timer *timer;
};

class tw_timer
{
public:
	tw_timer *next;
	tw_timer *prev;
	int rotation;
	int time_slot;
	client_data *user_data;
public:
	tw_timer(int rot, int ts) : next(nullptr), prev(nullptr), rotation(rot), time_slot(ts), user_data(nullptr){}
	void (*cb_func)(client_data *);
};

class time_wheel
{
public:
	time_wheel();
	~time_wheel();
	tw_timer* add_timer(int timeout);
	void del_timer(tw_timer *timer);
/**
 * @brief SI时间到达，调用该函数，时间轮向前滚动一个槽的间隔
 * 
 */
	void tick();


private:
/*< 时间轮槽数*/
	static const int N = 60;
/*< 时间轮指针步长*/
	static const int SI = 1;
/*< 时间轮每个槽对应的链表头节点,链表无序*/
	tw_timer *slots[N];
/*< 时间轮的当前槽*/
	int cur_slot;
};

#endif
