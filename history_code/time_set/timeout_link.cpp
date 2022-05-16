#include "timeout_link.h"

void sort_timer_list::add_timer(util_timer* timer, util_timer* lst_head) {
	util_timer *prev = lst_head;
	util_timer *tmp = prev->next;

	while(tmp) {
		if (timer->expire < tmp->expire) {
			prev->next = timer;
			timer->next = tmp;
			tmp->prev = timer;
			timer->prev = prev;
			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}
/*< 若遍历完后没有合适的中间位置，则将目标插入尾部，并设置为新的尾部节点*/
	if (!tmp) {
		prev->next = timer;
		timer->prev = prev;
		timer->next = nullptr;
		tail = timer;
	}
}

void sort_timer_list::add_timer(util_timer *timer) {
	if (!timer)
		return;
	if (!head) {
		head = tail = timer;
		return;
	}
/*< 若新添加的timer小于所有的节点，则将其作为头节点（使用overload add_timer)*/
	if (timer->expire < head->expire) {
		timer->next = head;
		head->prev = timer;
		head = timer;
		return;
	}
	add_timer(timer, head);
}

void sort_timer_list::adjust_timer(util_timer* timer) {
	if(!timer)
		return;
	util_timer *tmp = timer->next;
/*< 处于尾部或者仍旧小于下一节点的超时值*/
	if (!tmp || (timer->expire < tmp->expire)) {
		return;
	}
/*< 若目标是头节点，则将该定时器从链表中取出并重新插入链表*/
	if (timer == head) {
		head = head->next;
		head->prev = nullptr;
		timer->next = nullptr;
		add_timer(timer, head);
	} else {
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		add_timer(timer, timer->next);
	}
}

void sort_timer_list::del_timer(util_timer* timer) {
	if(!timer)
		return;
/*< 此时链表中只有一个定时器，即目标定时器*/
	if ((timer == head) && (timer == tail)) {
		delete timer;
		head = nullptr;
		tail = nullptr;
		return;
	} 
/*< 链表中有多个节点*/	
	else if(timer == head) {
		head = head->next;
		head->prev = nullptr;
		delete timer;
		return;
	}
/*< 链表中有多个节点*/
	else if (timer == tail) {
		tail = tail->prev;
		tail->next = nullptr;
		delete timer;
		return;
	} 
/*< 目标节点位于中间位置*/
	else {
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		delete timer;
	}
}

void sort_timer_list::tick() {
	if(!head)
		return;
	std::cout << "timer ticked " << std::endl;
	time_t cur = time(nullptr);
	util_timer *tmp = head;

	while(tmp) {
		if (cur < tmp->expire) {
			break;
		}

		tmp->cb_func(tmp->user_data);

		head = tmp->next;
		if (head) {
			head->prev = nullptr;
		}
		delete tmp;
		tmp = head;
	}
}