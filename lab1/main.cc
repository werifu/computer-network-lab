#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "client.hh"
#include "packet.hh"
#pragma comment(lib, "ws2_32.lib")

char *optarg = NULL;
int optind = 1;
using std::string;
// implemented by Microsoft
int getopt(int argc, char *const argv[], const char *optstring) {
    if ((optind >= argc) || (argv[optind][0] != '-') ||
        (argv[optind][0] == 0)) {
        return -1;
    }

    int opt = argv[optind][1];
    const char *p = strchr(optstring, opt);

    if (p == NULL) {
        optind++;
        return '?';
    }
    if (p[1] == ':') {
        optind++;
        if (optind >= argc) {
            return '?';
        }
        optarg = argv[optind];
    }
    optind++;
    return opt;
}
void print_help() {
    printf("Welcome to my-tftp-client!\n");
    printf(
        "Usage: ./my-tftp-client upload | download -m mode -f from_file_path "
        "-t to_file_path -p local_port\n");
    printf(
        "Example: ./my-tftp-client upload -f @server/a.txt -t local/b.txt -p "
        "12450 -m netascii\n");
    printf(
        "then the text file local/b.txt will be uploaded to server/a.txt on "
        "server\n");
    printf("Options and arguments:\n");
    printf("-m mode\t: mode to transport the file, can be netascii or octet\n");
    printf(
        "-f from\t: from, it means the file on your PC when uploading, and "
        "means it "
        "on the server when downloading\n");
    printf("-t to\t: to, something like -f\n");
    printf("-i ip\t: ip of th eserver\n");
    printf("-p port\t: the port of the client\n");
    printf("-h\t: show help\n");
}
bool check_param(string from, string to, string port, string ip, string &mode) {
    bool succ = true;
    std::cout << from;
    if (from.empty()) {
        printf("param -f is needed but not provided.\n");
        succ = false;
    }
    if (to.empty()) {
        printf("param -t is needed but not provided.\n");
        succ = false;
    }
    if (mode.empty()) {
        printf("param -m is needed but not provided.\n");
        succ = false;
    }
    if (port.empty()) {
        printf("param -p is needed but not provided.\n");
        succ = false;
    }
    if (ip.empty()) {
        printf("param -i is needed but not provided.\n");
        succ = false;
    }
    for (char c : port) {
        if (c < '0' || c > '9') {
            printf("the value of param -p must be integer.\n");
            succ = false;
        }
    }
    std::transform(mode.begin(), mode.end(), mode.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (mode.compare("octet") && mode.compare("netascii")) {
        printf("Mode %s is not supported. Only supports netascii && octet\n",
               mode.c_str());
        succ = false;
    }
    return succ;
}

int main(int argc, char *argv[]) {
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);
    const char *opt = "i:f:t:p:m:";
    int param;
    if (argc == 1) {
        // no any params, print help
        print_help();
        return 0;
    }

    bool is_upload = true;
    if (!strcmp(argv[1], "upload"))
        is_upload = true;
    else
        is_upload = false;

    std::string from, to, port, mode, ip;
    while ((param = getopt(argc - 1, &argv[1], opt)) != -1) {
        switch (param) {
            case 'f':
                from = optarg;
                break;
            case 't':
                to = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'i':
                ip = optarg;
                break;
            case '?':
                printf("Error opt: %c %s\n", param, argv[optind]);
                exit(1);
                break;
        }
    }

    if (strcmp(argv[1], "upload") && strcmp(argv[1], "download")) {
        printf("upload or download is needed\n");
        return 0;
    }

    if (!check_param(from, to, port, ip, mode)) {
        return 0;
    }
    Client client = Client("127.0.0.1", 12450, ip, mode);
    if (is_upload) {
        if (client.upload(from, to)) {
            printf("\nUpload OK!\n");
        }
    } else {
        if (client.download(from, to)) {
            printf("\nDownload OK!\n");
        }
    }
    return 0;
}