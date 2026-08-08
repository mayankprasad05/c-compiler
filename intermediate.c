#include "compiler.h"

IRInstr ir_code[MAX_IR];
int     ir_count    = 0;
static int temp_count  = 0;
static int label_count = 0;

static void new_temp(char *out)  { sprintf(out,"t%d",++temp_count); }
static void new_label(char *out) { sprintf(out,"L%d",++label_count); }

static void emit(IROpcode op, const char *result,
                 const char *arg1, const char *arg2) {
    IRInstr *ins = &ir_code[ir_count++];
    ins->opcode=op; ins->dead=0;
    strncpy(ins->result, result?result:"", 99);
    strncpy(ins->arg1,   arg1  ?arg1  :"", 99);
    strncpy(ins->arg2,   arg2  ?arg2  :"", 99);
}

static void gen_node(ASTNode *node);

static void gen_expr(ASTNode *node, char *out) {
    if (!node) { strcpy(out,"0"); return; }
    switch(node->type) {
        case NODE_NUMBER:   sprintf(out,"%d",node->ival); break;
        case NODE_STRING:   sprintf(out,"\"%s\"",node->sval); break;
        case NODE_VAR:      strncpy(out,node->sval,99); break;
        case NODE_POST_INC: {
            char tmp[100]; new_temp(tmp);
            emit(IR_ASSIGN,tmp,node->sval,"");
            emit(IR_INC,node->sval,node->sval,"");
            strncpy(out,tmp,99); break;
        }
        case NODE_UNARY: {
            char v[100],tmp[100];
            gen_expr(node->children[0],v); new_temp(tmp);
            emit(IR_UMINUS,tmp,v,""); strncpy(out,tmp,99); break;
        }
        case NODE_BINOP: {
            char l[100],r[100],tmp[100];
            gen_expr(node->children[0],l);
            gen_expr(node->children[1],r);
            new_temp(tmp);
            IROpcode op;
            if      (strcmp(node->op,"+" )==0) op=IR_ADD;
            else if (strcmp(node->op,"-" )==0) op=IR_SUB;
            else if (strcmp(node->op,"*" )==0) op=IR_MUL;
            else if (strcmp(node->op,"/" )==0) op=IR_DIV;
            else if (strcmp(node->op,"%" )==0) op=IR_MOD;
            else if (strcmp(node->op,"<" )==0) op=IR_LT;
            else if (strcmp(node->op,">" )==0) op=IR_GT;
            else if (strcmp(node->op,"<=")==0) op=IR_LEQ;
            else if (strcmp(node->op,">=")==0) op=IR_GEQ;
            else if (strcmp(node->op,"==")==0) op=IR_EQ;
            else if (strcmp(node->op,"!=")==0) op=IR_NEQ;
            else op=IR_ADD;
            emit(op,tmp,l,r); strncpy(out,tmp,99); break;
        }
        default: strcpy(out,"0"); break;
    }
}

static void gen_node(ASTNode *node) {
    if (!node) return;
    switch(node->type) {
        case NODE_PROGRAM: case NODE_BLOCK: case NODE_FUNC_DEF:
            for (int i=0;i<node->num_children;i++) gen_node(node->children[i]); break;
        case NODE_VAR_DECL: {
            char val[100]="0";
            if (node->num_children>0) gen_expr(node->children[0],val);
            emit(IR_ASSIGN,node->sval,val,""); break;
        }
        case NODE_ASSIGN: {
            char val[100]; gen_expr(node->children[0],val);
            emit(IR_ASSIGN,node->sval,val,""); break;
        }
        case NODE_PLUSEQ: {
            char val[100],tmp[100];
            gen_expr(node->children[0],val); new_temp(tmp);
            emit(IR_ADD,tmp,node->sval,val);
            emit(IR_ASSIGN,node->sval,tmp,""); break;
        }
        case NODE_INC:
            emit(IR_INC,node->sval,node->sval,""); break;

        // printf("text")  ->  IR_PRINT_STR  result="text"
        // printf(varname) ->  IR_PRINT_VAR  result=varname
        case NODE_PRINTF:
            if (node->is_var_print)
                emit(IR_PRINT_VAR, node->sval, "", "");
            else
                emit(IR_PRINT_STR, node->sval, "", "");
            break;

        case NODE_IF: {
            char cond[100],lelse[20],lend[20];
            gen_expr(node->children[0],cond);
            new_label(lelse); new_label(lend);
            emit(IR_JUMPF,lelse,cond,"");
            gen_node(node->children[1]);
            emit(IR_JUMP,lend,"","");
            emit(IR_LABEL,lelse,"","");
            if (node->num_children>2) gen_node(node->children[2]);
            emit(IR_LABEL,lend,"",""); break;
        }
        case NODE_WHILE: {
            char cond[100],lstart[20],lend[20];
            new_label(lstart); new_label(lend);
            emit(IR_LABEL,lstart,"","");
            gen_expr(node->children[0],cond);
            emit(IR_JUMPF,lend,cond,"");
            gen_node(node->children[1]);
            emit(IR_JUMP,lstart,"","");
            emit(IR_LABEL,lend,"",""); break;
        }
        case NODE_FOR: {
            char cond[100],lstart[20],lend[20];
            new_label(lstart); new_label(lend);
            if (node->num_children>0) gen_node(node->children[0]);
            emit(IR_LABEL,lstart,"","");
            if (node->num_children>1) gen_expr(node->children[1],cond);
            emit(IR_JUMPF,lend,cond,"");
            if (node->num_children>3) gen_node(node->children[3]);
            if (node->num_children>2) gen_node(node->children[2]);
            emit(IR_JUMP,lstart,"","");
            emit(IR_LABEL,lend,"",""); break;
        }
        case NODE_RETURN:
            if (node->num_children>0) {
                char val[100]; gen_expr(node->children[0],val);
                emit(IR_RETURN,val,"","");
            } else emit(IR_RETURN,"0","","");
            break;
        default: break;
    }
}

