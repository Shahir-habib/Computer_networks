#include <bits/stdc++.h>
using namespace std;
string Ones_complement(string data)
{
    for (int i = 0; i < data.length(); i++)
    {
        if (data[i] == '0')
            data[i] = '1';
        else
            data[i] = '0';
    }

    return data;
}
string generate_checksum(string data, int block_size = 8)
{
    int n = data.length();
    string result = "";
    for (int i = 0; i < block_size; i++)
    {
        result += data[i];
    }
    for (int i = block_size; i < n; i += block_size)
    {

        string next_block = "";
        for (int j = i; j < i + block_size; j++){
            next_block += data[j];
        }
        string additions = "";
        int sum = 0, carry = 0;
        for (int k = block_size - 1; k >= 0; k--){
            sum += (next_block[k] - '0') + (result[k] - '0');
            carry = sum / 2;
            if (sum == 0){
                additions = '0' + additions;
                sum = carry;
            }
            else if (sum == 1){
                additions = '1' + additions;
                sum = carry;
            }
            else if (sum == 2){
                additions = '0' + additions;
                sum = carry;
            }
            else
            {
                additions = '1' + additions;
                sum = carry;
            }
        }
        string final = "";
        if (carry == 1)
        {
            for (int l = additions.length() - 1; l >= 0;
                 l--)
            {
                if (carry == 0)
                {
                    final = additions[l] + final;
                }
                else if (((additions[l] - '0') + carry) % 2 == 0)
                {
                    final = "0" + final;
                    carry = 1;
                }
                else
                {
                    final = "1" + final;
                    carry = 0;
                }
            }
            result = final;
        }
        else
        {
            result = additions;
        }
    }
    return Ones_complement(result);
}

bool validate_checksum(vector<string> packets, string checksum)
{
    string sender_checksum = checksum;
    string rec_message = "";
    for (auto packet : packets)
    {
        rec_message += packet;
    }
    string receiver_checksum = generate_checksum(rec_message + sender_checksum, 8);
    if (count(receiver_checksum.begin(),
              receiver_checksum.end(), '0') == 8)
    {
        return true;
    }
    else
    {
        return false;
    }
}
