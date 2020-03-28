#include <iostream>
#include "Client.h"
int main() {
    Client client;
    std::string ip="127.0.0.1";
    client.connect(ip.c_str(),7777);
    char m[]={"hello"};
    client.send({m,5});
    while(client.isConnected())
    {
        Message message=client.fetch();
    }
    client.disconnect();
    return 0;
}