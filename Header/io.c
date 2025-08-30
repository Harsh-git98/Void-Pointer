#include "io.h"

typedef unsigned long size_t;

int string_len(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void integer_to_ascii(int n, char *buf) {
    char tmp[20];
    int i = 0, j;
    int neg = (n < 0);
    if (neg) n = -n;
    do {
        tmp[i++] = (n % 10) + '0';
        n /= 10;
    } while (n > 0);
    if (neg) tmp[i++] = '-';
    for (j = 0; j < i; j++) buf[j] = tmp[i - j - 1];
    buf[i] = '\0';
}

int ascii_to_integer(const char *s) {
    int num = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        num = num * 10 + (*s - '0');
        s++;
    }
    return neg ? -num : num;
}


long system_write(int fd, const void *buf, size_t count)
{
    long ret;
    asm volatile (
        "movl $4, %%eax;"     // syscall: sys_write
        "movl %1, %%ebx;"
        "movl %2, %%ecx;"
        "movl %3, %%edx;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=r"(ret) : "r"(fd), "r"(buf), "r"(count)
        : "eax","ebx","ecx","edx"
    );
    return ret;
}

long system_read(int fd, void *buf, size_t count) {
    long ret;
    asm volatile (
        "movl $3, %%eax;"     // syscall: sys_read
        "movl %1, %%ebx;"
        "movl %2, %%ecx;"
        "movl %3, %%edx;"
        "int $0x80;"
        "movl %%eax, %0;"
        : "=r"(ret) : "r"(fd), "r"(buf), "r"(count)
        : "eax","ebx","ecx","edx"
    );
    return ret;
}


int print_func(const char *fmt ,...)
{
    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i+1]) {
            i++;
            if (fmt[i] == 's') {
                char *str = *arg++;
                count += sys_write(1, str, string_len(str));
            } else if (fmt[i] == 'd') {
                int num = *(int*)arg++;
                char buf[32];
                integer_to_ascii(num, buf);
                count += sys_write(1, buf, string_len(buf));
            } else if (fmt[i] == 'c') {
                char c = (char)(long)*arg++;
                count += sys_write(1, &c, 1);
            }
        } else {
            sys_write(1, &fmt[i], 1);
            count++;
        }
    }
    return count;
}

int scanf(const char *fmt, ...) {
    char buffer[1024];
    int n = sys_read(0, buffer, 1024);
    if (n <= 0) return -1;
    buffer[n] = '\0';

    const char **arg = (const char**)(&fmt) + 1;
    char *p = buffer;

    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i+1]) {
            i++;
            while (*p==' ' || *p=='\n') p++;
            if (fmt[i] == 'd') {
                int *num = (int*)*arg++;
                *num = ascii_to_integer(p);
                while ((*p >= '0' && *p <= '9') || *p=='-') p++;
            } else if (fmt[i] == 's') {
                char *str = (char*)*arg++;
                int j=0;
                while (*p && *p!=' ' && *p!='\n') str[j++] = *p++;
                str[j] = '\0';
            } else if (fmt[i] == 'c') {
                char *c = (char*)*arg++;
                *c = *p++;
            }
        }
    }
    return 0;
}