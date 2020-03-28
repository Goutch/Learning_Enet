#include <iostream>
#include <enet/enet.h>
#include <set>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
std::set<ENetPeer*> clients;
std::queue<std::string> commands;
std::mutex commands_mutex;
std::atomic<bool> running(false);
void send(ENetPeer* client,enet_uint8* data,int length,int channel=0,bool reliable=true){
    ENetPacket *packet = enet_packet_create(data,length,
                                            reliable ? ENET_PACKET_FLAG_RELIABLE: ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(client, channel, packet);
    std::cout<<"sent "<<client->address.host<<":"<<data<<std::endl;
}
void sendAll(enet_uint8* data,int length,int channel=0,bool reliable=true)
{
    for(auto c:clients)
    {
        send(c,data,length,channel,reliable);
    }

}

void listenCommands(){
    while(running.load())
    {
        char command[80];
        std::cin.getline(command,80);
        commands_mutex.lock();
        commands.push(command);
        commands_mutex.unlock();
    }
}
void split(std::vector<std::string>& buffer,const std::string& s, char separator,int nb_split=INT_MAX) {

    std::size_t start = 0, end = 0;
    int count=0;
    while (count<nb_split&&(end =s.find(separator, start)) != std::string::npos) {
        buffer.emplace_back(s.substr(start, end - start));
        start = end + 1;
        count++;
    }
    buffer.emplace_back(s.substr(start));
}

void executeCommands(){
    commands_mutex.lock();
    while(!commands.empty())
    {
        std::vector<std::string> command;
        if(commands.front().size()>1)
        {
            split(command,commands.front(),' ',1);
            if(command.at(0)=="stop"){
                running.store(false);
                for (auto c:clients) {
                    enet_peer_disconnect(c,0);
                }
                std::cout<<"Shutting down..."<<std::endl;
            } else if(command.at(0)=="send")
            {
                if(command.size()>1)
                {
                    sendAll((enet_uint8 *) command.at(1).c_str(), command.at(1).size());
                }
                else{
                    std::cout<<"Cant send empty string"<<std::endl;
                }
            }
        }
        commands.pop();
    }
    commands_mutex.unlock();
}
std::string getAdress(enet_uint32 host)
{
    unsigned char bytes[4];
    bytes[3]=(host >> 24) & 0xFF;
    bytes[2]=(host>> 16) & 0xFF;
    bytes[1]=(host >> 8) & 0xFF;
    bytes[0]=host & 0xFF;
    std::string s="";

    s+=std::to_string(static_cast<int>(bytes[0]));
    s+='.';
    s+=std::to_string(static_cast<int>(bytes[1]));
    s+='.';
    s+=std::to_string(static_cast<int>(bytes[2]));
    s+='.';
    s+=std::to_string(static_cast<int>(bytes[3]));
    return s;
}
#include <string>
int main() {
    const int CONNECTIONS_LIMITS = 8;
    if (enet_initialize() != 0) {
        std::cout << "Error initializing enet" << std::endl;
    }
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 7777;

    ENetHost *server = enet_host_create(&address, CONNECTIONS_LIMITS, 1, 0, 0);
    if (server == nullptr) {
        std::cout << "Error creating server" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout<<"Server started!"<<std::endl;
    running.store(true);
    std::thread t=std::thread(&listenCommands);
    t.detach();
    ENetEvent event;
    while (running.load()||!clients.empty()) {
        executeCommands();
        if(enet_host_service(server, &event, 0) > 0) {

            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Client %u connected from %s.\n",
                           event.peer->connectID,
                           getAdress(event.peer->address.host).c_str(),
                           event.peer->address.port);
                    clients.emplace(event.peer);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("Client %u from %s disconected \n",
                           event.peer->connectID,
                           getAdress(event.peer->address.host).c_str(),
                           event.peer->address.port,event.peer->connectID);
                    clients.erase(event.peer);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:


                    printf("Received:%s \nFrom Client %u %s \nChannel:%i.\n",
                           event.packet->data,
                           event.peer->connectID,
                           getAdress(event.peer->address.host).c_str(),
                           event.channelID);

                    enet_packet_destroy(event.packet);
                    break;
            }
        }
    }

    enet_host_destroy(server);
    return 0;
}