#include <winsock2.h>

#include <fstream>
#include <string>

#include "packet.hh"
struct UDP_INFO {
    char src_ip[108];
    u_short src_port;
    char dst_ip[108];
    u_short dst_port;
};

int send_request(Opcode opcode, std::string filename, UDP_INFO& udp);
int send_read_request(std::string filename, UDP_INFO& udp);
int send_write_request(std::string filename, UDP_INFO& udp);
u_short receive_ack(u_short recv_port);
SOCKET create_udp_send_sock(UDP_INFO& udp, sockaddr_in& src);
int send_file(std::string filename, UDP_INFO udp);
int send_data_packet(SOCKET& sock, int block_num);
