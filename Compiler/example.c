// mini_compiler_ir.c
// Tiny compiler for the simple language that emits x86_64 assembly.
// Phases: tokenize -> parse -> AST -> constant-fold -> emit .s -> link with gcc
// Usage: ./mini_compiler_ir source.txt output_binary

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -----------------------
   TOKENIZER
   ----------------------- */
typedef enum {
    T_END, T_LBRACE, T_RBRACE, T_SEMI, T_AMP, T_ID, T_NUM,
    T_PLUS, T_MINUS, T_MUL, T_DIV, T_MOD, T_LP, T_RP,
    T_INPUT, T_OUTPUT, T_EQ
} TokenType;

typedef struct {
    TokenType type;
    char *text; // for ID or NUM
} Token;

typedef struct {
    Token *arr; int n, cap; int pos;
} TokenList;

void tl_init(TokenList *t){ t->n=0; t->cap=32; t->arr=malloc(t->cap*sizeof(Token)); t->pos=0; }
void tl_push(TokenList *t, Token tk){ if(t->n==t->cap) t->arr=realloc(t->arr,(t->cap*=2)*sizeof(Token)); t->arr[t->n++]=tk; }
Token tl_peek(TokenList *t,int off){ int idx=t->pos+off; if(idx<0||idx>=t->n){ Token e={T_END,NULL}; return e;} return t->arr[idx]; }
Token tl_next(TokenList *t){ if(t->pos>=t->n){ Token e={T_END,NULL}; return e;} return t->arr[t->pos++]; }

char *read_file(const char *path){
    FILE *f=fopen(path,"rb"); if(!f){ perror("open"); exit(1); }
    fseek(f,0,SEEK_END); long s=ftell(f); fseek(f,0,SEEK_SET);
    char *b=malloc(s+1); fread(b,1,s,f); b[s]=0; fclose(f); return b;
}

void tokenize(const char *src, TokenList *out){
    tl_init(out);
    int i=0;
    while(src[i]){
        char c = src[i];
        if(isspace((unsigned char)c)){ i++; continue; }
        if(c=='{'){ tl_push(out,(Token){T_LBRACE,NULL}); i++; continue; }
        if(c=='}'){ tl_push(out,(Token){T_RBRACE,NULL}); i++; continue; }
        if(c==';'){ tl_push(out,(Token){T_SEMI,NULL}); i++; continue; }
        if(c=='&'){ tl_push(out,(Token){T_AMP,NULL}); i++; continue; }
        if(c=='+'){ tl_push(out,(Token){T_PLUS,NULL}); i++; continue; }
        if(c=='-'){ tl_push(out,(Token){T_MINUS,NULL}); i++; continue; }
        if(c=='*'){ tl_push(out,(Token){T_MUL,NULL}); i++; continue; }
        if(c=='/'){ tl_push(out,(Token){T_DIV,NULL}); i++; continue; }
        if(c=='%'){ tl_push(out,(Token){T_MOD,NULL}); i++; continue; }
        if(c=='('){ tl_push(out,(Token){T_LP,NULL}); i++; continue; }
        if(c==')'){ tl_push(out,(Token){T_RP,NULL}); i++; continue; }
        if(c=='='){ tl_push(out,(Token){T_EQ,NULL}); i++; continue; }
        if(c=='>' && src[i+1]=='>'){ tl_push(out,(Token){T_INPUT,NULL}); i+=2; continue; }
        if(c=='<' && src[i+1]=='<'){ tl_push(out,(Token){T_OUTPUT,NULL}); i+=2; continue; }
        if(isdigit((unsigned char)c)){
            int j=i; while(isdigit((unsigned char)src[j])) j++;
            char *num=strndup(src+i,j-i); tl_push(out,(Token){T_NUM,num}); i=j; continue;
        }
        if(isalpha((unsigned char)c) || c=='_'){
            int j=i; while(isalnum((unsigned char)src[j]) || src[j]=='_') j++;
            char *id=strndup(src+i,j-i); tl_push(out,(Token){T_ID,id}); i=j; continue;
        }
        fprintf(stderr,"Unknown char '%c' at %d\n", c, i); exit(1);
    }
    tl_push(out,(Token){T_END,NULL});
}

