
#include "../include/RedisServer.h"
#include <iostream>
#include <thread>
#include  <chrono>

int main(int argc,char* argv[]){
    int port =6379;
    if(argc >=2){
        port=std::stoi(argv[1]);
    }
    RedisServer server(port);

    //Background persistance:dump the DB evry 300 secs
    std::thread persistancethread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            //dump the DB
        }
    });
    persistancethread.detach();
    server.run();
    return 0;
} 