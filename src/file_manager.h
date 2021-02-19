#ifndef __FILE_MANAGER_H
#define __FILE_MANAGER_H

#include <unistd.h>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utility.h"
#include "file_recipe_manager.h"
#include "storage_manager.h"

using std::string;


class FileManager
{
private:
    StorageManager *storage_manager_;
    FileRecipeManager *file_recipe_manager_;
    string original_file_dir_;
    ullint RM;
private:
    ullint cal_rabin_karp_hash(byte *buffer);
    inline void cal_set_chunk_hash(Chunk &chunk);
    void set_chunk_reset_buffer(Chunk &chunk,byte *buffer,int &file_pos,int &buffer_data_len,int breakpoint);
public:
    FileManager(string original_file_dir,StorageManager *container_manager_,FileRecipeManager *file_recipe_manager);
    ~FileManager();
    void partition_file(const string &relative_dir,const string &file_name);
    void restore_file(const string &relative_dir,const string &file_name);
    void delete_file(const string &relative_dir,const string &file_name);
};





#endif