#include "compiler.h"

static int is_number(const char *s) {
    if (!s||!s[0]) return 0;
    int i=(s[0]=='-')?1:0;
    for(;s[i];i++) if(!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int crosses_jump(int from, int to) {
    for (int k=from;k<to&&k<ir_count;k++) {
        IROpcode op=ir_code[k].opcode;
        if (op==IR_LABEL||op==IR_JUMP||op==IR_JUMPF) return 1;
    }
    return 0;
}

static void constant_folding(void) {
    int count=0;
    for (int i=0;i<ir_count;i++) {
        IRInstr *ins=&ir_code[i];
        if (ins->dead||!is_number(ins->arg1)||!is_number(ins->arg2)) continue;
        int a=atoi(ins->arg1),b=atoi(ins->arg2),result,folded=1;
        switch(ins->opcode){
            case IR_ADD: result=a+b; break; case IR_SUB: result=a-b; break;
            case IR_MUL: result=a*b; break;
            case IR_DIV: if(!b){folded=0;break;} result=a/b; break;
            case IR_MOD: if(!b){folded=0;break;} result=a%b; break;
            case IR_LT:  result=a< b; break; case IR_GT:  result=a> b; break;
            case IR_LEQ: result=a<=b; break; case IR_GEQ: result=a>=b; break;
            case IR_EQ:  result=a==b; break; case IR_NEQ: result=a!=b; break;
            default: folded=0; break;
        }
        if (folded) {
            ins->opcode=IR_ASSIGN; sprintf(ins->arg1,"%d",result);
            ins->arg2[0]='\0'; count++;
        }
    }
    if (count>0) printf("  Constant folding  : %d simplified\n",count);
}

static void copy_propagation(void) {
    int count=0;
    for (int i=0;i<ir_count;i++) {
        IRInstr *ins=&ir_code[i];
        if (ins->dead||ins->opcode!=IR_ASSIGN) continue;
        const char *dest=ins->result, *src=ins->arg1;
        if (!dest[0]||!src[0]) continue;
        if (dest[0]!='t'||!isdigit((unsigned char)dest[1])) continue;
        for (int j=i+1;j<ir_count;j++) {
            IRInstr *later=&ir_code[j]; if(later->dead) continue;
            if (strcmp(later->result,dest)==0) break;
            if (crosses_jump(i+1,j)) break;
            if (strcmp(later->arg1,dest)==0){strncpy(later->arg1,src,99);count++;}
            if (strcmp(later->arg2,dest)==0){strncpy(later->arg2,src,99);count++;}
        }
    }
    if (count>0) printf("  Copy propagation  : %d replaced\n",count);
}

static void dead_code_elimination(void) {
    int count=0;
    for (int i=0;i<ir_count;i++) {
        IRInstr *ins=&ir_code[i]; if(ins->dead) continue;
        const char *r=ins->result;
        if (!r[0]||r[0]!='t'||!isdigit((unsigned char)r[1])) continue;
        if (ins->opcode==IR_LABEL||ins->opcode==IR_JUMP||ins->opcode==IR_JUMPF||
            ins->opcode==IR_PRINT_STR||ins->opcode==IR_PRINT_VAR) continue;
        int used=0;
        for (int j=i+1;j<ir_count;j++) {
            IRInstr *later=&ir_code[j]; if(later->dead) continue;
            if (strcmp(later->arg1,r)==0||strcmp(later->arg2,r)==0||
                strcmp(later->result,r)==0){used=1;break;}
        }
        if (!used){ins->dead=1;count++;}
    }
    if (count>0) printf("  Dead code removal : %d removed\n",count);
}

void optimize(void) {
    int before=0; for(int i=0;i<ir_count;i++) if(!ir_code[i].dead) before++;
    constant_folding(); copy_propagation(); dead_code_elimination();
    int after=0; for(int i=0;i<ir_count;i++) if(!ir_code[i].dead) after++;
}
