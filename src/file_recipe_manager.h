#ifndef __FILE_RECIPE_MANAGER_H
#define __FILE_RECIPE_MANAGER_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <set>
#include <string>
#include "utility.h"

using std::string;
using std::set;

class FileRecipeManager
{
private:
    string file_recipe_save_dir_;
    set<FileRecipe> file_recipes_cache_;
    string convert_file_identifler_to_file_recipe_filename(const byte file_identifier[kFileIdentiferLen]);
    FileRecipe* read_file_recipe_from_disk(const string &relative_dir,const string &file_name);
    void save_file_recipe_to_disk(const FileRecipe &file_recipe);
public:
    FileRecipeManager(string file_recipe_save_dir);
    ~FileRecipeManager();
    void add_file_recipe(const FileRecipe &file_recipe);
    void delete_file_recipe(const string &relative_dir,const string &filename);
    void view_file_recipe(const FileRecipe &file_recipe);
    bool find_set_file_recipe(const string &relative_dir,const string &filename,FileRecipe *&file_recipe);
};

#endif