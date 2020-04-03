#pragma once
#include <iostream>
#include <enet/enet.h>
#include <set>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
class Server {
    std::set<ENetPeer*> clients;
    std::queue<std::string> commands;
    std::mutex commands_mutex;
    std::atomic<bool> running;
public:
    Server();
    void start();
    void send(ENetPeer* client,enet_uint8* data,int length,int channel=0,bool reliable=true);
    void sendAll(enet_uint8* data,int length,int channel=0,bool reliable=true);
    void listenCommands();
    void split(std::vector<std::string>& buffer,const std::string& s, char separator,int nb_split=INT_MAX) ;
    void executeCommands();
    std::string getAdress(enet_uint32 host);

    void disconnect(ENetPeer *client);
};
