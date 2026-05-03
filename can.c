#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include"log.h"
int my_can_send(u_int16_t id, u_int16_t dlc, short unsigned int* data_M)
{
       LOG_DEBUG("CAN:进入can发送函数");      

int s; //水管
struct ifreq ifr;//指定水管要接的口
struct sockaddr_can addr; //接起来
struct can_frame frame;//供水系统要提供什么样的水

//先创建socket，也就是告诉内核我要申请水管
if((s = socket(PF_CAN, SOCK_RAW , CAN_RAW)) < 0)  //协议域是can，然后采用原始套接字（不过udp和tcp），可以自由读写完整的can帧数据
{
   perror("socket");    
   LOG_ERROR("CAN:创建socket失败");      
   return -1;
}
 
//指定水管接口
strcpy(ifr.ifr_name, "vcan0"); //
ioctl(s,SIOCGIFINDEX,&ifr); //S是水管的编号。Socket IO Control ：Get InterFace Index
LOG_DEBUG("CAN:申请接口请求");

addr.can_family = AF_CAN;
addr.can_ifindex = ifr.ifr_ifindex;
bind(s, (struct sockaddr *)&addr, sizeof(addr));
LOG_DEBUG("CAN:绑定接口");

frame.can_id = id;
frame.can_dlc = dlc;
for(int i=0;i<dlc;i++)
{
    frame.data[i] = data_M[i];
}


if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
    perror("write");
    return 1;
    LOG_ERROR("CAN:数据发送失败");
}
LOG_INFO("CAN:初始化完成");

LOG_INFO("CAN:数据发送成功");
return 0;
}
