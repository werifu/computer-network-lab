
#include <winsock2.h>

#include <fstream>
#include <string>

#include "packet.hh"
#define MAX_TRY_TIMES 5
#define FINISH 2
class Client {
   private:
    SOCKET sock;
    sockaddr_in client_addr;
    sockaddr_in server_addr_begin;
    sockaddr_in server_addr_then;
    std::string mode;

   public:
    Client(std::string client_ip, u_short client_port, std::string server_ip,
           std::string mode_ = "octet");
    Client();
    ~Client();
    bool upload(std::string filename, std::string to_server_path);
    bool download(std::string remote_path, std::string output_path);

   private:
    int send_request(Opcode opcode, std::string filename);
    void handle_recv_err(int err_code);
    bool is_expected_ack(byte* ack_buf, int recv_size,
                         two_bytes expected_block);
    // upload
    bool send_valid_write_request(std::string filename);
    int send_file(std::string filename);
    int send_data_packet(DataPacket& packet);
    bool send_valid_data_packet(DataPacket& packet, two_bytes cur_block);
    // download
    bool is_expected_data(byte* pakcet_buf, int recv_size,
                          two_bytes expected_block);
    int send_valid_read_request(std::string filename, std::string output_path,
                                std::ofstream& fout);
    bool receive_file(std::string filename, std::ofstream& fout);
    int receive_data_packet(byte* data_buf);
    int receive_valid_data_packet(two_bytes ack_block, std::ofstream& fout);
    int send_ack(two_bytes block);

    // progress
    void show_speed(double time_cost, int packet_size);
};