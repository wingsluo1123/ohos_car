#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8051
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE] = {0};
    socklen_t client_addr_len = sizeof(client_addr);

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // 绑定套接字到地址
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听连接请求
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", SERVER_PORT);

    // 接受客户端连接
    while ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) >= 0) {
        printf("New client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // 循环读取客户端消息并回复
        while (read(client_fd, buffer, BUFFER_SIZE - 1) > 0) {
            buffer[strcspn(buffer, "\n")] = 0; // 移除换行符
            printf("Received from client: %s\n", buffer);

            // 发送自定义响应消息到客户端
            const char *response = "Your message has been received and processed!";
            send(client_fd, response, strlen(response), 0);
        }

        // 关闭客户端连接
        close(client_fd);
    }

    // 关闭服务器套接字
    close(server_fd);
    return 0;
}