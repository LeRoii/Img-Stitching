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

using namespace std;

sockaddr_in local_addr;
int len;

namespace udp_publisher
{

  UdpPublisher::UdpPublisher()
  {
    m_port = 9999;
    m_device_ip_str = "192.168.1.103";
    printf("Init udp server!!!!");

    // Convert ip string to ip address struct
    if (inet_aton(m_device_ip_str.c_str(), &devip_) == 0)
    {
      printf("ip error!!!!!!!!!!!! \n");
      return;
    }
  
    // Connect to UDP socket
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd == -1)
    {
      printf("socket error!!!!!!!!!!!!! \n");
      return;
    }
  
    // Prepare the socket structure
    memset(&local_addr, 0, sizeof(local_addr)); // Set to zeros
    local_addr.sin_family = AF_INET;            // Host byte order
    local_addr.sin_port = htons(m_port);        // Convert to network byte order
    local_addr.sin_addr.s_addr = INADDR_ANY;    // Fill local ip
    len = sizeof(local_addr);
    // Bind socket to ip
    if (bind(m_sockfd, (sockaddr *)&local_addr, sizeof(sockaddr)) == -1)
    {
      printf("bind error!!!!!!!!!!!!!!!!!! \n");
      return;
    }
  
    // Set fd for non-blocking mode
    if (fcntl(m_sockfd, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    {
      printf("Mode error !!!!!!!!!!!!!");
      return;
    }
  }
  

  void UdpPublisher::sendimage(unsigned char* image,int length)
  {
    struct sockaddr_in send_addr;
    memset(&send_addr,0,sizeof(struct sockaddr_in));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(10000);
    send_addr.sin_addr.s_addr = inet_addr("192.168.1.103");

		printf("Enter rgbimage to send:");

    // if(sendto(m_sockfd,image,length,0,(sockaddr *)&send_addr,sizeof(send_addr)) <= 0)
    // {
    //   printf("send image error");
    // }
    // printf("\n");


    int total_pack = 1+(length-1)/UP_UDP_PACK_SIZE;
    int ibuf[1];
    ibuf[0] = total_pack;
    sendto(m_sockfd,ibuf,sizeof(int),0,(sockaddr *)&send_addr,sizeof(send_addr));
    std::cout<<"pack number : "<<total_pack<<std::endl;
    for(int i = 0; i<total_pack; i++){
      sendto(m_sockfd,&image[i*UP_UDP_PACK_SIZE],UP_UDP_PACK_SIZE,0,(sockaddr *)&send_addr,sizeof(send_addr)) ;
    }

  }
  
  void UdpPublisher::sendData(targetInfo data,int length)
  {
    struct sockaddr_in send_addr;
    memset(&send_addr,0,sizeof(struct sockaddr_in));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(9999);
    send_addr.sin_addr.s_addr = inet_addr("192.168.1.103");

		printf("Enter data to send:");

    if(sendto(m_sockfd,(targetInfo*)&data,length,0,(sockaddr *)&send_addr,sizeof(send_addr)) <= 0)
    {
      printf("send data error");
    }
		
  }


  controlData UdpPublisher::recvData()
  {
    controlData data;
		printf("\n ~~~~~~~~~~~Enter data to recv:  ");

    if(  recvfrom(m_sockfd, (controlData*)&data, sizeof(data), 0, (struct sockaddr *)&local_addr, (socklen_t *)&len) <= 0)
    {
      printf("recv data error");
    } 
    return data;
  }
 


  UdpPublisher::~UdpPublisher(void)
  {
    (void)close(m_sockfd);
    m_sockfd = -1;
  }
} // namespace udp_publisher
