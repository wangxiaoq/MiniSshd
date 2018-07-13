#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "util.h"

int init_socket_and_connect(char *addr, short port)
{
    int sockfd;
    struct sockaddr_in c_addr = {
        .sin_family = AF_INET,
        .sin_port = PORT,
        .sin_addr.s_addr = inet_addr(addr)
    };

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
        return -1;
    }

    return sockfd;
}

void start_ssh_client(int skfd)
{
    char buf[MAX_CMD_LEN] = {0};
    char result[MAX_BUF_LEN] = {0};
    int rd_size = 0;
    int recv_size = 0;

    while (1) {
        memset(buf, 0, MAX_CMD_LEN);
        memset(result, 0, MAX_BUF_LEN);
        rd_size = read(STDIN_FILENO, buf, MAX_CMD_LEN);
        if (rd_size <= 0) {
            continue;
        }

        if (send(skfd, buf, rd_size, 0) < 0) {
            continue;
        }

        if (!strcmp(buf, "exit")) {
            exit(0);
        }

        recv_size = recv(skfd, result, MAX_BUF_LEN, 0);
        if (recv_size <= 0) {
            continue;
        }
        write(STDOUT_FILENO, result, recv_size);
    }
}

int main(int argc, char *argv[], char *envp[])
{
    int fd;

    fd = init_socket_and_connect(ADDR, PORT);
    if (fd < 0) {
        return -1;
    }

    start_ssh_client(fd);

    return 0;
}
