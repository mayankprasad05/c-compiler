#include "compiler.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { printf("Error: Cannot open '%s'\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f); rewind(f);
    char *buf = (char *)malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// Replace .c with .s to get assembly filename
static void make_asm_name(const char *src, char *out, int size) {
    strncpy(out, src, size - 1);
    char *dot = strrchr(out, '.');
    if (dot) strcpy(dot, ".s");
    else strncat(out, ".s", size - strlen(out) - 1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file.c>\n", argv[0]);
        return 1;
    }

    char *source = read_file(argv[1]);
    char asm_file[256];
    make_asm_name(argv[1], asm_file, sizeof(asm_file));

    // Stage 1 — Lexer
    lexer(source);
    write_tokens(argv[1]);

    // Stage 2 — Parser
    ASTNode *ast = parse();
    write_ast(ast, argv[1]);

    // Stage 3 — Semantic
    int errs = semantic_check(ast);
    write_semantic(argv[1]);
    if (errs > 0) {
        printf("          FAILED — %d error(s). See semantic.txt\n\n", errs);
        ast_free(ast); free(source); return 1;
    }

    // Stage 4 — IR
    generate_ir(ast);
    write_ir(argv[1]);

    // Stage 5 — Optimizer
    optimize();

    // Stage 6a — Generate assembly file
    codegen(asm_file);

    // Stage 6b — Run and print output to screen
    run_program();

    ast_free(ast);
    free(source);
    return 0;
}
