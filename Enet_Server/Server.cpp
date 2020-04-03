//
// Created by User on 28-Mar.-2020.
//

#include "Server.h"
Server::Server() {

}
void Server::start() {
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
        running.store(false);
    } else {
        std::cout << "Server started!" << std::endl;
        running.store(true);
        std::thread t = std::thread(&Server::listenCommands,this);
        t.detach();
        ENetEvent event;
        while (running.load() || !clients.empty()) {
            executeCommands();
            if (enet_host_service(server, &event, 0) > 0) {

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
                               event.peer->address.port, event.peer->connectID);
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
    }
    enet_host_destroy(server);
}

void Server::disconnect(ENetPeer *client){
    enet_peer_disconnect(client, 0);
    std::cout<<"Disconnecting client "<<std::to_string(client->connectID)<<"..."<<std::endl;
}
void Server::send(ENetPeer *client, enet_uint8 *data, int length, int channel , bool reliable ) {
    ENetPacket *packet = enet_packet_create(data, length,
                                            reliable ? ENET_PACKET_FLAG_RELIABLE
                                                     : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(client, channel, packet);
    std::cout << "sent " << client->address.host << ":" << data << std::endl;
}

void Server::sendAll(enet_uint8 *data, int length, int channel, bool reliable) {
    for (auto c:clients) {
        send(c, data, length, channel, reliable);
    }
}

void Server::listenCommands() {
    while (running.load()) {
        char command[80];
        std::cin.getline(command, 80);
        commands_mutex.lock();
        commands.push(command);
        commands_mutex.unlock();
    }
}

void Server::split(std::vector<std::string> &buffer, const std::string &s, char separator, int nb_split) {

    std::size_t start = 0, end = 0;
    int count = 0;
    while (count < nb_split && (end = s.find(separator, start)) != std::string::npos) {
        buffer.emplace_back(s.substr(start, end - start));
        start = end + 1;
        count++;
    }
    buffer.emplace_back(s.substr(start));
}

void Server::executeCommands() {
    commands_mutex.lock();
    while (!commands.empty()) {
        std::vector<std::string> command;
        if (commands.front().size() > 1) {
            split(command, commands.front(), ' ', 1);
            if (command.at(0) == "stop") {
                running.store(false);
                for (auto c:clients) {
                    disconnect(c);
                }
                std::cout << "Shutting down..." << std::endl;
            } else if (command.at(0) == "send") {
                if (command.size() > 1) {
                    sendAll((enet_uint8 *) command.at(1).c_str(), command.at(1).size());
                } else {
                    std::cout << "Cant send empty string" << std::endl;
                }
            } else if(command.at(0)=="kick")
            {
                if(command.size()==2)
                {
                    enet_uint32 id;
                    try {
                        enet_uint32 id = static_cast<enet_uint32>(std::stoi(command[1]));
                        bool found=false;
                        for(auto c:clients)
                        {
                            if(c->connectID==id){
                                disconnect(c);
                                found=true;
                                break;
                            }
                        }
                        if(!found)
                        {
                            std::cout<<"client "<<command[1]<<" was not found"<<std::endl;
                        }
                    }
                    catch(std::exception e) {
                        std::cout<<"To kick a client the argument must be the client id"<<std::endl;
                    }
                }
            }
        }
        commands.pop();
    }
    commands_mutex.unlock();
}

std::string Server::getAdress(enet_uint32 host) {
    unsigned char bytes[4];
    bytes[3] = (host >> 24) & 0xFF;
    bytes[2] = (host >> 16) & 0xFF;
    bytes[1] = (host >> 8) & 0xFF;
    bytes[0] = host & 0xFF;
    std::string s = "";

    s += std::to_string(static_cast<uint8_t>(bytes[0]));
    s += '.';
    s += std::to_string(static_cast<uint8_t>(bytes[1]));
    s += '.';
    s += std::to_string(static_cast<uint8_t>(bytes[2]));
    s += '.';
    s += std::to_string(static_cast<uint8_t>(bytes[3]));
    return s;
}

