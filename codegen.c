#include "compiler.h"
#include <stdarg.h>

static FILE *asm_out = NULL;

static void asmit(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    vfprintf(asm_out, fmt, args);
    fprintf(asm_out, "\n");
    va_end(args);
}

#define MAX_VARS_CG 200
static char cg_var_names[MAX_VARS_CG][100];
static int  cg_var_slots[MAX_VARS_CG];
static int  cg_var_count = 0;
static int  cg_next_slot = 1;

static int get_slot(const char *name) {
    for (int i = 0; i < cg_var_count; i++)
        if (strcmp(cg_var_names[i], name) == 0) return cg_var_slots[i];
    strncpy(cg_var_names[cg_var_count], name, 99);
    cg_var_slots[cg_var_count] = cg_next_slot++;
    return cg_var_slots[cg_var_count++];
}

static int soff(int slot) { return -(slot * 8); }

static void load_val(const char *val, const char *reg) {
    if (!val || !val[0]) { asmit("    li      %s, 0", reg); return; }
    if (val[0]=='-' || isdigit((unsigned char)val[0]))
        asmit("    li      %s, %s", reg, val);
    else
        asmit("    ld      %s, %d(s0)", reg, soff(get_slot(val)));
}

static void store_var(const char *name, const char *reg) {
    asmit("    sd      %s, %d(s0)", reg, soff(get_slot(name)));
}

#define MAX_STRS 200
static char str_vals[MAX_STRS][256];
static int  str_count = 0;

static int add_str(const char *s) {
    for (int i = 0; i < str_count; i++)
        if (strcmp(str_vals[i], s) == 0) return i;
    strncpy(str_vals[str_count], s, 255);
    return str_count++;
}

static void collect_all(void) {
    str_count = 0; cg_var_count = 0; cg_next_slot = 1;
    for (int i = 0; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i]; if (ins->dead) continue;
        if (ins->opcode == IR_PRINT_STR) add_str(ins->result);
        if (ins->result[0] && ins->opcode!=IR_LABEL && ins->opcode!=IR_JUMP &&
            ins->opcode!=IR_JUMPF && ins->opcode!=IR_PRINT_STR &&
            ins->opcode!=IR_PRINT_VAR && ins->opcode!=IR_RETURN)
            get_slot(ins->result);
        if (ins->arg1[0] && ins->arg1[0]!='"' &&
            !isdigit((unsigned char)ins->arg1[0]) && ins->arg1[0]!='-')
            get_slot(ins->arg1);
        if (ins->arg2[0] && ins->arg2[0]!='"' &&
            !isdigit((unsigned char)ins->arg2[0]) && ins->arg2[0]!='-')
            get_slot(ins->arg2);
    }
}

static void emit_rodata(void) {
    for (int i = 0; i < str_count; i++) {
        asmit(".str_%d:", i);
        fprintf(asm_out, "    .string \"");
        const char *s = str_vals[i];
        for (int j = 0; s[j]; j++) {
            if      (s[j]=='\\' && s[j+1]=='n') { fprintf(asm_out,"\\n"); j++; }
            else if (s[j]=='\\' && s[j+1]=='t') { fprintf(asm_out,"\\t"); j++; }
            else fprintf(asm_out, "%c", s[j]);
        }
        fprintf(asm_out, "\"\n");
    }
    asmit("");
}

static void emit_prologue(int n) {
    int fs = ((n+2)*8+15)&~15;
    asmit("    addi    sp, sp, -%d", fs);
    asmit("    sd      ra, %d(sp)", fs-8);
    asmit("    sd      s0, %d(sp)", fs-16);
    asmit("    addi    s0, sp, %d", fs);
    asmit("");
}

static void emit_epilogue(int n) {
    int fs = ((n+2)*8+15)&~15;
    asmit("    ld      ra, %d(sp)", fs-8);
    asmit("    ld      s0, %d(sp)", fs-16);
    asmit("    addi    sp, sp, %d", fs);
    asmit("    ret");
}

static void emit_instructions(int n) {
    for (int i = 0; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i]; if (ins->dead) continue;
        switch (ins->opcode) {
            case IR_ASSIGN:
                load_val(ins->arg1,"t0"); store_var(ins->result,"t0"); break;
            case IR_ADD:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    add     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_SUB:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    sub     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_MUL:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    mul     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_DIV:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    div     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_MOD:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    rem     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_LT:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    slt     t2, t0, t1"); store_var(ins->result,"t2"); break;
            case IR_GT:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    slt     t2, t1, t0"); store_var(ins->result,"t2"); break;
            case IR_LEQ:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    slt     t2, t1, t0");
                asmit("    xori    t2, t2, 1"); store_var(ins->result,"t2"); break;
            case IR_GEQ:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    slt     t2, t0, t1");
                asmit("    xori    t2, t2, 1"); store_var(ins->result,"t2"); break;
            case IR_EQ:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    sub     t2, t0, t1");
                asmit("    seqz    t2, t2"); store_var(ins->result,"t2"); break;
            case IR_NEQ:
                load_val(ins->arg1,"t0"); load_val(ins->arg2,"t1");
                asmit("    sub     t2, t0, t1");
                asmit("    snez    t2, t2"); store_var(ins->result,"t2"); break;
            case IR_UMINUS:
                load_val(ins->arg1,"t0");
                asmit("    neg     t1, t0"); store_var(ins->result,"t1"); break;
            case IR_INC:
                load_val(ins->result,"t0");
                asmit("    addi    t0, t0, 1"); store_var(ins->result,"t0"); break;
            case IR_LABEL:  asmit("%s:", ins->result); break;
            case IR_JUMP:   asmit("    j       %s", ins->result); break;
            case IR_JUMPF:
                load_val(ins->arg1,"t0");
                asmit("    beqz    t0, %s", ins->result); break;
            case IR_PRINT_STR:
                asmit("    la      a0, .str_%d", add_str(ins->result));
                asmit("    call    printf"); break;
            case IR_PRINT_VAR:
                asmit("    la      a0, fmt_int");
                load_val(ins->result,"a1");
                asmit("    call    printf"); break;
            case IR_RETURN:
                load_val(ins->result,"a0");
                emit_epilogue(n); break;
            default: break;
        }
    }
}

