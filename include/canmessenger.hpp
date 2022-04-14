/*
 * @Author: zpwang 
 * @Date: 2021-12-02 17:49:45 
 * @Last Modified by: zpwang
 * @Last Modified time: 2021-12-02 17:55:06
 */
#ifndef _CANSENDER_H_
#define _CANSENDER_H_

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <net/if.h>

#include "spdlog/spdlog.h"

class canmessenger
{
public:
    canmessenger(const char *canname)
    {
        struct sockaddr_can addr;
        struct ifreq ifr;
        struct can_frame frame[2] = {{0}};
        can_socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建套接字
        strcpy(ifr.ifr_name, canname );
        ioctl(can_socket_fd, SIOCGIFINDEX, &ifr); //指定 can0 设备
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        bind(can_socket_fd, (struct sockaddr *)&addr, sizeof(addr));//将套接字与 can0 绑定
        std::thread receivedTh = std::thread(&canmessenger::receiveCANMsg, this);
        receivedTh.detach();

    }
    ~canmessenger(){}
    void sendObjDetRet(std::vector<int> &msg)
    {
        if(msg.size() == 0)
            return;
        if(msg.size()%6 != 0)
        {
            spdlog::critical("obj det ret error!");
            return;
        }
        int objnum = msg.size()/6;
        //each obj takes 9 bytes, plus two bytes header, 1 byte obj num and 1 byte frame num
        int canframeNum = ceil((float)(9*objnum + 4)/8);
        // struct can_frame canmsg[canframeNum];
        unsigned char data[canframeNum*8];
        data[0] = 0xAF;
        data[1] = 0xBE;
        data[2] = canframeNum;
        data[3] = objnum;
        for(int i=0;i<objnum;i++)
        {
            data[3+i*9 + 1] = msg[6*i+4];//class
            data[3+i*9 + 2] = msg[6*i] >> 8;//x
            data[3+i*9 + 3] = msg[6*i];
            data[3+i*9 + 4] = msg[6*i + 1] >> 8;//y
            data[3+i*9 + 5] = msg[6*i + 1];
            data[3+i*9 + 6] = msg[6*i + 2] >> 8;//h
            data[3+i*9 + 7] = msg[6*i + 2];
            data[3+i*9 + 8] = msg[6*i + 3] >> 8;//w
            data[3+i*9 + 9] = msg[6*i + 3];
        }
        for(int i=0;i<canframeNum;i++)
        {
            struct can_frame canmsg;
            canmsg.can_id = 0x421;
            canmsg.can_dlc = 8;
            canmsg.data[0] = data[i*8 + 0];
            canmsg.data[1] = data[i*8 + 1];
            canmsg.data[2] = data[i*8 + 2];
            canmsg.data[3] = data[i*8 + 3];
            canmsg.data[4] = data[i*8 + 4];
            canmsg.data[5] = data[i*8 + 5];
            canmsg.data[6] = data[i*8 + 6];
            canmsg.data[7] = data[i*8 + 7];
            unsigned char nbytes = write(can_socket_fd, &canmsg, sizeof(can_frame));        

        }
    }

    void receiveCANMsg()
    {
        while(1)
        {
		    int nbytes = read(can_socket_fd, &reveivedMsg, sizeof(can_frame));
            spdlog::info("received {} byte", nbytes);
            for(int i=0;i<8;i++)
            {
                spdlog::info("date[{}]:{}", i, reveivedMsg.data[i]);
            }
        }
    }

    void  sendTest()
    {
        struct can_frame canmsg;
        canmsg.can_id = 0x421;
        canmsg.can_dlc = 8;
        canmsg.data[0] = 0xff;
        canmsg.data[1] = 0xff;
        canmsg.data[2] = 0xff;
        canmsg.data[3] = 0xff;
        canmsg.data[4] = 0xff;
        canmsg.data[5] = 0xff;
        canmsg.data[6] = 0xff;
        canmsg.data[7] = 0xff;
        unsigned char nbytes = write(can_socket_fd, &canmsg, sizeof(can_frame));        
    }
private:
    int can_socket_fd;
    unsigned char reveivedData[CAN_MAX_DLEN];
    struct can_frame reveivedMsg;
    
};

#endif