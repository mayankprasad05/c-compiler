#include "compiler.h"

static int pos = 0;

static Token cur()           { return tokens[pos]; }
static Token nxt()           { return tokens[pos++]; }
static int   is(TokenType t) { return tokens[pos].type == t; }

static void eat(TokenType t, char *what) {
    if (tokens[pos].type != t) {
        printf("[Syntax Error] Line %d: Expected '%s' but got '%s'\n",
               tokens[pos].line, what, tokens[pos].text);
        exit(1);
    }
    pos++;
}

ASTNode *ast_new(NodeType type) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    n->type = type;
    return n;
}

static void add_child(ASTNode *p, ASTNode *c) {
    if (c && p->num_children < MAX_CHILDREN)
        p->children[p->num_children++] = c;
}

static ASTNode *parse_expr(void);
static ASTNode *parse_statement(void);
static ASTNode *parse_block(void);

static ASTNode *parse_primary(void) {
    Token t = cur();
    if (t.type==TOK_NUMBER) {
        nxt(); ASTNode *n=ast_new(NODE_NUMBER);
        n->ival=atoi(t.text); n->line=t.line; return n;
    }
    if (t.type==TOK_STRING) {
        nxt(); ASTNode *n=ast_new(NODE_STRING);
        strncpy(n->sval,t.text,199); n->line=t.line; return n;
    }
    if (t.type==TOK_NAME) {
        nxt();
        if (is(TOK_INC)) {
            nxt(); ASTNode *n=ast_new(NODE_POST_INC);
            strncpy(n->sval,t.text,99); n->line=t.line; return n;
        }
        ASTNode *n=ast_new(NODE_VAR);
        strncpy(n->sval,t.text,99); n->line=t.line; return n;
    }
    if (t.type==TOK_LPAREN) {
        nxt(); ASTNode *e=parse_expr(); eat(TOK_RPAREN,")"); return e;
    }
    if (t.type==TOK_MINUS) {
        nxt(); ASTNode *n=ast_new(NODE_UNARY);
        strcpy(n->op,"-"); n->line=t.line;
        add_child(n,parse_primary()); return n;
    }
    printf("[Parse Error] Unexpected token '%s' at line %d\n",t.text,t.line);
    exit(1);
}

static ASTNode *parse_term(void) {
    ASTNode *left=parse_primary();
    while (is(TOK_MUL)||is(TOK_DIV)||is(TOK_MOD)) {
        Token op=nxt(); ASTNode *n=ast_new(NODE_BINOP);
        strncpy(n->op,op.text,3); n->line=op.line;
        add_child(n,left); add_child(n,parse_primary()); left=n;
    }
    return left;
}

static ASTNode *parse_additive(void) {
    ASTNode *left=parse_term();
    while (is(TOK_PLUS)||is(TOK_MINUS)) {
        Token op=nxt(); ASTNode *n=ast_new(NODE_BINOP);
        strncpy(n->op,op.text,3); n->line=op.line;
        add_child(n,left); add_child(n,parse_term()); left=n;
    }
    return left;
}

static ASTNode *parse_expr(void) {
    ASTNode *left=parse_additive();
    while (is(TOK_EQ)||is(TOK_NEQ)||is(TOK_LT)||
           is(TOK_GT)||is(TOK_LEQ)||is(TOK_GEQ)) {
        Token op=nxt(); ASTNode *n=ast_new(NODE_BINOP);
        strncpy(n->op,op.text,3); n->line=op.line;
        add_child(n,left); add_child(n,parse_additive()); left=n;
    }
    return left;
}

static ASTNode *parse_printf(void) {
    int line=cur().line; nxt();  // eat 'printf'
    eat(TOK_LPAREN,"(");
    ASTNode *n=ast_new(NODE_PRINTF); n->line=line;

    if (is(TOK_STRING)) {
        //print text
        strncpy(n->sval, cur().text, 199);
        n->is_var_print = 0;
        nxt();  // eat
    } else if (is(TOK_NAME)) {
        //print var
        strncpy(n->sval, cur().text, 99);
        n->is_var_print = 1;
        nxt();  // eat
    } else {
        printf("[Error] Line %d: printf must have a string or variable name.\n", line);
        exit(1);
    }

    eat(TOK_RPAREN,")");
    eat(TOK_SEMI,";");
    return n;
}

