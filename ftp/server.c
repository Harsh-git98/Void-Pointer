/* mini_ftp_server.c
 *
 * Minimal FTP server for learning purposes.
 * Compile:
 *    gcc -o mini_ftp_server mini_ftp_server.c
 *
 * Run:
 *    ./mini_ftp_server [port]
 * Default port: 2121
 *
 * NOTE: This is educational. Do NOT run as root on a public-facing machine.
 */

#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#define BACKLOG 10
#define BUF_SIZE 8192

/* Session state structure */
typedef struct {
    int ctrl_fd;            // control connection socket
    int data_fd;            // data connection socket (connected)
    int pasv_listen_fd;     // passive listening socket (if PASV)
    int is_logged_in;
    int is_binary;          // 1 = binary (TYPE I), 0 = ASCII (TYPE A)
    char cwdir[1024];       // current working dir (server side)
    struct sockaddr_in active_addr; // for PORT (active mode)
    int active_mode;        // 1 = use active (PORT), 0 = use PASV
} session_t;

/* Utility: send formatted FTP reply to control socket */
void ftp_reply(int fd, int code, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    char out[2048];
    snprintf(out, sizeof(out), "%d %s\r\n", code, buf);
    send(fd, out, strlen(out), 0);
    // also print to server console
    printf("-> %s", out);
}

/* Read a line (terminated by \r\n or \n) from fd into buf (null-terminated) */
ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    ssize_t n;
    while (i + 1 < maxlen) {
        n = recv(fd, &c, 1, 0);
        if (n <= 0) {
            if (n == 0) return 0;
            return -1;
        }
        if (c == '\r') continue;
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return i;
}

/* Create listening socket bound to specified port (0 for ephemeral) and return fd.
   Also fills sockaddr_in with bound address. */
int create_listen_socket(int port, struct sockaddr_in *bound_addr) {
    int s;
    int opt = 1;
    struct sockaddr_in addr;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(s); return -1;
    }
    if (listen(s, 1) < 0) { close(s); return -1; }
    if (bound_addr) {
        socklen_t len = sizeof(*bound_addr);
        if (getsockname(s, (struct sockaddr*)bound_addr, &len) == -1) {
            // ignore
        }
    }
    return s;
}

/* Accept the data connection for passive mode (blocking) */
int accept_pasv_connection(session_t *sess) {
    if (sess->pasv_listen_fd < 0) return -1;
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int ds = accept(sess->pasv_listen_fd, (struct sockaddr*)&client, &len);
    if (ds < 0) return -1;
    // close the passive listen socket right away (single use)
    close(sess->pasv_listen_fd);
    sess->pasv_listen_fd = -1;
    return ds;
}

/* Connect to client for active mode (PORT) */
int connect_active(session_t *sess) {
    int ds = socket(AF_INET, SOCK_STREAM, 0);
    if (ds < 0) return -1;
    if (connect(ds, (struct sockaddr*)&sess->active_addr, sizeof(sess->active_addr)) < 0) {
        close(ds);
        return -1;
    }
    return ds;
}

/* Open data connection according to current mode. On success returns fd (connected). */
int open_data_connection(session_t *sess) {
    if (sess->active_mode) {
        int ds = connect_active(sess);
        if (ds < 0) return -1;
        return ds;
    } else {
        // passive mode: accept connection on pasv_listen_fd
        return accept_pasv_connection(sess);
    }
}

