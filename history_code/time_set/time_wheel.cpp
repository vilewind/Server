#include "time_wheel.h"

time_wheel::time_wheel():cur_slot(0) {
	for (int i = 0; i < N; ++i) {
		slots[i] = nullptr;
	}
}

time_wheel::~time_wheel(){
	for (int i = 0; i < N; ++i) {
		tw_timer *tmp = slots[i];
		while(tmp) {
			slots[i] = tmp->next;
			delete tmp;
			tmp = slots[i];
		}
	}
}

tw_timer* time_wheel::add_timer(int timeout) {
	if (timeout < 0)
		return nullptr;

	int ticks = 0;
/*< 根据带插入定时器的超时值计算其将在时间轮转动多少个滴答后被触发，并将滴答数存储于变量ticks中。若带插入定时器的超时值小于时间轮的槽间隔SI，则ticks进一，否则就折合为timeout/SI*/
	if (timeout < SI) {
		ticks = 1;
	} else {
		ticks = timeout / SI;
	}
/*< 计算待插入定时器在时间轮转动多少圈后被触发*/
	int rotation = ticks / N;
/* 计算待插入的定时器应该被插入到哪个槽*/
	int ts = (cur_slot + (ticks % N) + 1) % N;

	tw_timer *timer = new tw_timer(rotation, ts);

	if (!slots[ts]) {
		std::cout << "add timer, rotation is " << rotation << " ts is " << ts << " cur_slot is " << cur_slot << std::endl;
		slots[ts] = timer;
	} else {
		timer->next = slots[ts];
		slots[ts]->prev = timer;
		slots[ts] = timer;
	}

	return timer;
}

void time_wheel::del_timer(tw_timer* timer) {
	if (!timer)
		return;
	int ts = timer->time_slot;
	if (timer == slots[ts]) {
		slots[ts] = slots[ts]->next;
		if (slots[ts])
			slots[ts]->prev = nullptr;
		delete timer;
	} else {
		timer->prev->next = timer->next;
		if (timer->next) {
			timer->next->prev = timer->prev;
		}
		delete timer;
	}
}

void time_wheel::tick() {
	tw_timer *tmp = slots[cur_slot];
	std::cout << "current slot is " << cur_slot << std::endl;

	while(tmp) {
		std::cout << "tick the timer once" << std::endl;

		if (tmp->rotation > 0) {
			--tmp->rotation;
			tmp = tmp->next;
		} else {
			tmp->cb_func(tmp->user_data);
			if (tmp==slots[cur_slot]) {
				std::cout << " delete header int cur_slot" << std::endl;
				slots[cur_slot] = tmp->next;
				delete tmp;

				if (slots[cur_slot]) {
					slots[cur_slot]->prev = nullptr;
				}
				tmp = slots[cur_slot];
			}  else {
				tmp->prev->next = tmp->next;
				if (tmp->next) {
					tmp->next->prev = tmp->prev;
				}
				tw_timer *tmp2 = tmp->next;
				delete tmp;
				tmp = tmp2;
			}
		}
	}
	cur_slot = ++cur_slot % N;
}