/* -----------------------
   AST (expressions) and statements
   ----------------------- */
typedef enum { N_NUM, N_VAR, N_OP } NodeType;
typedef struct Node {
    NodeType type;
    long long value;    // for N_NUM
    char *var;          // for N_VAR
    char op;            // for N_OP
    struct Node *l, *r; // for N_OP
} Node;

Node *new_num(long long v){ Node *n=malloc(sizeof(Node)); n->type=N_NUM; n->value=v; n->var=NULL; n->l=n->r=NULL; return n; }
Node *new_var(const char *s){ Node *n=malloc(sizeof(Node)); n->type=N_VAR; n->var=strdup(s); n->l=n->r=NULL; return n; }
Node *new_op(char op, Node *l, Node *r){ Node *n=malloc(sizeof(Node)); n->type=N_OP; n->op=op; n->l=l; n->r=r; n->var=NULL; return n; }

typedef enum { S_ASSIGN, S_INPUT, S_OUTPUT } StType;
typedef struct Stmt {
    StType type;
    char *var;
    Node *expr; // for assign
} Stmt;

/* dynamic arrays for stmts and vars */
Stmt **stmts = NULL; int stmts_n=0, stmts_cap=0;
char **var_list = NULL; int var_n=0, var_cap=0;

void add_stmt(Stmt *s){ if(stmts_n==stmts_cap){ stmts_cap = stmts_cap?stmts_cap*2:64; stmts = realloc(stmts, stmts_cap*sizeof(Stmt*)); } stmts[stmts_n++] = s; }
void add_varname(const char *v){ for(int i=0;i<var_n;i++) if(strcmp(var_list[i], v)==0) return; if(var_n==var_cap){ var_cap = var_cap?var_cap*2:64; var_list = realloc(var_list, var_cap*sizeof(char*)); } var_list[var_n++] = strdup(v); }

/* -----------------------
   PARSER (recursive descent) -> builds AST and statements
   ----------------------- */
TokenList *G; // global tokenlist pointer for parser

Node *parse_expr(); // forward

Node *parse_factor(){
    Token t = tl_peek(G,0);
    if(t.type == T_NUM){
        tl_next(G);
        return new_num(atoll(t.text));
    } else if(t.type == T_AMP){
        tl_next(G);
        Token id = tl_next(G);
        if(id.type != T_ID){ fprintf(stderr,"Expected identifier after &\n"); exit(1); }
        return new_var(id.text);
    } else if(t.type == T_LP){
        tl_next(G);
        Node *n = parse_expr();
        Token r = tl_next(G);
        if(r.type != T_RP){ fprintf(stderr,"Expected )\n"); exit(1); }
        return n;
    } else {
        fprintf(stderr,"Unexpected token in factor\n"); exit(1);
    }
}

Node *parse_term(){
    Node *left = parse_factor();
    while(1){
        Token t = tl_peek(G,0);
        if(t.type == T_MUL || t.type == T_DIV || t.type == T_MOD){
            tl_next(G);
            char op = (t.type==T_MUL?'*':(t.type==T_DIV?'/':'%'));
            Node *r = parse_factor();
            left = new_op(op,left,r);
        } else break;
    }
    return left;
}

Node *parse_expr(){
    Node *left = parse_term();
    while(1){
        Token t = tl_peek(G,0);
        if(t.type == T_PLUS || t.type == T_MINUS){
            tl_next(G);
            char op = (t.type==T_PLUS?'+':'-');
            Node *r = parse_term();
            left = new_op(op,left,r);
        } else break;
    }
    return left;
}

