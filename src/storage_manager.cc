#include "storage_manager.h"

int StorageManager::s_container_count_=0;

StorageManager::StorageManager(string container_save_dir,string file_restore_dir,string key_value_save_dir):
    file_restore_dir_(file_restore_dir),
    container_manager_(container_save_dir,kContainerCacheCapacity),
    key_value_storager_(key_value_save_dir)
{
    create_new_working_container();
}

StorageManager::~StorageManager()
{
    if(working_container_!=nullptr)
    {
        container_manager_.save_delete_container(working_container_);
    }
}

void StorageManager::add_chunk_set_its_container_id(Chunk &chunk)
{
    // check if this chunk exists in container cache
    Container *container=container_manager_.retrieve_container_from_cache(chunk.chunk_hash);
    if(container!=nullptr)
    {
        chunk.container_id=container->container_id;
        container->add_chunk_to_container(chunk);
        return;
    }
    
    // chunk if this chunk exists in working container
    if(working_container_->contains(chunk.chunk_hash))
    {
        chunk.container_id=working_container_->container_id;
        working_container_->add_chunk_to_container(chunk);
        return;
    }

    // check if this chunk exists in chunk hash manager
    int container_id=key_value_storager_.lookup(chunk.chunk_hash);
    if(container_id!=-1) // old chunk
    {
        Container *container=container_manager_.retrieve_container(container_id);
        if(container!=nullptr)
        {
            chunk.container_id=container->container_id;
            container->add_chunk_to_container(chunk);
            return;
        }
    }

    // new chunk, add it to memory
    if(working_container_->is_full())
    {
        // printf("inserting working container with container id %d\n",working_container_->container_id);
        key_value_storager_.insert(working_container_);
        container_manager_.save_delete_container(working_container_);
        create_new_working_container();
    }
    chunk.container_id=working_container_->container_id;
    working_container_->add_chunk_to_container(chunk);
}

void StorageManager::create_new_working_container()
{
    working_container_=new Container(s_container_count_);
    s_container_count_++;
}

void StorageManager::check_make_dir(FileRecipe &file_recipe)
{
    string &relative_dir=file_recipe.relative_dir;
    if(access((file_restore_dir_+relative_dir).c_str(),F_OK)==0)
        return;
    
    int dir_len=relative_dir.size();
    for(int i=0;i<dir_len;i++)
    {
        if(relative_dir[i]=='/')
        {
            string tmp_dir=file_restore_dir_+relative_dir.substr(0,i);
            if(access(tmp_dir.c_str(),F_OK)==0)
                continue;
            mkdir(tmp_dir.c_str(),0755);
        }
    }
}

// if relative file path does not exist, it will make directories recursively
bool StorageManager::restore_file(FileRecipe &file_recipe)
{
    check_make_dir(file_recipe);
    bool restore_result=true;
    string file_restore_path=file_restore_dir_+file_recipe.relative_dir+file_recipe.file_name;
    size_t fd=open(file_restore_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd==-1)
    {
        printf("file restore failed: failing to open file %s\n",file_restore_path.c_str());
        restore_result=false;
        return restore_result;
    }

    int chunk_count=file_recipe.chunk_infos.size();
    for(int i=0;i<chunk_count;i++)
    {
        ChunkInfo &chunk_info=file_recipe.chunk_infos[i];
        Container *possible_residing_container=nullptr;

        if(working_container_->container_id==chunk_info.container_id) // check container id
        {
            possible_residing_container=working_container_;
        }
        else
        {
            possible_residing_container=container_manager_.retrieve_container(chunk_info.container_id);
        }

        if(possible_residing_container!=nullptr)
        {
            set<ContainerChunkMetadata>::iterator it=possible_residing_container->find(chunk_info.chunk_hash);
            if(it==possible_residing_container->end())
            {
                printf("chunk residing in container %d not found in container %d\n",
                    chunk_info.container_id,possible_residing_container->container_id);
                restore_result=false;
                break;
            }
            
            vector<byte> &container_data=possible_residing_container->container_data;
            int start=chunk_info.start;
            int data_len=chunk_info.data_len;
            int container_data_offset=it->container_data_offset;


            if(lseek(fd,start,SEEK_SET)==-1) // reset write pointer
            {
                printf("failed to reset write pointer to %d\n",start);
                restore_result=false;
                break;
            }
            //printf("writing at position %d\n",start);
            if(write(fd,&container_data[container_data_offset],data_len)==-1)
            {
                printf("failed to write file at position %d\n",start);
                restore_result=false;
                break;
            }
        }
        else // no container with this container id
        {
            printf("the container %d does not exist either in memory or on disk\n",chunk_info.container_id);
            chunk_info.print();
            printf("-----------------------------------------\n");
            restore_result=false;
        }
    }

    close(fd);
    return restore_result;
}

