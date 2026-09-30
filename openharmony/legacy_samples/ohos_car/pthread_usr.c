/**
 ******************************************************************************
 * @file           : pthread_usr.c
 * @author         : Dylan
 * @brief          : 线程函数实现
 * @attention      : 正在构建
 * @date           : 2023.3.22
 ******************************************************************************
 */

#include "pthread_usr.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

PC_SendType send_data;
extern integral_acc integral_length;
float fAcc[3], fGyro[3], fAngle[3];
int mode_flag = 0;
int vel_temp = 0, yaw_temp = 0, ang_temp = 0;
float cumulativeZAngle = 0.0f;
int new_socket = -1;



#define ACC_UPDATE 0x01
#define GYRO_UPDATE 0x02
#define ANGLE_UPDATE 0x04
#define MAG_UPDATE 0x08
#define READ_UPDATE 0x80
/**
 * @attention 暂时未启用的串口读取函数
 * @return
 */


void *_serial_input_task(void)
{
    int i = 0;
    int ret = ERR;
    int buf = 0;
    char recv[FRAME_LEN] = {0};
    pthread_detach(pthread_self());

    char debug_message[100];
    int length = snprintf(debug_message, sizeof(debug_message), "test _ready\n");

    // 发送调试信息到TCP客户端（如果连接存在）
    if (new_socket > 0) {
        if (send(new_socket, debug_message, length, 0) == -1) {
            perror("send debug message failed");
        }
    }

    while (1)
    {
        for (i = 0; i < FRAME_LEN; i++)
        {
            ret = read(fd_chassis, &buf, 1);
            if (ret == ERR)
            {
                perror("read error\n");
                exit(0);
            }
            recv[i] = buf;
        }
        ret = data_proce(recv);
        if (ret == ERR)
        {
            perror("data process error\n");
            exit(0);
        }
    }
}

