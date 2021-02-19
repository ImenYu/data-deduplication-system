#ifndef __CHUNK_HASH_MANAGER_H
#define __CHUNK_HASH_MANAGER_H
#include <map>
#include "utility.h"

using std::map;
using std::pair;

// it correlates all the on-disk chunks with their corresponding container ids
// But it does not contain the chunks in working_container_
// the chunks with reference counitng equal to 0 are also included due to lazy deletion
class ChunkHashManager
{
private:
    map<ChunkHash,int> chunk_hash_index_;
private:
    bool is_full() const;
    void insert(pair<ChunkHash,int> key_value);
    void remove(const ChunkHash &chunk_hash);
public:
    ChunkHashManager();
    ~ChunkHashManager();
    void insert(const Container *container);
    void remove(const Container *container);
    bool contains(const ChunkHash &chunk_hash) const;
    int get_container_id(const ChunkHash &chunk_hash);
};



#endif