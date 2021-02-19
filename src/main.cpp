#include <cstdio>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <queue>
#include "file_manager.h"

using std::queue;

string origin_file_dir="../data/originalfiles/";
string container_save_dir="../data/containers/";
string file_recipe_save_dir="../data/filerecipe/";
string file_restore_dir="../data/restoredfiles/";
string key_value_save_dir="../data/keyvalues/";

void traverse_directory(string parent_directory,vector<pair<string,string>> &file_paths)
{
    queue<string> relative_dirs_q;
    
    relative_dirs_q.push("");
    while(!relative_dirs_q.empty())
    {
        DIR *dir;
        struct dirent *entry;
        string complete_dir=parent_directory+relative_dirs_q.front();
        string relative_dir=relative_dirs_q.front();
        relative_dirs_q.pop();

        if(!(dir=opendir(complete_dir.c_str())))
            return;
        
        while ((entry = readdir(dir)) != NULL) 
        {
            if(entry->d_type==DT_DIR && strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
            {
                relative_dirs_q.push(relative_dir+entry->d_name+"/");
            }
            else if(entry->d_type==DT_REG)
            {
                file_paths.push_back({relative_dir,entry->d_name});
            }
            
        }
        closedir(dir);
    }
}

void compare_two_files(string file_path1,string file_path2)
{
    int fd_1=open(file_path1.c_str(),O_RDONLY);
    int fd_2=open(file_path2.c_str(),O_RDONLY);
    char buffer1[8192];
    char buffer2[8192];
    int pos=0;
    bool output_when_unequal=true;
    bool output_when_equal=false;
    while (true)
    {
        int bytes_read_1=read(fd_1,buffer1,sizeof(buffer1));
        int bytes_read_2=read(fd_2,buffer2,sizeof(buffer2));
        if(bytes_read_1==0)
            break;
        if(bytes_read_1!=bytes_read_2)
        {
            printf("bytes read 1: %d; bytes read 2: %d\n",bytes_read_1,bytes_read_2);
            break;
        }
        for(int i=0;i<bytes_read_1;i++)
        {
            if(buffer1[i]!=buffer2[i] && output_when_unequal)
            {
                printf("different from %d\n",pos);
                output_when_unequal=false;
                output_when_equal=true;
            }
            if(buffer1[i]==buffer2[i] && output_when_equal)
            {
                printf("diffrence ending at %d\n",pos+i);
                output_when_unequal=true;
                output_when_equal=false;
            }
        }
        pos+=bytes_read_1;
    }
}    


int main(int argc, char const *argv[])
{ 
    vector<pair<string,string>> relative_file_paths; // the first element is relative dir, the second element is file name
    traverse_directory(origin_file_dir,relative_file_paths);
    StorageManager storage_manager(container_save_dir,file_restore_dir,key_value_save_dir);
    FileRecipeManager file_recipe_manager(file_recipe_save_dir);
    FileManager file_manager(origin_file_dir,&storage_manager,&file_recipe_manager);

    for(int i=0;i<relative_file_paths.size();i++)
    {
        // printf("partitioning file\n");
        file_manager.partition_file(relative_file_paths[i].first,relative_file_paths[i].second);
    }
    for(int i=0;i<relative_file_paths.size();i++)
    {
        file_manager.restore_file(relative_file_paths[i].first,relative_file_paths[i].second);
        compare_two_files(origin_file_dir+relative_file_paths[i].first+relative_file_paths[i].second,
            file_restore_dir+relative_file_paths[i].first+relative_file_paths[i].second);
    }


    return 0;
}
