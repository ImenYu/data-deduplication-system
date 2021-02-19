#ifndef __CUCKOO_HASH_H
#define __CUCKOO_HASH_H

#include "../utility.h"
#include <stdlib.h>
#include <math.h>

const int kTagValidBits=15;
const int kTagLen=2;
const int kBucketSize=4;
const int kKickedOutCountLimit=128;


struct Tag
{
    byte bytes[kTagLen];
    int offset;

    Tag()
    {
        offset=-1;
        memset(bytes,0,sizeof(bytes));
    }

    Tag(const Tag &other)
    {
        for(int i=0;i<kTagLen;i++)
            bytes[i]=other.bytes[i];
        offset=other.offset;

    }

    bool is_valid()
    {
        return (bytes[0]&0x80)==0x80;
    }

    bool operator==(const Tag &other) const
    {
        for(int i=0;i<kTagLen;i++)
        {
            if(bytes[i]!=other.bytes[i])
                return false;
        }
        return true;
    }
};

struct Bucket
{
    Tag entries[kBucketSize];
    // it will not actually insert the tag. Instead, it will return the tag to replace

    int lookup(const Tag &tag)
    {
        for(int i=0;i<kBucketSize;i++)
        {
            if(entries[i]==tag)
                return entries[i].offset;
        }
        return -1;
    }


    void insert_back(const Tag &tag,int bucket_pos)
    {
        entries[bucket_pos]=tag;
    }

    Tag insert_set_bucket_pos(const Tag &tag,int &bucket_pos)
    {
        for(int i=0;i<kBucketSize;i++)
        {
            if(!entries[i].is_valid())
            {
                entries[i]=tag;
                bucket_pos=i;
                Tag invalid_tag;
                return invalid_tag;
            }
        }
        bucket_pos=rand()%kBucketSize;
        Tag kicked_out_tag=entries[bucket_pos];
        entries[bucket_pos]=tag;
        return kicked_out_tag;
    }
};

class CuckooHash
{
private:
    Bucket *tables_[2];
    int count_;

public:
    int convert_tag_to_position(const Tag &tag);
    Tag convert_bytes_to_tag(byte *buffer,int start);
    Tag convert_pos_to_tag(int pos,int tag);
    bool insert(Tag tag,int table_pos,int table_index,int kicked_out_count);
public:
    CuckooHash();
    ~CuckooHash();
    int lookup_in_the_first_table(const ChunkHash &chunk_hash);
    int lookup_in_the_second_table(const ChunkHash &chunk_hash);
    bool insert(const ChunkHash &chunk_hash,int offset);
    double fullness_ratio();
};
#endif