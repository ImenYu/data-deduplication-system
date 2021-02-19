#ifndef __LOG_STORE_H
#define __LOG_STORE_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../utility.h"
#include "cuckoo_hash.h"

using std::pair;


class LogStore
{
    typedef pair<ChunkHash,int> key_value_type;
    static int s_log_store_count_;
private:
    int log_store_id_;
    string key_value_save_dir_;
    CuckooHash cuckoo_hash_;
    vector<key_value_type> key_value_store_;
    key_value_type read_from_disk(int offset);
    int lookup_in_disk(const ChunkHash &chunk_hash);
    int lookup_in_memory(const ChunkHash &chunk_hash);
public:
    LogStore(string key_value_save_dir);
    ~LogStore();
    bool save_key_value_pairs_to_disk();
    bool insert(const ChunkHash &chunk_hash,int container_id);
    int lookup(const ChunkHash &chunk_hash);
    double fullness_ratio();
};
#endif