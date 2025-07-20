#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h" 
#include <iostream>
#include <cstring>         // for memset
#include <thread>          // for std::thread
#include <vector>          // for std::vector
#include <unistd.h>        // for close()
#include <netinet/in.h>    // for sockaddr_in
#include <sys/socket.h>    // for socket(), bind(), listen(), accept()
#include <arpa/inet.h>     // for htons, htonl
#include <csignal>

static RedisServer* globalServer=nullptr;


void signalHandler(int signum){
    if(globalServer){
        std::cout<<"Caught signal"<<signum<<",Shutting Down...\n";
        globalServer->shutdown();

    }
    exit(signum);
}

void RedisServer::setupSignalHandler(){
    signal(SIGINT,signalHandler);
}
RedisServer::RedisServer(int port) :port(port),server_socket(-1) ,running(true){
    globalServer=this; 
    setupSignalHandler();
}

void RedisServer::shutdown(){
    running=false;

    if(server_socket!=-1){
         // Before shutdown, persist the database
        if (RedisDatabase::getInstance().dump("dump.my_rdb"))
            std::cout << "Database Dumped to dump.my_rdb\n";
        else 
            std::cerr << "Error dumping database\n";
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
    if(bind(server_socket,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0){
        std::cerr<<"Error Binding Socket\n";
        return ;
    }   
    //10 incoming connections can queue up before os will reject new cinections
    if(listen(server_socket,10)<0){
        std::cerr<<"Error Listening On Server Socket\n";
        return ;
    }
    //server is ready to accept clients
    std::cout<<"Redis Server Litening On port :" <<port<< "\n";

    std::vector<std::thread>threads;
    RedisCommandHandler cmdHandler;
    while(running){
        int client_socket =accept(server_socket,nullptr,nullptr);
        if(client_socket<0){
            if(running){
                std::cerr<< "Error Accepting Client Connection\n";
            }
            break;
        }
        threads.emplace_back([client_socket,&cmdHandler](){
            char buffer[1024];
            while(true){
                memset(buffer,0,sizeof(buffer));
                int bytes=recv(client_socket,buffer,sizeof(buffer)-1,0);
                if(bytes<=0)break;
                std::string request(buffer,bytes);
                std::string response=cmdHandler.processCommand(request);
                send(client_socket,response.c_str(),response.size(),0);
            }
            close(client_socket);
        });
        
    }
    for(auto& t:threads){
        if(t.joinable())t.join();

    }
    // //before shutting down.persist the db.
    // if(RedisDatabase::getInstance().dump("dump.my_rdb")){
    //     std::cout <<"Database Dumped to dump.my_rdb\n";
    // }else{
    //   std::cerr <<"Error Dumping Database\n";
    // }
}
