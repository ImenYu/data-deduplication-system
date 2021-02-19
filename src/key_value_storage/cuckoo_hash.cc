#include "cuckoo_hash.h"

CuckooHash::CuckooHash():count_(0)
{
    int table_size=pow(2,kTagValidBits);
    tables_[0]=new Bucket[table_size];
    tables_[1]=new Bucket[table_size];
}

CuckooHash::~CuckooHash()
{
    delete tables_[0];
    delete tables_[1];
}

int CuckooHash::convert_tag_to_position(const Tag &tag)
{
    int pos=0;
    pos=(pos|tag.bytes[0])&0x7f;
    pos=(pos<<8)|tag.bytes[1];
    return pos;
}

Tag CuckooHash::convert_pos_to_tag(int pos,int offset)
{
    Tag tag;
    tag.bytes[1]=tag.bytes[1]|pos;
    tag.bytes[0]=(tag.bytes[0]|(pos>>8))|0x80;
    tag.offset=offset;
    return tag;
}

int CuckooHash::lookup_in_the_first_table(const ChunkHash &chunk_hash)
{
    Tag tag;
    tag.bytes[0]=chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen]|0x80;
    tag.bytes[1]=chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen+1];
    tag.offset=-1;

    int table_pos=0;
    table_pos=(table_pos|chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen])&0x7f;
    table_pos=(table_pos<<8)|chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen+1];

    return tables_[0][table_pos].lookup(tag);
}

int CuckooHash::lookup_in_the_second_table(const ChunkHash &chunk_hash)
{
    Tag tag;
    tag.bytes[0]=chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen]|0x80;
    tag.bytes[1]=chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen+1];
    tag.offset=-1;

    int table_pos=0;
    table_pos=(table_pos|chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen])&0x7f;
    table_pos=(table_pos<<8)|chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen+1];
    
    return tables_[1][table_pos].lookup(tag);
}

bool CuckooHash::insert(Tag tag,int table_pos,int table_index,int kicked_out_count)
{
    if(kicked_out_count>kKickedOutCountLimit)
        return false;

    int bucket_pos=-1;
    Tag kicked_out_tag=tables_[table_index][table_pos].insert_set_bucket_pos(tag,bucket_pos);
    if(kicked_out_tag.is_valid())
    {
        kicked_out_count++;
        int swaped_pos=convert_tag_to_position(kicked_out_tag);
        Tag swaped_tag=convert_pos_to_tag(table_pos,kicked_out_tag.offset);
        if(!insert(swaped_tag,swaped_pos,(table_index+1)%2,kicked_out_count))
        {
            // undo insert
            tables_[table_index][table_pos].insert_back(kicked_out_tag,bucket_pos);
            return false;
        }
    }
    return true;
}

bool CuckooHash::insert(const ChunkHash &chunk_hash,int offset)
{
    Tag tag;
    tag.bytes[0]=chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen]|0x80;
    tag.bytes[1]=chunk_hash.chunk_hash_value[kChunkHashLen-2*kTagLen+1];
    tag.offset=offset;
    
    int table_pos=0;
    table_pos=(table_pos|chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen])&0x7f;
    table_pos=(table_pos<<8)|chunk_hash.chunk_hash_value[kChunkHashLen-kTagLen+1];

    if(insert(tag,table_pos,0,0))
    {
        count_++;
        return true;
    }
    return false;
}

double CuckooHash::fullness_ratio()
{
    return double(count_)/double(2*kBucketSize*pow(2,kTagValidBits));
}