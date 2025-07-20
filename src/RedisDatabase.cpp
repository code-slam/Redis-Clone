#include "../include/RedisDatabase.h"
#include <fstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}
//key/val oper
//list opers
//hash opers

/*
Memory->file -dump()
file->memory -load()
K= Key Vlaue
L=List
H=Hash
*/

bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename,std::ios::binary);
    return true;
}
bool RedisDatabase::load(const std::string& filename){
    return true;
}