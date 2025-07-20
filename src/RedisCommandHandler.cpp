#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
//PARSE TO RESP
/*
simple strings :   +OK\r\n
simple errors  :   -Error message\r\n
                   -ERR unknown command 'asdf'
integers       :   :[<+|->]<value>\r\n
Bulk strings   :   $<length>\r\n<data>\r\n
Arrays         :   *<number-of-elements>\r\n<element-1>...<element-n>
                   *0\r\n empty arr
                   *2\r\n$5\r\nhello\r\n$5\r\nworld\r\n array with "hello and "world"
                   *3\r\n:1\r\n:2\r\n:3\r\n  arraay of 3 ints
                   *5\r\n:1\r\n:2\r\n:3\r\n:4\r\n$5\r\nhello\r\n
nested array   :   *2\r\n
                        *3\r\n:1\r\n:2\r\n:3\r\n
                        *2\r\n+Hello\r\n-World\r\n
null array     :   *-1\r\n
null element in arr:*3\r\n$5\r\nhello\r\n$-1\r\n$5\r\nworld\r\n   ==["hello",nil,"world"]
null           :   -\r\n
bool           :   #<t|f>\r\n
doubles        :   ,[<+|->]<integral>[.<fractional>][<E|e>[sign]<exponent>]\r\n
big numbers    :   ([+|-]<number>\r\n
bulk error     :   !<length>\r\n<error>\r\n
verbatim string:   =<length>\r\n<encoding>:<data>\r\n
maps           :   %<number-of-entries>\r\n<key-1><value-1>...<key-n><value-n>
attributes     :   |1\r\n
                        +key-popularity\r\n
                        %2\r\n
                            $1\r\n
                             a\r\n
                            ,0.1923\r\n
                            $1\r\n
                            b\r\n
                            ,0.0012\r\n
                        *2\r\n
                            :2039123\r\n
                            :9543892\r\n
sets           :   ~<number-of-elements>\r\n<element-1>...<element-n>
pushes         :   ><number-of-elements>\r\n<element-1>...<element-n>
CLIENT HANDSHAKE: HELLO <protocol-version> [optional-arguments]
Sending Commands to a server:  C:*2\r\n$4\r\nLLEN\r\n$6\r\nmylist\r\n   S: S: :48293\r\n  (C:LLEN mylist  S: 48293)


*/

std::vector<std::string> ParseRespCommand(const std::string &input){
    std::vector<std::string>tokens;
    //if it doesnt start with '*' (does not look like its RSEP)just split by whitespace
    if(input.empty())return tokens;
    if(input[0] !='*'){
        std::stringstream iss(input);
        std::string token;
        while(iss>>token){
            tokens.push_back(token);
        }
        return tokens;
    }
    size_t pos =0;
    //expect '*' then a number 
    if(input[pos]!='*')return tokens;
    pos++;//skip *
    size_t crlf =input.find("\r\n",pos);
    if(crlf== std::string::npos)return tokens;

    int numElements = std::stoi(input.substr(pos,crlf-pos));
    pos= crlf+2;
    for(int i=0;i< numElements;i++){
        if(pos>=input.size() ||input[pos]!='$')break;
        pos++; //skip the $
        crlf=input.find("\r\n",pos);
        if(crlf== std::string::npos)break;
        int len= std::stoi(input.substr(pos,crlf-pos));
        pos=crlf+2;
        if(pos+len>=input.size())break;
        std::string token =input.substr(pos,len); 
        tokens.push_back(token);
        pos+=len+2;

    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler() {}
std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    RedisDatabase& db = RedisDatabase::getInstance();  // ✅ Add this line
    auto tokens = ParseRespCommand(commandLine);
    if(tokens.empty()) return "-ERR Empty command\r\n";
    std::cout <<commandLine <<"\n";
    for(auto& t:tokens){
    	std::cout<<t<< "\n";
    }
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    //common commands
    if(cmd == "PING") {
        response<< "+PONG\r\n";
    } else if(cmd=="ECHO"){
        if(tokens.size()<2){
            response<<"-ERR: ECHO requires a message\r\n";
        }else{
            response<< "+"<<tokens[1]<<"\r\n";
        }
    }else if(cmd=="FLUSHALL"){
        db.flushAll();
        response<<"+OK\r\n";
    }
    //key val operations
    else if(cmd == "SET") {
        if(tokens.size()<3){
            response<<"-ERR: SET requires key and Value\r\n";
        }else{
            db.set(tokens[1],tokens[2]);
            response<<"+OK\r\n";
        }
    } else if(cmd == "GET") {
        if(tokens.size()<2){
            response<<"-ERR: GET requires key\r\n";
        }else{
            std::string value;
            if(db.get(tokens[1],value))
                response<<"$"<<value.size()<<"\r\n"<<value<<"\r\n"; 
            else {
                response<< "$-1\r\n";
            } 
          } 
    }else if(cmd=="KEYS"){
        std::vector<std::string> allKeys = db.keys();
        response<<"*"<<allKeys.size()<<"\r\n";
        for(const auto& key:allKeys)
            response <<"$"<<key.size()<<"\r\n"<<key<< "\r\n";
    }else if(cmd=="TYPE"){
        if(tokens.size()<2){
            response<<"-ERR: TYPE requires key\r\n";
        }else{
            response<<"+"<<db.type(tokens[1])<<"\r\n";
        }
    }else if(cmd=="DEL" || cmd== "UNLINK"){
        if(tokens.size()<2)
                response<<"-ERR:"<<cmd<< "requires key\r\n";
        else{
            bool res= db.del(tokens[1]);
            response<<":"<<(res?1:0)<<"\r\n";
        }

    }else if(cmd=="EXPIRE"){
        if(tokens.size()<3){
            response<<"-ERR: EXPIRE requires Key and Time\r\n";
        }else{
            db.expire(tokens[1],stoi(tokens[2]));
            response <<"+OK\r\n";
        }
    }else if(cmd=="RENAME"){
        if(tokens.size()<3){
            response<<"-ERR: RENAME requires old Keyname and new Keyname\r\n";
        }else{
            db.rename(tokens[1],tokens[2]);
            response<<"+OK\r\n";
        }
    }
    //list operations
    //hash operations
    else {
        return "-ERR unknown command " + cmd + "\r\n";
    }
    return  response.str();
   
}
