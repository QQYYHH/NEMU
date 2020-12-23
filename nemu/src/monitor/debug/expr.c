#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <debug.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_REG, TK_0X

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"\\(", '('},           // left (
  {"\\*", '*'},         // multiple
  {"-", '-'},           // sub 
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"^0x[a-f|0-9]+", TK_0X},   // 先判断是否是十六进制数
  {"^[0-9]+", TK_NUM},  // num
  {"\\)", ')'},           // right ) 
  // 添加寄存器
  {"^\\$[a-z]{1,3}", TK_REG}, // regs
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[100] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  memset(tokens, 0, sizeof(tokens));
  
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        // printf("### %d\n", substr_len);

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        
        if(rules[i].token_type == TK_NOTYPE) continue;

        tokens[nr_token].type = rules[i].token_type;
        strncpy(tokens[nr_token].str, substr_start, substr_len);
        nr_token++;

        // switch (rules[i].token_type) {
        //   default: TODO();
        // }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}
static int stack_char[100];
static int stack_char_top = 0;
static word_t stack_num[100];
static int stack_num_top = 0;

extern int str2num(char*);

word_t cal(word_t a, word_t b, int op){
  word_t res = 0;
  switch (op)
  {
  case '+':
    res = a + b;
    break;
  case '-':
    res = a - b;
    break;
  case '*':
    res = a * b;
    break;
  case TK_EQ:
    res = a == b;
    break;
  default:
    break;
  }
  return res;
}

word_t eval(){ // 双栈计算表达式的值
  stack_char_top = 0;
  stack_num_top = 0;
  for(int i = 0; i < nr_token; i++){
    switch (tokens[i].type)
    {
    case '(':
    {
      stack_char[stack_char_top++] = '(';
      break;
    }
    case ')':
    {
      while(stack_char[stack_char_top - 1] != '('){
        word_t tmp = cal(stack_num[stack_num_top - 2], stack_num[stack_num_top - 1], stack_char[stack_char_top - 1]);
        stack_num_top -= 2;
        stack_num[stack_num_top++] = tmp;
        stack_char_top--;
      }
      stack_char_top--;
      break;
    }
    case '*':
    {
      stack_char[stack_char_top++] = '*';
      break;
    }
    case '+':
    {
      while(stack_char_top != 0 && stack_char[stack_char_top - 1] == '*'){
        word_t tmp = cal(stack_num[stack_num_top - 2], stack_num[stack_num_top - 1], stack_char[stack_char_top - 1]);
        stack_num_top -= 2;
        stack_num[stack_num_top++] = tmp;
        stack_char_top--;
      }
      stack_char[stack_char_top++] = '+';
      break;
    }
    case '-':
    {
      while(stack_char_top != 0 && stack_char[stack_char_top - 1] == '*'){
        word_t tmp = cal(stack_num[stack_num_top - 2], stack_num[stack_num_top - 1], stack_char[stack_char_top - 1]);
        stack_num_top -= 2;
        stack_num[stack_num_top++] = tmp;
        stack_char_top--;
      }
      stack_char[stack_char_top++] = '-';
      break;
    }
    case TK_EQ:
    {
      while(stack_char_top != 0 && stack_char[stack_char_top - 1] != '('){
        word_t tmp = cal(stack_num[stack_num_top - 2], stack_num[stack_num_top - 1], stack_char[stack_char_top - 1]);
        stack_num_top -= 2;
        stack_num[stack_num_top++] = tmp;
        stack_char_top--;
      }
      stack_char[stack_char_top++] = TK_EQ;
      break;
    }
    case TK_NUM:
    {
      word_t num = str2num(tokens[i].str);
      stack_num[stack_num_top++] = num;
      break;
    }
    case TK_REG:
    {
      extern word_t isa_reg_str2val(const char *, bool *);
      bool success = true;
      word_t tmp = isa_reg_str2val(tokens[i].str + 1, &success);
      Assert(success, "reg_str2val is wrong!!!");
      stack_num[stack_num_top++] = tmp;

      break;
    }
    case TK_0X: // 将十六进制 转换为 10进制
    {
      extern word_t he2de(char *);
      word_t tmp = he2de(tokens[i].str);
      stack_num[stack_num_top++] = tmp;
    }
    default:
      break;
    }
  }
  while(stack_char_top){
    word_t tmp = cal(stack_num[stack_num_top - 2], stack_num[stack_num_top - 1], stack_char[stack_char_top - 1]);
    stack_num_top -= 2;
    stack_num[stack_num_top++] = tmp;
    stack_char_top--;
  }
  return stack_num[stack_num_top - 1];
}


word_t expr(char *e, bool *success) {
  *success = true;
  if (!make_token(e)) { // 将 表达式 分解为 tokens数组
    *success = false;
    return 0;
  }
  if(!nr_token) return 0;
  for(int i = 0; i < nr_token; i++){
    printf("%s\n", tokens[i].str);
  }
  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  word_t res = eval();

  return res;
}

