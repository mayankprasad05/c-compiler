#include "compiler.h"

Symbol sym_table[MAX_VARS];
int    sym_count = 0;
static int errors = 0, warnings = 0;

#define MAX_LOGS 500
static char log_lines[MAX_LOGS][256];
static int  log_count = 0;

static void log_msg(const char *msg) {
    if (log_count < MAX_LOGS) strncpy(log_lines[log_count++],msg,255);
}

static int sym_exists(const char *name) {
    for (int i=0;i<sym_count;i++)
        if (strcmp(sym_table[i].name,name)==0) return 1;
    return 0;
}

static void sym_add(const char *name, int line) {
    char buf[256];
    if (sym_exists(name)) {
        snprintf(buf,sizeof(buf),"  [WARNING] Line %d: '%s' declared more than once.",line,name);
        log_msg(buf); warnings++; return;
    }
    strncpy(sym_table[sym_count].name,name,99);
    strcpy(sym_table[sym_count].type,"int");
    sym_table[sym_count].initialized=0;
    sym_count++;
    snprintf(buf,sizeof(buf),"  [OK]      Line %d: Variable '%s' declared (type: int).",line,name);
    log_msg(buf);
}

static void sym_init(const char *name) {
    for (int i=0;i<sym_count;i++)
        if (strcmp(sym_table[i].name,name)==0){ sym_table[i].initialized=1; return; }
}

static void sym_check_use(const char *name, int line) {
    if (strcmp(name,"return")==0||strcmp(name,"void")==0) return;
    char buf[256];
    if (!sym_exists(name)) {
        snprintf(buf,sizeof(buf),"  [ERROR]   Line %d: Variable '%s' used before declaration!",line,name);
        log_msg(buf); errors++;
    } else {
        snprintf(buf,sizeof(buf),"  [OK]      Line %d: Variable '%s' used — declared OK.",line,name);
        log_msg(buf);
    }
}

static void check_node(ASTNode *node) {
    if (!node) return;
    char buf[256];
    switch(node->type) {
        case NODE_VAR_DECL:
            sym_add(node->sval,node->line);
            if (node->num_children>0) { check_node(node->children[0]); sym_init(node->sval); }
            break;
        case NODE_ASSIGN:
            sym_check_use(node->sval,node->line); sym_init(node->sval);
            check_node(node->children[0]); break;
        case NODE_PLUSEQ: case NODE_INC:
            sym_check_use(node->sval,node->line);
            if (node->num_children>0) check_node(node->children[0]); break;
        case NODE_VAR: case NODE_POST_INC:
            sym_check_use(node->sval,node->line); break;
        case NODE_PRINTF:
            if (node->is_var_print) {
                // printf(varname) — variable must be declared
                sym_check_use(node->sval, node->line);
                snprintf(buf,sizeof(buf),
                    "  [OK]      Line %d: printf(variable) — will print value of '%s'.",
                    node->line, node->sval);
            } else {
                // printf("text") — always fine
                snprintf(buf,sizeof(buf),
                    "  [OK]      Line %d: printf(string) — will print \"%s\".",
                    node->line, node->sval);
            }
            log_msg(buf); break;
        case NODE_BINOP:
            if ((strcmp(node->op,"/")==0||strcmp(node->op,"%")==0) &&
                node->num_children>1 &&
                node->children[1]->type==NODE_NUMBER &&
                node->children[1]->ival==0) {
                snprintf(buf,sizeof(buf),"  [ERROR]   Line %d: Division by zero!",node->line);
                log_msg(buf); errors++;
            }
            for (int i=0;i<node->num_children;i++) check_node(node->children[i]); break;
        case NODE_FOR:
            for (int i=0;i<node->num_children;i++) check_node(node->children[i]); break;
        default:
            for (int i=0;i<node->num_children;i++) check_node(node->children[i]); break;
    }
}

int semantic_check(ASTNode *root) {
    sym_count=0; errors=0; warnings=0; log_count=0;
    check_node(root);
    for (int i=0;i<sym_count;i++) {
        if (!sym_table[i].initialized) {
            char buf[256];
            snprintf(buf,sizeof(buf),"  [WARNING] Variable '%s' declared but never assigned.",sym_table[i].name);
            log_msg(buf); warnings++;
        }
    }
    return errors;
}

void write_semantic(const char *source_file) {
    FILE *f=fopen("semantic.txt","w");
    if (!f) { printf("[Error] Cannot write semantic.txt\n"); return; }
    fprintf(f,"\n====================================================\n");
    fprintf(f,"  SYMBOL TABLE\n");
    fprintf(f,"====================================================\n\n");
    fprintf(f,"  %-20s  %-8s  %s\n","Variable","Type","Initialized");
    fprintf(f,"  %-20s  %-8s  %s\n","--------------------","--------","-----------");
    for (int i=0;i<sym_count;i++)
        fprintf(f,"  %-20s  %-8s  %s\n",
                sym_table[i].name,sym_table[i].type,
                sym_table[i].initialized?"YES":"NO");
    fprintf(f,"\n====================================================\n");
    fprintf(f,"  SUMMARY\n");
    fprintf(f,"====================================================\n\n");
    fprintf(f,"  Variables : %d\n  Errors    : %d\n  Warnings  : %d\n\n",sym_count,errors,warnings);
    fprintf(f,"  RESULT: %s\n\n",errors==0?"PASSED — No errors.":"FAILED — Fix errors above.");
    fclose(f);
}
