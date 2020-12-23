#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  struct watchpoint *pre;

  /* TODO: Add more members if necessary */
  char watch_expr[100]; // 监视的表达式
  word_t pre_val; // 上一次表达式的值，用于做比较

} WP;

WP* new_wp(char *);
bool delete_watchpoint(int);
bool check_watchpoint();
void print_all_watchpoint();

#endif
