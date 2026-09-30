#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include "../include/uart_base.h"

#define BUFFER_SIZE 4096
#define DEFAULT_PORT 8888

int serial_fd, server_fd, client_fd;

// 前向声明
void signal_handler(int sig);

// 信号处理函数
void signal_handler(int sig)
{
    printf("\n正在关闭程序...\n");
    if (client_fd > 0)
        close(client_fd);
    if (server_fd > 0)
        close(server_fd);
    if (serial_fd > 0)
    {
        UartClose(serial_fd);
    }
    exit(0);
}

// 获取并打印网络信息
void print_network_info()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    printf("网络信息:\n");

    // 获取WIFI名称
    FILE *fp = popen("iwconfig 2>/dev/null | grep 'ESSID:' | head -1 | awk '{print $4}' | sed 's/ESSID://' | tr -d '\"'", "r");
    if (fp) {
        char wifi_name[256] = {0};
        if (fgets(wifi_name, sizeof(wifi_name), fp)) {
            wifi_name[strcspn(wifi_name, "\n")] = 0; // 移除换行符
            printf("  WIFI名称: %s\n", wifi_name[0] ? wifi_name : "未连接");
        } else {
            printf("  WIFI名称: 未知\n");
        }
        pclose(fp);
    } else {
        printf("  WIFI名称: 无法获取\n");
    }

    // 获取IP地址
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }

            // 只显示非本地回环的IP
            if (strcmp(ifa->ifa_name, "lo") != 0 && strncmp(host, "127.", 4) != 0) {
                printf("  IP地址 (%s): %s\n", ifa->ifa_name, host);
            }
        }
    }

    freeifaddrs(ifaddr);
    printf("\n");
}

// 打开并配置串口
int open_serial_port(const char *port, int baudrate)
{
    // 打开串口
    int fd = UartOpen(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == UART_ERR)
    {
        perror("打开串口失败");
        return -1;
    }

    // 配置串口参数
    struct SerialPortConfig config = {
        .baudrate = baudrate,
        .databits = 8,
        .stopbits = 1,
        .parity = 'n',                    // 无校验
        .vtime = 1,                       // 读取超时 0.1秒
        .vmin = 0,                        // 最小读取字节数 0（非阻塞）
        .flow_control = SERIAL_FLOW_NONE, // 无流控
        .raw_mode = 1                     // 原始模式
    };

    if (UartInit(fd, &config) != UART_OK)
    {
        perror("配置串口失败");
        UartClose(fd);
        return -1;
    }

    printf("串口 %s 打开成功，波特率：%d\n", port, baudrate);
    return fd;
}

// 创建TCP服务器
int create_tcp_server(int port)
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // 创建socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket创建失败");
        return -1;
    }

    // 设置socket选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt失败");
        close(server_fd);
        return -1;
    }

    // 设置地址结构
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 绑定地址
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind失败");
        close(server_fd);
        return -1;
    }

    // 开始监听
    if (listen(server_fd, 1) < 0)
    {
        perror("listen失败");
        close(server_fd);
        return -1;
    }

    printf("TCP服务器启动成功，端口：%d\n", port);
    return server_fd;
}

// 处理客户端连接
void handle_client_connection()
{
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int max_fd, n;
    struct timeval timeout;

    printf("客户端已连接，开始数据透传...\n");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);
        FD_SET(client_fd, &readfds);

        max_fd = (serial_fd > client_fd) ? serial_fd : client_fd;

        // 设置超时（1秒）
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
                continue; // 被信号中断，继续
            perror("select错误");
            break;
        }

        if (activity == 0)
        {
            // 超时，检查连接是否仍然有效
            char test_buf[1];
            if (recv(client_fd, test_buf, sizeof(test_buf), MSG_PEEK | MSG_DONTWAIT) == 0)
            {
                printf("客户端连接已断开\n");
                break;
            }
            continue;
        }

        // 从串口读取数据并发送到TCP客户端
        if (FD_ISSET(serial_fd, &readfds))
        {
            n = UartRead(serial_fd, buffer, BUFFER_SIZE);
            if (n > 0)
            {
                if (send(client_fd, buffer, n, 0) < 0)
                {
                    perror("发送到TCP客户端失败");
                    break;
                }
                printf("串口→网络: %d 字节\n", n);
            }
            else if (n < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("读取串口失败");
                    break;
                }
            }
        }

        // 从TCP客户端读取数据并写入串口
        if (FD_ISSET(client_fd, &readfds))
        {
            n = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (n > 0)
            {
                if (UartWrite(serial_fd, buffer, n) < 0)
                {
                    perror("写入串口失败");
                    break;
                }
                printf("网络→串口: %d 字节\n", n);
            }
            else if (n == 0)
            {
                printf("客户端断开连接\n");
                break;
            }
            else
            {
                perror("从TCP客户端接收失败");
                break;
            }
        }
    }

    close(client_fd);
    client_fd = -1;
    printf("客户端连接处理结束\n");
}

int main(int argc, char *argv[])
{
    const char *serial_port = "/dev/ttyS7";
    int baudrate = 115200; // 改为更通用的115200
    int port = DEFAULT_PORT;

    // 解析命令行参数
    if (argc > 1)
        serial_port = argv[1];
    if (argc > 2)
        baudrate = atoi(argv[2]);
    if (argc > 3)
        port = atoi(argv[3]);

    printf("串口转TCP服务器\n");
    printf("串口: %s, 波特率: %d, TCP端口: %d\n", serial_port, baudrate, port);

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 打开串口
    serial_fd = open_serial_port(serial_port, baudrate);
    if (serial_fd < 0)
    {
        return 1;
    }

    // 创建TCP服务器
    server_fd = create_tcp_server(port);
    if (server_fd < 0)
    {
        UartClose(serial_fd);
        return 1;
    }

    // 打印网络信息
    print_network_info();

    // 主循环：等待客户端连接
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        printf("等待客户端连接...\n");
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept失败");
            continue;
        }

        printf("新的客户端连接: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client_connection();
    }

    return 0;
}
