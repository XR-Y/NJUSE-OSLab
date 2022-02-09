
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

PUBLIC int colors[NR_TASKS] = {0x09, 0x02, 0x0D, 0x0B, 0x0E, 0x06};
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p = p_proc_ready;
	while(1){
		int t = get_ticks();
		p++;
		// 超出范围则回到第一个
		if(p >= proc_table + NR_TASKS){
			p = proc_table;
		}
		// 遍历搜索，找到一个不在睡眠且没被阻塞的则跳出循环
		if(p->wait_sem == 0 && p->wake_time <= t){
			// myprint(p->p_name);
			p_proc_ready = p;
			break;
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}
/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int  milli_seconds){
	p_proc_ready->wake_time = get_ticks() + milli_seconds / 1000 * HZ;	// 修正
	schedule();
}
/*======================================================================*
                           sys_myprint
 *======================================================================*/
// 颜色输出
PUBLIC void sys_myprint(char * str){
	int color = colors[p_proc_ready - proc_table];
	disp_color_str(str, color);
}
/*======================================================================*
                           sys_p
 *======================================================================*/
PUBLIC void sys_p(SEMAPHORE* s){
	disable_irq(CLOCK_IRQ);
	s->value--;
	if(s->value < 0){
		p_proc_ready->wait_sem = 1;
		s->list[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % SEMAPHORE_SIZE;
		schedule();
	}
	enable_irq(CLOCK_IRQ);
}
/*======================================================================*
                           sys_v
 *======================================================================*/
PUBLIC void sys_v(SEMAPHORE* s){
	disable_irq(CLOCK_IRQ);
	s->value++;
	if(s->value <= 0){
		s->list[s->head]->wait_sem = 0;
		s->head = (s->head + 1) % SEMAPHORE_SIZE;
	}
	enable_irq(CLOCK_IRQ);
}
// 初始化信号量
PUBLIC  void init_sem(SEMAPHORE*s, int val){
	s->value = val;
	s->tail = 0;
	s->head = 0;
}

// void restartAll()
// {
// 	PROCESS* p;
// 	for(p = proc_table; p < proc_table + NR_TASKS - 1; p++){
// 		if(!p->isFinished) return;
// 	}
// 	myprint("reset!");
// 	myprint("\n");
// 	for(p = proc_table; p < proc_table + NR_TASKS; p++){
// 		p->isFinished = FALSE;
// 	}
// }