/* Make simple 'ls -l' style directory listing into buf; returns 0 on success */
int make_list_output(const char *path, char **out_buf, size_t *out_len) {
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *entry;
    size_t cap = 8192;
    size_t used = 0;
    char *buf = malloc(cap);
    if (!buf) { closedir(d); return -1; }
    buf[0] = '\0';

    // Minimal listing: permissions, size, name, mtime
    while ((entry = readdir(d)) != NULL) {
        char full[2048];
        struct stat st;
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        if (stat(full, &st) == -1) {
            continue;
        }
        char timestr[64];
        struct tm *t = localtime(&st.st_mtime);
        strftime(timestr, sizeof(timestr), "%b %d %H:%M", t);

        // Simplified permission string
        char perm[11] = "----------";
        if (S_ISDIR(st.st_mode)) perm[0] = 'd';
        if (st.st_mode & S_IRUSR) perm[1] = 'r';
        if (st.st_mode & S_IWUSR) perm[2] = 'w';
        if (st.st_mode & S_IXUSR) perm[3] = 'x';
        if (st.st_mode & S_IRGRP) perm[4] = 'r';
        if (st.st_mode & S_IWGRP) perm[5] = 'w';
        if (st.st_mode & S_IXGRP) perm[6] = 'x';
        if (st.st_mode & S_IROTH) perm[7] = 'r';
        if (st.st_mode & S_IWOTH) perm[8] = 'w';
        if (st.st_mode & S_IXOTH) perm[9] = 'x';

        char line[4096];
        snprintf(line, sizeof(line), "%s 1 owner group %10lld %s %s\r\n",
                 perm, (long long)st.st_size, timestr, entry->d_name);

        size_t need = strlen(line);
        if (used + need + 1 > cap) {
            cap = cap * 2 + need;
            char *nb = realloc(buf, cap);
            if (!nb) { free(buf); closedir(d); return -1; }
            buf = nb;
        }
        memcpy(buf + used, line, need);
        used += need;
        buf[used] = '\0';
    }
    closedir(d);
    *out_buf = buf;
    *out_len = used;
    return 0;
}

/* Handle LIST command */
void handle_LIST(session_t *sess, const char *arg) {
    char path[1024];
    if (!arg || strlen(arg) == 0) snprintf(path, sizeof(path), "%s", sess->cwdir);
    else {
        if (arg[0] == '/') snprintf(path, sizeof(path), "%s", arg);
        else snprintf(path, sizeof(path), "%s/%s", sess->cwdir, arg);
    }

    // Prepare listing in memory
    char *listbuf = NULL;
    size_t listlen = 0;
    if (make_list_output(path, &listbuf, &listlen) < 0) {
        ftp_reply(sess->ctrl_fd, 550, "Failed to list directory.");
        return;
    }

    ftp_reply(sess->ctrl_fd, 150, "Here comes the directory listing.");

    int ds = open_data_connection(sess);
    if (ds < 0) {
        ftp_reply(sess->ctrl_fd, 425, "Can't open data connection.");
        free(listbuf);
        return;
    }
    // send listing
    ssize_t sent = 0;
    while (sent < (ssize_t)listlen) {
        ssize_t n = send(ds, listbuf + sent, listlen - sent, 0);
        if (n <= 0) break;
        sent += n;
    }
    close(ds);
    ftp_reply(sess->ctrl_fd, 226, "Directory send OK.");
    free(listbuf);
}

/* Handle RETR command (download) */
void handle_RETR(session_t *sess, const char *filename) {
    if (!filename) { ftp_reply(sess->ctrl_fd, 501, "No filename provided."); return; }

    char full[2048];
    if (filename[0] == '/') snprintf(full, sizeof(full), "%s", filename);
    else snprintf(full, sizeof(full), "%s/%s", sess->cwdir, filename);

    int fd = open(full, O_RDONLY);
    if (fd < 0) { ftp_reply(sess->ctrl_fd, 550, "Failed to open file."); return; }

    struct stat st;
    fstat(fd, &st);

    ftp_reply(sess->ctrl_fd, 150, "Opening data connection for file transfer.");

    int ds = open_data_connection(sess);
    if (ds < 0) {
        ftp_reply(sess->ctrl_fd, 425, "Can't open data connection.");
        close(fd);
        return;
    }

    ssize_t sent = 0;
#ifdef __linux__
    // use sendfile on linux if available
    off_t offset = 0;
    while (offset < st.st_size) {
        ssize_t n = sendfile(ds, fd, &offset, st.st_size - offset);
        if (n <= 0) break;
    }
#else
    // portable fallback
    char buf[BUF_SIZE];
    ssize_t r;
    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        char *p = buf;
        ssize_t left = r;
        while (left > 0) {
            ssize_t w = send(ds, p, left, 0);
            if (w <= 0) goto done_send;
            p += w; left -= w;
        }
    }
done_send:;
#endif

    close(ds);
    close(fd);
    ftp_reply(sess->ctrl_fd, 226, "Transfer complete.");
}

