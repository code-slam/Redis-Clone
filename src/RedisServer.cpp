#include "../include/RedisServer.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include<netinet/in.h>

static RedisServer* globalServer=nullptr;

RedisServer::RedisServer(int port) :port(port),server_socket(-1) ,running(true){
    globalServer=this; 
}

void RedisServer::shutdown(){
    running=false;

    if(server_socket!=-1){
        close(server_socket);

    }
    std::cout <<"Server Shutdown Gracefully!\n";
}

void RedisServer::run(){
    //create a socket(TCP/IPV4)
    server_socket=socket(AF_INET,SOCK_STREAM,0);
    //ipv4,reliable connection oriented byte stream.default proto type 0
    if(server_socket<0){
        std::cerr<<"Error Creating Server Socket\n";
        return;
    } 
    //even if the port is showing occupied we will bind the socket to this port
    int opt=1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in serverAddr{};

    serverAddr.sin_family =AF_INET; //IPv4
    serverAddr.sin_port=htons(port); //hosttonetwork byte order to the port
    serverAddr.sin_addr.s_addr= INADDR_ANY; //listen to all local inetrface
    //bind socket to that IP/port else thro error
    if(bind(server_socket,(struct sockAddr*)&serverAddr,sizeof(serverAddr))<0){
        std::cerr<<"Error Binding Socket\n";
        return ;
    }   
    //10 incoming connections can queue up before os will reject new cinections
    if(listen(server_socket,10)<0){
        std::cerr<<"Error Listening On Server Socket\n";
        return ;
    }
    //server is ready to accept clients
    std::cout<<"Redis Server Litening On port" <<port<< "\n";

}