#ifndef __UTILITY_H
#define __UTILITY_H

#include <gcrypt.h>
#include <vector>
#include <cstdio>
#include <string>
#include <set>

//#define DEBUG_CONTAINER_MANAGER

using std::string;
using std::to_string;
using std::vector;
using std::set;

typedef unsigned char byte;
typedef unsigned int uint4;
typedef unsigned long long int ullint;


const int kChunkHashAlgorithm=GCRY_MD_SHA1;
const int kChunkHashLen=20; // change with hash algorithm
const int kFileIdentifierHashAlgorithm=GCRY_MD_SHA1;
const int kFileIdentiferLen=20;
const ullint kMinChunkSize=4096;
const ullint kMaxChunkSize=8192;
const int kRabinWindowSize=8;
const ullint R=256;
const ullint Q=11497;
const ullint kPattern=Q-1;
const int kContainerCapacity=1024; // num of chunks each container can store
const int kContainerCacheCapacity=10;
const int kFAASize=1024; // set forward assembly area to the size of a container

struct ChunkHash
{
    byte chunk_hash_value[kChunkHashLen];
    bool operator<(const ChunkHash &other) const
    {
        int i=0;
        while (i<kChunkHashLen &&
            chunk_hash_value[i]==other.chunk_hash_value[i] )
        {
            i++;
        }
        
        if(i==kChunkHashLen)
            return false;
        return chunk_hash_value[i]<other.chunk_hash_value[i];
    }
    void print() const
    {
        for(int i=0;i<kChunkHashLen;i++)
        {
            std::printf("%02x",chunk_hash_value[i]);
        }
        std::printf("\n");
    }

    bool operator==(const ChunkHash &other) const
    {
        for(int i=0;i<kChunkHashLen;i++)
        {
            if(chunk_hash_value[i]!=other.chunk_hash_value[i])
                return false;
        }
        return true;
    }
};

struct ChunkData
{
    byte data[kMaxChunkSize];
    int data_len;
};

struct Chunk
{   
    struct ChunkHash chunk_hash;
    struct ChunkData chunk_data;
    int container_id;
    int start;
    int finish;
};

struct ChunkInfo
{
    struct ChunkHash chunk_hash;
    int container_id;
    int start;
    int finish;
    int data_len;

    ChunkInfo()
    {

    }

    ChunkInfo(const ChunkInfo &other)
    {
        chunk_hash=other.chunk_hash;
        container_id=other.container_id;
        start=other.start;
        finish=other.finish;
        data_len=other.data_len;
    }

    ChunkInfo(Chunk &chunk)
    {
        chunk_hash=chunk.chunk_hash;
        container_id=chunk.container_id;
        start=chunk.start;
        finish=chunk.finish;
        data_len=chunk.chunk_data.data_len;
    }

    void print() const
    {
        chunk_hash.print();
        printf("container id: %d; start %d; finish %d; data len %d\n",container_id,start,finish,data_len);
    }
};

struct FileRecipe
{
    byte file_identifier[kFileIdentiferLen];
    string relative_dir;
    string file_name;
    vector<ChunkInfo> chunk_infos;

    FileRecipe(string relative_dir,string file_name):
        relative_dir(relative_dir),
        file_name(file_name)
    {
        cal_file_identifier();
    }

    void cal_file_identifier()
    {
        string relative_file_path(relative_dir+file_name);
        gcry_md_hash_buffer(kFileIdentifierHashAlgorithm,
            file_identifier,
            relative_file_path.c_str(),
            relative_file_path.size());
    }
    void add_chunk(Chunk &chunk)
    {
        ChunkInfo chunk_info(chunk);
        chunk_infos.push_back(chunk_info);
    }
    bool operator<(const FileRecipe &other) const
    {
        int i=0;
        while (i<kFileIdentiferLen &&
            file_identifier[i]==other.file_identifier[i] )
        {
            i++;
        }
        
        if(i==kFileIdentiferLen)
            return false;
        return file_identifier[i]<other.file_identifier[i];
    }

    void print() const
    {
        string relative_file_path(relative_dir+file_name);
        printf("file identifier: ");
        for(int i=0;i<kFileIdentiferLen;i++)
        {
            std::printf("%02x",file_identifier[i]);
        }
        std::printf("\n");
        printf("relative restore file path: %s\n",relative_file_path.c_str());
        for(int i=0;i<chunk_infos.size();i++)
        {
            chunk_infos[i].print();
        }
    }
};

struct ContainerChunkMetadata
{
    ChunkHash chunk_hash;
    mutable int reference_count;
    int container_data_offset;
    int data_len;

    ContainerChunkMetadata()
    {

    }

    ContainerChunkMetadata(const ChunkHash &chunk_hash):
        chunk_hash(chunk_hash)
    {

    }

    // ContainerChunkMetadata(const ContainerChunkMetadata &other):
    //     chunk_hash(other.chunk_hash),
    //     reference_count(other.reference_count),
    //     container_data_offset(other.container_data_offset),
    //     data_len(other.data_len)
    // {

    // }

    ContainerChunkMetadata(const Chunk &chunk,int container_data_offset):
        chunk_hash(chunk.chunk_hash),
        reference_count(1),
        container_data_offset(container_data_offset),
        data_len(chunk.chunk_data.data_len)
    {
    }

