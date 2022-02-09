
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE void changeRed(u8* start, int len);
PRIVATE int popPstack(CONSOLE* p_con);
PRIVATE void pushPstack(CONSOLE* p_con, int pos);
PRIVATE void pushCstack(CONSOLE* p_con, char ch);
PRIVATE char popCstack(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	/* 初始化为普通模式、不屏蔽输入、位置栈和操作内容栈指针均为0 */
	p_tty->p_console->mode = NORMAL_MODE;
	p_tty->p_console->isBlocked = UNBLOCKED;
	p_tty->p_console->pStack.ptr = 0;
	p_tty->p_console->cStack.ptr = 0;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
	/* 清空初始化打印的信息 */
	cleanConsole(p_tty->p_console);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	if(p_con->isBlocked != UNBLOCKED) return;
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	if(p_con->mode == SEARCH_MODE && *(p_vmem - 1) != RED_CHAR_COLOR && ch == '\b'){
		return;
	}
	char color = DEFAULT_CHAR_COLOR;
	pushCstack(p_con, ch);	// 操作内容入栈
	switch(ch) {
	case '\n':
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
			pushPstack(p_con, p_con->cursor);
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		break;
	// 退格逻辑：从地址栈弹出光标上一个位置，并将位置差中全部置为空格，并把光标也移至上一个位置
	case '\b':
		if (p_con->cursor > p_con->original_addr) {
			int cur = p_con->cursor;
			p_con->cursor = popPstack(p_con);
			for(int i = 0; i < cur - p_con->cursor; i++){
				*(p_vmem-2-2*i) = ' ';
				*(p_vmem-1-2*i) = DEFAULT_CHAR_COLOR;
			}
		}
		break;
	// TAB
	case '\t':
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 4){
			pushPstack(p_con, p_con->cursor);
			for(int i = 0; i < 4; i++) {
				*p_vmem++ = ' ';
				*p_vmem++ = TAB_CHAR_COLOR;
				p_con->cursor++;
			}
		}
		break;
	default:
		// 搜索模式输入改为红色
		if(p_con->mode == SEARCH_MODE){
			color = RED_CHAR_COLOR;
		}
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			pushPstack(p_con, p_con->cursor);
			*p_vmem++ = ch;
			*p_vmem++ = color;
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	if (is_current_console(p_con)) {
		set_cursor(p_con->cursor);
		set_video_start_addr(p_con->current_start_addr);
	}
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	flush(&console_table[nr_console]);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	flush(p_con);
}

/*======================================================================*
			   cleanConsole
 *======================================================================*/
PUBLIC void cleanConsole(CONSOLE* p_con){
	u8* p_vmem;
	while(p_con->cursor > p_con->original_addr){
		p_vmem = (u8*)(p_con->cursor * 2 + V_MEM_BASE);
		p_con->cursor -= 1;
		*(p_vmem - 2) = ' ';
		*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
	}
	p_con->cursor = p_con->current_start_addr;
	flush(p_con);
}

/*======================================================================*
			   search
 *======================================================================*/
PUBLIC void search(CONSOLE* p_con, char* target, int len){
	// 直接暴力匹配
	for(int i=p_con->original_addr; i<p_con->cursor - len * 2 + 1; i++){
		u8* cur = (u8*)(V_MEM_BASE + i * 2);
		if(*(cur+1) == RED_CHAR_COLOR){
			continue;
		}
		int found = 1;
		int tap_num = 0;
		for(int j=0;j<len;j++){
			// TAB特殊处理，利用先前随便使用的另外一种颜色
			if(target[j] == '\t'){
				cur++;
				for(int i=0;i<4;i++){
					if(*cur != TAB_CHAR_COLOR){
						found = 0;
						break;
					}
					cur += 2;
				}
				cur--;
				tap_num++;
			}
			else if(*cur == target[j] && *(cur+1) != TAB_CHAR_COLOR){
				cur += 2;
			}
			else{
				found = 0;
				break;
			}
		}
		if(found == 1){
			cur = (u8*)(V_MEM_BASE + i * 2 + 1);
			changeRed(cur, len + 3 * tap_num);	// 搜索到的目标由默认改为红色
		}
	}
}

PRIVATE void changeRed(u8* start, int len){
	for (int i=0;i<len;i++){
		if(*start != TAB_CHAR_COLOR){
			*start = RED_CHAR_COLOR;
		}
		start += 2;
	}
}

PUBLIC void endSearch(CONSOLE* p_con, int len){
	// 删除搜索模式后的输入
	for (int i=0;i<len;i++){
		out_char(p_con, '\b');
	}
	// 将剩余全部输入文本置为默认颜色，TAB别改颜色
	for (int i=p_con->original_addr;i<p_con->cursor;i++){
		u8* start = (u8*)(V_MEM_BASE + i * 2) + 1;
		if(*start != TAB_CHAR_COLOR){
			*start = DEFAULT_CHAR_COLOR;
		}
	}
}

// 实现地址栈和操作信息栈的push和pop方法
PRIVATE void pushPstack(CONSOLE* p_con, int pos){
	p_con->pStack.pos[p_con->pStack.ptr++] = pos;
}

PRIVATE int popPstack(CONSOLE* p_con){
	if(p_con->pStack.ptr < 1){
		return 0;
	}
	else{
		p_con->pStack.ptr--;
		return p_con->pStack.pos[p_con->pStack.ptr];
	}
}

PRIVATE void pushCstack(CONSOLE* p_con, char ch){
	p_con->cStack.input[p_con->cStack.ptr++] = ch;
}

PRIVATE char popCstack(CONSOLE* p_con){
	if(p_con->cStack.ptr < 1){
		return 0;
	}
	else{
		p_con->cStack.ptr--;
		return p_con->cStack.input[p_con->cStack.ptr];
	}
}
// 撤销：当上一个操作不是退格时直接执行一个退格即可，当上一个操作是退格时打印等于连续退格数量的内容
PUBLIC void undoOne(CONSOLE* p_con){
	char ch = popCstack(p_con);
	if(ch == 0) return;
	if(ch != '\b'){
		out_char(p_con, '\b');
		popCstack(p_con);
	}
	else{
		int cnt = 0;
		while(ch == '\b'){
			ch = popCstack(p_con);
			cnt++;
		}
		pushCstack(p_con, ch);
		char chs[SCREEN_SIZE];
		for(int i = 0; i < cnt; i++){
			ch = popCstack(p_con);
			chs[i] = ch;
		}
		for(int i = cnt - 1; i >= 0; i--){
			out_char(p_con, chs[i]);
			popCstack(p_con);
		}
	}
}