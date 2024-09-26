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
public:
    Frame(string data, string source_addr, string dest_addr, unsigned int len, unsigned short frame_seq)
    {
        this->data = 'b' + data;
        this->source_addr = source_addr;
        this->dest_addr = dest_addr;
        this->len = len;
        this->frame_seq = frame_seq;
    }
    Frame(string data, string source_addr, string dest_addr, unsigned int len, unsigned short frame_seq, string trailer)
    {
        this->data = data;
        this->source_addr = source_addr;
        this->dest_addr = dest_addr;
        this->len = len;
        this->frame_seq = frame_seq;
        this->trailer = trailer;
    }
    void addCRC()
    {
        string crcData = crc_remainder(data, get_divisor("CRC-32"));
        trailer = crcData;
    }
    bool validateCRC()
    {
        return true;
        string crcData = crc_remainder(data, get_divisor("CRC-32"));
        return crcData == trailer;
    }
    void addChecksum()
    {
        string checksumData = generate_checksum(data, 8);
        trailer = checksumData;
    }
    bool validateChecksum()
    {
        string checksumData = generate_checksum(data, 8);
        return checksumData == trailer;
    }
    static Frame parseFrame(string &frame_str)
    {
        string tp;
        int delpos;
        for (auto x : frame_str)
        {
            if (x != '-')
            {
                tp += x;
            }
            if (x == 'b')
            {
                delpos = tp.size();
            }
        }
        frame_str = tp;
        frame_str.erase(frame_str.begin() + delpos - 1);
        string source_addr = frame_str.substr(0, 12);
        string dest_addr = frame_str.substr(12, 12);
        unsigned int len = stoi(frame_str.substr(24, 1));
        unsigned short frame_seq = static_cast<unsigned short>(stoi(frame_str.substr(25, delpos - 26)));
        string data = frame_str.substr(delpos - 1, 8);
        string trailer = frame_str.substr(frame_str.size() - 33, 33);

        cout << "Source Address: " << source_addr << endl;
        cout << "Destination Address: " << dest_addr << endl;
        cout << "Length: " << len << endl;
        cout << "Frame Sequence: " << frame_seq << endl;
        cout << "Data: " << data << endl;
        cout << "Trailer: " << trailer << endl;
        cout << "Trailer: " << crc_remainder(data, get_divisor("CRC-32")) << endl;
        if (trailer == crc_remainder(data, get_divisor("CRC-32")))
            cout << "valid";
        return Frame(data, source_addr, dest_addr, len, frame_seq, trailer);
    }

    // Function to process the received frame (implement according to your requirements)
    static void processFrame(const Frame &frame)
    {
        FILE *file = fopen("received_data.txt", "a");
        if (file)
        {
            fprintf(file, "%s", frame.data.c_str());
            fclose(file);
        }
    }
    static void show(const Frame &frame)
    {
        cout << "Source Address: " << frame.source_addr << endl;
        cout << "Destination Address: " << frame.dest_addr << endl;
        cout << "Length: " << frame.len << endl;
        cout << "Frame Sequence: " << frame.frame_seq << endl;
        cout << "Data: " << frame.data << endl;
        cout << "Trailer: " << frame.trailer << endl;
    }
};