    bool operator<(const ContainerChunkMetadata &other) const
    {
        return chunk_hash<other.chunk_hash;
    }

    bool operator==(const ContainerChunkMetadata &other) const
    {
        return data_len==other.data_len &&
            reference_count==other.reference_count &&
            container_data_offset== other.container_data_offset &&
            chunk_hash==other.chunk_hash;
    }
    
    void print() const
    {
        chunk_hash.print();
        printf("reference count %d; offset: %d; data len: %d\n",
            reference_count,container_data_offset,data_len);
    }
};

struct Container
{
    int container_id;
    int valid_chunks_count;
    set<ContainerChunkMetadata> container_chunk_metadata_set;
    vector<byte> container_data;
    const int capacity;
    Container():
        container_id(-1),
        capacity(kContainerCapacity),
        valid_chunks_count(0)
    {
    }
    Container(int container_id):
        container_id(container_id),
        capacity(kContainerCapacity),
        valid_chunks_count(0)
    {
    }

    Container(const Container &other):
        container_id(other.container_id),
        valid_chunks_count(other.valid_chunks_count),
        container_chunk_metadata_set(other.container_chunk_metadata_set),
        container_data(other.container_data),
        capacity(other.capacity)
    {
    }

    bool is_empty()
    {
        return valid_chunks_count==0;
    }

    bool is_full()
    {
        return valid_chunks_count>=capacity;
    }
    
    set<ContainerChunkMetadata>::iterator begin() const
    {
        return container_chunk_metadata_set.begin();
    }

    set<ContainerChunkMetadata>::iterator end() const
    {
        return container_chunk_metadata_set.end();
    }

    set<ContainerChunkMetadata>::iterator find(const ChunkHash &chunk_hash)
    {
        ContainerChunkMetadata c_c_meta_data(chunk_hash);
        set<ContainerChunkMetadata>::iterator it=container_chunk_metadata_set.find(c_c_meta_data);
        if(it!=container_chunk_metadata_set.end()&&it->reference_count>0)
        {
            return it;
        }
        return container_chunk_metadata_set.end();
    }

    bool contains(const ChunkHash &chunk_hash)
    {
        if(find(chunk_hash)==end())
        {
            return false;
        }
        return true;
    }

    // no duplicate data exists within a container
    void add_chunk_to_container(Chunk &chunk)
    {
        ContainerChunkMetadata cc_metadata(chunk.chunk_hash);
        cc_metadata.data_len=chunk.chunk_data.data_len;
        cc_metadata.reference_count=1;

        set<ContainerChunkMetadata>::iterator it=container_chunk_metadata_set.find(cc_metadata);
        if(it==container_chunk_metadata_set.end()) // new chunk to this container, try to add it to this container
        {
            if(is_full())
                throw "try to add new chunk to a full container\n";
            if(container_chunk_metadata_set.size()>=capacity)
            {
                purge();
            }
            cc_metadata.container_data_offset=container_data.size();

            container_chunk_metadata_set.insert(cc_metadata);
            valid_chunks_count++;
            container_data.insert(container_data.end(),
                chunk.chunk_data.data,
                chunk.chunk_data.data+chunk.chunk_data.data_len);
        }
        else // old chunk, update container chunk metadata
        {
            if(it->reference_count==0)
                valid_chunks_count++;
            it->reference_count++;
        }
    }
    
    // just decrease the corresponding chunk's reference count, lazy delete
    void delete_chunk(ChunkInfo &chunk_info)
    {
        if(container_id!=chunk_info.container_id)
            return;

        ContainerChunkMetadata cc_metadata(chunk_info.chunk_hash);
        set<ContainerChunkMetadata>::iterator it=container_chunk_metadata_set.find(cc_metadata);
        if(it==container_chunk_metadata_set.end())
            return;
        if(it->reference_count>0)
        {
            it->reference_count--;
            if(it->reference_count==0)
            {
                valid_chunks_count--;
            }
        }
    }

    // remove invalid container chunk metadatas, and their coresponding container data
    void purge()
    {
        set<ContainerChunkMetadata> new_container_chunk_metadata_set;
        vector<byte> new_container_data;
        int data_moved=0;
        for(set<ContainerChunkMetadata>::iterator it=begin();it!=end();it++)
        {
            if(it->reference_count!=0)
            {
                // set and insert new_cc_metadata
                ContainerChunkMetadata new_cc_metadata=*it;
                new_cc_metadata.container_data_offset=data_moved;
                new_container_chunk_metadata_set.insert(new_cc_metadata);

                // move data
                new_container_data.insert(new_container_data.end(),
                    container_data[it->container_data_offset],
                    container_data[it->container_data_offset+it->data_len]);
                
                data_moved+=it->data_len;
            }
        }
        container_chunk_metadata_set=new_container_chunk_metadata_set;
        container_data=new_container_data;
    }

    void print()
    {
        set<ContainerChunkMetadata>::iterator it=begin();
        for(;it!=end();it++)
        {
            it->print();
        }
        printf("%ld bytes stored in container %d\n",container_data.size(),container_id);
    }

    bool operator==(const Container &other) const
    {
        return container_id==other.container_id &&
            valid_chunks_count==other.valid_chunks_count &&
            capacity==other.capacity &&
            container_chunk_metadata_set==other.container_chunk_metadata_set &&
            container_data==other.container_data;
    }
};
#endif