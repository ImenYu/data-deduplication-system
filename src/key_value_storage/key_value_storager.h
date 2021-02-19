#ifndef __KEY_VALUE_STORAGER_H
#define __KEY_VALUE_STORAGER_H

#include "log_store.h"
#include "../utility.h"
#include <vector>
#include <string>

using std::vector;
using std::string;

class KeyValueStorager
{
private:
    vector<LogStore *> log_stores_;
    LogStore* working_log_store_;
    string key_value_save_dir_;
public:
    KeyValueStorager(const string &key_value_save_dir_);
    ~KeyValueStorager();
    int lookup(const ChunkHash &chunk_hash);
    void insert(const Container *container);
    
};
#endif