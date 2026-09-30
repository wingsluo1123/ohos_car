/*
 * @Author: Camera TCP Server
 * @Date: 2025-12-02
 * @Description: TCP服务器 - 发送原始MJPEG视频流
 */

#include "main.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <time.h>

/**
 * @brief 全局视频设备信息
 */
static struct _v4l2_video g_video = {0};

/**
 * @brief 线程控制变量
 */
static volatile int g_streaming = 0;
static volatile int g_client_connected = 0;
static int g_server_socket = -1;
static int g_fps = 30; // 默认帧率

/**
 * @brief 打印使用帮助信息
 */
void print_usage(const char *program_name)
{
    printf("[%s] Usage: %s --device <device_path> --width <width> --height <height> --port <port> --fps <frames_per_second>\n", __func__, program_name);
    printf("[%s] Example: %s --device /dev/video0 --width 640 --height 480 --port 8888 --fps 30\n", __func__, program_name);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @param device_ptr 指向设备路径指针的指针
 * @param width_ptr 指向宽度的指针
 * @param height_ptr 指向高度的指针
 * @param port_ptr 指向端口的指针
 * @param fps_ptr 指向帧率的指针
 * @return 成功返回 0，失败返回 -1
 */
int parse_arguments(int argc, char **argv, const char **device_ptr, int *width_ptr, int *height_ptr, int *port_ptr, int *fps_ptr)
{
    // 检查参数数量
    if (argc != 11)
    { // 程序名 + 5个参数对
        printf("[%s] Error: Invalid number of arguments\n", "parse_arguments");
        print_usage(argv[0]);
        return -1;
    }

    for (int i = 1; i < argc;)
    {
        if (strcmp(argv[i], "--device") == 0 && i + 1 < argc)
        {
            *device_ptr = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            *width_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            *height_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
        {
            *port_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc)
        {
            *fps_ptr = atoi(argv[i + 1]);
            i += 2;
        }
        else
        {
            printf("[%s] Error: Invalid argument '%s'\n", "parse_arguments", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    // 检查所有必需参数是否都已设置
    if (*device_ptr == NULL || *width_ptr <= 0 || *height_ptr <= 0 || *port_ptr <= 0 || *fps_ptr <= 0)
    {
        printf("[%s] Error: All parameters must be specified with valid values\n", "parse_arguments");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief 视频流发送线程函数 - 发送原始MJPEG流
 *
 * @param arg 客户端套接字指针
 * @return NULL
 */
void *stream_thread(void *arg)
{
    int client = *(int *)arg;
    int index = 0, bytesused = 0;
    int ret = 0;
    long frame_interval_ns = 1000000000L / g_fps; // 根据帧率计算帧间隔（纳秒）

    printf("[%s] Stream thread started, sending MJPEG frames at %d fps\n", __func__, g_fps);

    while (g_streaming && g_client_connected)
    {
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        // 从队列取出数据
        ret = CameraReadFrame(&g_video, &index, &bytesused);
        if (ret < 0)
        {
            printf("[%s] Failed to read frame\n", __func__);
            break;
        }

        // 检查是否为JPEG/MJPEG数据
        unsigned char *framePtr = (unsigned char *)g_video.mmap_buf[index].addr;
        int is_jpeg = (bytesused >= 2 && framePtr[0] == 0xFF && framePtr[1] == 0xD8);
        if (!is_jpeg)
        {
            printf("[%s] Frame is not JPEG/MJPEG format\n", __func__);
            CameraQueueBuffer(&g_video, index);
            continue;
        }

        // 发送帧头：4字节帧大小（网络字节序）+ JPEG数据
        uint32_t frame_size = htonl(bytesused);
        int sent = send(client, &frame_size, sizeof(frame_size), 0);
        if (sent <= 0)
        {
            printf("[%s] Client disconnected or send error\n", __func__);
            CameraQueueBuffer(&g_video, index);
            break;
        }

        // 发送JPEG图像数据
        sent = send(client, g_video.mmap_buf[index].addr, bytesused, 0);
        if (sent <= 0)
        {
            printf("[%s] Client disconnected or send error\n", __func__);
            CameraQueueBuffer(&g_video, index);
            break;
        }

        // 将缓冲区重新放入队列
        CameraQueueBuffer(&g_video, index);

        // 帧率控制：计算已用时间，休眠剩余时间
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        long elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
        long sleep_ns = frame_interval_ns - elapsed_ns;

        if (sleep_ns > 0)
        {
            struct timespec ts;
            ts.tv_sec = sleep_ns / 1000000000L;
            ts.tv_nsec = sleep_ns % 1000000000L;
            nanosleep(&ts, NULL);
        }
    }

    printf("[%s] Stream thread stopped\n", __func__);
    return NULL;
}

/**
 * @brief 信号处理函数
 *
 * @param sig 信号编号
 */
void signal_handler(int sig)
{
    printf("[%s] Received signal %d\n", __func__, sig);
    g_streaming = 0;
    g_client_connected = 0;

    if (g_server_socket != -1)
    {
        close(g_server_socket);
        g_server_socket = -1;
    }
}

/**
 * @brief 初始化摄像头设备
 *
 * @param device 设备路径
 * @param width 图像宽度
 * @param height 图像高度
 * @return 成功返回 0，失败返回 -1
 */
int init_camera(const char *device, int width, int height)
{
    int ret = 0;

    // 打开设备
    g_video.fd = CameraOpen(device);
    if (g_video.fd < 0)
    {
        printf("[%s] Failed to open camera device: %s\n", __func__, device);
        return -1;
    }

    // 查询设备能力
    if (CameraQueryCapability(&g_video) < 0)
    {
        printf("[%s] Failed to query camera capability\n", __func__);
        ret = -1;
        goto error;
    }

    // 设置参数
    if (CameraSetParameters(&g_video, width, height) < 0)
    {
        printf("[%s] Failed to set camera parameters\n", __func__);
        ret = -1;
        goto error;
    }

    // 设置内存映射(使用4个缓冲区)
    if (CameraSetMemoryMap(&g_video, 4) < 0)
    {
        printf("[%s] Failed to set memory map\n", __func__);
        ret = -1;
        goto error;
    }

    // 启动数据流
    if (CameraStreamOn(&g_video) < 0)
    {
        printf("[%s] Failed to start camera stream\n", __func__);
        ret = -1;
        goto error;
    }

    return 0;

error:
    CameraClose(&g_video);
    return ret;
}

/**
 * @brief 主函数入口
 *
 * 程序执行的起始点，用于启动TCP服务器并发送摄像头MJPEG视频流
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 返回程序执行状态码，正常退出返回0
 */
int main(int argc, char **argv)
{
    int width = 0;
    int height = 0;
    int port = 0;
    const char *device = NULL;

    if (parse_arguments(argc, argv, &device, &width, &height, &port, &g_fps) != 0)
    {
        return -1;
    }

    // 初始化摄像头
    if (init_camera(device, width, height) != 0)
    {
        printf("[%s] Failed to initialize camera\n", __func__);
        return -1;
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 启动流传输标志
    g_streaming = 1;

    // 启动TCP服务器
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0)
    {
        printf("[%s] socket failed: %s\n", __func__, strerror(errno));
        CameraClose(&g_video);
        return -1;
    }

    g_server_socket = srv;

    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("[%s] bind failed: %s\n", __func__, strerror(errno));
        close(srv);
        CameraClose(&g_video);
        return -1;
    }

    if (listen(srv, 1) < 0)
    {
        printf("[%s] listen failed: %s\n", __func__, strerror(errno));
        close(srv);
        CameraClose(&g_video);
        return -1;
    }

    printf("[%s] TCP server started on port %d\n", __func__, port);
    printf("[%s] Video stream settings: %dx%d @ %d fps\n", __func__, width, height, g_fps);
    printf("[%s] Waiting for client connection...\n", __func__);

    while (g_streaming)
    {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int client = accept(srv, (struct sockaddr *)&cli, &clen);
        if (client < 0)
        {
            if (g_streaming)
            {
                printf("[%s] accept failed: %s\n", __func__, strerror(errno));
            }
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[%s] Client connected from %s:%d\n", __func__, client_ip, ntohs(cli.sin_port));

        // 设置客户端连接标志
        g_client_connected = 1;

        // 创建视频流线程
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, stream_thread, &client);
        if (ret != 0)
        {
            printf("[%s] Failed to create stream thread: %s\n", __func__, strerror(errno));
            g_client_connected = 0;
            close(client);
            continue;
        }

        // 等待线程结束（客户端断开）
        pthread_join(tid, NULL);

        close(client);
        printf("[%s] Client disconnected, waiting for next connection...\n", __func__);
    }

    // 关闭摄像头
    CameraStreamOff(&g_video);
    CameraClose(&g_video);

    // 关闭服务器套接字
    if (srv != -1)
    {
        close(srv);
    }

    printf("[%s] Server stopped\n", __func__);
    return 0;
}
