#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOK_NUMBER,     // 42
    TOK_NAME,       // x, sum
    TOK_STRING,     // "hello"

    // Keywords
    TOK_INT,        // int
    TOK_IF,         // if
    TOK_ELSE,       // else
    TOK_WHILE,      // while
    TOK_FOR,        // for
    TOK_PRINTF,     // printf

    // Arithmetic
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MUL,        // *
    TOK_DIV,        // /
    TOK_MOD,        // %

    // Comparison
    TOK_EQ,         // ==
    TOK_NEQ,        // !=
    TOK_LT,         // <
    TOK_GT,         // >
    TOK_LEQ,        // <=
    TOK_GEQ,        // >=

    // Assignment
    TOK_ASSIGN,     // =
    TOK_INC,        // ++
    TOK_PLUSEQ,     // +=

    // Punctuation
    TOK_LPAREN,     // (
    TOK_RPAREN,     // )
    TOK_LBRACE,     // {
    TOK_RBRACE,     // }
    TOK_SEMI,       // ;
    TOK_COMMA,      // ,

    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    char      text[100];
    int       line;
} Token;

#define MAX_TOKENS 2000
extern Token tokens[MAX_TOKENS];
extern int   num_tokens;

typedef enum {
    NODE_PROGRAM,
    NODE_FUNC_DEF,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_ASSIGN,
    NODE_PLUSEQ,
    NODE_INC,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_PRINTF,    // printf("text")  OR  printf(varname)
    NODE_RETURN,
    NODE_BINOP,
    NODE_UNARY,
    NODE_NUMBER,
    NODE_STRING,
    NODE_VAR,
    NODE_POST_INC
} NodeType;

#define MAX_CHILDREN 64
typedef struct ASTNode {
    NodeType        type;
    char            sval[200];   // string value or variable name
    int             ival;        // integer literal value
    char            op[4];       // operator: + - * / == etc.
    int             is_var_print; // 1 = printf(varname), 0 = printf("text")
    struct ASTNode *children[MAX_CHILDREN];
    int             num_children;
    int             line;
} ASTNode;

#define MAX_VARS 100
typedef struct {
    char name[100];
    char type[16];
    int  initialized;
} Symbol;

extern Symbol sym_table[MAX_VARS];
extern int    sym_count;

typedef enum {
    IR_ASSIGN,
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_LT, IR_GT, IR_LEQ, IR_GEQ, IR_EQ, IR_NEQ,
    IR_UMINUS, IR_INC,
    IR_LABEL, IR_JUMP, IR_JUMPF,
    IR_PRINT_STR,   // print string literal
    IR_PRINT_VAR,   // print variable value
    IR_RETURN
} IROpcode;

#define MAX_IR 2000
typedef struct {
    IROpcode opcode;
    char     result[100];
    char     arg1[100];
    char     arg2[100];
    int      dead;
} IRInstr;

extern IRInstr ir_code[MAX_IR];
extern int     ir_count;

#define MAX_RT_VARS 100
typedef struct {
    char name[100];
    int  value;
} RTVar;

extern RTVar rt_vars[MAX_RT_VARS];
extern int   rt_count;

// lex
void     lexer(char *src);
void     print_tokens(void);
void     write_tokens(const char *source_file);

// parser
ASTNode *parse(void);
void     ast_print(ASTNode *node, int indent);
void     ast_free(ASTNode *node);
ASTNode *ast_new(NodeType type);
void     write_ast(ASTNode *root, const char *source_file);

// semantic
int      semantic_check(ASTNode *root);
void     write_semantic(const char *source_file);

// intermediate
void     generate_ir(ASTNode *root);
void     print_ir(void);
void     write_ir(const char *source_file);

// optimizer
void     optimize(void);

// code generator
void     codegen(const char *output_file);
void     run_program(void);

#endif
