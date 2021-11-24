#include <winsock2.h>

#include <iostream>

#include "client.hh"
#include "packet.hh"

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);

    std::string filename = "client.cc";
    Client client = Client("192.168.99.128", 12450, "192.168.99.128");
    // if (client.upload(filename)) {
    //     printf("OK!\n");
    // }
    if (client.download("test.txt", "test.txt")) {
        printf("DOWNLOAD OK\n");
    }
    return 0;
}