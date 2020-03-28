//
// Created by User on 27-Mar.-2020.
//
#include "Client.h"
#include <iostream>

#include <thread>
Client::Client() {
    connected.store(false);
    if (enet_initialize() != 0) {
        std::cout << "Error: Cant initalize enet" << std::endl;
        throw std::exception();
    }
    client = enet_host_create(nullptr, 1, 0, 0, 0);
    if (client == nullptr) {
        std::cout << "Error: Cant initalize client" << std::endl;
        throw std::exception();
    }
}

Client::~Client() {
    enet_deinitialize();
}

void Client::connect(const char *ip_address, int port) {
    ENetAddress address;

    enet_address_set_host(&address, ip_address);
    address.port = port;
    std::cout << "Finding server..." << std::endl;
    server = enet_host_connect(client, &address, 1, 0);
    if (server == nullptr) {
        std::cout << "Error: No server available at the port or address specified" << std::endl;
    }
    ENetEvent event;
    if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        connected.store(true);
        std::cout << "Connected to server " <<ip_address<< std::endl;
    } else {
        enet_peer_reset(server);
        std::cout << "Error: Cant find server at " <<ip_address<< std::endl;
    }

    std::thread t=std::thread(&Client::listen,this);
    t.detach();
}

void Client::send(Message message) {
    if (connected.load()) {
        ENetPacket *packet = enet_packet_create(message.data.get(), message.length,
                                                message.reliable ? ENET_PACKET_FLAG_RELIABLE
                                                                 : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        enet_peer_send(server, message.channel, packet);
        std::cout << "Sent: " <<message.data<< std::endl;
    } else {
        std::cout << "Error:Cant send message while disconnected" << std::endl;
    }
}

Message Client::fetch() {
    if (connected.load()) {
        std::lock_guard<std::mutex> lockGuard(messages_mutex);
        if (!messages.empty()) {
            Message m = messages.front();
            messages.pop();
            std::cout << "Received:" <<m.data<< std::endl;
            return m;
        } else{
            return {nullptr,0};
        }
    } else {
        std::cout << "Error:Cant fetch messages while disconnected" << std::endl;
    }
}

void Client::listen() {
    ENetEvent event;
    while (connected.load()) {
        if(enet_host_service(client, &event, 0) > 0) {

            switch (event.type) {
                case ENET_EVENT_TYPE_NONE:
                    break;
                case ENET_EVENT_TYPE_CONNECT:
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    connected.store(false);
                    std::cout<<"Disconnected"<<std::endl;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    messages_mutex.lock();
                    messages.emplace((char*)event.packet->data,
                            (int)event.packet->dataLength,
                            event.packet->flags!=ENET_PACKET_FLAG_RELIABLE,
                            (int)event.channelID);
                    messages_mutex.unlock();

                    break;
            }
        }
    }
}

void Client::disconnect() {
    if(connected.load())
    {
        enet_peer_disconnect(server, 0);
        connected.store(false);
    }
}

bool Client::isConnected() {
    return connected.load();
}
