#include <winsock2.h>

#include <iostream>

#include "client.hh"
#include "packet.hh"

#pragma comment(lib, "ws2_32.lib")

int main() {
    std::string filename = "test.txt";
    UDP_INFO ips = {"192.168.99.128", 12450, "192.168.99.128", 69};
    int payload_size = send_write_request(filename, ips);
    u_short port = receive_ack(12450);
    ips.dst_port = port;
    send_file("client.cc", ips);
    return 0;
}