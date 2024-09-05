#include <bits/stdc++.h>
using namespace std;
#include "../Assignment1/checksum.h"
#include "../Assignment1/crc.h"

// Considering the Frame having data of 48 bytes
class Frame
{
public:
    string source_addr;       // 6 Bytes
    string dest_addr;         // 6 Bytes
    unsigned int len;         // 2 Bytes
    unsigned short frame_seq; // 2 Bytes
    string data;              // 48 Bytes
    string trailer;           // 4 Bytes
    Frame(string data, string source_addr, string dest_addr, unsigned int len, unsigned short frame_seq)
    {
        this->data = data;
        this->source_addr = source_addr;
        this->dest_addr = dest_addr;
        this->len = len;
        this->frame_seq = frame_seq;
    }
    void addCRC()
    {
        string crcData = crc_remainder(data, get_divisor("CRC-32"));
        trailer = crcData;
    }
    bool validateCRC()
    {
        string crcData = crc_remainder(data, get_divisor("CRC-32"));
        return crcData == trailer;
    }
    void addChecksum()
    {
        string checksumData = generate_checksum(data, 32);
        trailer = checksumData;
    }
    bool validateChecksum()
    {
        string checksumData = generate_checksum(data, 32);
        return checksumData == trailer;
    }
};