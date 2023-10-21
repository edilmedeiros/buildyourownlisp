#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
include <string.h>

static char input[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#endif

/* Create Enumeration of Possible lval Types */
typedef enum {
  LVAL_NUM,
  LVAL_ERR
} lval_t;

/* Create Enumeration of Possible Error Types */
typedef enum {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM
} err_t;

/* Declare New lisp value Struct */
typedef struct {
  lval_t type;
  long num;
  err_t err;
} lval;

/* Create a new number type lval */
lval lval_num(long n) {
  lval v;
  v.type = LVAL_NUM;
  v.num = n;
  return v;
}

/* Create a new error type lval */
lval lval_err(err_t e) {
  lval v;
  v.type = LVAL_ERR;
  v.err = e;
  return v;
}

/* Print an lval */
void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number, print it */
    /* then break out of the switch. */
  case LVAL_NUM: printf("%li", v.num); break;

    /* In the case the type is an error */
  case LVAL_ERR:
    /* Check what type of error it is and print it. */
    switch (v.err) {
    case LERR_DIV_ZERO: printf("Error: Division by zero!"); break;
    case LERR_BAD_OP: printf("Error: Invalid operator!"); break;
    case LERR_BAD_NUM: printf("Error: Invalid number!"); break;
    }
  break;
  }
}

/* Print an lval followed by a newline */
void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

int number_of_nodes(mpc_ast_t* t) {
  if (t->children_num == 0) {
    printf("Node %s with: %s\n", t->tag, t->contents);
    return 1;
  }
  if (t->children_num >= 1) {
    int total = 1;
    printf("Node %s has %d children. Recursing...\n", t->tag, t->children_num);
    for (int i = 0; i < t->children_num; i++) {
      total += number_of_nodes(t->children[i]);
    }
    return total;
  }
  return 0;
}

lval eval_op(lval x, char* op, lval y) {

  /* If either value is an error, return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise, do maths on the number values */
  if(strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if(strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if(strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if(strcmp(op, "/") == 0) {
    /* If second operator is zero, return error */
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

  /* If tagged as number return it directly */
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* The operator is always second child */
  char* op = t->children[1]->contents;

  /* We store the third child in 'x' */
  lval x = eval(t->children[2]);

  /* Iterate the remaining children and combining */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main (int argc, char** argv) {

  /* Create some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
    "  \
    number   : /-?[0-9]+/ ;                            \
    operator : '+' | '-' | '*' | '/' ;                 \
    expr     : <number> | '(' <operator> <expr>+ ')' ; \
    lispy    : /^/ <operator> <expr>+ /$/ ;            \
    ",
            Number, Operator, Expr, Lispy);


  /* Print Version and Exit Information */
  puts("Lispy Vewrsion 0.1");
  puts("Press Ctrl+c to Exit\n");

  /* In a never ending loop */
  while(1) {

    /* Output our prompt and get input*/
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On success print the AST */
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
