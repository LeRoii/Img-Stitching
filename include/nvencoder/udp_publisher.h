#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H


#include "jetsonEncoder.h"

#include <string>
#include <cstdint>
#include <vector>
#include <netinet/in.h>

#include "PracticalSocket.h"

#define UP_UDP_PACK_SIZE 1500

namespace defaults
{
  static const std::size_t buf_size = 28; // Setting up buffer length for 1 MB
}

namespace udp_publisher
{
  class UdpPublisher
  {
    public:
      UdpPublisher();
      ~UdpPublisher();
      void sendimage(unsigned char* image, int length);
      void setAddr();

    private:
    
      //! socket filedescriptor
      int m_sockfd;
      //! port number
      int m_port;
    
      //! device ip, default is localhost
      in_addr devip_;

      std::string m_device_ip_str;
    
      const char *m_node_name;

      UDPSocket sock;
      unsigned short servPort;
      string servAddress;
  };

} // namespace udp_publisher

#endif
