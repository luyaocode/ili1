#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


//
void parse_expr();

// 词法分析部分（不变）
typedef enum {
    TOKEN_NUM,    // 数字
    TOKEN_PLUS,   // '+'
    TOKEN_MINUS,  // '-'
    TOKEN_MUL,    // '*'
    TOKEN_DIV,    // '/'
    TOKEN_LPAREN, // '('
    TOKEN_RPAREN, // ')'
    TOKEN_EOF     // 结束符
} TokenType;

typedef struct {
    TokenType type;
    int value;
} Token;

static const char* input;
static int pos;

void lexer_init(const char* src) {
    input = src;
    pos = 0;
}

void skip_whitespace() {
    while (isspace(input[pos])) {
        pos++;
    }
}

Token next_token() {
    skip_whitespace();
    char c = input[pos];

    if (c == '\0') {
        return (Token){TOKEN_EOF, 0};
    } else if (c == '+') {
        pos++;
        return (Token){TOKEN_PLUS, 0};
    } else if (c == '-') {
        pos++;
        return (Token){TOKEN_MINUS, 0};
    } else if (c == '*') {
        pos++;
        return (Token){TOKEN_MUL, 0};
    } else if (c == '/') {
        pos++;
        return (Token){TOKEN_DIV, 0};
    } else if (c == '(') {
        pos++;
        return (Token){TOKEN_LPAREN, 0};
    } else if (c == ')') {
        pos++;
        return (Token){TOKEN_RPAREN, 0};
    } else if (isdigit(c)) {
        int num = 0;
        while (isdigit(input[pos])) {
            num = num * 10 + (input[pos] - '0');
            pos++;
        }
        return (Token){TOKEN_NUM, num};
    } else {
        fprintf(stderr, "错误：未知字符 '%c'\n", c);
        exit(1);
    }
}

// 语法分析部分（修改汇编生成逻辑为AT&T语法）
static Token current_token;

void advance() {
    current_token = next_token();
}

void match(TokenType expected) {
    if (current_token.type != expected) {
        fprintf(stderr, "语法错误：预期令牌类型 %d，实际 %d\n", expected, current_token.type);
        exit(1);
    }
    advance();
}

// 解析因子（数字或括号表达式）
void parse_factor() {
    switch (current_token.type) {
        case TOKEN_NUM:
            // AT&T语法：mov $立即数, %寄存器（源在前，目的在后）
            printf("    movl $%d, %%eax\n", current_token.value);  // 先存到32位eax（兼容64位）
            printf("    movq %%rax, %%rax\n");  // 扩展到64位rax
            advance();
            break;
        case TOKEN_LPAREN:
            advance();
            parse_expr();
            match(TOKEN_RPAREN);
            break;
        default:
            fprintf(stderr, "语法错误：意外令牌 %d\n", current_token.type);
            exit(1);
    }
}

// 解析项（乘除运算）
void parse_term() {
    parse_factor();
    while (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV) {
        TokenType op = current_token.type;
        advance();
        // 保存当前rax到栈（AT&T的push格式：pushq %寄存器）
        printf("    pushq %%rax\n");
        parse_factor();
        // 从栈恢复到rbx
        printf("    popq %%rbx\n");

        if (op == TOKEN_MUL) {
            // AT&T：imul 源, 目的（rax = rbx * rax）
            printf("    imulq %%rbx, %%rax\n");
        } else {
            // 除法：rax = rbx / rax（注意操作数顺序）
            printf("    cqo\n");               // 符号扩展（rax -> rdx:rax）
            printf("    idivq %%rbx\n");       // rdx:rax / rbx，商存rax
        }
    }
}

// 解析表达式（加减运算）
void parse_expr() {
    parse_term();
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        TokenType op = current_token.type;
        advance();
        // 保存当前rax到栈
        printf("    pushq %%rax\n");
        parse_term();
        // 从栈恢复到rbx
        printf("    popq %%rbx\n");

        if (op == TOKEN_PLUS) {
            // AT&T：add 源, 目的（rax += rbx → add rbx, rax）
            printf("    addq %%rbx, %%rax\n");
        } else {
            // 减法：rax = rbx - rax（源在前，目的在后）
            printf("    subq %%rax, %%rbx\n");  // rbx = rbx - rax
            printf("    movq %%rbx, %%rax\n");  // rax = rbx
        }
    }
}

// 主函数（生成AT&T语法汇编头部）
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法：%s \"表达式\"\n", argv[0]);
        fprintf(stderr, "示例：%s \"1 + 2 * 3 + (4 - 5)\"\n", argv[0]);
        return 1;
    }

    lexer_init(argv[1]);
    advance();

    // AT&T语法的汇编头部（.section, .globl）
    printf(".section .text\n");
    printf(".globl _start\n");
    printf("_start:\n");

    parse_expr();

    // 系统调用（Linux x86_64，AT&T语法）
    printf("    movq %%rax, %%rdi    ; 退出码为表达式结果\n");
    printf("    movq $60, %%rax      ; sys_exit系统调用号\n");
    printf("    syscall\n");

    return 0;
}
