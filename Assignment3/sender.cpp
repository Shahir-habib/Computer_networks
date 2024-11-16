#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <string>

#pragma comment(lib, "ws2_32.lib")  // Link Winsock2 library

using namespace std;
const int TRANSMISSION_TIME = 100;
const int TRANSMISSION_DELAY = 100;
// Shared Channel Class
class Channel {
private:
    bool isChannelBusy = false;
    chrono::time_point<chrono::steady_clock> transmissionStartTime;
    const int maxSlot = 10;
    const int slotTime = 100;           // Slot time in ms
    //mutex mtx;
    //random_device rd;
   //mt19937 gen{rd()};

public:
        void name() {
        cout<<"Channel PCLAB1"<<endl;
    }
    // Check if the channel is idle
    Channel() {

    }
    bool isIdle() {
        //lock_guard<mutex> lock(mtx);
        return !isChannelBusy;
    }

    // Start transmission (set channel busy)
    void startTransmission() {
        //lock_guard<mutex> lock(mtx);
        isChannelBusy = true;
        transmissionStartTime = chrono::steady_clock::now();
    }

    // Stop transmission (set channel idle)
    void stopTransmission() {
        //lock_guard<mutex> lock(mtx);
        isChannelBusy = false;
    }

    // Check if a collision occurred
    bool checkCollision() {
        //lock_guard<mutex> lock(mtx);
        auto currentTime = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(currentTime - transmissionStartTime).count() < TRANSMISSION_TIME;
    }

    // Generate a random backoff time
    int getBackoffTime() {
        return rand() % maxSlot * slotTime;
    }
  
};

// Sender Class

class Sender {
private:
    static const double P;             // Probability for p-persistent CSMA
    
    Channel& sharedChannel;            // Shared channel
    int numFrames;                     // Number of frames to send
    string senderName;                 // Sender name
    SOCKET socket;

    // Simulate sensing the channel and transmission
    bool senseChannel() {
        int randomValue = rand() % 100 + 1;
        return randomValue <= P * 100;
    }

public:
    Sender(Channel& channel, const string& name, int frames, const string& serverIP, int serverPort)
        : sharedChannel(channel), senderName(name), numFrames(frames) {
        // Initialize socket connection to receiver
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << senderName << " failed to initialize Winsock.\n";
            exit(EXIT_FAILURE);
        }

        if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            cerr << senderName << " failed to create socket.\n";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

        if (connect(socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << senderName << " failed to connect to receiver.\n";
            closesocket(socket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        cout << senderName << " connected to receiver.\n";
    }

    ~Sender() {
        closesocket(socket);
        WSACleanup();
    }

    void transmitData() {
        for (int i = 1; i <= numFrames; ++i) {
            if (sharedChannel.isIdle() && senseChannel()) {
                // Start transmission
                sharedChannel.startTransmission();
                cout << senderName << " transmitting frame " << i << endl;
                string frame = senderName + ": Frame " + to_string(i);
                send(socket, frame.c_str(), frame.size(), 0);
                this_thread::sleep_for(chrono::milliseconds(TRANSMISSION_DELAY));

                // Check for collision
                if (sharedChannel.checkCollision()) {
                    cout << senderName << ": Collision occurred on frame " << i << ", backing off...\n";
                    sharedChannel.stopTransmission();
                    int backoffTime = sharedChannel.getBackoffTime();
                    this_thread::sleep_for(chrono::milliseconds(backoffTime));
                    --i;  // Retry the same frame
                } else {
                    cout << senderName << " successfully sent frame " << i << endl;
                    sharedChannel.stopTransmission();
                }
            } else {
                // Wait due to channel being busy
                cout << senderName << ": Waiting for channel...\n";
                this_thread::sleep_for(chrono::milliseconds(TRANSMISSION_DELAY));
                --i;  // Retry the same frame
            }
        }
    }

};

// Initialize static member
const double Sender::P = 0.5;

// Main Function
int main() {
    const string SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 5000;
    const int NUM_FRAMES = 5;
    Channel sharedChannel;
    sharedChannel.name();
    
    vector<thread> senderThreads;
    // Create sender threads
    senderThreads.emplace_back([&sharedChannel,NUM_FRAMES, SERVER_IP, SERVER_PORT]() {
             Sender sender1(sharedChannel, "Sender1", NUM_FRAMES, SERVER_IP, SERVER_PORT);
             sender1.transmitData();
            sharedChannel.name();
        });

    senderThreads.emplace_back([&sharedChannel,NUM_FRAMES, SERVER_IP, SERVER_PORT]() {
             Sender sender2(sharedChannel, "Sender2", NUM_FRAMES, SERVER_IP, SERVER_PORT);
             sender2.transmitData();
            sharedChannel.name();
        });
    auto startTime = chrono::steady_clock::now();

      // Wait for all threads to complete
    for (auto& t : senderThreads) {
        t.join();
    }
    auto endTime = chrono::steady_clock::now();
    auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();

    // Calculate throughput and forwarding delay
    cout << "Throughput: " << static_cast<int>(80 / (static_cast<float>(elapsedTime) / 1000)) << " bits/sec\n";
    cout << "Forwarding Delay: " << elapsedTime / 10 << " ms\n";

    cout << "Both senders have completed transmission.\n";

    return 0;
}
