#include "stitcherglobal.h"
#include "udp_publisher.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <iterator>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include "spdlog/spdlog.h"



using namespace std;

sockaddr_in local_addr;
int len;

namespace udp_publisher
{

  UdpPublisher::UdpPublisher()
  {
    // m_port = 9999;
    // m_device_ip_str = "192.168.1.102";
    // printf("Init udp server!!!!");

    // // Convert ip string to ip address struct
    // if (inet_aton(m_device_ip_str.c_str(), &devip_) == 0)
    // {
    //   printf("ip error!!!!!!!!!!!! \n");
    //   return;
    // }
  
    // // Connect to UDP socket
    // m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (m_sockfd == -1)
    // {
    //   printf("socket error!!!!!!!!!!!!! \n");
    //   return;
    // }
  
    // // Prepare the socket structure
    // memset(&local_addr, 0, sizeof(local_addr)); // Set to zeros
    // local_addr.sin_family = AF_INET;            // Host byte order
    // local_addr.sin_port = htons(m_port);        // Convert to network byte order
    // local_addr.sin_addr.s_addr = INADDR_ANY;    // Fill local ip
    // len = sizeof(local_addr);
    // // Bind socket to ip
    // if (bind(m_sockfd, (sockaddr *)&local_addr, sizeof(sockaddr)) == -1)
    // {
    //   printf("bind error!!!!!!!!!!!!!!!!!! \n");
    //   return;
    // }
  
    // // Set fd for non-blocking mode
    // if (fcntl(m_sockfd, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    // {
    //   printf("Mode error !!!!!!!!!!!!!");
    //   return;
    // }

    servPort = Socket::resolveService(UDP_PORT, "udp");
    servAddress = "127.0.0.1";
    servAddress = UDP_SERVADD;
  }

  void UdpPublisher::setAddr()
  {
    servPort = Socket::resolveService(UDP_PORT, "udp");
    servAddress = UDP_SERVADD;
  }
  

  void UdpPublisher::sendimage(unsigned char* image,int length)
  {
    // struct sockaddr_in send_addr;
    // memset(&send_addr,0,sizeof(struct sockaddr_in));
    // send_addr.sin_family = AF_INET;
    // send_addr.sin_port = htons(10000);
    // send_addr.sin_addr.s_addr = inet_addr("192.168.1.102");

		spdlog::debug("Enter rgbimage to send:");

    // if(sendto(m_sockfd,image,length,0,(sockaddr *)&send_addr,sizeof(send_addr)) <= 0)
    // {
    //   printf("send image error");
    // }
    // printf("\n");


    // int total_pack = 1+(length-1)/UP_UDP_PACK_SIZE;
    // int lastpack_size = length%UP_UDP_PACK_SIZE;
    // int ibuf[1];
    // ibuf[0] = total_pack;
    // sock.sendTo(ibuf, sizeof(int), servAddress, servPort);
    // // sendto(m_sockfd,ibuf,sizeof(int),0,(sockaddr *)&send_addr,sizeof(send_addr));
    // spdlog::critical("length:{}, pack number :{}",length, total_pack);
    // for(int i = 0; i<total_pack-1; i++){
    //   // sendto(m_sockfd,&image[i*UP_UDP_PACK_SIZE],UP_UDP_PACK_SIZE,0,(sockaddr *)&send_addr,sizeof(send_addr)) ;
    //   sock.sendTo( & image[i * UP_UDP_PACK_SIZE], UP_UDP_PACK_SIZE, servAddress, servPort);

    // }
    // sock.sendTo( & image[total_pack * UP_UDP_PACK_SIZE], lastpack_size, servAddress, servPort);

    int total_pack = 1+(length-1)/UP_UDP_PACK_SIZE;
    int ibuf[1];
    ibuf[0] = total_pack;
    sock.sendTo(ibuf, sizeof(int), servAddress, servPort);
    // sendto(m_sockfd,ibuf,sizeof(int),0,(sockaddr *)&send_addr,sizeof(send_addr));
    spdlog::debug("udp buf length:{}, pack number :{}",length, total_pack);
    for(int i = 0; i<total_pack; i++){
      // sendto(m_sockfd,&image[i*UP_UDP_PACK_SIZE],UP_UDP_PACK_SIZE,0,(sockaddr *)&send_addr,sizeof(send_addr)) ;
      sock.sendTo( & image[i * UP_UDP_PACK_SIZE], UP_UDP_PACK_SIZE, servAddress, servPort);

    }

  }
  
  UdpPublisher::~UdpPublisher(void)
  {
    (void)close(m_sockfd);
    m_sockfd = -1;
  }
} // namespace udp_publisher
