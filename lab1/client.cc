
#include "client.hh"

#include "packet.hh"

using std::ifstream;

/**
 * @brief Create a udp sock object (I'm the end to send udp)
 *
 * @param udp
 * @return SOCKET
 */
SOCKET create_udp_send_sock(UDP_INFO& udp, sockaddr_in& src) {
    src.sin_family = AF_INET;
    src.sin_addr.s_addr = inet_addr(udp.src_ip);
    src.sin_port = htons(udp.src_port);  // choose any

    // create the socket
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // bind to the src address
    // bind(s, (sockaddr*)&src, sizeof(src));
    return s;
}

/**
 * @brief send Write Request Packet or Read Request Packet
 *
 * @param opcode
 * @param filename
 * @param src_ip
 * @param dst_ip
 * @return int
 */
int send_request(Opcode opcode, std::string filename, UDP_INFO& udp) {
    auto packet = RequestPacket(opcode, filename, "netascii");
    byte pkt[MAX_PACKET_BUF];
    size_t pkt_length = packet.encode(pkt, MAX_PACKET_BUF);

    // destination addr
    sockaddr_in dst;
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(udp.dst_ip);
    dst.sin_port = htons(udp.dst_port);

    sockaddr_in src;
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);
    SOCKET s = create_udp_send_sock(udp, src);
    // send the pkt
    int ret = sendto(s, (const char*)pkt, pkt_length, 0, (sockaddr*)&dst,
                     sizeof(dst));
    closesocket(s);
    return ret;
}

/**
 * @brief send rrq
 */
int send_read_request(std::string filename, UDP_INFO& udp) {
    int payload_size = send_request(Opcode::READ, filename, udp);
    return payload_size;
}

/**
 * @brief send wrq
 */
int send_write_request(std::string filename, UDP_INFO& udp) {
    int payload_size = send_request(Opcode::WRITE, filename, udp);
    return payload_size;
}

/**
 * @brief send dara packet, a assemblied packet should be pass as a param
 *
 */
int send_data_packet(DataPacket& packet, UDP_INFO& udp) {}

/**
 * @brief before this, the client sent a write request, and the tftp server
 *        should send a ack packet and tell the client the PORT
 * @param recv_port
 * @return the tid(or port) of the end which would like to be written
 */
u_short receive_ack(u_short recv_port) {
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in add;
    add.sin_family = AF_INET;
    add.sin_addr.s_addr = htonl(INADDR_ANY);
    add.sin_port = htons(recv_port);

    bind(sock, (sockaddr*)&add, sizeof(add));

    byte buf[MAX_PACKET_BUF];

    // waiting for data
    sockaddr_in from;
    int size = sizeof(from);
    printf("start to receive at port %u...\n", recv_port);
    int ret =
        recvfrom(sock, (char*)buf, MAX_PACKET_BUF, 0, (sockaddr*)&from, &size);
    if (ret < 0) {
        throw "recvfrom ret error\n";
    }
    buf[ret] = 0;
    printf("receive ack from %s:%d\n", inet_ntoa(from.sin_addr), htons(from.sin_port));
    if (htons(*(two_bytes*)buf) == Opcode::ERR) {
        printf("receive error packet...%s\n", &buf[4]);
        throw "recvfrom ret error\n";
    }
    closesocket(sock);
    // end should change
    return htons(from.sin_port);
}

int send_data_packet(SOCKET& sock, sockaddr_in& dst, DataPacket& packet) {
    byte packet_buf[MAX_PACKET_BUF];
    size_t packet_size = packet.encode(packet_buf, MAX_PACKET_BUF);
    int ret = sendto(sock, (const char*)packet_buf, packet_size, 0,
                     (sockaddr*)&dst, sizeof(dst));
    return ret;
}

int send_file(std::string filename, UDP_INFO udp) {
    ifstream fin(filename);
    if (!fin) {
        throw "Cannot open file" + filename;
    }
    byte data_buf[MAX_DATA_LEN];
    two_bytes cur_block = 1;

    while (!fin.eof()) {
        memset(data_buf, 0, MAX_DATA_LEN);
        fin.read((char*)data_buf, MAX_DATA_LEN);
        std::streamsize cur_p = fin.gcount();
        // compose the data buf and send all the chunks

        // destination addr
        sockaddr_in dst;
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = inet_addr(udp.dst_ip);
        dst.sin_port = htons(udp.dst_port);

        // send a data chunk to server
        size_t data_size;
        if (fin.eof()) {
            // this is the last data packet
            data_size = strnlen((const char*)data_buf, MAX_DATA_LEN);
        } else {
            data_size = MAX_DATA_LEN;
        }
        sockaddr_in src;
        WSAData data;
        WSAStartup(MAKEWORD(2, 2), &data);
        SOCKET sock = create_udp_send_sock(udp, src);
        DataPacket packet =
            DataPacket(Opcode::DATA, cur_block, data_buf, data_size);
        send_data_packet(sock, dst, packet);
        closesocket(sock);

        // waiting for ack for this block

        u_short server_port = receive_ack(udp.src_port);
        udp.dst_port = server_port;
        cur_block++;
    }
    return 0;
}