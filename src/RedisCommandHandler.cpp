#include "../include/RedisCommandHandler.h"
#include <vector>
#include <sstream>
#include <algorithm>
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
        if(crlf== std::string::npos)return break;
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
    //use Resp parser
    auto& tokens =ParseRespCommand(commandLine);
    if(tokens.empty())return "-Error: Empty command\r\n";
    std::string cmd=tokens[0];
    std::transform(cmd.begin(),cmd.end(),cmd.begin(),::toupper);
    std::ostringstream response;

    //connext to DB

    //check commands


    return response.str();
}