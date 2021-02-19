#include "file_recipe_manager.h"


FileRecipeManager::FileRecipeManager(string file_recipe_save_dir):
    file_recipe_save_dir_(file_recipe_save_dir)
{

}

FileRecipeManager::~FileRecipeManager()
{
    
}

string FileRecipeManager::convert_file_identifler_to_file_recipe_filename(const byte file_identifier[kFileIdentiferLen])
{
    char file_recipe_file_name[kFileIdentiferLen*2+1];
    char *p=file_recipe_file_name;
    for(int i=0;i<kFileIdentiferLen;i++,p+=2)
    {
        sprintf(p,"%02x",file_identifier[i]);
    }
    file_recipe_file_name[2*kFileIdentiferLen]='\0';
    return string(file_recipe_file_name);
}

void FileRecipeManager::save_file_recipe_to_disk(const FileRecipe &file_recipe)
{
    bool write_success=true;
    string file_recipe_filename=convert_file_identifler_to_file_recipe_filename(file_recipe.file_identifier);
    string file_recipe_file_path=file_recipe_save_dir_+file_recipe_filename;

    size_t fd=open(file_recipe_file_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if(fd==-1)
    {
        printf("failed to open %s with error: %s\n",file_recipe_file_path.c_str(),strerror(errno));
        return;
    }

    // save relative_dir of file_recipe
    int relative_dir_len=file_recipe.relative_dir.size();
    if(write(fd,&relative_dir_len,sizeof(relative_dir_len))==-1)
    {
        printf("failed to write relative_dir_len when writing file recipe\n");
        write_success=false;
    }
    if(write(fd,file_recipe.relative_dir.c_str(),file_recipe.relative_dir.size())==-1)
    {
        printf("failed to write relative_dir when writing file recipe\n");
        write_success=false;
    }

    // save file_name of file_recipe
    int file_name_len=file_recipe.file_name.size();
    if(write(fd,&file_name_len,sizeof(file_name_len))==-1)
    {
        printf("failed to write file_name_len when writing file recipe\n");
        write_success=false;
    }
    if(write(fd,file_recipe.file_name.c_str(),file_recipe.file_name.size())==-1)
    {        
        printf("failed to write file_name when writing file recipe\n");
        write_success=false;
    }
    // save chunk_infos of file_recipe
    const vector<ChunkInfo> &chunk_infos=file_recipe.chunk_infos;
    for(int i=0;i<chunk_infos.size();i++)
    {
        if(write(fd,&chunk_infos[i],sizeof(ChunkInfo))==-1)
        {
            printf("failed to write chunk infos %d when saving file recipe with error: %s\n",i,strerror(errno));
            write_success=false;
            break;
        }
    }
    close(fd);
    if(write_success==false)
    {
        remove(file_recipe_file_path.c_str());
    }
}

FileRecipe* FileRecipeManager::read_file_recipe_from_disk(const string &relative_dir,const string &file_name)
{
    bool read_success=true;
    FileRecipe *file_recipe=new FileRecipe(relative_dir,file_name);
    string file_recipe_filename=convert_file_identifler_to_file_recipe_filename(file_recipe->file_identifier);
    string file_recipe_file_path=file_recipe_save_dir_+file_recipe_filename;

    size_t fd=open(file_recipe_file_path.c_str(),O_RDONLY,S_IRUSR|S_IWUSR);
    if(fd==-1)
    {
        printf("failed to open %s when reading a file recipe with error %s\n",file_recipe_file_path.c_str(),strerror(errno));
        delete file_recipe;
        read_success=false;
        return nullptr;
    }
    
    // although relative path is stored on disk, it is not needed here
    int relative_dir_len;
    if(read(fd,&relative_dir_len,sizeof(relative_dir_len))==-1)
    {
        printf("failed to read relative_dir_len with error %s\n",strerror(errno));
        read_success=false;
    }
    char *relative_dir_buffer=new char[relative_dir_len+1];
    if(read(fd,relative_dir_buffer,relative_dir_len)!=relative_dir_len)
    {
        printf("failed to read relative_die with error %s\n",strerror(errno));
        read_success=false;
    }
    relative_dir_buffer[relative_dir_len]='\0';
    delete relative_dir_buffer; 

    // although filename is stored on disk, it is not needed here
    int file_name_len;
    if(read(fd,&file_name_len,sizeof(file_name_len))==-1)
    {
        printf("failed to read file_name_len with error %s\n",strerror(errno));
        read_success=false;
    }
    char *file_name_buffer=new char[file_name_len+1];
    if(read(fd,file_name_buffer,file_name_len)!=file_name_len)
    {
        printf("failed to read file_name with error %s\n",strerror(errno));
        read_success=false;
    }
    file_name_buffer[file_name_len]='\0';
    delete file_name_buffer;

    ChunkInfo chunk_info;
    while (true)
    {
        int bytes_read=read(fd,&chunk_info,sizeof(ChunkInfo));
        if(bytes_read==0)
        {
            break;
        }
        if(bytes_read==-1 || bytes_read!=sizeof(ChunkInfo))
        {
            read_success=false;
            break;
        }
        file_recipe->chunk_infos.push_back(chunk_info);
    }
    close(fd);
    
    if(!read_success)
    {
        delete file_recipe;
        file_recipe=nullptr;
    }
    return file_recipe;

}

void FileRecipeManager::add_file_recipe(const FileRecipe &file_recipe)
{
    save_file_recipe_to_disk(file_recipe);
}

void FileRecipeManager::delete_file_recipe(const string &relative_dir,const string &file_name)
{
    FileRecipe file_recipe(relative_dir,file_name);
    string file_recipe_filename=convert_file_identifler_to_file_recipe_filename(file_recipe.file_identifier);
    string file_recipe_file_path=file_recipe_save_dir_+file_recipe_filename;
    if(access(file_recipe_file_path.c_str(),F_OK)==0)
        remove(file_recipe_file_path.c_str());
}

bool FileRecipeManager::find_set_file_recipe(const string &relative_dir,const string &file_name,FileRecipe *&file_recipe)
{
    file_recipe=read_file_recipe_from_disk(relative_dir,file_name);
    if(file_recipe==nullptr)
    {
        return false;
    }
    return true;
}