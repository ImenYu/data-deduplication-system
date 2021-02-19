#include "key_value_storager.h"

KeyValueStorager::KeyValueStorager(const string &key_value_save_dir):key_value_save_dir_(key_value_save_dir)
{
    working_log_store_=new LogStore(key_value_save_dir_);
}

KeyValueStorager::~KeyValueStorager()
{
    delete working_log_store_;
    for(int i=0;i<log_stores_.size();i++)
        delete log_stores_[i];
}

int KeyValueStorager::lookup(const ChunkHash &chunk_hash)
{
    int container_id=working_log_store_->lookup(chunk_hash);
    if(container_id!=-1)
        return container_id;
    for(int i=0;i<log_stores_.size();i++)
    {
        container_id=log_stores_[i]->lookup(chunk_hash);
        if(container_id!=-1)
            return container_id;
    }
    return -1;
}
void KeyValueStorager::insert(const Container *container)
{
    set<ContainerChunkMetadata>::iterator it=container->begin();
    for(;it!=container->end();it++)
    {
        if(!working_log_store_->insert(it->chunk_hash,container->container_id))
        {
            // printf("old working log store with fullness %.2f\n",working_log_store_->fullness_ratio());
            working_log_store_->save_key_value_pairs_to_disk();
            log_stores_.push_back(working_log_store_);
            working_log_store_=new LogStore(key_value_save_dir_);
            working_log_store_->insert(it->chunk_hash,container->container_id);
        }
    }
}