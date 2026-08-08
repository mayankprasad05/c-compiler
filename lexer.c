#include "compiler.h"

Token tokens[MAX_TOKENS];
int   num_tokens = 0;

void lexer(char *src) {
    int i = 0, line = 1;
    num_tokens = 0;

    while (src[i] != '\0') {
        if (src[i]==' '||src[i]=='\t') { i++; continue; }
        if (src[i]=='\n') { line++; i++; continue; }
        if (src[i]=='#') {
            while (src[i]!='\n'&&src[i]!='\0') i++;
            continue;
        }
        if (src[i]=='/'&&src[i+1]=='/') {
            while (src[i]!='\n'&&src[i]!='\0') i++;
            continue;
        }

        Token t; t.line = line;

        if (src[i]=='"') {
            i++;
            int j=0;
            while (src[i]!='"'&&src[i]!='\0') t.text[j++]=src[i++];
            t.text[j]='\0'; i++;
            t.type=TOK_STRING; tokens[num_tokens++]=t; continue;
        }
        if (isdigit(src[i])) {
            int j=0;
            while (isdigit(src[i])) t.text[j++]=src[i++];
            t.text[j]='\0'; t.type=TOK_NUMBER; tokens[num_tokens++]=t; continue;
        }
        if (isalpha(src[i])||src[i]=='_') {
            int j=0;
            while (isalnum(src[i])||src[i]=='_') t.text[j++]=src[i++];
            t.text[j]='\0';
            if      (strcmp(t.text,"int")   ==0) t.type=TOK_INT;
            else if (strcmp(t.text,"if")    ==0) t.type=TOK_IF;
            else if (strcmp(t.text,"else")  ==0) t.type=TOK_ELSE;
            else if (strcmp(t.text,"while") ==0) t.type=TOK_WHILE;
            else if (strcmp(t.text,"for")   ==0) t.type=TOK_FOR;
            else if (strcmp(t.text,"printf")==0) t.type=TOK_PRINTF;
            else                                  t.type=TOK_NAME;
            tokens[num_tokens++]=t; continue;
        }
        if (src[i]=='='&&src[i+1]=='='){strcpy(t.text,"==");t.type=TOK_EQ;    i+=2;tokens[num_tokens++]=t;continue;}
        if (src[i]=='!'&&src[i+1]=='='){strcpy(t.text,"!=");t.type=TOK_NEQ;   i+=2;tokens[num_tokens++]=t;continue;}
        if (src[i]=='<'&&src[i+1]=='='){strcpy(t.text,"<=");t.type=TOK_LEQ;   i+=2;tokens[num_tokens++]=t;continue;}
        if (src[i]=='>'&&src[i+1]=='='){strcpy(t.text,">=");t.type=TOK_GEQ;   i+=2;tokens[num_tokens++]=t;continue;}
        if (src[i]=='+'&&src[i+1]=='+'){strcpy(t.text,"++");t.type=TOK_INC;   i+=2;tokens[num_tokens++]=t;continue;}
        if (src[i]=='+'&&src[i+1]=='='){strcpy(t.text,"+=");t.type=TOK_PLUSEQ;i+=2;tokens[num_tokens++]=t;continue;}

        t.text[0]=src[i]; t.text[1]='\0';
        switch(src[i]){
            case '+':t.type=TOK_PLUS;  break; case '-':t.type=TOK_MINUS; break;
            case '*':t.type=TOK_MUL;   break; case '/':t.type=TOK_DIV;   break;
            case '%':t.type=TOK_MOD;   break; case '<':t.type=TOK_LT;    break;
            case '>':t.type=TOK_GT;    break; case '=':t.type=TOK_ASSIGN;break;
            case '(':t.type=TOK_LPAREN;break; case ')':t.type=TOK_RPAREN;break;
            case '{':t.type=TOK_LBRACE;break; case '}':t.type=TOK_RBRACE;break;
            case ';':t.type=TOK_SEMI;  break; case ',':t.type=TOK_COMMA; break;
            default: i++; continue;
        }
        tokens[num_tokens++]=t; i++;
    }
    tokens[num_tokens].type=TOK_EOF;
    strcpy(tokens[num_tokens].text,"EOF");
    tokens[num_tokens].line=line;
    num_tokens++;
}

void write_tokens(const char *source_file) {
    char *names[]={"NUMBER","NAME","STRING","INT","IF","ELSE","WHILE","FOR","PRINTF",
                   "PLUS","MINUS","MUL","DIV","MOD","EQ","NEQ","LT","GT","LEQ","GEQ",
                   "ASSIGN","INC","PLUSEQ","LPAREN","RPAREN","LBRACE","RBRACE","SEMI","COMMA","EOF"};
    FILE *f=fopen("tokens.txt","w");
    if(!f){printf("[Error] Cannot write tokens.txt\n");return;}
    fprintf(f,"  Total tokens found: %d\n\n",num_tokens);
    fprintf(f,"  %-6s  %-16s  %s\n","LINE","TOKEN TYPE","VALUE");
    fprintf(f,"  %-6s  %-16s  %s\n","------","----------------","--------------------");
    for(int i=0;i<num_tokens;i++)
        fprintf(f,"  %-6d  %-16s  %s\n",tokens[i].line,names[tokens[i].type],tokens[i].text);
    int nums=0,ids=0,kws=0,ops=0,pts=0,strs=0;
    for(int i=0;i<num_tokens-1;i++){
        TokenType tt=tokens[i].type;
        if(tt==TOK_NUMBER)nums++;
        else if(tt==TOK_NAME)ids++;
        else if(tt==TOK_STRING)strs++;
        else if(tt==TOK_INT||tt==TOK_IF||tt==TOK_ELSE||tt==TOK_WHILE||tt==TOK_FOR||tt==TOK_PRINTF)kws++;
        else if(tt==TOK_LPAREN||tt==TOK_RPAREN||tt==TOK_LBRACE||tt==TOK_RBRACE||tt==TOK_SEMI||tt==TOK_COMMA)pts++;
        else ops++;
    }
    fclose(f);
}

void print_tokens(void){
    char *names[]={"NUMBER","NAME","STRING","INT","IF","ELSE","WHILE","FOR","PRINTF",
                   "PLUS","MINUS","MUL","DIV","MOD","EQ","NEQ","LT","GT","LEQ","GEQ",
                   "ASSIGN","INC","PLUSEQ","LPAREN","RPAREN","LBRACE","RBRACE","SEMI","COMMA","EOF"};
    printf("\n====== TOKEN LIST ======\n");
    printf("%-5s  %-14s  %s\n","LINE","TYPE","VALUE");
    for(int i=0;i<num_tokens;i++)
        printf("%-5d  %-14s  %s\n",tokens[i].line,names[tokens[i].type],tokens[i].text);
    printf("========================\n\n");
}
