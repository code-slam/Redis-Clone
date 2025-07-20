#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
//PARSE TO RESP
/*
simple strings :   +OK\r\n
simple errors  :   -ERR message\r\n
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

//----------------------
// Common Commands
//----------------------
static std::string handlePing(const std::vector<std::string>& /*tokens*/, RedisDatabase& /*db*/) {
    return "+PONG\r\n";
}

static std::string handleEcho(const std::vector<std::string>& tokens, RedisDatabase& /*db*/) {
    if (tokens.size() < 2)
        return "-ERR: ECHO requires a message\r\n";
    return "+" + tokens[1] + "\r\n";
}

static std::string handleFlushAll(const std::vector<std::string>& /*tokens*/, RedisDatabase& db) {
    db.flushAll();
    return "+OK\r\n";
}

//----------------------
// Key/Value Operations
//----------------------
static std::string handleSet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-ERR: SET requires key and value\r\n";
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleGet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-ERR: GET requires key\r\n";
    std::string value;
    if (db.get(tokens[1], value))
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    return "$-1\r\n";
}

static std::string handleKeys(const std::vector<std::string>& /*tokens*/, RedisDatabase& db) {
    auto allKeys = db.keys();
    std::ostringstream oss;
    oss << "*" << allKeys.size() << "\r\n";
    for (const auto& key : allKeys)
        oss << "$" << key.size() << "\r\n" << key << "\r\n";
    return oss.str();
}

static std::string handleType(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-ERR: TYPE requires key\r\n";
    return "+" + db.type(tokens[1]) + "\r\n";
}

static std::string handleDel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-ERR: DEL requires key\r\n";
    bool res = db.del(tokens[1]);
    return ":" + std::to_string(res ? 1 : 0) + "\r\n";
}

static std::string handleExpire(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-ERR: EXPIRE requires key and time in seconds\r\n";
    try {
        int seconds = std::stoi(tokens[2]);
        if (db.expire(tokens[1], seconds))
            return "+OK\r\n";
        else
            return "-ERR: Key not found\r\n";
    } catch (const std::exception&) {
        return "-ERR: Invalid expiration time\r\n";
    }
}