void codegen(const char *output_file) {
    asm_out = fopen(output_file, "w");
    if (!asm_out) { printf("[Error] Cannot write %s\n", output_file); return; }

    collect_all();
    int n = cg_next_slot - 1;

    emit_rodata();
    asmit("    .text");
    asmit("    .globl  main");
    asmit("    .type   main, @function");
    asmit("");
    asmit("main:");
    emit_prologue(n);
    emit_instructions(n);
    asmit("    li      a0, 0");
    emit_epilogue(n);

    fclose(asm_out);
}


// ============================================================
//  PART B — Interpreter (runs IR, prints output to screen)
// ============================================================

RTVar rt_vars[MAX_RT_VARS];
int   rt_count = 0;

static int rt_get(const char *name) {
    for (int i = 0; i < rt_count; i++)
        if (strcmp(rt_vars[i].name, name) == 0) return rt_vars[i].value;
    return 0;
}

static void rt_set(const char *name, int val) {
    for (int i = 0; i < rt_count; i++) {
        if (strcmp(rt_vars[i].name, name) == 0) { rt_vars[i].value=val; return; }
    }
    strncpy(rt_vars[rt_count].name, name, 99);
    rt_vars[rt_count].value = val;
    rt_count++;
}

static int resolve(const char *s) {
    if (!s||!s[0]) return 0;
    if (isdigit((unsigned char)s[0])||(s[0]=='-'&&isdigit((unsigned char)s[1])))
        return atoi(s);
    return rt_get(s);
}

#define MAX_LBLS 200
static char lbl_names[MAX_LBLS][20];
static int  lbl_idx[MAX_LBLS];
static int  lbl_cnt = 0;

static void build_labels(void) {
    lbl_cnt = 0;
    for (int i = 0; i < ir_count; i++)
        if (ir_code[i].opcode==IR_LABEL && !ir_code[i].dead) {
            strncpy(lbl_names[lbl_cnt], ir_code[i].result, 19);
            lbl_idx[lbl_cnt] = i;
            lbl_cnt++;
        }
}

static int find_lbl(const char *name) {
    for (int i = 0; i < lbl_cnt; i++)
        if (strcmp(lbl_names[i], name)==0) return lbl_idx[i];
    return 0;
}

static void print_str(const char *s) {
    for (int i = 0; s[i]; i++) {
        if      (s[i]=='\\' && s[i+1]=='n') { printf("\n"); i++; }
        else if (s[i]=='\\' && s[i+1]=='t') { printf("\t"); i++; }
        else printf("%c", s[i]);
    }
}

void run_program(void) {
    rt_count = 0;
    build_labels();
    int pc = 0;
    while (pc < ir_count) {
        IRInstr *ins = &ir_code[pc];
        if (ins->dead) { pc++; continue; }
        switch (ins->opcode) {
            case IR_ASSIGN:  rt_set(ins->result, resolve(ins->arg1)); break;
            case IR_ADD:     rt_set(ins->result, resolve(ins->arg1)+resolve(ins->arg2)); break;
            case IR_SUB:     rt_set(ins->result, resolve(ins->arg1)-resolve(ins->arg2)); break;
            case IR_MUL:     rt_set(ins->result, resolve(ins->arg1)*resolve(ins->arg2)); break;
            case IR_DIV:
                if (!resolve(ins->arg2)){printf("[Error] Div by zero\n");pc++;continue;}
                rt_set(ins->result, resolve(ins->arg1)/resolve(ins->arg2)); break;
            case IR_MOD:
                if (!resolve(ins->arg2)){printf("[Error] Mod by zero\n");pc++;continue;}
                rt_set(ins->result, resolve(ins->arg1)%resolve(ins->arg2)); break;
            case IR_LT:  rt_set(ins->result, resolve(ins->arg1)< resolve(ins->arg2)?1:0); break;
            case IR_GT:  rt_set(ins->result, resolve(ins->arg1)> resolve(ins->arg2)?1:0); break;
            case IR_LEQ: rt_set(ins->result, resolve(ins->arg1)<=resolve(ins->arg2)?1:0); break;
            case IR_GEQ: rt_set(ins->result, resolve(ins->arg1)>=resolve(ins->arg2)?1:0); break;
            case IR_EQ:  rt_set(ins->result, resolve(ins->arg1)==resolve(ins->arg2)?1:0); break;
            case IR_NEQ: rt_set(ins->result, resolve(ins->arg1)!=resolve(ins->arg2)?1:0); break;
            case IR_UMINUS: rt_set(ins->result, -resolve(ins->arg1)); break;
            case IR_INC:    rt_set(ins->result, rt_get(ins->result)+1); break;
            case IR_LABEL:  break;
            case IR_JUMP:   pc=find_lbl(ins->result); continue;
            case IR_JUMPF:
                if (!resolve(ins->arg1)){pc=find_lbl(ins->result);continue;} break;
            case IR_PRINT_STR: print_str(ins->result); break;
            case IR_PRINT_VAR: printf("%d", rt_get(ins->result)); break;
            case IR_RETURN:    return;
            default: break;
        }
        pc++;
    }
}
