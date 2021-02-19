#include "log_store.h"

int LogStore::s_log_store_count_=0;

LogStore::LogStore(string key_value_save_dir):
    log_store_id_(s_log_store_count_++),
    key_value_save_dir_(key_value_save_dir)
{
}

LogStore::~LogStore()
{
    save_key_value_pairs_to_disk();
}

bool LogStore::save_key_value_pairs_to_disk()
{
    string save_path=key_value_save_dir_+to_string(log_store_id_);
    size_t fd=open(save_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd==-1)
    {
        printf("opening file for log store %d failed when writting\n",log_store_id_);
        return false;
    }
    if(write(fd,&*key_value_store_.begin(),
        sizeof(key_value_type)*key_value_store_.size())==-1)
    {
        printf("failed to write key value pairs to disk with log store id %d\n",log_store_id_);
        return false;
    }
    vector<key_value_type>().swap(key_value_store_); // empty key_value_store_
    close(fd);
    return true;
}

bool LogStore::insert(const ChunkHash &chunk_hash,int container_id)
{
    int offset=key_value_store_.size();
    if(cuckoo_hash_.insert(chunk_hash,offset))
    {
        key_value_store_.push_back({chunk_hash,container_id});
        return true;
    }
    else
    {
        return false;
    }
}

LogStore::key_value_type LogStore::read_from_disk(int offset)
{
    string save_path=key_value_save_dir_+to_string(log_store_id_);
    size_t fd=open(save_path.c_str(),O_RDONLY,S_IRUSR|S_IWUSR);
    key_value_type result={ChunkHash(),-1};
    if(fd==-1)
    {
        printf("opening log store %d failed when reading\n",log_store_id_);
        return result;
    }
    if(lseek(fd,offset*sizeof(key_value_type),SEEK_SET)==-1)
    {
        printf("seeking position failed when read log store %d",log_store_id_);
        return result;
    }
    if(read(fd,&result,sizeof(key_value_type))==-1)
    {
        printf("reading key value pair failed when reading log store %d\n",log_store_id_);
    }
    close(fd);
    return result;
    
}

int LogStore::lookup_in_memory(const ChunkHash &chunk_hash)
{
    int offset=cuckoo_hash_.lookup_in_the_first_table(chunk_hash);
    if(offset!=-1)
    {
        ChunkHash chunk_hash_found=key_value_store_[offset].first;
        int container_id=key_value_store_[offset].second;
        if(chunk_hash_found==chunk_hash)
            return container_id;
    }

    offset=cuckoo_hash_.lookup_in_the_second_table(chunk_hash);
    if(offset!=-1)
    {
        ChunkHash chunk_hash_found=key_value_store_[offset].first;
        int container_id=key_value_store_[offset].second;
        if(chunk_hash_found==chunk_hash)
            return container_id;
    }
    return -1;
}

int LogStore::lookup_in_disk(const ChunkHash &chunk_hash)
{
    int offset=cuckoo_hash_.lookup_in_the_first_table(chunk_hash);
    if(offset!=-1)
    {
        key_value_type result=read_from_disk(offset);
        if(chunk_hash==result.first)
            return result.second;
    }

    offset=cuckoo_hash_.lookup_in_the_second_table(chunk_hash);
    if(offset!=-1)
    {
        key_value_type result=read_from_disk(offset);
        if(chunk_hash==result.first)
            return result.second;
    }
    return -1;
}

int LogStore::lookup(const ChunkHash &chunk_hash)
{
    if(!key_value_store_.empty())
        return lookup_in_memory(chunk_hash);
    else
        return lookup_in_disk(chunk_hash);
}

double LogStore::fullness_ratio()
{
    return cuckoo_hash_.fullness_ratio();
}
