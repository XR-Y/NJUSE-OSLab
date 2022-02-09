
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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

// TODO: 参数设置
#define READ_FIRST FALSE
#define MAX_READER_NUM 2
#define STARVATION TRUE

SEMAPHORE* rmutex, max_reader, wmutex;
SEMAPHORE* readcount_control, writecount_control, x;
int readcount, writecount, reading_num;

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	disp_pos = 0;
	for (i = 0; i < 80*25; i++) {
		disp_str(" ");
	}
	disp_pos = 0;

	init_sem(&max_reader, MAX_READER_NUM);
	init_sem(&rmutex, 1);
	init_sem(&wmutex, 1);
	init_sem(&readcount_control, 1);
	init_sem(&writecount_control, 1);
	init_sem(&x, 1);

	readcount = 0;
	writecount = 0;
	reading_num = 0;

	k_reenter = 0;
	ticks  =  0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

	while(1){}
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA(){
	reader("A", 2);
}
/*======================================================================*
                               TestB
 *======================================================================*/
void TestB(){
	reader("B", 3);
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC(){
	reader("C", 3);
}
/*======================================================================*
                               TestD
 *======================================================================*/
void TestD(){
	writer("D", 3);
}
/*======================================================================*
                               TestE
 *======================================================================*/
void TestE(){
	writer("E", 4);
}

/*======================================================================*
                               TestF
 *======================================================================*/
void TestF(){
	while(1){
		disp_str("F-Print: ");
		if(readcount > 0){
			char x[2];
			x[0] = reading_num + '0';	// readcount是阻塞+在读进程总数，reading_num是实际在读进程数
			x[1] = '\0';
			disp_str(x);
			disp_str(" process reading!    ");
			
		}
		else{
			// 只可能同时有一个进程在写操作
			disp_str("1 process writing!    ");
		}
		sleep(TIME_SLICE);
		if(disp_pos >= 80 * 50) clean_screen();
	}
}

void reader(char*name, int slice_num){
	while(1){	
		// if(READ_FIRST && !STARVATION){
		if(READ_FIRST){
			//读者优先
			myprint(name);
			myprint(" arrives!    ");
			p(&rmutex);
			if(readcount == 0) {
				p(&wmutex);
			}
			readcount ++ ;
			v(&rmutex);
			// 正在读，控制最多读者数
			p(&max_reader);
			reading_num++;
			myprint(name);
			myprint(" is reading!    ");
			milli_delay(slice_num * TIME_SLICE);
			reading_num--;
			v(&max_reader);
			// 读完
			p(&rmutex);
			readcount--;
			if(readcount == 0) v(&wmutex);
			v(&rmutex);
			myprint(name);
			myprint(" finish reading!    ");
			// 饥饿问题
			if(STARVATION){
				milli_delay(TIME_SLICE * (NR_TASKS - 1));
			}
		}
		else{
			//写者优先
			myprint(name);
			myprint(" arrives!    ");

			p(&x); // 只让一个读进程在rmutex上排队
			p(&rmutex);
			p(&readcount_control); // 互斥对readcount的修改
			readcount++;
			if(readcount == 1) {
				p(&wmutex);	// wmutex和rmutex保持互斥
			}
			v(&readcount_control);
			v(&rmutex);
			v(&x);

			p(&max_reader);
			reading_num++;
			myprint(name);
			myprint(" is reading!    ");
			milli_delay(slice_num * TIME_SLICE);
			reading_num--;
			v(&max_reader);

			p(&readcount_control);
			readcount--;
			if(readcount == 0) v(&wmutex);
			v(&readcount_control);
			myprint(name);
			myprint(" finish reading!    ");
		}
	}
}

void writer(char*name, int slice_num){
	while(1){
		if(READ_FIRST){
			//读者优先
			myprint(name);
			myprint(" arrives!    ");
			p(&wmutex);
			myprint(name);
			myprint(" is writing!    ");
			milli_delay(slice_num * TIME_SLICE);	
			v(&wmutex);
			myprint(name);
			myprint(" finish writing!    ");
		}
		// else if(STARVATION){
		// 	myprint(name);
		// 	myprint(" arrives!    ");
		// 	p(&x);
		// 	p(&wmutex);
		// 	myprint(name);
		// 	myprint(" is writing!    ");
		// 	milli_delay(slice_num * TIME_SLICE);	
		// 	v(&wmutex);
		// 	v(&x);
		// 	myprint(name);
		// 	myprint(" finish writing!    ");
		// }
		else{
			//写者优先
			myprint(name);
			myprint(" arrives!    ");
			p(&writecount_control);
			writecount++;
			if(writecount == 1) p(&rmutex);
			v(&writecount_control);

			p(&wmutex);
			myprint(name);
			myprint(" is writing!    ");
			milli_delay(slice_num * TIME_SLICE);
			v(&wmutex);

			p(&writecount_control);
			writecount--;
			if(writecount==0) v(&rmutex);
			v(&writecount_control);
			myprint(name);
			myprint(" finish writing!    ");

			if(STARVATION){
				milli_delay(TIME_SLICE * (NR_TASKS - 1));
			}
		}
	}

}

void clean_screen()
{
	disp_pos = 0;
	for (int i = 0; i < 80*50; i++) {
		disp_str(" ");
	}
	disp_pos = 0;
}
