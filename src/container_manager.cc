#include "container_manager.h"

ContainerManager::ContainerManager(string container_save_dir,int container_cache_capacity):
    container_save_dir_(container_save_dir),
    container_cache_capacity_(container_cache_capacity)
{
}

ContainerManager::~ContainerManager()
{
    listiterator it=lru_cache_.begin();
    for(;it!=lru_cache_.end();it++)
    {
        save_delete_container(*it);
    }
}

bool ContainerManager::is_cache_full()
{
    return lru_cache_.size()>=container_cache_capacity_;
}

bool ContainerManager::container_cache_contains(const ChunkHash &chunk_hash)
{
    listiterator it=lru_cache_.begin();
    for(;it!=lru_cache_.end();it++)
    {
        Container *container=*it;
        if(container->contains(chunk_hash))
            return true;
    }
    return false;
}

// it is not very efficient
Container* ContainerManager::retrieve_container_from_cache(const ChunkHash &chunk_hash)
{
    listiterator it=lru_cache_.begin();
    for(;it!=lru_cache_.end();it++)
    {
        Container *container=*it;
        if(container->contains(chunk_hash))
            return container;
    }
    return nullptr;
}

void ContainerManager::save_delete_container(const Container *container)
{
    save_container_to_disk(container_save_dir_,container);
    delete container;
}
// return a nullptr if the container does not exist either in cache or on disk
Container* ContainerManager::retrieve_container(int container_id)
{
    map<int,listiterator>::iterator map_it=cid_cpos_.find(container_id);
    if(map_it==cid_cpos_.end()) // not in this cache
    {
        Container* container=read_container_from_disk(container_save_dir_,container_id);
        if(container==nullptr)
        {
            printf("failed to retrieve container %d\n",container_id);
            return container;
        }
        else
        {
            if(is_cache_full()) // remove the last element, update the map
            {
                int removed_container_id=lru_cache_.back()->container_id;
                save_delete_container(lru_cache_.back());
                lru_cache_.pop_back();
                cid_cpos_.erase(removed_container_id);
            }
            lru_cache_.push_front(container);
            cid_cpos_[container->container_id]=lru_cache_.begin();
            return container;
        }
    }
    else // in this cache
    {
        listiterator list_it=map_it->second;
        Container *container=*list_it;
        lru_cache_.erase(list_it); // remove container from list
        lru_cache_.push_front(container); // add this container at the front of the list
        cid_cpos_[container->container_id]=lru_cache_.begin(); // update the map
        return container;
    }
}

void ContainerManager::save_container_to_disk(const string &container_save_dir,const Container *container)
{
    bool write_success=true;
    string data_path=container_save_dir+to_string(container->container_id);
    string metadata_path=data_path+".metadata";
    
    // save metadata
    size_t fd_metadata=open(metadata_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd_metadata==-1)
    {
        printf("failed to open %s with error: %s\n",metadata_path.c_str(),strerror(errno));
        return;
    }

    if(write(fd_metadata,&container->container_id,sizeof(container->container_id))==-1)
    {
        printf("error writting container id for container %d\n",container->container_id);
        write_success=false;
    }

    if(write(fd_metadata,&container->valid_chunks_count,sizeof(container->valid_chunks_count))==-1)
    {
        printf("error writting valid chunks count for container %d\n",container->container_id);
        write_success=false;
    }

    set<ContainerChunkMetadata>::iterator it=container->container_chunk_metadata_set.begin();
    set<ContainerChunkMetadata>::iterator end=container->container_chunk_metadata_set.end();
    for(;it!=end;it++)
    {
        int bytes_written=write(fd_metadata,&(*it),sizeof(ContainerChunkMetadata));
        if(bytes_written==-1)
        {
            printf("error writting %s with error: %s\n",metadata_path.c_str(),strerror(errno));
            write_success=false;
            break;
        }
    }
    close(fd_metadata);

    // save all the chunk data
    size_t fd_data=open(data_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd_data==-1)
    {
        printf("failed to open %s with error: %s\n",data_path.c_str(),strerror(errno));
        return;
    }
    int bytes_written=write(fd_data,&*container->container_data.begin(),
        container->container_data.size());
    if(bytes_written==-1)
    {
        printf("error writting %s with error: %s\n",data_path.c_str(),strerror(errno));
        write_success=false;
    }
    close(fd_data);
    if(!write_success)
    {
        remove(metadata_path.c_str());
        remove(data_path.c_str());
    }
}

Container* ContainerManager::read_container_from_disk(const string &container_save_dir,int container_id)
{
    bool read_success=true;
    Container *container=new Container();
    string data_path=container_save_dir+to_string(container_id);
    string metadata_path=data_path+".metadata";

    // read meta_data from disk
    size_t fd_metadata=open(metadata_path.c_str(),O_RDONLY,S_IRUSR|S_IWUSR);
    if(fd_metadata==-1)
    {
        printf("failed to open %s when reading a container with error %s\n",metadata_path.c_str(),strerror(errno));
        read_success=false;
    }

    if(read(fd_metadata,&container->container_id,sizeof(container->container_id))==-1)
    {
        printf("failed tor read container id for container %d\n",container_id);
        read_success=false;
    }

    if(read(fd_metadata,&container->valid_chunks_count,sizeof(container->valid_chunks_count))==-1)
    {
        printf("failed tor read valid chunks count for container %d\n",container_id);
        read_success=false;
    }

    ContainerChunkMetadata c_c_metadata;
    while(true)
    {
        int bytes_read=read(fd_metadata,&c_c_metadata,sizeof(ContainerChunkMetadata));
        if(bytes_read==0)
        {
            break;
        }
        if(bytes_read==-1 || bytes_read!=sizeof(ContainerChunkMetadata))
        {
            read_success=false;
            break;
        }
        container->container_chunk_metadata_set.insert(c_c_metadata);
    }
    close(fd_metadata);

    // read container data from disk
    size_t fd_data=open(data_path.c_str(),O_RDONLY,S_IRUSR|S_IWUSR);
    byte buffer[4096];
    vector<byte> &container_data=container->container_data;
    if(fd_data==-1)
    {
        printf("failed to open %s when reading a containe with error no %d\n",data_path.c_str(),errno);
        read_success=false;
    }
    while (true)
    {
        int bytes_read=read(fd_data,buffer,sizeof(buffer));
        if(bytes_read==0)
        {
            break;
        }
        if(bytes_read==-1)
        {
            read_success=false;
            break;
        }
        container_data.insert(container_data.end(),buffer,buffer+bytes_read);
    }
    close(fd_data);

    if(!read_success)
    {
        delete container;
        container=nullptr;
    }
    return container;
}