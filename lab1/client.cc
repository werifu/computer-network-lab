#include "client.hh"

#include <time.h>
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "packet.hh"
using std::ifstream;
using std::ofstream;
using std::to_string;
// calculate speed per 10 packets
#define CLOCK_UNIT_NUM 30
Client::Client() {}
Client::Client(std::string client_ip, u_short client_port,
               std::string server_ip, std::string mode_) {
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip.c_str());
    client_addr.sin_port = htons(client_port);

    server_addr_begin.sin_family = AF_INET;
    server_addr_begin.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr_begin.sin_port = htons(69);

    // create the client socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // set timeout
    DWORD l_buf[2] = {0, 0};
    l_buf[0] = static_cast<DWORD>(2000);
    int l_size = sizeof(DWORD);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)l_buf, l_size);

    // check mode
    std::transform(mode_.begin(), mode_.end(), mode_.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (mode_.compare("octet") && mode_.compare("netascii")) {
        printf("Mode %s is not supported. Only supports netascii && octet\n",
               mode_.c_str());
        exit(1);
    }
    mode = mode_;
}

Client::~Client() {}

/**
 * @brief print speed on terminal
 *
 * @param time_cost unit: milisecond
 * @param packet_size unit: byte
 */
void Client::show_speed(double time_cost, int packet_size) {
    double speed = (double)(packet_size) / (time_cost / CLOCKS_PER_SEC);
    if (speed < 1024) {
        // use many tabs to clear the line
        printf("%.2lfb/s            \r", speed);
    } else if (speed < 1024 * 1024) {
        printf("%.2lfKb/s           \r", speed / 1024);
    } else {
        printf("%.2lfMb/s           \r", speed / (1024 * 1024));
    }
}

void Client::handle_recv_err(int err_code) {
    if (err_code == WSAETIMEDOUT) {
        this->logger.error("Recv timeout");
    } else {
        this->logger.error("Last recv error: " + to_string(WSAGetLastError()));
    }
}

bool Client::is_expected_ack(byte* ack_buf, int recv_size,
                             two_bytes expected_block) {
    two_bytes packet_type = htons(*(two_bytes*)ack_buf);
    if (packet_type == Opcode::ERR) {
        this->logger.error("Receive error packet from server, ");
        // keep memory save
        if (recv_size < 4) return false;
        two_bytes err_code = htons(*(two_bytes*)(ack_buf + 2));
        std::string str((char*)(ack_buf + 4));
        this->logger.error("error code: " + to_string((two_bytes)err_code) +
                           "\nerror message: " + str);
        return false;
    } else if (packet_type != Opcode::ACK || recv_size < 4) {
        this->logger.error("Receive invalid packet, opcode: " +
                           to_string((two_bytes)packet_type));
        return false;
    }

    // check the block num
    two_bytes recv_block = htons(*(two_bytes*)(ack_buf + 2));
    if (recv_block != expected_block) {
        this->logger.error("Receive unexpected block, expect " +
                           to_string((two_bytes)expected_block) +
                           " but received " + to_string((two_bytes)recv_block));
        return false;
    }
    return true;
}
// upload

// send Write Request Packet or Read Request Packet
// return the size of the packet sent or SOCK_ERROR
int Client::send_request(Opcode opcode, std::string filename) {
    auto packet = RequestPacket(opcode, filename, this->mode);
    byte pkt[MAX_PACKET_BUF];
    size_t pkt_length = packet.encode(pkt, MAX_PACKET_BUF);
    // send the pkt
    int size = sendto(this->sock, (const char*)pkt, pkt_length, 0,
                      (sockaddr*)&this->server_addr_begin,
                      sizeof(this->server_addr_begin));
    return size;
}

/**
 * @brief send write request packet, receive an ack packet, and handle the cases
 *        for the errors happened
 *
 * @param filename
 * @return true if receive ack successfully
 * @return false if cannot receive ack after many resendings
 */
