#include "packet.hh"

inline byte low_byte(two_bytes num) { return (byte)(num & 0x00ff); }
inline byte high_byte(two_bytes num) { return (byte)((num >> 8) & 0x00ff); }

RequestPacket::RequestPacket(two_bytes _opcode, std::string _filename,
                             std::string _mode)
    : opcode(_opcode), filename(_filename), mode(_mode) {}

size_t RequestPacket::encode(byte* buf, size_t max_size) {
    buf[0] = high_byte(opcode);
    buf[1] = low_byte(opcode);
    int i = 2;
    if (i + filename.size() > MAX_FILENAME || i + filename.size() >= max_size) {
        throw "Filename too long";
    }
    for (byte ch : filename) {
        buf[i++] = ch;
    }
    if (i + mode.size() + 2 >= max_size) {
        throw "Packet too long";
    }
    buf[i++] = 0;
    for (byte ch : mode) {
        buf[i++] = ch;
    }
    buf[i] = 0;
    return i;
}

DataPacket::DataPacket(two_bytes _opcode, two_bytes _block, byte* _data,
                       size_t _data_size)
    : opcode(_opcode), block(_block), data_size(_data_size) {
    for (size_t i = 0; i < _data_size; i++) {
        data[i] = _data[i];
    }
}

size_t DataPacket::encode(byte* buf, size_t max_size) {
    buf[0] = 0;
    buf[1] = 0x3;
    buf[2] = high_byte(block);
    buf[3] = low_byte(block);
    if (4 + data_size > max_size) {
        throw "data too long";
    }
    for (size_t i = 0; i < data_size; i++) {
        buf[4 + i] = data[i];
    }
    return 4 + data_size;
}

ACKPacket::ACKPacket(two_bytes _opcode, two_bytes _block)
    : opcode(_opcode), block(_block) {}

size_t ACKPacket::encode(byte* buf, size_t max_size) {
    if (max_size < 4) {
        throw "ack packet needs 4 bytes.";
    }
    buf[0] = high_byte(opcode);
    buf[1] = low_byte(opcode);
    buf[2] = high_byte(block);
    buf[3] = low_byte(block);
    return 4;
}

ErrorPacket::ErrorPacket(two_bytes _opcode, two_bytes _err_code,
                         std::string _err_string)
    : opcode(_opcode), err_code(_err_code), err_string(_err_string) {}

size_t ErrorPacket::encode(byte* buf, size_t max_size) {
    buf[0] = high_byte(opcode);
    buf[1] = low_byte(opcode);
    buf[2] = high_byte(err_code);
    buf[3] = low_byte(err_code);
    auto str_size = err_string.size();
    if (4 + str_size > max_size) {
        throw "error string too long";
    }
    for (size_t i = 0; i < str_size; i++) {
        buf[4 + i] = err_string[i];
    }
    return 4 + str_size;
}