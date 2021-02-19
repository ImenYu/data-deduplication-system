#ifndef __STORAGE_MANAGER_H
#define __STORAGE_MANAGER_H

#include "utility.h"
#include <math.h>
#include <dirent.h>
#include "container_manager.h"
#include "chunk_hash_manager.h"
#include "key_value_storage/key_value_storager.h"

using std::min;

// responsible for managing working containers, it interacts with container manager to retrieve/store containers and add/delete chunks
class StorageManager
{
    static int s_container_count_;
private:
    Container *working_container_;
    string file_restore_dir_;
    ContainerManager container_manager_;
    // ChunkHashManager chunk_hash_manager_;
    KeyValueStorager key_value_storager_;

    
private:
    void create_new_working_container();
    void check_make_dir(FileRecipe &file_recipe);
public:
    StorageManager(string container_save_dir,string file_restore_dir,string key_value_save_dir);
    ~StorageManager();
    void add_chunk_set_its_container_id(Chunk &chunk);
    bool restore_file(FileRecipe &file_recipe);
    bool restore_file_with_faa(FileRecipe &file_recipe);
    void delete_file_from_storage(FileRecipe &file_recipe);
};

#endif