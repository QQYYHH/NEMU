#include "watchpoint.h"
#include "expr.h"
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
static int num_watchpoint = 0; // 当前用到的 监测点，也就是 head 链表的长度

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    if(!i) wp_pool[i].pre = NULL;
    else wp_pool[i].pre = &wp_pool[i - 1];
    if(i == NR_WP - 1) wp_pool[i].next = NULL;
    else wp_pool[i].next = &wp_pool[i + 1];
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
// 申请 监视点，从 free_ 移动到 head，链表操作
// 同时初始化 表达式

extern word_t expr(char *, bool *);

WP* new_wp(char *w_expr){
  Assert(free_ != NULL, "can't new a watchpoint");

  WP *nxt = free_->next;
  if(NULL != nxt) nxt->pre = NULL;
  free_->next = head;
  if(NULL != head) head->pre = free_;
  head = free_;
  free_ = nxt;

  num_watchpoint++;
  strcpy(head->watch_expr, w_expr);
  bool success = true;
  word_t res = expr(head->watch_expr, &success);
  Assert(success, "there is wrong when calculating the value of expression");
  head->pre_val = res;
  return head;
}


// 释放指定的 监视点
// 指定监视点 head 转移到 free_
void free_wp(WP *wp){
  WP *nxt = wp->next;
  WP *pre = wp->pre;
  if(NULL != pre) pre->next = nxt;
  if(NULL != nxt) nxt->pre = pre;
  if(wp == head){ // 如果是头节点
    head = nxt;
  }
  if(NULL != free_) free_->pre = wp;
  wp->next = free_;
  free_ = wp;

  num_watchpoint--;
}

bool delete_watchpoint(int num){
  WP *cur = head;
  WP *wp = NULL;
  while(NULL != cur){
    if(cur->NO == num){
      wp = cur;
      break;
    }
    cur = cur->next;
  }
  if(NULL == wp){
    return false;
  }
  free_wp(wp);
  return true;
}

int get_num(){
  return num_watchpoint;
}
void print_change(int n, char *w_expr, word_t old, word_t cur){ // 打印变化信息
  printf("WatchPoint %d changed and the expr is \"%s\" :\n", n, w_expr);
  printf("old value : %u\n", old);
  printf("new value : %u\n", cur);
  putchar('\n');
}

void print_all_watchpoint(){ // 打印所有的 监视点
  WP *cur = head;
  while(NULL != cur){
    printf("WatchPoint %d : \"%s\" \n", cur->NO, cur->watch_expr);
    printf("current WatchPoint value is %u\n", cur->pre_val);
    putchar('\n');
    cur = cur->next;
  }
}
// 检查所有 监视点的值是否变化
// 并输出所有的变化信息
bool check_watchpoint(){ 
  WP *cur = head;
  bool flag = false;
  while(NULL != cur){ // 遍历所有已经申请的监视点
    bool success = true;
    word_t res = expr(cur->watch_expr, &success);
    Assert(success, "there is wrong when calculating the value of expression");
    if(res != cur->pre_val){ // 发生变化
      print_change(head->NO, head->watch_expr, head->pre_val, res);
      cur->pre_val = res;
      flag = true;
    }
    cur = cur->next;
  }
  return flag;
}


