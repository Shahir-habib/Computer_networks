#include <bits/stdc++.h>
using namespace std;

void singlebit_random_error(string &data)
{
    int n = data.length();
    int random_index = rand() % n;
    if (data[random_index] == '0')
        data[random_index] = '1';
    else
        data[random_index] = '0';
}
void singlebit_error(string &data, int index)
{
    if (data[index] == '0')
        data[index] = '1';
    else
        data[index] = '0';
}
void isolated_doublebit_error(string &data, int index1, int index2)
{
    if (data[index1] == '0')
        data[index1] = '1';
    else
        data[index1] = '0';
    if (data[index2] == '0')
        data[index2] = '1';
    else
        data[index2] = '0';
}
void odd_errors(string &data , int n)
{
    while(n--)
    {
        int random_index = rand() % data.length();
        if (data[random_index] == '0')
            data[random_index] = '1';
        else
            data[random_index] = '0';
    }
}
void burst_error(string &data, int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        if (data[i] == '0')
            data[i] = '1';
        else
            data[i] = '0';
    }
}

void  ask(string &data)
{
    cout << "Enter the type of error you want to inject: " << endl;
    cout << "1. Single bit random error" << endl;
    cout << "2. Single bit error" << endl;
    cout << "3. Isolated double bit error" << endl;
    cout << "4. Odd number of errors" << endl;
    cout << "5. Burst error" << endl;
    cout << "6. Exit" << endl;
    cout << "Enter your choice: ";
    int choice;
    cin >> choice;
    switch(choice)
    {
        case 1:
            singlebit_random_error(data);
            break;
        case 2:
            cout << "Enter the index at which you want to inject error: ";
            int index;
            cin >> index;
            singlebit_error(data, index);
            break;
        case 3:
            cout << "Enter the indices at which you want to inject error: ";
            int index1, index2;
            cin >> index1 >> index2;
            isolated_doublebit_error(data, index1, index2);
            break;
        case 4:
            cout << "Enter the number of errors you want to inject: ";
            int n;
            cin >> n;
            odd_errors(data, n);
            break;
        case 5:
            cout << "Enter the start and end index of the burst error: ";
            int start, end;
            cin >> start >> end;
            burst_error(data, start, end);
            break;
        case 6:
            break;
        default:
            cout << "Invalid choice\n";
    }
    
}