static ASTNode *parse_if(void) {
    int line=cur().line; nxt();
    eat(TOK_LPAREN,"(");
    ASTNode *n=ast_new(NODE_IF); n->line=line;
    add_child(n,parse_expr()); eat(TOK_RPAREN,")");
    add_child(n,parse_statement());
    if (is(TOK_ELSE)) { nxt(); add_child(n,parse_statement()); }
    return n;
}

static ASTNode *parse_while(void) {
    int line=cur().line; nxt();
    eat(TOK_LPAREN,"(");
    ASTNode *n=ast_new(NODE_WHILE); n->line=line;
    add_child(n,parse_expr()); eat(TOK_RPAREN,")");
    add_child(n,parse_statement());
    return n;
}

static ASTNode *parse_for(void) {
    int line=cur().line; nxt();
    eat(TOK_LPAREN,"(");
    ASTNode *n=ast_new(NODE_FOR); n->line=line;
    if (is(TOK_INT)) nxt();
    ASTNode *init=ast_new(NODE_VAR_DECL);
    strncpy(init->sval,cur().text,99);
    eat(TOK_NAME,"variable"); eat(TOK_ASSIGN,"=");
    add_child(init,parse_expr()); eat(TOK_SEMI,";");
    add_child(n,init);
    add_child(n,parse_expr()); eat(TOK_SEMI,";");
    ASTNode *upd=NULL;
    if (is(TOK_NAME)) {
        Token vt=nxt();
        if (is(TOK_INC))    { nxt(); upd=ast_new(NODE_INC);   strncpy(upd->sval,vt.text,99); }
        else if(is(TOK_PLUSEQ)){ nxt(); upd=ast_new(NODE_PLUSEQ); strncpy(upd->sval,vt.text,99); add_child(upd,parse_expr()); }
        else if(is(TOK_ASSIGN)){ nxt(); upd=ast_new(NODE_ASSIGN);  strncpy(upd->sval,vt.text,99); add_child(upd,parse_expr()); }
    }
    add_child(n,upd); eat(TOK_RPAREN,")");
    add_child(n,parse_statement());
    return n;
}

static ASTNode *parse_block(void) {
    eat(TOK_LBRACE,"{");
    ASTNode *b=ast_new(NODE_BLOCK); b->line=tokens[pos].line;
    while (!is(TOK_RBRACE)&&!is(TOK_EOF)) add_child(b,parse_statement());
    eat(TOK_RBRACE,"}");
    return b;
}

static void skip_to_semi(void) {
    while (!is(TOK_SEMI)&&!is(TOK_LBRACE)&&!is(TOK_RBRACE)&&!is(TOK_EOF)) nxt();
    if (is(TOK_SEMI)) nxt();
}

static ASTNode *parse_statement(void) {
    if (is(TOK_LBRACE))  return parse_block();
    if (is(TOK_IF))      return parse_if();
    if (is(TOK_WHILE))   return parse_while();
    if (is(TOK_FOR))     return parse_for();
    if (is(TOK_PRINTF))  return parse_printf();

    if (is(TOK_INT)) {
        nxt();
        char name[100]; strncpy(name,cur().text,99);
        eat(TOK_NAME,"name");
        if (is(TOK_LPAREN)) {  // int main() { }
            while (!is(TOK_RPAREN)&&!is(TOK_EOF)) nxt(); nxt();
            ASTNode *fn=ast_new(NODE_FUNC_DEF); strncpy(fn->sval,name,99);
            if (is(TOK_LBRACE)) add_child(fn,parse_block());
            return fn;
        }
        ASTNode *n=ast_new(NODE_VAR_DECL); strncpy(n->sval,name,99); n->line=tokens[pos].line;
        if (is(TOK_ASSIGN)) { nxt(); add_child(n,parse_expr()); }
        eat(TOK_SEMI,";"); return n;
    }

    // return / void — skip gracefully
    if (is(TOK_NAME) && (strcmp(tokens[pos].text,"return")==0||strcmp(tokens[pos].text,"void")==0)) {
        int line=cur().line; nxt();
        ASTNode *n=ast_new(NODE_RETURN); n->line=line;
        if (!is(TOK_SEMI)&&!is(TOK_RBRACE)&&!is(TOK_EOF)) add_child(n,parse_expr());
        if (is(TOK_SEMI)) nxt();
        return n;
    }

    // x = expr;  x += expr;  x++;
    if (is(TOK_NAME)) {
        Token vt=nxt();
        if (is(TOK_ASSIGN)) {
            nxt(); ASTNode *n=ast_new(NODE_ASSIGN);
            strncpy(n->sval,vt.text,99); n->line=vt.line;
            add_child(n,parse_expr()); eat(TOK_SEMI,";"); return n;
        }
        if (is(TOK_PLUSEQ)) {
            nxt(); ASTNode *n=ast_new(NODE_PLUSEQ);
            strncpy(n->sval,vt.text,99); n->line=vt.line;
            add_child(n,parse_expr()); eat(TOK_SEMI,";"); return n;
        }
        if (is(TOK_INC)) {
            nxt(); ASTNode *n=ast_new(NODE_INC);
            strncpy(n->sval,vt.text,99); n->line=vt.line;
            eat(TOK_SEMI,";"); return n;
        }
        skip_to_semi(); return NULL;
    }
    nxt(); return NULL;
}

