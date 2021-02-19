#include "chunk_hash_manager.h"

ChunkHashManager::ChunkHashManager()
{
}

ChunkHashManager::~ChunkHashManager()
{
}

void ChunkHashManager::insert(pair<ChunkHash,int> key_value)
{
    chunk_hash_index_.insert(key_value);
}


void ChunkHashManager::insert(const Container *container)
{
    const int &container_id=container->container_id;
    const set<ContainerChunkMetadata> &container_chunk_metadata_set= container->container_chunk_metadata_set;
    set<ContainerChunkMetadata>::iterator it=container_chunk_metadata_set.begin();
    set<ContainerChunkMetadata>::iterator end=container_chunk_metadata_set.end();
    for(;it!=end;it++)
    {
        const ChunkHash &chunk_hash=it->chunk_hash;
        chunk_hash_index_.insert({chunk_hash,container_id});
    }
}

void ChunkHashManager::remove(const ChunkHash &chunk_hash)
{
    map<ChunkHash,int>::iterator it=chunk_hash_index_.find(chunk_hash);
    if(it!=chunk_hash_index_.end())
    {
        chunk_hash_index_.erase(it);
    }
}

void ChunkHashManager::remove(const Container *container)
{
    const set<ContainerChunkMetadata> &container_chunk_metadata_set= container->container_chunk_metadata_set;
    set<ContainerChunkMetadata>::iterator it=container_chunk_metadata_set.begin();
    set<ContainerChunkMetadata>::iterator end=container_chunk_metadata_set.end();
    for(;it!=end;it++)
    {
        const ChunkHash &chunk_hash=it->chunk_hash;
        remove(chunk_hash);
    }
}

bool ChunkHashManager::contains(const ChunkHash &chunk_hash) const
{
    if(chunk_hash_index_.find(chunk_hash)==chunk_hash_index_.end())
    {
        return false;
    }
    return true;
}

int ChunkHashManager::get_container_id(const ChunkHash &chunk_hash)
{
    map<ChunkHash,int>::iterator it=chunk_hash_index_.find(chunk_hash);
    if(it!=chunk_hash_index_.end())
    {
        return (*it).second;
    }
    return -1;
    
}