#include <isa.h>
#include "expr.h"
#include "watchpoint.h"
#include <memory/paddr.h>
#include <time.h>

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MALLOC_PTR malloc(5 * sizeof(int))
#define pt(fmt, ...) printf(fmt, ##__VA_ARGS__)

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
/*
str : the string to parse to arguments
ags : pointers array
each pointer is a char pointer, 
indicate the first char of the argument string.
*/
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
word_t he2de(char *he){ // 十六进制 转 十进制
  int len = strlen(he);
  word_t res = 0;
  for(int i = 2; i < len; i++){
    int num;
    if(he[i] >= '0' && he[i] <= '9') num = he[i] - '0';
    else num = 10 + he[i] - 'a';
    res = res * 16 + num;
  }
  return res;
}
int de2he(word_t de, char *he){ // 十进制 转 十六进制
  he[0] = '0';
  he[1] = 'x';
  int len = 2;
  while(de){
    int re = de % 16;
    char ch;
    if(re >= 10) ch = re - 10 + 'a';
    else ch = re + '0';
    he[len++] = ch;
    de /= 16;
  }
  int i = 2, j = len - 1;
  while(i < j){ // reverse
    char ch = he[i];
    he[i] = he[j];
    he[j] = ch;
    i++;
    j--;
  }
  he[len] = '\0';
  return len;
}
/*
  解析地址
  将 十六进制地址转换为10进制
  或者 解析为相应寄存器中的内容（寄存器内此时存放的应该是地址）
  返回 模拟器的物理内存地址，也就是 pmem数组的 index
*/
paddr_t analyze_addr(char *addr){
  paddr_t res = 0;
  if(addr[0] == '$'){ // 寄存器
    extern word_t isa_reg_str2val(const char *, bool*);
    bool *success = false;
    res = isa_reg_str2val(addr + 1, success);
  }
  else{ // 十六进制地址
    res = he2de(addr);
    pt("paddr : %u\n", res);
  }
  return res;
}

/*
  从 pmem_index开始，打印连续n 个 4字节的数据
  每行输出 4个, 16B, 相当于16个寻址单位, 0x10
  static uint8_t pmem[PMEM_SIZE] PG_ALIGN = {}
*/

void scan_mem(int n, paddr_t pmem_index){ 
  int meet_boundary = 0; // 是否输出到内存末端
  paddr_t cur_index = pmem_index;
  int re = n % 4, line = n / 4;
  char title[15] = {0};
  // char data_0x[15] = {0};
  for(int i = 0; i < line; i++){
    int len = de2he(cur_index, title);
    Assert(len > 0, "wrong on translating de to he");
    pt("%s : ", title);
    for(int j = 0; j < 4; j++){ // 打印每行的4个4B数据
      if(cur_index >= PMEM_SIZE + PMEM_BASE){ // 超出物理内存范围
        meet_boundary = 1;
        break;
      }
      word_t data = paddr_read(cur_index, 4);
      // int len = de2he(data, data_0x);
      // Assert(len > 0, "wrong on translating de to he");
      pt("%0x ", data);
      cur_index += 4;

    }
    putchar('\n');
    if(meet_boundary) break;
  }
  if(meet_boundary) return;
  // 打印余下的4B 数据
  int len = de2he(cur_index, title);
  Assert(len > 0, "wrong on translating de to he");
  pt("%s : ", title);
  for(int i = 0; i < re; i++){
    if(cur_index >= PMEM_BASE + PMEM_SIZE){
      break;
    }
    word_t data = paddr_read(cur_index, 4);
    // int len = de2he(data, data_0x);
    // Assert(len > 0, "wrong on translating de to he");
    cur_index += 4;
    pt("%0x ", data);
  }
  putchar('\n');
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
  char **ags = MALLOC_PTR;
  int len = parse_args(args, ags);
  Assert(len <= 1, "too much arguments!!!");
  if(len == 1){
    Assert(ags[0][0]=='r' || ags[0][0]=='w', "wrong arguments!!!");
  }
  if(!len || (len == 1 && ags[0][0] == 'r')){ // print registers
    extern void isa_reg_display();
    // 先直接打印 all registers
    isa_reg_display();
  }
  else{ // print watchers
    print_all_watchpoint();
  }
  free(ags);
  return 0;
}

// 主要是表达式求值
static int cmd_p(char *args){
  if(NULL == args) return 0;
  extern word_t expr(char *, bool *);
  bool success = true;
  word_t res = expr(args, &success);
  Assert(success, "there is wrong when calculating the expression");
  pt("the \"%s\" value is %u \n", args, res);
  return 0;
}
/*
  打印 从某一起始地址开始 连续 n 个 4字节的内容（十六进制输出）
  为了方便调试，我先用10进制输出
  地址 ： 内容
  每一行输出 4个 4字节内容（共16B，16个编址单位 0x10）
  输入的地址是 客户机 物理地址 paddr 
*/
static int cmd_x(char *args){
  if(NULL == args) return 0;
  char *args_end = args + strlen(args);
  char *_n = strtok(args, " ");
  size_t len = strlen(_n);
  char *expr_start = args + len + 1;
  if(expr_start >= args_end) return 0;

  int n = str2num(_n); // n 个 4字节
  extern word_t expr(char *, bool *);
  bool success = false;
  word_t val = expr(expr_start, &success); // 算出来起始物理地址（模拟机）
  Assert(success, "there is wrong when calculating the expression");
  paddr_t paddr = val;
  scan_mem(n, paddr);
  return 0;
}


static int cmd_w(char *args){
  if(NULL == args) return 0;
  new_wp(args);
  print_all_watchpoint();
  return 0;
}
static int cmd_d(char *args){
  if(NULL == args) return 0;
  int num = str2num(args);
  bool success = delete_watchpoint(num);
  if(!success){
    printf("no such watchpoint num\n");
    return 0;
  }
  print_all_watchpoint();
  return 0;
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
    while(*args == ' ' && args < str_end) args++;
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