bool Client::send_valid_write_request(std::string filename) {
    bool success = false;
    int try_times = 0;
    while (!success && try_times < MAX_TRY_TIMES) {
        // send write request
        int send_size = this->send_request(Opcode::WRITE, filename);
        try_times++;
        if (send_size == SOCKET_ERROR) {
            this->logger.error("Fail to send write request, error_code: " +
                               to_string(WSAGetLastError()));
            Sleep(100);
            continue;
        }
        // wait for ack
        byte packet_buf[MAX_PACKET_BUF];
        int packet_len = sizeof(this->server_addr_then);
        int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                              (sockaddr*)&this->server_addr_then, &packet_len);
        if (r_size == SOCKET_ERROR) {
            this->handle_recv_err(WSAGetLastError());
        } else if (this->is_expected_ack(packet_buf, r_size, 0)) {
            success = true;
        }
    }
    return success;
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

/**
 * @brief send data packet, receive an ack packet, and handle the cases
 *        for the errors happened
 *
 * @param packet
 * @param cur_block
 * @return true if receive ack successfully
 * @return false if cannot receive ack after many resendings
 */
bool Client::send_valid_data_packet(DataPacket& packet, two_bytes cur_block) {
    bool success = false;
    int try_times = 0;
    while (!success && try_times < MAX_TRY_TIMES) {
        int send_size = this->send_data_packet(packet);
        try_times++;
        if (send_size == SOCKET_ERROR) {
            // send fail
            this->logger.error("Last send error: " +
                               to_string(WSAGetLastError()));
            Sleep(100);
            continue;
        }

        // waiting for ack for this block
        byte ack_buf[MAX_PACKET_BUF];
        int addr_len = sizeof(this->server_addr_then);
        // printf("Send block %d to port: %u\n", cur_block,
        //        htons(this->server_addr_then.sin_port));
        int recv_size = recvfrom(this->sock, (char*)ack_buf, MAX_PACKET_BUF, 0,
                                 (sockaddr*)&this->server_addr_then, &addr_len);
        if (recv_size == SOCKET_ERROR) {
            // cannot receive ack from server, we consume that our data packet
            // was not received by the server, so send again
            this->handle_recv_err(WSAGetLastError());
        } else if (this->is_expected_ack(ack_buf, recv_size, packet.block)) {
            success = true;
        }
    }
    return success;
}

int Client::send_file(std::string filename) {
    ifstream fin;
    if (this->mode == "octet") {
        fin.open(filename, std::ios::binary);
    } else {
        fin.open(filename);
    }
    if (!fin) {
        this->logger.panic("Cannot open file " + filename);
        printf("Cannot open file %s\n", filename.c_str());
        return false;
    }
    byte data_buf[MAX_DATA_LEN];
    two_bytes cur_block = 1;

    clock_t start, end;
    while (!fin.eof()) {
        memset(data_buf, 0, MAX_DATA_LEN);
        fin.read((char*)data_buf, MAX_DATA_LEN);
        // compose the data buf and send all the chunks
        // send a data chunk to server
        size_t data_size;
        if (fin.eof()) {
            // this is the last data packet
            data_size = strnlen((const char*)data_buf, MAX_DATA_LEN);
        } else {
            data_size = MAX_DATA_LEN;
        }

        if (cur_block % CLOCK_UNIT_NUM == 0) {
            start = clock();
        }

        DataPacket packet = DataPacket(cur_block, data_buf, data_size);
        int succ = this->send_valid_data_packet(packet, cur_block);
        if (!succ) {
            printf("Lose connection, will exit..");
            return false;
        }
        cur_block++;

        // there is cur_block++ above
        if (cur_block % CLOCK_UNIT_NUM == 0) {
            end = clock();
            this->show_speed(double(end - start), MAX_DATA_LEN* CLOCK_UNIT_NUM);
        }
    }
    return true;
}

