
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

PRIVATE int targetLen;
PRIVATE int tmpLen;
PRIVATE char target[SCREEN_SIZE];
PRIVATE char tmp[SCREEN_SIZE];

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	targetLen = 0;
	tmpLen = 0;
	select_console(0);
	while (1) {
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
			put_key(p_tty, key);
        }
        else {
            int raw_code = key & MASK_RAW;
			// 屏蔽时只有esc不屏蔽，因而单独拿出
			if(raw_code == ESC){
				if(p_tty->p_console->mode == SEARCH_MODE){
					p_tty->p_console->mode = NORMAL_MODE;
					p_tty->p_console->isBlocked = UNBLOCKED;
					endSearch(p_tty->p_console, targetLen);
					targetLen = 0;
					tmpLen = 0;
				}
				else if(p_tty->p_console->mode == NORMAL_MODE){
					p_tty->p_console->mode = SEARCH_MODE;
				}
			}
			if(p_tty->p_console->isBlocked == UNBLOCKED){
				switch(raw_code) {
					case ENTER:
						if(p_tty->p_console->mode == SEARCH_MODE){
							p_tty->p_console->isBlocked = BLOCKED;
							search(p_tty->p_console, target, targetLen);
						}
						else{
							put_key(p_tty, '\n');
						}
						break;
					case TAB:
						put_key(p_tty, '\t');
						break;
					case BACKSPACE:
						if(tmpLen > 0 || p_tty->p_console->mode != SEARCH_MODE){
							put_key(p_tty, '\b');
						}
						else{
							// 删完了红色的就不能再删了
							if(targetLen > 0){
								tmpLen--;
								targetLen--;
							}
						}
						break;
					case UP:
						if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_DN);
						}
						break;
					case DOWN:
						if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_UP);
						}
						break;
					case CTRL_Z:
						if(tmpLen != 0 || p_tty->p_console->mode != SEARCH_MODE){
							undoOne(p_tty->p_console);
							// 查找模式里撤销+退格同时出现的被迫特殊处理
							if(tmp[tmpLen - 1] != '\b'){
								targetLen--;
								tmpLen--;
							}
							else{
								int cnt = 0;
								// 万恶的撤销连续退格情况
								while(tmp[tmpLen - 1 - cnt] == '\b'){
									cnt++;
								}
								for(int j = cnt - 1; j >= 0; j--){
									target[targetLen++] = tmp[tmpLen - 1 - cnt - j];
								}
							}
						}
						break;
					case F1:
					case F2:
					case F3:
					case F4:
					case F5:
					case F6:
					case F7:
					case F8:
					case F9:
					case F10:
					case F11:
					case F12:
						/* Alt + F1~F12 */
						if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
							select_console(raw_code - F1);
						}
						break;
					default:
						break;
				}
			}
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;
		if(p_tty->p_console->mode == SEARCH_MODE && p_tty->p_console->isBlocked == UNBLOCKED) {
			// 搜索模式记录搜索文本，退格则长度指针减一，否则记录一个，其中tab被记录为'\t'
			if(ch=='\b'){
				if(targetLen > 0) targetLen--;
			}
			else{
				target[targetLen++] = ch;
			}
			tmp[tmpLen++] = ch;
		}
		out_char(p_tty->p_console, ch);
	}
}

/*======================================================================*
                              tty_write
*======================================================================*/
PUBLIC void tty_write(TTY* p_tty, char* buf, int len)
{
        char* p = buf;
        int i = len;

        while (i) {
                out_char(p_tty->p_console, *p++);
                i--;
        }
}

/*======================================================================*
                              sys_write
*======================================================================*/
PUBLIC int sys_write(char* buf, int len, PROCESS* p_proc)
{
        tty_write(&tty_table[p_proc->nr_tty], buf, len);
        return 0;
}

/*======================================================================*
							 clean console
*======================================================================*/
PUBLIC void task_refresh(TTY* p_tty){
	while (1) {
		milli_delay(110000);
		for (TTY* p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			while (p_tty->p_console->mode == SEARCH_MODE) {}
			milli_delay(10000);
			cleanConsole(p_tty->p_console);
		}
	}
}