/* Handle STOR command (upload) */
void handle_STOR(session_t *sess, const char *filename) {
    if (!filename) { ftp_reply(sess->ctrl_fd, 501, "No filename provided."); return; }

    char full[2048];
    if (filename[0] == '/') snprintf(full, sizeof(full), "%s", filename);
    else snprintf(full, sizeof(full), "%s/%s", sess->cwdir, filename);

    int fd = open(full, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) { ftp_reply(sess->ctrl_fd, 550, "Failed to create file."); return; }

    ftp_reply(sess->ctrl_fd, 150, "Opening data connection for file upload.");

    int ds = open_data_connection(sess);
    if (ds < 0) {
        ftp_reply(sess->ctrl_fd, 425, "Can't open data connection.");
        close(fd);
        return;
    }

    char buf[BUF_SIZE];
    ssize_t r;
    while ((r = recv(ds, buf, sizeof(buf), 0)) > 0) {
        ssize_t written = 0;
        while (written < r) {
            ssize_t w = write(fd, buf + written, r - written);
            if (w <= 0) goto done_recv;
            written += w;
        }
    }
done_recv:;
    close(ds);
    close(fd);
    ftp_reply(sess->ctrl_fd, 226, "Transfer complete.");
}

/* Parse PORT argument: "h1,h2,h3,h4,p1,p2" and set active_addr */
int handle_PORT_arg(session_t *sess, const char *arg) {
    int h1,h2,h3,h4,p1,p2;
    if (sscanf(arg, "%d,%d,%d,%d,%d,%d", &h1,&h2,&h3,&h4,&p1,&p2) != 6) return -1;
    char ip[64];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1,h2,h3,h4);
    int port = p1 * 256 + p2;
    sess->active_addr.sin_family = AF_INET;
    sess->active_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &sess->active_addr.sin_addr) != 1) return -1;
    return 0;
}

