
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

typedef struct pos_stack
{
	int pos[SCREEN_SIZE];
	int ptr;
}PSTACK;

typedef struct char_stack
{
	char input[SCREEN_SIZE];
	int ptr;
}CSTACK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			/* 当前光标位置 */
	int				mode;
	int				isBlocked;
	PSTACK			pStack;
	CSTACK			cStack;
}CONSOLE;

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */
#define RED_CHAR_COLOR	0x04		/* 0000 0100 黑底红字 */
#define TAB_CHAR_COLOR	0x01		/* 用蓝色标记tab，以区分空格 */

#define SEARCH_MODE	1
#define NORMAL_MODE	0
#define UNBLOCKED 2
#define BLOCKED -1


#endif /* _ORANGES_CONSOLE_H_ */
