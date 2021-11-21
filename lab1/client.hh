#include <winsock2.h>

#include <fstream>
#include <string>

#include "packet.hh"


class Client {
   private:
    SOCKET sock;
    sockaddr_in client_addr;
    sockaddr_in server_addr_begin;
    sockaddr_in server_addr_then;
   public:
    Client(std::string client_ip, u_short client_port, std::string server_ip);
    ~Client();
    int upload(std::string filename);
    int download(std::string remote_path, std::string output_path);

   private:
    int send_request(Opcode opcode, std::string filename);
    // upload
    int send_write_request(std::string filename);
    int wait_for_first_ack();
    int send_file(std::string filename);
    int send_data_packet(DataPacket& packet);
    // download
    int send_read_request(std::string filename);
    int receive_file(std::string filename, std::string output_path);
    int receive_data_packet(byte* data_buf);
    int send_ack(two_bytes block);
};