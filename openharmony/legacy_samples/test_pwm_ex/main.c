/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "hilog/log.h"
#include "pwm.h"

#define INIT_DUTY 500000
#define MAX_DUTY 2500000

void user_help(void)
{
    printf("Please input like this: pwm_test <int> <int>\n");
    printf("the first number is to write the PWM_PIN to the PWM range[1,2] \n");
    printf("the second number is to write the duty_cycle to the PWM range[-2500000,2500000] \n");
}

int get_final_duty(int pwm_duty, int final_duty)
{
    int real_duty = 0;
    // 判断最终转到的角度是否超过溢出值，输出实际转到的角度
    if (final_duty < INIT_DUTY)
    {
        real_duty = INIT_DUTY;
        printf("Beyond the angle of rotation");
    }
    else if (final_duty > MAX_DUTY)
    {
        real_duty = MAX_DUTY;
        printf("Beyond the angle of rotation");
    }
    else
    {
        real_duty = final_duty;
    }
    return real_duty;
}

int main(int argc, char **argv)
{
    int pwmChannel = PWM1;
    int cur_duty = 0;


    int pwm=0;
    init_pwm(pwmChannel);
    set_pwm_period(pwmChannel, 2600000L);

    set_pwm_dutyCycle(pwmChannel, INIT_DUTY);
    cur_duty = get_pwm_dutyCycle(pwmChannel);

    while (pwm < 500)
    {
        pwm++;
        set_pwm_dutyCycle(1, pwm);
        sleep(100L);
    }
    while (pwm)
    {
        pwm--;
        set_pwm_dutyCycle(1, pwm);
        sleep(100L);
    }

    return 0;
}