void parse_program(TokenList *tl){
    G = tl;
    Token t = tl_next(tl);
    if(t.type != T_LBRACE){ fprintf(stderr,"Program must start with '{'\n"); exit(1); }
    while(1){
        Token cur = tl_peek(tl,0);
        if(cur.type == T_RBRACE){ tl_next(tl); break; }
        if(cur.type == T_AMP){
            tl_next(tl);
            Token id = tl_next(tl); if(id.type!=T_ID){ fprintf(stderr,"Expected id after &\n"); exit(1); }
            Token eq = tl_next(tl); if(eq.type!=T_EQ){ fprintf(stderr,"Expected =\n"); exit(1); }
            Node *e = parse_expr();
            Token semi = tl_next(tl); if(semi.type!=T_SEMI){ fprintf(stderr,"Expected ;\n"); exit(1); }
            add_varname(id.text);
            Stmt *s = malloc(sizeof(Stmt)); s->type=S_ASSIGN; s->var=strdup(id.text); s->expr=e; add_stmt(s);
            continue;
        }
        if(cur.type == T_INPUT){
            tl_next(tl);
            Token a = tl_next(tl); if(a.type!=T_AMP){ fprintf(stderr,"Expected & after >>\n"); exit(1); }
            Token id = tl_next(tl); if(id.type!=T_ID){ fprintf(stderr,"Expected id after &\n"); exit(1); }
            Token semi = tl_next(tl); if(semi.type!=T_SEMI){ fprintf(stderr,"Expected ;\n"); exit(1); }
            add_varname(id.text);
            Stmt *s = malloc(sizeof(Stmt)); s->type=S_INPUT; s->var=strdup(id.text); s->expr=NULL; add_stmt(s);
            continue;
        }
        if(cur.type == T_OUTPUT){
            tl_next(tl);
            Token a = tl_next(tl); if(a.type!=T_AMP){ fprintf(stderr,"Expected & after <<\n"); exit(1); }
            Token id = tl_next(tl); if(id.type!=T_ID){ fprintf(stderr,"Expected id after &\n"); exit(1); }
            Token semi = tl_next(tl); if(semi.type!=T_SEMI){ fprintf(stderr,"Expected ;\n"); exit(1); }
            add_varname(id.text);
            Stmt *s = malloc(sizeof(Stmt)); s->type=S_OUTPUT; s->var=strdup(id.text); s->expr=NULL; add_stmt(s);
            continue;
        }
        fprintf(stderr,"Unexpected token in program\n"); exit(1);
    }
}

/* -----------------------
   SIMPLE OPT: constant folding
   ----------------------- */
Node *fold_constants(Node *n){
    if(!n) return NULL;
    if(n->type == N_OP){
        n->l = fold_constants(n->l);
        n->r = fold_constants(n->r);
        if(n->l->type == N_NUM && n->r->type == N_NUM){
            long long a = n->l->value, b = n->r->value;
            long long res = 0;
            switch(n->op){
                case '+': res = a + b; break;
                case '-': res = a - b; break;
                case '*': res = a * b; break;
                case '/': if(b==0){ fprintf(stderr,"Division by zero in constant folding\n"); exit(1);} res = a / b; break;
                case '%': if(b==0){ fprintf(stderr,"Modulo by zero in constant folding\n"); exit(1);} res = a % b; break;
                default: fprintf(stderr,"Unknown op in fold\n"); exit(1);
            }
            // free children nodes? skip for brevity
            return new_num(res);
        }
    }
    return n;
}

void apply_constant_folding(){
    for(int i=0;i<stmts_n;i++){
        if(stmts[i]->type == S_ASSIGN){
            stmts[i]->expr = fold_constants(stmts[i]->expr);
        }
    }
}

/* -----------------------
   CODEGEN: emit AT&T x86_64 assembly
   Strategy:
   - For expression: recursively evaluate, leaving result in %rax.
   - Use push/pop for short-term temporaries (simple, safe).
   - After expression, store %rax into variable memory.
   ----------------------- */

FILE *outf;

void emit_header(){
    fprintf(outf,"    .text\n");
    fprintf(outf,"    .globl main\n");
    fprintf(outf,"    .type main, @function\n");
    fprintf(outf,"main:\n");
    fprintf(outf,"    pushq %%rbp\n");
    fprintf(outf,"    movq %%rsp, %%rbp\n");
}

