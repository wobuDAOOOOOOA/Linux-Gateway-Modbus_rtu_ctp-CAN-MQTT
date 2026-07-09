/*********************************************
*  BMP280 应用层测试程序（超级详细注释）
*  核心原理：完全依赖Linux标准系统调用
*  不包含任何内核函数、不操作硬件、不依赖驱动源码
*  纯用户态调用：open -> read -> 解析数据 -> 打印
*  规避内核浮点限制：所有物理换算放在用户层
**********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
// 和驱动创建的设备节点保持一致
#define DEV_PATH "/dev/bmp280_dev"
int fd;
char buf[64] = {0};
u_int32_t raw_p, raw_t;
float temp, press;
/*********************************************
* 函数：bmp280_calc_data
* 功能：原始ADC值 --> 真实物理温湿度、气压
* 大白话：
* 内核不敢做浮点运算，全部丢到用户层算
* 按照BMP280官方手册换算公式
**********************************************/
void bmp280_calc_data(u_int32_t raw_press, u_int32_t raw_temp, float *temp, float *press)
{
    *temp = raw_temp / 100.0f;
    *press = raw_press / 10.0f;
}

/*********************************************
* 主函数：完整用户层调用链路
* 1、open打开内核驱动生成的设备文件
* 2、while循环不断read读取内核数据
* 3、解析字符串拿到原始数据
* 4、用户层浮点换算
* 5、打印真实物理值
**********************************************/
int BMP280_READ_Init(void)
{
    // 1、只读方式打开设备节点
     fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open bmp280 dev fail");
        return -1;
    }
}


int BMP280_READ(void)
{
        memset(buf, 0, sizeof(buf));

        // 2、系统调用read，触发内核驱动的.read回调
        // 这里是用户层进入内核驱动的【唯一入口】
        read(fd, buf, sizeof(buf));

        // 3、解析内核传回的字符串数据
        sscanf(buf, "press:%u,temp:%u", &raw_p, &raw_t);

        // 4、用户层浮点运算，转换成真实物理值
        bmp280_calc_data(raw_p, raw_t, &temp, &press);

        // 5、打印最终结果
        printf("【BMP280采集数据】\n");
       // printf("温度: %.2f ℃ \n", temp);
        printf("气压: %.2f Pa \n\n", press);
       return press;
 }
int BMP280_READ_close(void)
{
// 关闭设备文件（永远不会执行，因为while(1)）
    close(fd);
    return 0;
}
    
