
#include "client.hh"

#include "packet.hh"

using std::ifstream;
using std::ofstream;
Client::Client(std::string client_ip, u_short client_port,
               std::string server_ip) {
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip.c_str());
    client_addr.sin_port = htons(client_port);

    server_addr_begin.sin_family = AF_INET;
    server_addr_begin.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr_begin.sin_port = htons(69);

    // create the client socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

Client::~Client() {}

// upload

// send Write Request Packet or Read Request Packet
int Client::send_request(Opcode opcode, std::string filename) {
    auto packet = RequestPacket(opcode, filename, "netascii");
    byte pkt[MAX_PACKET_BUF];
    size_t pkt_length = packet.encode(pkt, MAX_PACKET_BUF);
    // send the pkt
    int ret = sendto(this->sock, (const char*)pkt, pkt_length, 0,
                     (sockaddr*)&this->server_addr_begin,
                     sizeof(this->server_addr_begin));
}

int Client::send_write_request(std::string filename) {
    return this->send_request(Opcode::WRITE, filename);
}

int Client::wait_for_first_ack() {
    byte packet_buf[MAX_PACKET_BUF];
    int packet_len = sizeof(this->server_addr_then);
    int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                          (sockaddr*)&this->server_addr_then, &packet_len);
    if (r_size == SOCKET_ERROR) {
        printf("last error: %d\n", WSAGetLastError());
    }
    return r_size;
}
int Client::send_data_packet(DataPacket& packet) {
    byte packet_buf[MAX_PACKET_BUF];
    size_t packet_size = packet.encode(packet_buf, MAX_PACKET_BUF);
    int addr_len;
    int ret = sendto(this->sock, (const char*)packet_buf, packet_size, 0,
                     (sockaddr*)&this->server_addr_then,
                     sizeof(this->server_addr_then));
    return true;
}

int Client::send_file(std::string filename) {
    ifstream fin(filename);
    if (!fin) {
        printf("Cannot open file %s", filename.c_str());
        return false;
    }
    byte data_buf[MAX_DATA_LEN];
    two_bytes cur_block = 1;

    while (!fin.eof()) {
        memset(data_buf, 0, MAX_DATA_LEN);
        fin.read((char*)data_buf, MAX_DATA_LEN);
        std::streamsize cur_p = fin.gcount();
        // compose the data buf and send all the chunks
        // send a data chunk to server
        size_t data_size;
        if (fin.eof()) {
            // this is the last data packet
            data_size = strnlen((const char*)data_buf, MAX_DATA_LEN);
        } else {
            data_size = MAX_DATA_LEN;
        }
        DataPacket packet = DataPacket(cur_block, data_buf, data_size);
        this->send_data_packet(packet);
        // waiting for ack for this block
        byte ack_buf[MAX_PACKET_BUF];
        int addr_len = sizeof(this->server_addr_then);
        printf("send block %d to port: %u\n", cur_block,
               htons(this->server_addr_then.sin_port));
        // int try_times = 0;
        int recv_size = recvfrom(this->sock, (char*)ack_buf, MAX_PACKET_BUF, 0,
                                 (sockaddr*)&this->server_addr_then, &addr_len);
        if (recv_size == SOCKET_ERROR) {
            printf("last error: %d\n", WSAGetLastError());
        }
        cur_block++;
    }
    return true;
}

int Client::upload(std::string filename) {
    this->send_write_request(filename);
    this->wait_for_first_ack();
    this->send_file(filename);
    return true;
}

// download
int Client::send_read_request(std::string filename) {
    return this->send_request(Opcode::READ, filename);
}

// returns the length of data (len < 512 means last packet)
int Client::receive_data_packet(byte* data_buf) {
    int packet_len = sizeof(this->server_addr_then);
    byte packet_buf[MAX_PACKET_BUF];
    int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                          (sockaddr*)&this->server_addr_then, &packet_len);
    if (r_size == SOCKET_ERROR) {
        printf("recvfrom error: %d", WSAGetLastError());
        return -1;
    }
    int data_size = r_size - 4;
    for (int i = 0; i < data_size; i++) {
        data_buf[i] = packet_buf[i+4];
    }
    return data_size;
}
int Client::send_ack(two_bytes block) {
    ACKPacket packet(block);
    byte packet_buf[MAX_PACKET_BUF];
    size_t packet_size = packet.encode(packet_buf, MAX_PACKET_BUF);
    int addr_len;
    int ret = sendto(this->sock, (const char*)packet_buf, packet_size, 0,
                     (sockaddr*)&this->server_addr_then,
                     sizeof(this->server_addr_then));
    return true;
}

int Client::receive_file(std::string remote_path, std::string output_path) {
    ofstream fout(output_path);
    two_bytes block = 1;
    int finish = false;
    while (true) {
        byte data_buf[MAX_DATA_LEN];
        int data_size = this->receive_data_packet(data_buf);
        if (data_size == -1) {
            break;
        } else if (data_size < MAX_DATA_LEN) {
            finish = true;
        }
        fout.write((const char*)data_buf, data_size);
        this->send_ack(block);
        block++;
        if (finish) break;
    }
    return true;
}

int Client::download(std::string remote_path, std::string output_path) {
    this->send_read_request(remote_path);
    this->receive_file(remote_path, output_path);
    return true;
}