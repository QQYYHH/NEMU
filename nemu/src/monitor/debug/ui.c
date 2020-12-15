#include <isa.h>
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MALLOC_PTR malloc(5 * sizeof(int))
void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

int parse_args(char *str, char **ags){
  if(str == NULL) return 0;
  char *str_end = str + strlen(str);
  int cnt = 0;
  while(str < str_end){
    str = strtok(str, " ");
    ags[cnt++] = str;
    str = str + strlen(str) + 1;
  }
  return cnt;
}

int str2num(char *str){
  int len = strlen(str);
  int num = 0;
  for(int i = 0; i < len; i++){
    Assert(str[i] >= '0' && str[i] <= '9', "illegal input!!!");
    num = num*10 + str[i] - '0';
  }
  return num;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}
int cmd_si(char *args){
  char **ags = MALLOC_PTR;
  int len = parse_args(args, ags);
  Assert(len <= 1, "too much arguments!!!");
  switch (len)
  {
  case 0:
    cpu_exec(1);
    break;
  case 1:
    {
      int steps = str2num(ags[0]);
      cpu_exec(steps);
      break;
    }
  default:
    break;
  }
  free(ags);
  extern void monitor_instruction_num();
  monitor_instruction_num();
  return 0;
}

static int cmd_info(char *args){
  extern void isa_reg_display();
  // 先直接打印 all registers
  isa_reg_display();
  return 0;
}
static int cmd_p(char *args){
  return -1;
}
static int cmd_x(char *args){
  return -1;
}
static int cmd_w(char *args){
  return -1;
}
static int cmd_d(char *args){
  return -1;
}
static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "execute the program step by step",  cmd_si}, 
  { "info", "print registers' status",  cmd_info}, 
  { "p", "calculate the value of indicate expression",  cmd_p},
  { "x", "x N EXPR: calculate the val of EXPR, using the result as the start mem addresss and output \
    N consecutive 4 bytes in hexademical form",  cmd_x}, 
  { "w", "w EXPR: Pause the program when the value of EXPR changes", cmd_w},
  { "d", "d N : delete the watch point which serial number is N", cmd_d},
  

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
