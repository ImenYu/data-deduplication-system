#include "file_manager.h"


FileManager::FileManager(string original_file_dir,StorageManager *container_manager,FileRecipeManager *file_recipe_manager):
    original_file_dir_(original_file_dir),
    storage_manager_(container_manager),
    file_recipe_manager_(file_recipe_manager)
{
    // compute RM
    RM=1;
    for(int i=1;i<kRabinWindowSize;i++)
    {
        RM=(R*RM)%Q;
    }
}

FileManager::~FileManager()
{

}

void FileManager::partition_file(const string &relative_dir,const string &file_name)
{
    string original_file_path=original_file_dir_+relative_dir+file_name;
    size_t fd=open(original_file_path.c_str(),O_RDONLY);
    if(fd==-1)
    {
        printf("failed to open file: %s\n",original_file_path.c_str());
        return;
    }
    
    FileRecipe file_recipe(relative_dir,file_name);
    byte buffer[kMaxChunkSize];
    int buffer_data_len=0;
    int bytes_read=0;
    int file_pointer=0;
    while (true)
    {
        bytes_read=read(fd,buffer+buffer_data_len,kMaxChunkSize-buffer_data_len);
        if(bytes_read==-1)// error reading file
        {
            printf("Error when reading file\n");
            break;
        }
        buffer_data_len+=bytes_read;
        
        if(bytes_read==0 && buffer_data_len<=kMinChunkSize) // no more data read and data left is smaller than kMinChunkSize
        {
            if(buffer_data_len==0)
                break;
            Chunk chunk;
            int break_point=buffer_data_len;
            set_chunk_reset_buffer(chunk,buffer,file_pointer,buffer_data_len,break_point);
            storage_manager_->add_chunk_set_its_container_id(chunk);
            file_recipe.add_chunk(chunk);
        }
        else if(buffer_data_len>kMinChunkSize)
        {
            Chunk chunk;
            int break_point;
            ullint rabin_karp_hash=cal_rabin_karp_hash(buffer);

            for(break_point=kMinChunkSize;break_point<buffer_data_len;break_point++)
            {
                if(rabin_karp_hash==kPattern)
                {
                    break;
                }
                rabin_karp_hash=(rabin_karp_hash+Q-RM*buffer[break_point-kMinChunkSize]%Q)%Q; // remove leading byte
                rabin_karp_hash=(rabin_karp_hash*R+buffer[break_point])%Q; // add trailing byte
            }
            set_chunk_reset_buffer(chunk,buffer,file_pointer,buffer_data_len,break_point);
            storage_manager_->add_chunk_set_its_container_id(chunk);
            file_recipe.add_chunk(chunk);
        }
    }
    close(fd);
    file_recipe_manager_->add_file_recipe(file_recipe);
}

void FileManager::restore_file(const string &relative_dir,const string &file_name)
{
    string relative_file_path=relative_dir+file_name;
    FileRecipe *file_recipe=nullptr;
    if(file_recipe_manager_->find_set_file_recipe(relative_dir,file_name,file_recipe))
    {
        printf("successfully retrieving file_recipe for file : %s\n",relative_file_path.c_str());
        if(storage_manager_->restore_file_with_faa(*file_recipe))
        {
            printf("file %s restored successfully\n",relative_file_path.c_str());
        }
        else
        {
            printf("failed to restore file %s\n",relative_file_path.c_str());
        }
    }
    else
    {
        printf("failed to retrieve file recipe for file: %s\n",relative_file_path.c_str());
    }
    if(file_recipe!=nullptr)
    {
        delete file_recipe;
    }    
}

void FileManager::delete_file(const string &relative_dir,const string &file_name)
{
    FileRecipe *file_recipe;
    if(file_recipe_manager_->find_set_file_recipe(relative_dir,file_name,file_recipe))
    {
        storage_manager_->delete_file_from_storage(*file_recipe);
        file_recipe_manager_->delete_file_recipe(relative_dir,file_name);
    }
}

void FileManager::set_chunk_reset_buffer(Chunk &chunk,byte *buffer,int &file_pointer,int &buffer_data_len,int breakpoint)
{
    // save data from buffer to chunk and calculate chunk_hash
    chunk.chunk_data.data_len=breakpoint;
    for(int i=0;i<breakpoint;i++)
    {
        chunk.chunk_data.data[i]=buffer[i];
    }
    cal_set_chunk_hash(chunk);

    // set chunk_info
    chunk.start=file_pointer;
    chunk.finish=file_pointer+chunk.chunk_data.data_len;
    file_pointer+=chunk.chunk_data.data_len;

    // reset buffer
    for(int i=breakpoint,j=0;i<buffer_data_len;i++,j++)
    {
        buffer[j]=buffer[i];
    }
    buffer_data_len-=breakpoint;
}

inline void FileManager::cal_set_chunk_hash(Chunk &chunk)
{
    gcry_md_hash_buffer(kChunkHashAlgorithm,chunk.chunk_hash.chunk_hash_value,chunk.chunk_data.data,chunk.chunk_data.data_len);
}

ullint FileManager::cal_rabin_karp_hash(byte *buffer)
{
    ullint rabin_karp_hash=0;
    for(int i=kMinChunkSize-kRabinWindowSize;i<kMinChunkSize;i++)
    {
        rabin_karp_hash=(R*rabin_karp_hash+buffer[i])%Q;
    }
    return rabin_karp_hash;
}