bool Client::upload(std::string filename, std::string to_server_path) {
    // test file's existence
    ifstream fin(filename);
    if (fin.fail()) {
        this->logger.panic("Cannot open file " + filename);
        printf("Cannot open file %s\n", filename.c_str());
        return false;
    } else {
        fin.close();
    }
    try {
        clock_t start = clock();
        int wrq_succ = this->send_valid_write_request(to_server_path);
        if (!wrq_succ) {
            throw "Cannot build a connection to the server";
        }
        clock_t end = clock();
        this->show_speed(double(end - start), 4);

        this->send_file(filename);
    } catch (const char* str) {
        printf("Upload error: %s\n", str);
        std::string s(str);
        this->logger.error("Upload error: " + s);
        return false;
    }
    return true;
}

/* download */
//          //
//          //

// returns the length of data (len < 512 means last packet)
int Client::receive_data_packet(byte* data_buf) {
    int packet_len = sizeof(this->server_addr_then);
    byte packet_buf[MAX_PACKET_BUF];
    int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                          (sockaddr*)&this->server_addr_then, &packet_len);
    if (r_size == SOCKET_ERROR) {
        this->logger.error("recvfrom error: " + to_string(WSAGetLastError()));
        return SOCKET_ERROR;
    }
    int data_size = r_size - 4;
    for (int i = 0; i < data_size; i++) {
        data_buf[i] = packet_buf[i + 4];
    }
    return data_size;
}

/**
 * @brief Firstly send a ack for the last data packet and wait for the next
 * data packet. If cannot receive the next data packet, we assume that the ack
 * is not received by the server so resend the ack for the last packet.
 *
 * @param ack_block
 * @param fout
 * @return false if lose connection, success if ok and not finish, FINISH if ok
 * and all received.
 */
int Client::receive_valid_data_packet(two_bytes ack_block, ofstream& fout) {
    int state = false;
    int try_times = 0;
    while (!state && try_times < MAX_TRY_TIMES) {
        // send ack request for ack_block
        int send_size = this->send_ack(ack_block);
        try_times++;
        if (send_size == SOCKET_ERROR) {
            this->logger.error("Fail to send ack for block" +
                               to_string(ack_block) +
                               to_string(WSAGetLastError()));
            Sleep(100);
            continue;
        }

        // wait for the next data packet.
        byte data_buf[MAX_DATA_LEN];
        byte packet_buf[MAX_PACKET_BUF];
        int packet_len = sizeof(this->server_addr_then);
        int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                              (sockaddr*)&this->server_addr_then, &packet_len);

        if (r_size == SOCKET_ERROR) {
            this->handle_recv_err(WSAGetLastError());
        } else {
            if (this->is_expected_data(packet_buf, r_size, ack_block + 1)) {
                int data_size = r_size - 4;
                fout.write((const char*)(packet_buf + 4), data_size);
                if (data_size == MAX_DATA_LEN) {
                    state = true;
                } else {
                    state = FINISH;
                    // no need to implement valid ack, server's timeout is ok.
                    this->send_ack(ack_block + 1);
                }
            }
        }
    }
    return state;
}

int Client::send_ack(two_bytes block) {
    ACKPacket packet(block);
    byte packet_buf[MAX_PACKET_BUF];
    size_t packet_size = packet.encode(packet_buf, MAX_PACKET_BUF);
    int addr_len;
    int ret = sendto(this->sock, (const char*)packet_buf, packet_size, 0,
                     (sockaddr*)&this->server_addr_then,
                     sizeof(this->server_addr_then));
    return ret;
}

/**
 * @brief receive the file from block2 to the last packet.
 *
 * @param remote_path
 * @param fout
 * @return bool whether receive all the file successfully
 */