void generate_ir(ASTNode *root) {
    ir_count=0; temp_count=0; label_count=0;
    gen_node(root);
}

void write_ir(const char *source_file) {
    FILE *f=fopen("intermediate.txt","w");
    if (!f) { printf("[Error] Cannot write intermediate.txt\n"); return; }

    for (int i=0;i<ir_count;i++) {
        IRInstr *ins=&ir_code[i];
        if (ins->dead) continue;
        if (ins->opcode==IR_LABEL) { fprintf(f,"\n  %s:\n",ins->result); continue; }
        fprintf(f,"  [%3d]  ",i);
        switch(ins->opcode) {
            case IR_ASSIGN:    fprintf(f,"%s = %s",         ins->result,ins->arg1); break;
            case IR_ADD:       fprintf(f,"%s = %s + %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_SUB:       fprintf(f,"%s = %s - %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_MUL:       fprintf(f,"%s = %s * %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_DIV:       fprintf(f,"%s = %s / %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_MOD:       fprintf(f,"%s = %s %% %s",   ins->result,ins->arg1,ins->arg2); break;
            case IR_LT:        fprintf(f,"%s = %s < %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_GT:        fprintf(f,"%s = %s > %s",    ins->result,ins->arg1,ins->arg2); break;
            case IR_LEQ:       fprintf(f,"%s = %s <= %s",   ins->result,ins->arg1,ins->arg2); break;
            case IR_GEQ:       fprintf(f,"%s = %s >= %s",   ins->result,ins->arg1,ins->arg2); break;
            case IR_EQ:        fprintf(f,"%s = %s == %s",   ins->result,ins->arg1,ins->arg2); break;
            case IR_NEQ:       fprintf(f,"%s = %s != %s",   ins->result,ins->arg1,ins->arg2); break;
            case IR_UMINUS:    fprintf(f,"%s = -%s",         ins->result,ins->arg1); break;
            case IR_INC:       fprintf(f,"%s = %s + 1",     ins->result,ins->arg1); break;
            case IR_JUMP:      fprintf(f,"goto %s",           ins->result); break;
            case IR_JUMPF:     fprintf(f,"if NOT (%s)  goto %s",ins->arg1,ins->result); break;
            case IR_RETURN:    fprintf(f,"return %s",         ins->result); break;
            case IR_PRINT_STR: fprintf(f,"PRINT_STR  \"%s\"", ins->result); break;
            case IR_PRINT_VAR: fprintf(f,"PRINT_VAR  %s",     ins->result); break;
            default: break;
        }
        fprintf(f,"\n");
    }

    // Summary
    int assigns=0,arith=0,jumps=0,labels=0,prints=0;
    for (int i=0;i<ir_count;i++) {
        if (ir_code[i].dead) continue;
        switch(ir_code[i].opcode){
            case IR_ASSIGN: assigns++; break;
            case IR_ADD:case IR_SUB:case IR_MUL:case IR_DIV:case IR_MOD:
            case IR_INC:case IR_UMINUS:case IR_LT:case IR_GT:
            case IR_LEQ:case IR_GEQ:case IR_EQ:case IR_NEQ: arith++; break;
            case IR_JUMP:case IR_JUMPF: jumps++; break;
            case IR_LABEL: labels++; break;
            case IR_PRINT_STR:case IR_PRINT_VAR: prints++; break;
            default: break;
        }
    }

    fclose(f);
}

void print_ir(void) {
    printf("\n=== IR CODE ===\n");
    for (int i=0;i<ir_count;i++) {
        IRInstr *ins=&ir_code[i]; if(ins->dead) continue;
        switch(ins->opcode){
            case IR_ASSIGN:    printf("  %s = %s\n",       ins->result,ins->arg1); break;
            case IR_ADD:       printf("  %s = %s + %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_SUB:       printf("  %s = %s - %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_MUL:       printf("  %s = %s * %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_DIV:       printf("  %s = %s / %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_MOD:       printf("  %s = %s %% %s\n", ins->result,ins->arg1,ins->arg2); break;
            case IR_INC:       printf("  %s = %s + 1\n",   ins->result,ins->arg1); break;
            case IR_LT:        printf("  %s = %s < %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_GT:        printf("  %s = %s > %s\n",  ins->result,ins->arg1,ins->arg2); break;
            case IR_LEQ:       printf("  %s = %s <= %s\n", ins->result,ins->arg1,ins->arg2); break;
            case IR_GEQ:       printf("  %s = %s >= %s\n", ins->result,ins->arg1,ins->arg2); break;
            case IR_EQ:        printf("  %s = %s == %s\n", ins->result,ins->arg1,ins->arg2); break;
            case IR_NEQ:       printf("  %s = %s != %s\n", ins->result,ins->arg1,ins->arg2); break;
            case IR_LABEL:     printf("\n  %s:\n",          ins->result); break;
            case IR_JUMP:      printf("  goto %s\n",         ins->result); break;
            case IR_JUMPF:     printf("  if !%s goto %s\n", ins->arg1,ins->result); break;
            case IR_RETURN:    printf("  return %s\n",       ins->result); break;
            case IR_PRINT_STR: printf("  PRINT_STR \"%s\"\n",ins->result); break;
            case IR_PRINT_VAR: printf("  PRINT_VAR %s\n",    ins->result); break;
            default: break;
        }
    }
    printf("===============\n\n");
}