ASTNode *parse(void) {
    pos=0;
    ASTNode *root=ast_new(NODE_PROGRAM); root->line=1;
    while (!is(TOK_EOF)) { ASTNode *s=parse_statement(); if(s) add_child(root,s); }
    return root;
}

// ── AST writing ───────────────────────────────────────────────
static const char *node_name(NodeType t) {
    switch(t){
        case NODE_PROGRAM:  return "PROGRAM";
        case NODE_FUNC_DEF: return "FUNC_DEF";
        case NODE_VAR_DECL: return "VAR_DECL";
        case NODE_BLOCK:    return "BLOCK";
        case NODE_ASSIGN:   return "ASSIGN";
        case NODE_PLUSEQ:   return "PLUS_ASSIGN";
        case NODE_INC:      return "INCREMENT";
        case NODE_IF:       return "IF";
        case NODE_WHILE:    return "WHILE";
        case NODE_FOR:      return "FOR";
        case NODE_PRINTF:   return "PRINTF";
        case NODE_RETURN:   return "RETURN";
        case NODE_BINOP:    return "BINARY_OP";
        case NODE_UNARY:    return "UNARY_OP";
        case NODE_NUMBER:   return "NUMBER";
        case NODE_STRING:   return "STRING";
        case NODE_VAR:      return "VARIABLE";
        case NODE_POST_INC: return "POST_INCREMENT";
        default:            return "UNKNOWN";
    }
}

static void write_ast_node(FILE *f, ASTNode *node, char *prefix, int is_last) {
    if (!node) return;
    fprintf(f,"%s%s",prefix,is_last?"└── ":"├── ");
    fprintf(f,"[%s]",node_name(node->type));
    if (node->type==NODE_PRINTF) {
        if (node->is_var_print)
            fprintf(f,"  print variable: %s", node->sval);
        else
            fprintf(f,"  print text: \"%s\"", node->sval);
    } else {
        if (node->sval[0])             fprintf(f,"  \"%s\"",node->sval);
        if (node->op[0])               fprintf(f,"  op=%s",node->op);
        if (node->type==NODE_NUMBER)   fprintf(f,"  val=%d",node->ival);
    }
    fprintf(f,"\n");
    char new_prefix[256];
    snprintf(new_prefix,sizeof(new_prefix),"%s%s",prefix,is_last?"    ":"│   ");
    for (int i=0;i<node->num_children;i++)
        write_ast_node(f,node->children[i],new_prefix,i==node->num_children-1);
}

void write_ast(ASTNode *root, const char *source_file) {
    FILE *f=fopen("ast.txt","w");
    if (!f) { printf("[Error] Cannot write ast.txt\n"); return; }
    fprintf(f,"[PROGRAM]\n");
    for (int i=0;i<root->num_children;i++)
        write_ast_node(f,root->children[i],"",i==root->num_children-1);
    fprintf(f,"\n");
    fclose(f);
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    printf("%*s[%s]",indent,"",node_name(node->type));
    if (node->sval[0]) printf("  \"%s\"",node->sval);
    if (node->op[0])   printf("  op=%s",node->op);
    if (node->type==NODE_NUMBER) printf("  val=%d",node->ival);
    printf("\n");
    for (int i=0;i<node->num_children;i++) ast_print(node->children[i],indent+4);
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i=0;i<node->num_children;i++) ast_free(node->children[i]);
    free(node);
}
