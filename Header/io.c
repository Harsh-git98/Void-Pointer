
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)


static long sys_write(int fd, const void *buf, unsigned long count) {
    long ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(1), "D"(fd), "S"(buf), "d"(count)
                 : "rcx", "r11", "memory");
    return ret;
}

static long sys_read(int fd, void *buf, unsigned long count) {
    long ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(0), "D"(fd), "S"(buf), "d"(count)
                 : "rcx", "r11", "memory");
    return ret;
}


static void print_int(int num) {
    char buf[32];
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else {
        if (num < 0) {
            sys_write(1, "-", 1);
            num = -num;
        }
        char tmp[32];
        int j = 0;
        while (num > 0) {
            tmp[j++] = '0' + (num % 10);
            num /= 10;
        }
        while (j > 0) buf[i++] = tmp[--j];
    }
    sys_write(1, buf, i);
}

static int read_int() {
    char buf[32];
    int len = sys_read(0, buf, sizeof(buf));
    int num = 0, i = 0, sign = 1;
    if (buf[0] == '-') { sign = -1; i++; }
    for (; i < len && buf[i] >= '0' && buf[i] <= '9'; i++) {
        num = num * 10 + (buf[i] - '0');
    }
    return num * sign;
}

int print_func(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == 'd') {
                int val = va_arg(args, int);
                print_int(val);
            } else if (*p == 's') {
                char *str = va_arg(args, char*);
                while (*str) sys_write(1, str++, 1);
            } else if (*p == 'c') {
                char c = (char)va_arg(args, int);
                sys_write(1, &c, 1);
            } else if (*p == '%') {
                sys_write(1, "%", 1);
            }
        } else {
            sys_write(1, p, 1);
        }
    }

    va_end(args);
    return 0;
}

int scan_func(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == 'd') {
                int *out = va_arg(args, int*);
                *out = read_int();
            } else if (*p == 's') {
                char *out = va_arg(args, char*);
                int len = sys_read(0, out, 100);
                if (len > 0) {
                    if (out[len-1] == '\n') out[len-1] = '\0';
                    else out[len] = '\0';
                }
            } else if (*p == 'c') {
                char *out = va_arg(args, char*);
                sys_read(0, out, 1);
            }
        }
    }

    va_end(args);
    return 0;
}
