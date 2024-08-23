#include <bits/stdc++.h>
using namespace std;
unordered_map<string, string> crc_table = {
    {"CRC-8", "x8+x7+x6+x4+x2+1"},
    {"CRC-10", "x10+x9+x5+x4+x+1"},
    {"CRC-16", "x16+x15+x2+1"},
    {"CRC-32", "x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1"}};
string get_divisor(string crc_type)
{
    if (crc_type == "CRC-8")
        return "111010101"; // x^8 + x^7 + x^6 + x^4 + x^2 + 1
    else if (crc_type == "CRC-10")
        return "11000110011"; // x^10 + x^9 + x^5 + x^4 + x + 1
    else if (crc_type == "CRC-16")
        return "11000000000000101"; // x^16 + x^15 + x^2 + 1
    else if (crc_type == "CRC-32")
        return "100000100110000010001110110110111"; // x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
    return "";
}
string xor_operation(const string &a, const string &b)
{
    string result;
    for (size_t i = 0; i < b.size(); ++i)
    {
        result += (a[i] == b[i]) ? '0' : '1';
    }
    return result;
}
string crc_remainder(const string &data, const string &divisor)
{
    int n = data.size(), m = divisor.size();
    string padded_data = data + string(m - 1, '0');
    for (int i = 0; i <= n; ++i)
    {
        if (padded_data[i] == '1')
        {
            string current_dividend = padded_data.substr(i, m);
            if (current_dividend.size() < m)
                current_dividend += string(m - current_dividend.size(), '0');
            padded_data.replace(i, m, xor_operation(current_dividend, divisor));
        }
    }
    return padded_data.substr(n); // Return the remainder
}
vector<string> generate_crc(const vector<string> &packets, const string &crc_type)
{
    string divisor = get_divisor(crc_type);
    vector<string> result;
    for (const string &packet : packets)
    {
        string remainder = crc_remainder(packet, divisor);
        result.push_back(packet + remainder);
    }
    return result;
}
bool validate_crc(const vector<string> &packets, const string &divisor)
{
    for (const string &packet : packets)
    {
        string remainder = crc_remainder(packet, divisor);
        if (remainder.find('1') != string::npos)
            return false;
    }
    return true;
}