/* Session loop handling control commands */
void session_loop(session_t *sess) {
    char line[2048];
    ftp_reply(sess->ctrl_fd, 220, "mini-ftp ready.");
    sess->is_binary = 1;
    sess->is_logged_in = 0;
    sess->pasv_listen_fd = -1;
    sess->data_fd = -1;
    sess->active_mode = 0;
    getcwd(sess->cwdir, sizeof(sess->cwdir));

    while (1) {
        ssize_t n = recv_line(sess->ctrl_fd, line, sizeof(line));
        if (n <= 0) break;
        // remove trailing newline
        while (n > 0 && (line[n-1] == '\r' || line[n-1] == '\n')) { line[--n] = '\0'; }

        printf("<- %s\n", line);
        // parse command and arg
        char cmd[16], arg[1024];
        arg[0] = '\0';
        int cnt = sscanf(line, "%15s %1023[^\n]", cmd, arg);
        for (char *p = cmd; *p; ++p) *p = toupper((unsigned char)*p);

        if (strcmp(cmd, "USER") == 0) {
            // accept any username, request password
            ftp_reply(sess->ctrl_fd, 331, "User name okay, need password.");
        } else if (strcmp(cmd, "PASS") == 0) {
            // very simple auth: accept 'password' or anonymous
            if (strcmp(arg, "password") == 0 || strcmp(arg, "anonymous") == 0) {
                sess->is_logged_in = 1;
                ftp_reply(sess->ctrl_fd, 230, "User logged in, proceed.");
            } else {
                ftp_reply(sess->ctrl_fd, 530, "Authentication failed.");
                // still let client try again
            }
        } else if (strcmp(cmd, "SYST") == 0) {
            ftp_reply(sess->ctrl_fd, 215, "UNIX Type: L8");
        } else if (strcmp(cmd, "PWD") == 0) {
            ftp_reply(sess->ctrl_fd, 257, "\"%s\"", sess->cwdir);
        } else if (strcmp(cmd, "CWD") == 0) {
            if (arg[0] == '\0') { ftp_reply(sess->ctrl_fd, 501, "No directory specified."); }
            else {
                if (chdir(arg) == 0) {
                    getcwd(sess->cwdir, sizeof(sess->cwdir));
                    ftp_reply(sess->ctrl_fd, 250, "Directory successfully changed.");
                } else {
                    ftp_reply(sess->ctrl_fd, 550, "Failed to change directory.");
                }
            }
        } else if (strcmp(cmd, "TYPE") == 0) {
            if (arg[0] == 'I') {
                sess->is_binary = 1;
                ftp_reply(sess->ctrl_fd, 200, "Switching to Binary mode.");
            } else if (arg[0] == 'A') {
                sess->is_binary = 0;
                ftp_reply(sess->ctrl_fd, 200, "Switching to ASCII mode.");
            } else ftp_reply(sess->ctrl_fd, 504, "Command not implemented for that parameter.");
        } else if (strcmp(cmd, "PASV") == 0) {
            // close old passive socket if any
            if (sess->pasv_listen_fd >= 0) { close(sess->pasv_listen_fd); sess->pasv_listen_fd = -1; }
            struct sockaddr_in bound;
            int listen_fd = create_listen_socket(0, &bound);
            if (listen_fd < 0) {
                ftp_reply(sess->ctrl_fd, 425, "Couldn't open passive connection.");
            } else {
                sess->pasv_listen_fd = listen_fd;
                sess->active_mode = 0;
                // Build reply with server IP and port
                // Use server's local address; here use 127.0.0.1 if INADDR_ANY bound
                char ipstr[64];
                struct sockaddr_in sa;
                socklen_t len = sizeof(sa);
                getsockname(sess->pasv_listen_fd, (struct sockaddr*)&sa, &len);
                uint32_t ip = sa.sin_addr.s_addr;
                // For simplicity show 127.0.0.1 (clients on same machine can connect). Better: detect external IP.
                char use_ip[32] = "127,0,0,1";
                unsigned short port = ntohs(sa.sin_port);
                int p1 = port / 256;
                int p2 = port % 256;
                ftp_reply(sess->ctrl_fd, 227, "Entering Passive Mode (%s,%d,%d).", use_ip, p1, p2);
            }
        } else if (strcmp(cmd, "PORT") == 0) {
            if (handle_PORT_arg(sess, arg) < 0) {
                ftp_reply(sess->ctrl_fd, 501, "Syntax error in parameters or arguments.");
            } else {
                sess->active_mode = 1;
                ftp_reply(sess->ctrl_fd, 200, "PORT command successful. Consider using PASV.");
            }
        } else if (strcmp(cmd, "LIST") == 0) {
            handle_LIST(sess, (cnt >= 2) ? arg : NULL);
        } else if (strcmp(cmd, "RETR") == 0) {
            handle_RETR(sess, arg);
        } else if (strcmp(cmd, "STOR") == 0) {
            handle_STOR(sess, arg);
        } else if (strcmp(cmd, "QUIT") == 0) {
            ftp_reply(sess->ctrl_fd, 221, "Goodbye.");
            break;
        } else if (strcmp(cmd, "NOOP") == 0) {
            ftp_reply(sess->ctrl_fd, 200, "NOOP ok.");
        } else {
            ftp_reply(sess->ctrl_fd, 502, "Command not implemented.");
        }
    }

    close(sess->ctrl_fd);
}

void handle_client(int client_fd) {
    session_t sess;
    memset(&sess, 0, sizeof(sess));
    sess.ctrl_fd = client_fd;
    sess.pasv_listen_fd = -1;
    sess.data_fd = -1;
    sess.active_mode = 0;
    getcwd(sess.cwdir, sizeof(sess.cwdir));
    session_loop(&sess);
    // cleanup
    if (sess.pasv_listen_fd >= 0) close(sess.pasv_listen_fd);
    close(client_fd);
    _exit(0);
}

void sigchld_handler(int signo) {
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

int main(int argc, char *argv[]) {
    int port = 2121;
    if (argc >= 2) port = atoi(argv[1]);
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(listen_fd, BACKLOG) < 0) { perror("listen"); return 1; }

    signal(SIGCHLD, sigchld_handler);
    printf("mini-ftp server listening on port %d\n", port);
    while (1) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int client_fd = accept(listen_fd, (struct sockaddr*)&cli, &len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept"); break;
        }
        printf("Connection from %s:%d\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork"); close(client_fd);
        } else if (pid == 0) {
            // child
            close(listen_fd);
            handle_client(client_fd);
            // never returns
        } else {
            close(client_fd);
        }
    }
    close(listen_fd);
    return 0;
}
