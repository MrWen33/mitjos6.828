// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "lifegame", "LifeGame demo", mon_lifegame}
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int get_status(int* status, int x, int y, int line)
{
	if(x<0||y<0||x>=line||y>=line){
		return 0;
	}
	return status[x+y*line];
}

int neighbors(int* status, int location, int line){	
	int x = location%line;
	int y = location/line;
	int dx[] = {0,1,1,1,0,-1,-1,-1};
	int dy[] = {-1,-1,0,1,1,1,0,-1};
	int dir = 0;
	int result = 0;
	for(;dir<8;++dir){
		int cx = x+dx[dir];
		int cy = y+dy[dir];
		result+=get_status(status, cx, cy, line);
	}
	return result;
}

extern int kbd_proc_data();

#define STATUS_NUM 4
const int init_status[STATUS_NUM][400]={
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	},
	{
		0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	},
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	},
	{
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,
		0,1,0,0,1,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
		0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,
		0,0,0,1,1,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
		0,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,
		0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
		0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,0,0,
		0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,1,0,1,0,0,
		0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,
		0,0,1,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,
		0,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,0,0,0,0,
		0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
		0,0,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
	}
	};/* 表示游戏状态.方格20x20 */

int 
mon_lifegame(int argc, char **argv, struct Trapframe *tf)
{
	int init_code=0;
	if(argc>1){
		init_code = argv[1][0]-'0';
	}
	if(init_code>=STATUS_NUM){
		return 0;
	}
	int line = 20;
	int status[line*line];
	memcpy(status, init_status[init_code], sizeof(status));
	int new_status[line*line];
	char screen_buf[(line+1)*(line+1)];/* 屏幕缓冲区字符串 */
	char reset[3*line+1];/* 复位字符串 */
	int i=line;
	int exitflag = 0;
	while(i--){
		strcat(reset, "\r\b\r");
	}
	/* 逻辑与渲染循环 */
	for(;;){
		/* 将游戏状态转换为字符串 */
		memset(screen_buf, 0, sizeof(screen_buf));
		int x;
		int y;
		for(y=0;y<line;++y){
			for(x=0;x<line;++x){
				if(status[x+y*line]){
					strcat(screen_buf, "\333");
				}else{
					strcat(screen_buf, " ");
				}
			}
			strcat(screen_buf, "\n");
		}

		/* 打印游戏状态 */
		cprintf(screen_buf);

		/* TODO:更新游戏状态 */
		int i = 0;
		for(i=0;i<line*line;i++){
			if(neighbors(status, i, line) == 2)
				new_status[i] = status[i];
			else if(neighbors(status, i, line) == 3)
				new_status[i] = 1;
			else if(neighbors(status, i, line) > 3)
				new_status[i] = 0;
			else
				new_status[i] = 0;
		}
		
		for(i=0;i<line*line;i++)		
			status[i] = new_status[i];

		/* 死循环模拟睡眠 */
		int sleep=10000000;
		while(sleep--){
			/* 输入Ctrl+C时退出游戏 */
			if(kbd_proc_data() == 3){
				exitflag=1;
				break;
			}
		}
		if(exitflag){
			break;
		}
		/* 复位光标 */
		cprintf(reset);
	}
	
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