bool Client::receive_file(std::string remote_path, ofstream& fout) {
    int ack_block = 1;
    int state = true;
    clock_t start, end;
    // state == true means ok but not finish
    while (state) {
        // printf("ack block%d\n", ack_block);

        if (ack_block % CLOCK_UNIT_NUM == 0) {
            start = clock();
        }

        int ret = this->receive_valid_data_packet(ack_block, fout);
        if (ret == FINISH) return true;
        if (ret == false) return false;
        ack_block++;

        // there is cur_block++ above
        if (ack_block % CLOCK_UNIT_NUM == 0) {
            end = clock();
            this->show_speed(double(end - start), MAX_DATA_LEN* CLOCK_UNIT_NUM);
        }
    }
    return true;
}

bool Client::is_expected_data(byte* packet_buf, int recv_size,
                              two_bytes expected_block) {
    if (recv_size < 4) {
        this->logger.error("Receive a packet less than 4 bytes");
        return false;
    }
    two_bytes packet_type = htons(*(two_bytes*)packet_buf);
    if (packet_type == Opcode::ERR) {
        this->logger.error("Receive error packet from server, ");
        two_bytes err_code = htons(*(two_bytes*)(packet_buf + 2));
        std::string str((char*)packet_buf + 4);
        this->logger.error("error code: " + to_string(err_code) +
                           "error message: " + str);
        return false;
    } else if (packet_type != Opcode::DATA) {
        this->logger.error("Receive invalid packet, opcode: " +
                           to_string(packet_type));
        return false;
    }

    // check the block num
    two_bytes recv_block = htons(*(two_bytes*)(packet_buf + 2));
    if (recv_block != expected_block) {
        this->logger.error("Received unexpected block, expect " +
                           to_string(expected_block) + " but received " +
                           to_string(recv_block));
        return false;
    }
    return true;
}

/**
 * @brief send a valid read request (receving the first data packet means
 * success) and write the first data packet into fout.
 *
 * @param filename  the remote file path
 * @param fout  the ofstream
 * @returns false:fail; true:success and more than one data packet; FINISH:
 * success and only one data packet
 */
int Client::send_valid_read_request(std::string filename,
                                    std::string output_path, ofstream& fout) {
    int state = false;
    int try_times = 0;
    while (!state && try_times < MAX_TRY_TIMES) {
        // send read request
        int send_size = this->send_request(Opcode::READ, filename);
        try_times++;
        if (send_size == SOCKET_ERROR) {
            this->logger.error("Fail to send read request, error_code: " +
                               to_string(WSAGetLastError()));
            Sleep(100);
            continue;
        }

        // wait for the first data packet.
        byte data_buf[MAX_DATA_LEN];
        byte packet_buf[MAX_PACKET_BUF];
        int packet_len = sizeof(this->server_addr_then);
        int r_size = recvfrom(this->sock, (char*)packet_buf, MAX_PACKET_BUF, 0,
                              (sockaddr*)&this->server_addr_then, &packet_len);
        if (r_size == SOCKET_ERROR) {
            this->handle_recv_err(WSAGetLastError());
        } else {
            if (this->is_expected_data(packet_buf, r_size, 1)) {
                int data_size = r_size - 4;

                // create file after receive one data packet,
                // to avoid create file but remote file not exists.
                if (this->mode == "octet") {
                    fout.open(output_path, std::ios::binary | std::ios::out);
                } else {
                    fout.open(output_path, std::ios::out);
                }

                fout.write((const char*)(packet_buf + 4), data_size);
                if (data_size == MAX_DATA_LEN) {
                    state = true;
                } else {
                    state = FINISH;
                }
            }
        }
    }
    return state;
}

bool Client::download(std::string remote_path, std::string output_path) {
    // test file's existence
    try {
        ofstream fout;
        int ret = this->send_valid_read_request(remote_path, output_path, fout);
        if (ret == FINISH) {
            return true;
        }
        if (ret == false) {
            throw "Fail to build a conection";
        }
        // receive the last all blocks
        bool succ = this->receive_file(remote_path, fout);
        // printf("succ: %d", succ);
        if (succ) {
            return true;
        } else {
            throw "Lose connection to server";
        }
    } catch (const char* str) {
        std::string s(str);
        this->logger.error("Download error: " + s);
        return false;
    }
}