static std::string handleRename(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-ERR: RENAME requires old key and new key\r\n";
    if (db.rename(tokens[1], tokens[2]))
        return "+OK\r\n";
    return "-ERR: Key not found or rename failed\r\n";
}
//-------------------------
//LIST COMMANDS
//-------------------------
static std::string handleLlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<2)
        return "-ERR: LLEN requires a key\r\n";
    ssize_t len=db.llen(tokens[1]);
    return ":"+ std::to_string(len)+"\r\n";
}
static std::string handleLpush(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<3)
        return "-ERR: LPUSH requires a key and value\r\n";
    db.lpush(tokens[1],tokens[2]);
    ssize_t len=db.llen(tokens[1]);
    return ":"+ std::to_string(len)+"\r\n";
}
static std::string handleRpush(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<3)
        return "-ERR: RPUSH requires a key and a value\r\n";
    db.rpush(tokens[1],tokens[2]);
    ssize_t len=db.llen(tokens[1]);
    return ":"+ std::to_string(len)+"\r\n";
}
static std::string handleLpop(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<2)
        return "-ERR: LPOP requires a key\r\n";
    std::string val;
    if(db.lpop(tokens[1],val))
        return "$" +std::to_string(val.size())+"\r\n"+val +"\r\n";
    return "$-1\r\n";
}
static std::string handleRpop(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<2)
        return "-ERR: Rpop requires a key\r\n";
     std::string val;
    if(db.lpop(tokens[1],val))
        return "$" +std::to_string(val.size())+"\r\n"+val +"\r\n";
    return "$-1\r\n";
}
static std::string handleLrem(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<4)
        return "-ERR: LREM requires a key,count and value\r\n";
    try{
        int count= std::stoi(tokens[2]);
        int removed =db.lrem(tokens[1],count,tokens[3]);
        return ":" +std::to_string(removed)+"\r\n";

    }
    catch(const std::exception&){
        return "-ERR:Invalid Count\r\n";
    }
}
static std::string handleLindex(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<3)
        return "-ERR: LINDEX requires a key and index\r\n";
    try{
        int index= std::stoi(tokens[2]);
        std::string value;
        if(db.lindex(tokens[1],index,value))
            return "$" +std::to_string(value.size())+"\r\n"+value +"\r\n";
         else 
            return "$-1\r\n";
    }
    catch(const std::exception&){
        return "-ERR:Invalid Index\r\n";
    }
}
static std::string handleLset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<4)
        return "-ERR: LSET requires a key,index,value\r\n";
    try{
        int index= std::stoi(tokens[2]);
        std::string value=tokens[3];
        if(db.lset(tokens[1],index,value))
            return "+OK\r\n";
        else 
            return "-ERR:Index out of Range\r\n";

    }
    catch(const std::exception&){
        return "-ERR:Invalid Index\r\n";
    }
}
//-----------------------------
//HASH COMMANDS
//------------------------------
static std::string handleHset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<4)
        return "-ERR: HSET requires a key ,field,value\r\n";
    db.hset(tokens[1],tokens[2],tokens[3]);
    return ":1\r\n"
}
static std::string handleHget(const std::vector<std::string>& tokens, RedisDatabase& db) {
        if(tokens.size()<3)
            return "-ERR: HGET requires a key and a field\r\n";
    std::string value;
    if(db.hget(tokens[1],tokens[2],value))
        return "$" + std::to_string(value.size())+"\r\n"+value +"\r\n";
    return "$-1\r\n";
}
static std::string handleHexists(const std::vector<std::string>& tokens, RedisDatabase& db) {
     if(tokens.size()<3)
            return "-ERR: HEXISTS requires a key and a field\r\n";
    bool exists =db.hexists(tokens[1],tokens[2]);
    return ":"+std::to_string(exists?1:0)+"\r\n";

}
static std::string handleHdel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<3)
            return "-ERR: HDEL requires a key and a field\r\n";
    bool res=db.hdel(tokens[1],tokens[2]);
    return ":"+std::to_string(res?1:0)+"\r\n";
}
static std::string handleHgetall(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<2)
            return "-ERR: HGETALL requires a key\r\n";
}
static std::string handleHkeys(const std::vector<std::string>& tokens, RedisDatabase& db) {
 if(tokens.size()<2)
            return "-ERR: HKEYS requires a key\r\n";
}
static std::string handleHvals(const std::vector<std::string>& tokens, RedisDatabase& db) {
  if(tokens.size()<2)
            return "-ERR: HVALS requires a key\r\n";
}
static std::string handleHlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
     if(tokens.size()<2)
            return "-ERR: HLEN requires a key\r\n";

}
static std::string handleHmset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if(tokens.size()<4 || (tokens.size()%2)==1){
            return "-ERR: HMSET requires a key followed by key value pairs\r\n";

    }
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
   // Common Commands
    if (cmd == "PING")
        return handlePing(tokens, db);
    else if (cmd == "ECHO")
        return handleEcho(tokens, db);
    else if (cmd == "FLUSHALL")
        return handleFlushAll(tokens, db);
    // Key/Value Operations
    else if (cmd == "SET")
        return handleSet(tokens, db);
    else if (cmd == "GET")
        return handleGet(tokens, db);
    else if (cmd == "KEYS")
        return handleKeys(tokens, db);
    else if (cmd == "TYPE")
        return handleType(tokens, db);
    else if (cmd == "DEL" || cmd == "UNLINK")
        return handleDel(tokens, db);
    else if (cmd == "EXPIRE")
        return handleExpire(tokens, db);
    else if (cmd == "RENAME")
        return handleRename(tokens, db);
    //list operations
    else if(cmd=="LLEN")
        return handleLlen(tokens,db);
    else if(cmd=="LPUSH")
        return handleLpush(tokens,db);
    else if(cmd=="RPUSH")
        return handleRpush(tokens,db);
    else if(cmd=="LPOP")
        return handleLpop(tokens,db);
    else if(cmd=="RPOP")
        return handleRpop(tokens,db);
    else if(cmd=="LREM")
        return handleLrem(tokens,db);
    else if(cmd=="LINDEX")
        return handleLindex(tokens,db);
    else if(cmd=="LSET")
        return handleLset(tokens,db);
    //hash operations
    else if(cmd=="HSET")
        return handleHset(tokens,db);
    else if(cmd=="HGET")
        return handleHget(tokens,db);
    else if(cmd=="HDEL")
        return handleHdel(tokens,db);
    else if(cmd=="HGETALL")
        return handleHgetall(tokens,db);
    else if(cmd=="HEXISTS")
        return handleHexists(tokens,db);
    else if(cmd=="HKEYS")
        return handleHkeys(tokens,db);
    else if(cmd=="HVALS")
        return handleHvals(tokens,db);
    else if(cmd=="HLEN")
        return handleHlen(tokens,db);
    else if(cmd=="HMSET")
        return handleHmset(tokens,db);
    else {
        return "-ERR unknown command " + cmd + "\r\n";
    }
    
   
}
