#ifndef PACKET
#define PACKET
#define byte unsigned char
#define two_bytes unsigned short
#include <memory>
#include <string>
#define MAX_FILENAME 512
#define MAX_PACKET_BUF 1024
#define MAX_DATA_LEN 512
#define MAX_TRY_TIMES 5
enum Opcode {
    READ = 1,
    WRITE,
    DATA,
    ACK,
    ERR,
};

class RequestPacket {
   public:
    two_bytes opcode;
    std::string filename;
    std::string mode;

   public:
    RequestPacket(two_bytes _opcode, std::string _filename, std::string _mode);
    size_t encode(byte* buf, size_t max_size);
};

// @params opcode 03
class DataPacket {
   public:
    two_bytes opcode;
    two_bytes block;
    byte data[512];
    size_t data_size;

   public:
    DataPacket(two_bytes _block, byte* _data, size_t _data_size);
    size_t encode(byte* buf, size_t max_size);
};

class ACKPacket {
   public:
    two_bytes opcode;
    two_bytes block;

   public:
    ACKPacket(two_bytes _block);
    size_t encode(byte* buf, size_t max_size);
};

class ErrorPacket {
   public:
    two_bytes opcode;
    two_bytes err_code;
    std::string err_string;

   public:
    ErrorPacket(two_bytes _err_code, std::string _err_string);
    size_t encode(byte* buf, size_t max_size);
};
#endif