void emit_footer(){
    fprintf(outf,"    movl $0, %%eax\n");
    fprintf(outf,"    leave\n");
    fprintf(outf,"    ret\n");
    fprintf(outf,"    .size main, .-main\n");
    // data and rodata
    fprintf(outf,"    .section .rodata\n");
    fprintf(outf,"fmt_in: .string \"%%lld\"\n");
    fprintf(outf,"fmt_out: .string \"%%lld\\n\"\n");
    fprintf(outf,"    .data\n");
    for(int i=0;i<var_n;i++){
        fprintf(outf,"%s: .quad 0\n", var_list[i]);
    }
    fprintf(outf,"    .extern printf\n");
    fprintf(outf,"    .extern scanf\n");
}

// recursively generate code; result in %rax
void gen_expr(Node *n){
    if(n->type == N_NUM){
        fprintf(outf,"    movq $%lld, %%rax\n", n->value);
        return;
    }
    if(n->type == N_VAR){
        fprintf(outf,"    movq %s(%%rip), %%rax\n", n->var);
        return;
    }
    if(n->type == N_OP){
        // Evaluate left -> rax, push; evaluate right -> rax (right), move to rbx, pop left to rax
        gen_expr(n->l);
        fprintf(outf,"    pushq %%rax\n");
        gen_expr(n->r);
        fprintf(outf,"    movq %%rax, %%rbx\n");
        fprintf(outf,"    popq %%rax\n");
        switch(n->op){
            case '+': fprintf(outf,"    addq %%rbx, %%rax\n"); break;
            case '-': fprintf(outf,"    subq %%rbx, %%rax\n"); break;
            case '*': fprintf(outf,"    imulq %%rbx, %%rax\n"); break;
            case '/':
                fprintf(outf,"    cqto\n"); // sign extend rax -> rdx:rax
                fprintf(outf,"    idivq %%rbx\n");
                break;
            case '%':
                fprintf(outf,"    cqto\n");
                fprintf(outf,"    idivq %%rbx\n");
                fprintf(outf,"    movq %%rdx, %%rax\n");
                break;
            default: fprintf(stderr,"Unknown op in codegen\n"); exit(1);
        }
        return;
    }
}

/* Emit code for each statement */
void gen_stmt_assign(Stmt *s){
    gen_expr(s->expr); // result in rax
    fprintf(outf,"    movq %%rax, %s(%%rip)\n", s->var);
}
void gen_stmt_input(Stmt *s){
    fprintf(outf,"    leaq %s(%%rip), %%rsi\n", s->var);
    fprintf(outf,"    leaq fmt_in(%%rip), %%rdi\n");
    fprintf(outf,"    movl $0, %%eax\n");
    fprintf(outf,"    call scanf@PLT\n");
}
void gen_stmt_output(Stmt *s){
    fprintf(outf,"    movq %s(%%rip), %%rsi\n", s->var);
    fprintf(outf,"    leaq fmt_out(%%rip), %%rdi\n");
    fprintf(outf,"    movl $0, %%eax\n");
    fprintf(outf,"    call printf@PLT\n");
}

/* -----------------------
   ENTRY: main compiler driver
   ----------------------- */
int main(int argc, char **argv){
    if(argc < 3){ fprintf(stderr,"Usage: %s source.txt output_binary\n", argv[0]); return 1; }
    char *src = read_file(argv[1]);
    TokenList toks; tokenize(src, &toks);
    parse_program(&toks);

    // optimization pass: constant folding
    apply_constant_folding();

    // produce assembly file
    const char *tmpasm = "tmp_out.s";
    outf = fopen(tmpasm,"w");
    if(!outf){ perror("open tmp asm"); return 1; }

    emit_header();
    for(int i=0;i<stmts_n;i++){
        Stmt *s = stmts[i];
        if(s->type == S_ASSIGN) gen_stmt_assign(s);
        else if(s->type == S_INPUT) gen_stmt_input(s);
        else if(s->type == S_OUTPUT) gen_stmt_output(s);
    }
    emit_footer();
    fclose(outf);

    // assemble+link with gcc
    char cmd[1024];
    snprintf(cmd,sizeof(cmd),"gcc %s -no-pie -o %s", tmpasm, argv[2]);
    printf("Running: %s\n", cmd);
    int r = system(cmd);
    if(r != 0){ fprintf(stderr,"gcc failed: %d\n", r); return 1; }
    printf("Compiled %s successfully.\n", argv[2]);
    return 0;
}
