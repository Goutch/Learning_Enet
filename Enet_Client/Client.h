#pragma once
#include <queue>
#include <enet/enet.h>
#include <mutex>
#include <memory>
#include <atomic>
struct Message{
    std::shared_ptr<char> data;
    int length;
    bool reliable;
    int channel;
    bool empty;
    Message(char* data,int length,bool reliable= true,int channel=0){
        this->data=std::shared_ptr<char>(data);
        this->length=length;
        this->reliable=reliable;
        this->channel=channel;
        this->empty=length<=0;
    }
};
class Client {
    std::queue<Message> messages;
    std::atomic<bool> connected;
    ENetHost* client;
    ENetPeer* server;
    std::mutex messages_mutex;

public:
    Client();
    ~Client();
    void listen();
    void connect(const char * ip_address,int port);
    void disconnect();
    void send(Message message);
    bool isConnected();
    Message fetch();
};