bool StorageManager::restore_file_with_faa(FileRecipe &file_recipe)
{
    // try to open the corresponeding file
    check_make_dir(file_recipe);
    bool restore_result=true;
    string file_restore_path=file_restore_dir_+file_recipe.relative_dir+file_recipe.file_name;
    size_t fd=open(file_restore_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd==-1)
    {
        printf("file restore failed: failing to open file %s\n",file_restore_path.c_str());
        restore_result=false;
        return restore_result;
    }

    bool *is_restored=new bool[kFAASize];
    byte *buffer=new byte[kFAASize*kMaxChunkSize];

    int num_chunks=file_recipe.chunk_infos.size();
    for(int i=0;i<num_chunks;i+=kFAASize)
    {
        memset(is_restored,false,kFAASize);

        // [start_chunk_index,end_chunk_index)
        int start_chunk_index=i;
        int end_chunk_index=min(start_chunk_index+kFAASize,num_chunks);

        ChunkInfo &start_chunk_info=file_recipe.chunk_infos[start_chunk_index];
        ChunkInfo &end_chunk_info=file_recipe.chunk_infos[end_chunk_index-1];

        for(int j=start_chunk_index;j<end_chunk_index;j++)
        {
            if(is_restored[j-start_chunk_index])
                continue;

            Container *container;
            if(file_recipe.chunk_infos[j].container_id==working_container_->container_id)
            {
                container=working_container_;
                // printf("using working container with container id %d\n",container->container_id);
            }
            else
            {
                container=container_manager_.retrieve_container(file_recipe.chunk_infos[j].container_id);
                if(container==nullptr)
                {
                    restore_result=false;
                    break;
                }
                // printf("using retrieved container with container id %d\n",container->container_id);
            }
           

            for(int k=j;k<end_chunk_index;k++)
            {
                if(file_recipe.chunk_infos[k].container_id==container->container_id && !is_restored[k-start_chunk_index])
                {
                    set<ContainerChunkMetadata>::iterator it=container->find(file_recipe.chunk_infos[k].chunk_hash);
                    if(it==container->end())
                    {
                        restore_result=false;
                        break;
                    }
                    // copy the data from container to buffer
                    vector<byte> &container_data=container->container_data;
                    int buffer_start=file_recipe.chunk_infos[k].start-start_chunk_info.start;
                    int data_len=file_recipe.chunk_infos[k].data_len;
                    int container_data_start=it->container_data_offset;
                    memcpy(buffer+buffer_start,&container_data[container_data_start],data_len);
                    is_restored[k-start_chunk_index]=true;
                }
            }
            if(restore_result==false)
                break;
        }
        if(restore_result==true)
        {
            int start=start_chunk_info.start;
            if(lseek(fd,start,SEEK_SET)==-1) // reset write pointer
            {
                printf("failed to reset write pointer to %d\n",start);
                restore_result=false;
            }
            //printf("writing at position %d\n",start);
            int len_data_to_write=end_chunk_info.finish-start_chunk_info.start;
            if( write(fd,buffer,len_data_to_write)!=len_data_to_write)
            {
                printf("failed to write file at position %d\n",start);
                restore_result=false;
            }
        }

        if(restore_result==false)
            break;
    }

    delete is_restored;
    delete buffer;
    close(fd);
    return restore_result;
}

void StorageManager::delete_file_from_storage(FileRecipe &file_recipe)
{
    int chunk_count=file_recipe.chunk_infos.size();
    for(int i=0;i<chunk_count;i++)
    {
        ChunkInfo &chunk_info=file_recipe.chunk_infos[i];
        if(working_container_->container_id==chunk_info.container_id)
        {
            working_container_->delete_chunk(chunk_info);
        }
        else
        {
            Container *possbile_residing_container=container_manager_.retrieve_container(chunk_info.container_id);
            if(possbile_residing_container!=nullptr)
                possbile_residing_container->delete_chunk(chunk_info);
        }
    }
}