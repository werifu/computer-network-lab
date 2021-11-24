#include <winsock2.h>

#include <iostream>

#include "client.hh"
#include "packet.hh"

#pragma comment(lib, "ws2_32.lib")


void print_help() {

}
int main(int argc, char* argv[]) {
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);

    std::string filename = "a.exe";
    Client client = Client("192.168.99.128", 12450, "192.168.99.128", "octet");
    // if (client.upload(filename)) {
    //     printf("UPLOAD OK!\n");
    // }
    // filename = "tftpd64.exe";
    // if (client.download(filename, filename)) {
    //     printf("DOWNLOAD OK\n");
    // }
    return 0;
}