void _serial_output_task(void)
{

    pthread_detach(pthread_self());
    send_data = Power(1);
    write(fd_chassis, (unsigned char *)send_data.send_data, 12);
    int forward_setAngle = -1000;

    while (1)
    {
        // pthread_mutex_lock(&lock);

        if (mode_flag == 1) {
            static int turn_counter = 0;  // 记录转弯次数

            static int mode_select_FLAG = 0;  // 添加静态变量声明

            if (mode_select_FLAG == 0) {
                // 角度控制阶段 - 转向90度
                // 计算角度误差
                float angle_error = pid.SetAngle - pid.ActualAngle;

                // 处理角度跨越±180度的情况
                if (angle_error > 180.0f) {
                    angle_error -= 360.0f;
                } else if (angle_error < -180.0f) {
                    angle_error += 360.0f;
                }

                char debug_message[200];
                int length = snprintf(debug_message, sizeof(debug_message),
                                    "TURN MODE - Target: %.1f, Current: %.1f, Error: %.1f\n",
                                    pid.SetAngle, pid.ActualAngle, angle_error);

                // 发送调试信息到TCP客户端
                if (new_socket > 0) {
                    if (send(new_socket, debug_message, length, 0) == -1) {
                        perror("send debug message failed");
                    }
                }

                // 简化的角度控制：直接根据误差方向控制转向
                if (fabs(angle_error) > 5.0f) {  // 减小容差到5度，提高精度
                    // 添加角速度限制，避免过度转向
                    float angular_velocity = 10.0f;  // 降低角速度从15到10

                    // 根据误差大小调整角速度
                    if (fabs(angle_error) > 45.0f) {
                        angular_velocity = 15.0f;  // 大误差时使用较高角速度
                    } else if (fabs(angle_error) > 20.0f) {
                        angular_velocity = 12.0f;  // 中等误差
                    } else {
                        angular_velocity = 8.0f;   // 小误差时使用较低角速度
                    }

                    if (angle_error > 0) {
                        // 需要顺时针转向
                        send_data = ChangeSendType(0, 0, (int)angular_velocity);
                    } else {
                        // 需要逆时针转向
                        send_data = ChangeSendType(0, 0, -(int)angular_velocity);
                    }
                    write(fd_chassis, (unsigned char *) send_data.send_data, 12);
                } else {
                    // 角度到达，停止转向并切换到前进模式
                    send_data = ChangeSendType(0, 0, 0);  // 停止转向
                    write(fd_chassis, (unsigned char *) send_data.send_data, 12);

                    mode_select_FLAG = 1;

                    length = snprintf(debug_message, sizeof(debug_message),
                                    "ANGLE REACHED! Switching to FORWARD mode\n");

                    // 发送调试信息到TCP客户端
                    if (new_socket > 0) {
                        if (send(new_socket, debug_message, length, 0) == -1) {
                            perror("send debug message failed");
                        }
                    }
                }

            } else if (mode_select_FLAG == 1) {
                // 直线前进阶段 - 前进固定时间而不是距离
                static float forward_time = 0;
                forward_time += 0.1f;  // 每100ms增加0.1秒

                float target_time = 2.0f;  // 前进2秒

                char debug_message[200];
                int length = snprintf(debug_message, sizeof(debug_message),
                                    "FORWARD MODE - Time: %.1f/%.1f seconds\n",
                                    forward_time, target_time);

                // 发送调试信息到TCP客户端
                if (new_socket > 0) {
                    if (send(new_socket, debug_message, length, 0) == -1) {
                        perror("send debug message failed");
                    }
                }

                if (forward_time < target_time) {
                    // 继续前进
                    send_data = ChangeSendType(25, 0, 0);  // 适中速度前进
                    write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                } else {
                    // 前进时间到达，停止并准备下一次转向
                    send_data = ChangeSendType(0, 0, 0);  // 停止
                    write(fd_chassis, (unsigned char *)send_data.send_data, 12);

                    turn_counter++;
                    if (turn_counter >= 4) {
                        // 完成一个方形，停止
                        mode_flag = 0;
                        vel_temp = 0; yaw_temp = 0; ang_temp = 0;
                        turn_counter = 0;

                        length = snprintf(debug_message, sizeof(debug_message),
                                        "SQUARE COMPLETED! Stopping.\n");

                        // 发送调试信息到TCP客户端
                        if (new_socket > 0) {
                            if (send(new_socket, debug_message, length, 0) == -1) {
                                perror("send debug message failed");
                            }
                        }
                    } else {
                        // 准备下一次转向
                        mode_select_FLAG = 0;
                        pid.SetAngle += 90;  // 下一个90度目标
                        if (pid.SetAngle > 180) {
                            pid.SetAngle -= 360;
                        }
                        forward_time = 0;  // 重置前进时间

                        length = snprintf(debug_message, sizeof(debug_message),
                                        "FORWARD DONE! Next turn target: %.1f\n", pid.SetAngle);

                        // 发送调试信息到TCP客户端
                        if (new_socket > 0) {
                            if (send(new_socket, debug_message, length, 0) == -1) {
                                perror("send debug message failed");
                            }
                        }
                    }
                }
            }
            usleep(100000);  // 100ms循环
        } else if (mode_flag == 2) {
            if (forward_setAngle == -1000){
                forward_setAngle = cumulativeZAngle;
            }
            if (cumulativeZAngle >= (forward_setAngle-2) && cumulativeZAngle <= (forward_setAngle+2)){
                vel_temp = 20;
                yaw_temp = 0;
                ang_temp = 0;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            } else if ((cumulativeZAngle - forward_setAngle) <= 180 && (cumulativeZAngle - forward_setAngle) >0) {
                vel_temp = 0;
                yaw_temp = 0;
                ang_temp = -10;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            } else {
                vel_temp = 0;
                yaw_temp = 0;
                ang_temp = 10;
                send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
                write(fd_chassis, (unsigned char *)send_data.send_data, 12);
                usleep(300000);
            }
        } else {
            forward_setAngle = -1000;
            // printf("vel_temp:%d yaw_temp:%d ang_temp:%d\n",vel_temp, yaw_temp, ang_temp);
            send_data = ChangeSendType(vel_temp, yaw_temp, ang_temp);
            // printf("vel_mid: %d \n", vel_temp);
            write(fd_chassis, (unsigned char *)send_data.send_data, 12);
            // printf("vel_after:%d \n", vel_temp);
            usleep(300000);
        }
        // pthread_mutex_lock(&lock);
        // if (ret > 0)
        // {
        //      printf("send success, data is  \n");
        // }
        // else
        // {
        //     printf("send error!\r\n");
        // }

    }
}
/**
 * @brief imu调用
 */

