#ifndef __CONTAINER_MANAGER_H
#define __CONTAINER_MANAGER_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <map>
#include <set>
#include <list>
#include "utility.h"

using std::set;
using std::map;
using std::pair;
using std::list;
/* 
responsible for saving/reading container and maintains a container cache in memory
*/
class ContainerManager
{
private:
    string container_save_dir_;

    typedef list<Container *>::iterator listiterator;
    list<Container *> lru_cache_;
    map<int,listiterator> cid_cpos_; // container id to container pos in the list
    const int container_cache_capacity_;
private:
    bool is_cache_full();
    void save_container_to_disk(const string &container_save_dir,const Container *c);
    Container *read_container_from_disk(const string &container_save_dir,int container_id);
public:
    ContainerManager(string container_save_dir,int container_cache_capacity);
    ~ContainerManager();
    bool container_cache_contains(const ChunkHash &chunk_hash);
    Container* retrieve_container_from_cache(const ChunkHash &chunkhash);
    void save_delete_container(const Container *container);
    Container* retrieve_container(int container_id);
};

#endif