float normalizeAngle(float angle) {
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

// 计算在y轴上的重力加速度分量
float computeGravityY(float pitchAngle) {
    // 计算重力加速度分量
    return sin(pitchAngle * M_PI / 180.0);
}


void _serial_IMU_input_task(void)
{
    int i;
    char cBuff[1];

    const float deltaTime = 0.01f;  // 10毫秒
    while (1)
    {
        while (serial_read_data(fd_imu, cBuff, 1))
        {
            WitSerialDataIn(cBuff[0]);
        }
        // printf("\n");
        usleep(10000);
        if (s_cDataUpdate)
        {
            // pthread_mutex_lock(&lock);
            for (i = 0; i < 3; i++)
            {
                fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
                fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
            }
            // 更新累计z轴角度
            cumulativeZAngle += fGyro[2] * deltaTime;
            cumulativeZAngle = normalizeAngle(cumulativeZAngle);

            // 更新PID控制器的实际角度 - 关键修复！
            static float last_angle = 0.0f;
            static int angle_update_counter = 0;

            // 检查IMU数据是否有效更新
            if (s_cDataUpdate & ANGLE_UPDATE) {
                pid.ActualAngle = fAngle[2];  // 使用IMU的Yaw角度作为实际角度
                last_angle = fAngle[2];
                angle_update_counter++;

                // 实时调试信息 - 每次角度更新都打印
                char imu_debug[200];
                int len = snprintf(imu_debug, sizeof(imu_debug),
                                 "IMU ANGLE UPDATE - Raw: %d, Angle: %.3f, Counter: %d\n",
                                 sReg[Roll + 2], fAngle[2], angle_update_counter);
                if (new_socket > 0) {
                    send(new_socket, imu_debug, len, 0);
                }
             } else {
                // 如果没有新的角度数据，使用上次的角度值
                pid.ActualAngle = last_angle;

                // 警告：没有收到角度更新
                static int no_update_counter = 0;
                no_update_counter++;
                if (no_update_counter % 50 == 0) {  // 每50次循环警告一次
                    char warning[200];
                    int len = snprintf(warning, sizeof(warning),
                                     "WARNING: No IMU angle update! Using last angle: %.3f\n",
                                     last_angle);
                    if (new_socket > 0) {
                        send(new_socket, warning, len, 0);
                    }
                  }
            }

            s_cDataUpdate = 0;  // 重置标志，等待下次数据更新
            // if (s_cDataUpdate & ACC_UPDATE)
            // {
            //     sprintf(imu_acc_buf, "acc:%.3f,%.3f,%.3f\r\n", fAcc[0], fAcc[1], fAcc[2]);
            //     printf("acc:%.3f %.3f %.3f\r\n", fAcc[0], fAcc[1], fAcc[2]);
            // }
            // if (s_cDataUpdate & GYRO_UPDATE)
            // {
            //     printf("gyro:%.3f %.3f %.3f\r\n", fGyro[0], fGyro[1], fGyro[2]);
            // }
            // if (s_cDataUpdate & ANGLE_UPDATE)
            // {
            //     printf("angle:%.3f %.3f %.3f\r\n", fAngle[0], fAngle[1], fAngle[2]);
            // }
            // if (s_cDataUpdate & MAG_UPDATE)
            // {
            //     printf("mag:%d %d %d\r\n", sReg[HX], sReg[HY], sReg[HZ]);
            // }
            // pthread_mutex_unlock(&lock);
        }
    }

    serial_close(fd_imu);
}

/**
 * @brief tcp_client_socket线程
 */


char buffer[BUFFER_SIZE];

int set_non_blocking(int sockfd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
        flags = 0;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(sockfd, FIONBIO, &flags);
#endif
}

void _tcp_server_socket(void) {
    int server_fd, local_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *message = "Sever recevied!";

    // 创建TCP套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项，允许地址重用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);  // 确保在错误情况下关闭套接字
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定套接字到指定端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char debug_message[200];
    int length = snprintf(debug_message, sizeof(debug_message), "Waiting for connection...\n");

    // 发送调试信息到TCP客户端（如果连接存在）
    if (new_socket > 0) {
        if (send(new_socket, debug_message, length, 0) == -1) {
            perror("send debug message failed");
        }
    }

    while (1) {
        if ((local_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // 将局部套接字赋值给全局变量
        new_socket = local_socket;

        length = snprintf(debug_message, sizeof(debug_message), "Connection established with client\n");

        // 发送调试信息到TCP客户端
        if (send(new_socket, debug_message, length, 0) == -1) {
            perror("send debug message failed");
        }

        // 循环等待客户端数据
        while (1) {
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                length = snprintf(debug_message, sizeof(debug_message), "Client disconnected or read error\n");

                // 发送调试信息到TCP客户端（如果连接仍然有效）
                if (new_socket > 0) {
                    send(new_socket, debug_message, length, 0);  // 尝试发送，但不检查错误
                }
                close(new_socket);
                break;
            }


            int filtered_length = 0;
            for (int i = 0; i < valread; ++i) {
                if (buffer[i] != '\r' && buffer[i] != '\n') {
                    buffer[filtered_length++] = buffer[i];
                }
            }
            buffer[filtered_length] = '\0';
            if (filtered_length > 0) {
                length = snprintf(debug_message, sizeof(debug_message),
                                "Received from client: %.*s\nFiltered length: %d\n",
                                filtered_length, buffer, filtered_length);

                // 发送调试信息到TCP客户端
                if (send(new_socket, debug_message, length, 0) == -1) {
                    perror("send debug message failed");
                }
                pthread_mutex_lock(&lock);
                if (strcmp(buffer, "1") == 0) {
                    mode_flag = 0;
                    vel_temp = 20; yaw_temp = 0; ang_temp = 0;
                } else if (strcmp(buffer, "2") == 0) {
                    mode_flag = 0;
                    vel_temp = -20; yaw_temp = 0; ang_temp = 0;
                } else if (strcmp(buffer, "3") == 0) {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 20;
                } else if (strcmp(buffer, "4") == 0) {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = -20;
                } else if (strcmp(buffer, "5") == 0) {
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 200; ang_temp = 0;
                } else if (strcmp(buffer, "6") == 0) {
                    mode_flag = 0;
                    vel_temp = 20; yaw_temp = -20; ang_temp = 0;
                } else if (strcmp(buffer, "7") == 0) {
                    mode_flag = 0;
                    vel_temp = 200; yaw_temp = 0; ang_temp = 0;
                } else if (strcmp(buffer, "square") == 0) {
                    // 初始化方形轨迹模式
                    mode_flag = 1;

                    // 使用当前IMU角度作为起始点，设置第一个90度转向目标
                    pid.SetAngle = fAngle[2] + 90;  // 基于当前实际角度设置目标
                    if (pid.SetAngle > 180) {
                        pid.SetAngle = pid.SetAngle - 360;
                    } else if (pid.SetAngle < -180) {
                        pid.SetAngle = pid.SetAngle + 360;
                    }

                    // 重置PID相关变量
                    pid.integral = 0;  // 重置PID积分项
                    pid.err = 0;       // 重置误差

                   char pid_message[100];
                    int length = snprintf(pid_message, sizeof(pid_message),
                                        "Current IMU Angle: %.3f\nFirst target angle: %.3f\n",
                                        fAngle[2], pid.SetAngle);

                    if (send(new_socket, pid_message, length, 0) == -1) {

                        perror("send");
                        close(new_socket);
                        break;
                    }
                } else if (strcmp(buffer, "forward") == 0) {
                    mode_flag = 2;
                } else if (strcmp(buffer, "stop") == 0) {
                    // 添加停止指令
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 0;
                } else {
                    mode_flag = 0;
                    vel_temp = 0; yaw_temp = 0; ang_temp = 0;
                }

                length = snprintf(debug_message, sizeof(debug_message),
                                "mode_flag: %d\nUpdated values: vel_temp = %d, yaw_temp = %d, ang_temp = %d\n",
                                mode_flag, vel_temp, yaw_temp, ang_temp);

                // 发送调试信息到TCP客户端
                if (send(new_socket, debug_message, length, 0) == -1) {
                    perror("send debug message failed");
                }

                pthread_mutex_unlock(&lock);
                if (send(new_socket, message, strlen(message), 0) == -1) {
                    length = snprintf(debug_message, sizeof(debug_message), "defeat\n");

                    // 发送调试信息到TCP客户端（如果连接仍然有效）
                    if (new_socket > 0) {
                        send(new_socket, debug_message, length, 0);  // 尝试发送，但不检查错误
                    }
                    perror("send");
                    close(new_socket);
                    break;
                }
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    close(server_fd);
}
