/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include "rdkv_cdl_log_wrapper.h"
#include "system_utils.h"
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <time.h>
#include <fnmatch.h>

/** Description: File present check.
 *
 *  @param file_name: The pointer to hold the file name.
 *  @return The function return status is int.
 *          RDK_API_SUCCESS 0
 *      RDK_API_FAILURE -1
 */
int filePresentCheck(const char *file_name) {
    int ret = RDK_API_FAILURE;
    struct stat sfile;

    if(file_name == NULL) {
        SWLOG_ERROR("%s Invalid Parameter\n", __FUNCTION__);
        return ret;
    }
    
    ret = stat(file_name, &sfile);
    //SWLOG_INFO("%s name:%s:ret:%d\n", __FUNCTION__, file_name, ret);
    return ret;
}

/** Description: Execute the Linux commands from process and return the output back.
 *
 *  @param cmd the pointer to hold the linux command to be executed.
 *  @param output the pointer to hold the command output.
 *  @size_buff: Buffer size of output parameter.
 *  @return The function return status int.
 *          RDK_API_SUCCESS 0
 *          RDK_API_FAILURE -1
 *  NOTE: The size_buff should not be less than equal zero or
 *  greater than 4096 byte
 */
int cmdExec(const char *cmd, char *output, unsigned int size_buff) {
    FILE *fp = NULL;
    int ret = RDK_API_FAILURE;
    int nbytes_read = RDK_API_FAILURE;

    if(cmd == NULL || output == NULL || size_buff == 0) {
        SWLOG_ERROR("%s Invalid Parameter\n", __FUNCTION__);
        return ret;
    }
    if(size_buff > MAX_OUT_BUFF_POPEN) {
        SWLOG_ERROR("%s size_buff should not be > %d. Provided buffer size=%u\n", __FUNCTION__, MAX_OUT_BUFF_POPEN, size_buff);
        return ret;
    }
    fp = popen(cmd, "r");
    if(fp == NULL) {
        SWLOG_ERROR("%s Failed to open pipe command execution:%s\n", __FUNCTION__, cmd);
        return ret;
    }
    nbytes_read = fread(output, 1, size_buff - 1, fp);
    if(nbytes_read != -1) {
        SWLOG_INFO("%s Successful read %d bytes\n", __FUNCTION__, nbytes_read);
        output[nbytes_read] = '\0';
    }else {
        SWLOG_ERROR("%s fread fails:%d\n", __FUNCTION__, nbytes_read);
        pclose(fp);
        return ret;
    }
//    SWLOG_INFO("%s cmd:%s\n", __FUNCTION__, cmd);
    pclose(fp);
    return RDK_API_SUCCESS;
}
/* Description: Function will provide file size
 * @param file_name: file name
 * @return int Success return file size and failure return -1
 * */
int getFileSize(const char *file_name)
{
    int file_size = 0;
    int ret = RDK_API_FAILURE;
    struct stat st;

    if (file_name == NULL) {
        SWLOG_ERROR("%s : File name not present\n", __FUNCTION__);
        return ret;
    }
    ret = stat(file_name, &st);
    if (ret == -1) {
        SWLOG_ERROR("%s : File unable to stat\n", __FUNCTION__);
        return ret;
    }
    file_size = st.st_size;
    return file_size;
}

/*Description: Use For read the file data and print on the stdout.
 * @param file_path: file name with path
 * @return int
 * */
int logFileData(const char *file_path) {
    FILE *fp = NULL;
    int ret = -1;
    char tbuff[80];

    if(file_path == NULL) {
        SWLOG_INFO("logFileData() File path is NULL\n");
        return ret;
    }
    fp = fopen(file_path, "r");
    if(fp == NULL) {
        SWLOG_INFO("logFileData() File unable to open\n");
        return ret;
    }
    while((fgets(tbuff, sizeof(tbuff), fp) != NULL)) {
        SWLOG_INFO("%s\n", tbuff);
        ret = 1;
    }
    fclose(fp);
    return ret;
}

/*Description: Creating directory
 * @param dirname: Directory name with path
 * @return int : Success :RDK_API_SUCCESS and fail: RDK_API_FAILURE
 * */
int createDir(const char *dirname) {
    int ret = RDK_API_SUCCESS;
    DIR *folder_fd = NULL;

    if (dirname != NULL) {
        folder_fd = opendir(dirname);
        if (folder_fd == NULL) {
            ret = mkdir(dirname, 0777);
            if (-1 == ret) {
                SWLOG_INFO("%s : Unable to create folder:%s\n", __FUNCTION__, dirname);
                ret = RDK_API_FAILURE;
            }
        } else {
            SWLOG_INFO("%s :Already Folder exist:%s\n", __FUNCTION__, dirname);
            closedir(folder_fd);
        }
    }
    else
    {
       SWLOG_ERROR("%s parameter is NULL\n", __FUNCTION__);
       ret = RDK_API_FAILURE;
    }
    return ret ;
}

/* Description: Use for clean Folder except file match with file_name.
 * @param folder: Folder name
 * @param file_name: File name pattern which are not to be deleted.
 * @return int : Fail RDK_API_FAILURE and Success RDK_API_SUCCESS
 * */
int eraseFolderExcePramaFile(const char *folder, const char* file_name, const char *model_num)
{
    int ret = RDK_API_FAILURE;
    DIR *folder_fd = NULL;
    struct dirent *dir = NULL;
    char oldfile[512];

    if (folder == NULL || file_name == NULL || model_num == NULL) {
        SWLOG_ERROR("%s parameter is NULL\n", __FUNCTION__);
        return ret;
    }
    folder_fd = opendir(folder);
    if (folder_fd == NULL) {
        SWLOG_ERROR("%s : Unable to open folder=%s and file=%s\n", __FUNCTION__, folder, file_name);
        return ret;
    }
    while((dir = readdir(folder_fd)) != NULL) {
        if (dir->d_type == DT_DIR || (strstr(dir->d_name, file_name))) {
            continue;
        } else if(strstr(dir->d_name, model_num)) {
            snprintf(oldfile, sizeof(oldfile), "%s/%s", folder, dir->d_name);
            SWLOG_INFO("%s Deleting old software file.%s\n", dir->d_name, oldfile);
            unlink(oldfile);
        }
    }
    closedir(folder_fd);
    return RDK_API_SUCCESS;
}


/* Description: Creating new file
 * @param:file_name : name of the file to create
 * @return void:
 * */
int createFile(const char *file_name) {
    int ret = RDK_API_FAILURE;
    FILE *fp = NULL;

    if (file_name == NULL) {
        SWLOG_ERROR("%s: Parameter is NULL\n", __FUNCTION__);
        return ret;
    }
    SWLOG_INFO("%s: Trying to create file=%s\n", __FUNCTION__, file_name);
    fp = fopen(file_name, "w");
    if (fp == NULL) {
        SWLOG_ERROR("%s: Unable to open file=%s\n", __FUNCTION__, file_name);
        return ret;
    } else {
        fclose(fp);
        return RDK_API_SUCCESS;
    }
}

/* Description: Erase files with .tgz extension containing
 * filename as part of the basename in the folder directory. 
 * @param:file_name : the directory to look for 
 * @param:file_name : the filename to match
 * @return int: 0 if any file was deleted, 1 otherwise
 * */

int eraseTGZItemsMatching( const char *folder, const char* file_name )
{
    int ret = 1;
    DIR *folder_fd;
    struct dirent *dir;
    char *pTmp;
    char *pDirFile;
    size_t szAllocSize;

    if( folder != NULL && file_name != NULL )
    {            
        folder_fd = opendir( folder );
        if( folder_fd != NULL )
        {
            while( (dir=readdir(folder_fd)) != NULL)
            {
                if( (strstr( dir->d_name, file_name ) != NULL) && (strstr( dir->d_name, ".tgz" ) != NULL) )
                {
                    if( (pTmp=strrchr( dir->d_name, '.' )) != NULL )
                    {
                        if( strstr( pTmp + 1, "tgz" ) != NULL )
                        {
                            szAllocSize = (sizeof( dir->d_name ) * 2) + 2 ; // about 514 bytes
                            pDirFile = malloc( szAllocSize );
                            if( pDirFile != NULL )
                            {
                                snprintf( pDirFile, szAllocSize, "%s/%s", folder, dir->d_name );
                                SWLOG_INFO( "Deleting file = %s\n", pDirFile );
                                unlink( pDirFile );
                                free( pDirFile );
                                ret = 0;
                            }
                        }
                    }
                }
            }
            closedir(folder_fd);
        }
        else
        {
            SWLOG_INFO("%s : Unable to open folder=%s and file=%s\n", __FUNCTION__, folder, file_name);
        }
    }
    else
    {
        SWLOG_ERROR("%s parameter is NULL\n", __FUNCTION__);
    }
    return ret;
}

/** @brief This Function gets the MAC Address of the device hardware.
 *  @param:iface: Network Interface name.
 *  @param:pMac: pointer to the char Buffer to store the output.
 *  @param:szBuffSize: size of the char buffer.
 *  @return  Returns the free space available.
 */
size_t GetHwMacAddress( char *iface, char *pMac, size_t szBufSize )
{
    size_t ret = 0;
    struct ifreq ifr;
    unsigned char *mac = NULL;
    if (iface != NULL && pMac != NULL)
    {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1)
        {
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);

            if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1)
            {
                SWLOG_ERROR("GetHwMacAddress: ioctl SIOCGIFHWADDR failed:%s\n", strerror(errno));
            }
            else
            {
                mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
                ret = snprintf(pMac, szBufSize, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                SWLOG_INFO("GetHwMacAddress: MAC address: %s and ret=%zu\n", pMac, ret);
            }
            close(fd);
        }
        else
        {
            SWLOG_ERROR("GetHwAddress: socket create failed:%s\n", strerror(errno));
        }
    }
    else
    {
        SWLOG_ERROR("GetHwAddress: Invalid Parameter\n");
    }
    return ret;
}

/** @brief This Function returns the free space available in MB.
 *
 *  @param[in]  path mounted path
 *
 *  @return  Returns the free space available.
 *  @retval  Returns the free space available.
 */
unsigned int getFreeSpace(char *path)
{
    struct statfs vfs;

    if(path == NULL) {
        SWLOG_ERROR("filesystem path is NULL\n");
        return 0;
    }
    if(path[0] != '/') {
        SWLOG_ERROR("Invalid path %s\n", path);
        return 0;
    }
    if (statfs(path, & vfs) != 0) {
        SWLOG_ERROR("Failed to get the file system details: %s\n", path);
        return 0;
    }

    return (vfs.f_bavail * vfs.f_bsize)/RDK_MB_SIZE;
}

/** @brief This Function checks the whether file system works or not.
 *
 *  @param[in]  path - mounted path
 *
 *  @return  Returns the free space available.
 *  @retval  Returns the free space available.
 */
unsigned int checkFileSystem(char *path)
{
    char file_path[RDK_FILEPATH_LEN] = {0};
    FILE *fptest = NULL;

    if(path == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return 0;
    }
    snprintf(file_path, RDK_FILEPATH_LEN, "%s/testfile", path);

    fptest = fopen(file_path, "w");
    if(fptest == NULL) {
        SWLOG_ERROR("Unable create file: %s\n", file_path);
        return 0;
    }
    fclose(fptest);

    if(remove(file_path)) {
        SWLOG_ERROR("%s is in working condition\n", path);
        return 0;
    }
    return 1;
}

/** @brief This Function returns the size of the file.
 *
 *  @param[in]  file name
 *
 *  @return  Returns the file size.
 *  @retval  Returns the file size.
 */
int findSize(char *fileName)
{
    struct stat st;


    if(fileName == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return 0;
    }

    if (stat(fileName, &st) != 0)
    {
        SWLOG_ERROR("Failed to get the file size: %s\n",fileName);
        return 0;
    }

    return st.st_size;
}

/** @brief This Function search for the folder/file in given path.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int findFile(char *dir, char *search)
{
    DIR *dp;
    int found = 0;
    struct dirent *entry;
    struct stat statbuf;

    if(dir == NULL) {
        SWLOG_ERROR("Invalid directory path\n");
        return 0;
    }
    if(search == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return 0;
    }

    if((dp = opendir(dir)) == NULL) {
        return found;
    }
    if (chdir(dir) != 0) {
        SWLOG_ERROR("Failed to change directory to %s \n", dir);
    }
    while((entry = readdir(dp)) != NULL) {
        if (lstat(entry->d_name, &statbuf) == -1) {
            SWLOG_ERROR("lstat failed for %s: %s\n", entry->d_name, strerror(errno));
        }       
        if(!strcmp(entry->d_name, search)) {
            found = 1;
            break;
        }
        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 ||
                strcmp("..",entry->d_name) == 0)
                continue;

            /* Recurse at a new indent level */
            found = findFile(entry->d_name, search);
            if(found) {
                break;
            }
        }
    }
    if (chdir("..") != 0) {
        SWLOG_ERROR("Failed to change directory to parent\n");
    }
    closedir(dp);
    return found;
}

/** @brief This Function search for the folder/file in given path
 *  with partial name.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *  @param[in]  out    - path of found file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int findPFile(char *path, char *search, char *out)
{
    size_t path_len;
    char *full_path = NULL;
    DIR *dir;
    int found = 0;
    struct dirent *entry;

    if(path == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return found;
    }
    if(search == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return 0;
    }
    if(out == NULL) {
        SWLOG_ERROR("Invalid out pointer\n");
        return 0;
    }
    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        SWLOG_ERROR("Can't open the directory\n");
        return found;
    }
    // the length of the path
    path_len = strlen(path);

    // iteration through entries in the directory
    while ((found == 0) && (entry = readdir(dir)) != NULL) {

        // skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        size_t allocate_size = path_len + strlen(entry->d_name) + 2;
        if (entry->d_type == DT_DIR) {
            // determinate a full path of an entry
            full_path = calloc(allocate_size, sizeof(char));
            snprintf(full_path, allocate_size, "%s/%s", path, entry->d_name);
            SWLOG_INFO(" findPFile : Constructed full path: %s", full_path);
            found = findPFile(full_path, search, out);
        }
        else if(fnmatch(search, entry->d_name, 0) == 0) {
            if(out) {
                snprintf(out, allocate_size, "%s/%s", path, entry->d_name);
                SWLOG_INFO(" findPFile : Constructed  path  out : %s", out);
            }
            found = 1;
        }

        if(full_path) {
            free(full_path);
            full_path = NULL;
        }
    }

    closedir(dir);
    return found;
}

/** @brief This Function search for the folder/file in given path
 *  with partial name.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *  @param[in]  out    - path of found file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int findPFileAll(char *path, char *search, char **out, int *found_t, int max_list)
{
    size_t path_len;
    char *full_path = NULL;
    DIR *dir;
    int found = 0;
    struct dirent *entry;

    if(path == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return found;
    }
    if(search == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return 0;
    }
    if(out == NULL) {
        SWLOG_ERROR("Invalid out pointer\n");
        return 0;
    }
    if(found_t == NULL) {
        SWLOG_ERROR("Invalid found_t pointer\n");
        return 0;
    }

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        SWLOG_ERROR("Can't open the directory\n");
        return found;
    }

    // the length of the path
    path_len = strlen(path);

    // iteration through entries in the directory
    while ((found == 0) && (entry = readdir(dir)) != NULL) {
        // skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        size_t allocate_size = path_len + strlen(entry->d_name) + 2;
        if (entry->d_type == DT_DIR) {
            // determinate a full path of an entry
            full_path = calloc(allocate_size, sizeof(char));
            snprintf(full_path, allocate_size, "%s/%s", path, entry->d_name);
            SWLOG_INFO(" findPFileAll : Constructed full path: %s", full_path);

            found = findPFileAll(full_path, search, out, found_t, max_list);
        }
        else if(fnmatch(search, entry->d_name, 0) == 0) {
            if(out[*found_t]) {
                snprintf(out[*found_t], allocate_size , "%s/%s", path, entry->d_name);
                SWLOG_INFO(" findPFileAll : Constructed  path, out : %s", full_path);
                (*found_t)++;
            }
            if((*found_t) >= max_list)
                found = 1;
        }

        if(full_path) {
            free(full_path);
            full_path = NULL;
        }
    }

    closedir(dir);
    return found;
}
/** @brief This Function find the data present in the list
 *  with partial name.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *  @param[in]  out    - path of found file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int isDataInList(char **pList,char *pData,int count)
{
    int index = 0;
    int found = 0;

    if(pList == NULL) {
        SWLOG_ERROR("Invalid list pointer\n");
        return 0;
    }
    if(pData == NULL) {
        SWLOG_ERROR("Invalid data pointer\n");
        return 0;
    }

    for(index = 0; index < count; index++){
        if(strcmp(pList[index], pData) == 0){
            SWLOG_INFO("Data %s is present in the list\n",pData);
            found = 1;
            break;
        }
    }

    return found;
}
/** @brief This Function search for the folder/file in given path.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int emptyFolder(char *folderPath)
{
    DIR *dir = opendir(folderPath);
    struct dirent *entry;
    char filePath[RDK_APP_PATH_LEN+1];   
    if (dir == NULL) {
        SWLOG_ERROR("Error opening directory\n");
        return RDK_API_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(filePath, RDK_APP_PATH_LEN+1, "%s/%s", folderPath, entry->d_name);
        if (entry->d_type == DT_DIR) {
            
            if (emptyFolder(filePath) != RDK_API_SUCCESS) {
                SWLOG_ERROR("Failed to empty folder: %s\n", filePath);
            }
            rmdir(filePath);
        }
        else {
             if (remove(filePath) != 0) {
                 closedir(dir);
                 return RDK_API_FAILURE;
             }                        
        }
    }

    closedir(dir);

    return RDK_API_SUCCESS;
}

/** @brief This Function search for the folder/file in given path.
 *
 *  @param[in]  dir    - directory path
 *  @param[in]  search - search file
 *
 *  @return  Returns the if file/folder present of not.
 *  @retval  Returns the if file/folder present of not.
 */
int removeFile(char *filePath)
{
    if (filePath == NULL) {
        SWLOG_ERROR("Invalid file path\n");
        return RDK_API_FAILURE;
    }

    if (remove(filePath) != 0) {
        return RDK_API_FAILURE;
    }

    return RDK_API_SUCCESS;
}

/** @brief This Function copies the file to destination path.
 *
 *  @param[in]  src    - source path
 *  @param[in]  dst    - destination file
 *
 *  @return  returns the status of the function.
 *  @retval  Returns RDK_API_SUCCESS on success, RDK_API_FAILURE otherwise.
 */
int copyFiles(char *src, char *dst)
{
    FILE *fpin, *fpout;
    int  buff[1024];
    int n = 0;

    if(src == NULL) {
        SWLOG_ERROR("Invalid src file\n");
        return RDK_API_FAILURE;
    }
    if(dst == NULL) {
        SWLOG_ERROR("Invalid dst file\n");
        return RDK_API_FAILURE;
    }

    fpin = fopen(src, "rb");
    if(fpin == NULL) {
        SWLOG_ERROR("Failed to open src file: %s\n", src);
        return RDK_API_FAILURE;
    }

    fpout = fopen(dst, "wb");
    if(fpout == NULL) {
        SWLOG_ERROR("Failed to open dst file: %s\n", dst);
        fclose(fpin);
        return RDK_API_FAILURE;
    }

    do {
        n = fread(buff, 1, sizeof(buff), fpin);
        if(n) fwrite(buff, 1, n, fpout);
    }while(n == sizeof(buff));

    fclose(fpin);
    fclose(fpout);

    return RDK_API_SUCCESS;
}

/** @brief This Function checks if file present or not.
 *
 *  @param[in]  pFilepath - file path
 *
 *  @return  Returns if file present or not.
 *  @retval  Returns 1 if file present 0 otherwise.
 */
int fileCheck(char *pFilepath)
{
    FILE *fp = fopen(pFilepath, "r");
    if(fp) {
        fclose(fp);
        return 1;
    }
    else {
        return 0;
    }
}

/** @brief This Function checks if file present or not.
 *
 *  @param[in]  pFilepath - file path
 *
 *  @return  Returns if file present or not.
 *  @retval  Returns 1 if file present 0 otherwise.
 */
int folderCheck(char *path)
{
    struct stat stat_path;

    if(path == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return 0;
    }

    // stat for the path
    if (stat(path, &stat_path) != 0) {
        SWLOG_ERROR("Error accessing the path: %s\n", path);
    }

    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0) {
        return 0;
    }

    return 1;
}

/** @brief This Function returns the extension of file.
 *
 *  @param[in]  filename - file name
 *
 *  @return  Returns pointer to char.
 *  @retval  Returns extension value.
 */
char* getExtension(char *filename)
{
    char *extension;

    if(filename == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return NULL;
    }

    extension = strrchr(filename, '.');

    if (extension) {
        return (extension + 1);
    }

    return NULL;
}

/** @brief This Function extract the file name from path.
 *
 *  @param[in]  filename - path
 *  @param[in]  delim    - char to search
 *
 *  @return  Returns pointer to char.
 *  @retval  Returns extension value.
 */
char* getPartStr(char *fullpath, char *delim)
{
    char *extension;

    if(fullpath == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return NULL;
    }
    if(delim == NULL) {
        SWLOG_ERROR("Invalid delim\n");
        return NULL;
    }

    extension = strstr(fullpath, delim);

    if (extension) {
        return (extension + 1);
    }

    return NULL;
}

/** @brief This Function extract the file name from path.
 *
 *  @param[in]  filename - path
 *  @param[in]  delim    - char to search
 *
 *  @return  Returns pointer to char.
 *  @retval  Returns extension value.
 */
char* getPartChar(char *fullpath, char delim)
{
    char *extension;

    if(fullpath == NULL) {
        SWLOG_ERROR("Invalid file name\n");
        return NULL;
    }

    extension = strrchr(fullpath, delim);

    if (extension) {
        return (extension + 1);
    }

    return NULL;
}

/** @brief This Function executes the system command and copies the output.
 *
 *  @param[in]  cmd  command to be executed
 *  @param[out] out  Output buffer
 *  @param[in]  len  Output buffer length
 *
 *  @return  None.
 *  @retval  None.
 */
void copyCommandOutput (char *cmd, char *out, int len)
{
    FILE *fp;

    if(out != NULL)
        out[0] = 0;

    fp = popen (cmd, "r");
    if (fp) {
        if(out) {
            if (fgets (out, len, fp) != NULL) {
                size_t outLen = strlen (out);
                if ((outLen > 0) && (out[outLen - 1] == '\n'))
                    out[outLen - 1] = 0;
            }
        }
        pclose (fp);
    }
    else {
        SWLOG_WARN("Failed to run the command: %s\n", cmd);
    }
}

/** @brief This Function extracts the tar file in given path.
 *
 *  @param[in]  in_file  input tar file path
 *  @param[out] out_path Output directory
 *
 *  @return  Returns status of the function.
 *  @retval  Returns RDK_API_SUCCESS on success, RDK_API_FAILURE otherwise.
 */
int tarExtract(char *in_file, char *out_path)
{
    char buff[MAX_BUFF_SIZE] = {0};

    if(in_file == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return RDK_API_FAILURE;
    }
    if(out_path == NULL) {
        SWLOG_ERROR("Invalid output path\n");
        return RDK_API_FAILURE;
    }

    snprintf(buff, MAX_BUFF_SIZE, "tar -xvf %s -C %s", in_file, out_path);

    copyCommandOutput(buff, NULL, 0);

    return RDK_API_SUCCESS;
}

/** @brief This Function extracts the tar file in given path.
 *
 *  @param[in]  in_file  input tar file path
 *  @param[out] out_path Output directory
 *
 *  @return  Returns status of the function.
 *  @retval  Returns RDK_API_SUCCESS on success, RDK_API_FAILURE otherwise.
 */
int arExtract(char *in_file, char *out_path)
{
    char buff[MAX_BUFF_SIZE] = {0};

    if(in_file == NULL) {
        SWLOG_ERROR("Invalid input path\n");
        return RDK_API_FAILURE;
    }
    if(out_path == NULL) {
        SWLOG_ERROR("Invalid output path\n");
        return RDK_API_FAILURE;
    }

    snprintf(buff, MAX_BUFF_SIZE, "cd %s;ar -x %s", out_path, in_file);
    copyCommandOutput(buff, NULL, 0);

    return RDK_API_SUCCESS;
}

/* Description: Use for to get last modified time of the file
 * @param : file_name: File name
 * @return: int Success: return time and -1: fail
 * */
unsigned int getFileLastModifyTime(char *file_name)
{
    struct stat attr;
    int ret = 0;
    if (file_name == NULL) {
        SWLOG_ERROR("Parameter is NULL\n");
        return ret;
    }
    memset(&attr, '\0', sizeof(attr));
    ret = stat(file_name, &attr);
    if (ret != 0) {
        SWLOG_ERROR("File: %s not present: %d\n", file_name, ret);
        return 0;
    }
    SWLOG_INFO("Last mod time: %lu\n", attr.st_mtime);
    return attr.st_mtime;
}

/* Description: Use for to get current system time
 * @param : void:
 * @return: int Success: return time and 0: fail
 * */
time_t getCurrentSysTimeSec(void)
{
    time_t curtime = time(0);
    if (curtime == ((time_t) -1)) {
        SWLOG_INFO("time return error\n");
        return 0;
    } else {
        SWLOG_INFO("current system time=%lu\n", curtime);
    }
    return curtime;
}

/** @brief This Function searches for the process and provides the relevant process pid.
 *
 *  @param[in]  in_file  input file process name
 *  @param[out] out_path Output process pid
 *
 *  @return  Returns process pid of the function.
 *  @retval  Returns valid process pid on success, return 0 otherwise.
 */
int getProcessID(char *in_file, char *out_path)
{
    char inbuff[MAX_BUFF_SIZE]  = {0};
    char outbuff[MAX_BUFF_SIZE] = {0};

    snprintf(inbuff, MAX_BUFF_SIZE, "pgrep -f '%s'",in_file);
    copyCommandOutput(inbuff, outbuff, MAX_BUFF_SIZE);

    return atoi(outbuff);
}

/*****************************************************************************
                           String Operations
*****************************************************************************/
static void swap(char **a, char **b)
{
    char *temp = *a;
    *a = *b;
    *b = temp;
}

void qsString(char *arr[], unsigned int length)
{
    unsigned int i, piv = 0;

    if (length <= 1)
        return;

    for (i = 0; i < length; i++) {
        if (strcmp(arr[i], arr[length -1]) > 0)
            swap(arr + i, arr + piv++);
    }

    swap(arr + piv, arr + length - 1);

    //recursively sort upper and lower
    qsString(arr, piv++);
    qsString(arr + piv, length - piv);
}

int strSplit(char *in, char *tok, char **out, int len)
{
    int count = 0;

    char *token = strtok(in, tok);

    while (token != NULL && count < len) {
        out[count] = token;
        token = strtok(NULL, " ");
        count++;
    }
    return count;
}

int strRmDuplicate(char **in, int len)
{
    int count = 0;
    int i = 0;

    for(i = 1; i < len; i++) {
        if(strcmp(in[count], in[i])) {
            count++;
            in[count] = in[i];
        }
    }

    return count + 1;
}

void getStringValueFromFile(char* path, char* strtokvalue, char* string, char* outValue){
    char lines[1024];
    char *token;
    FILE *file = fopen(path,"r");

    if( (strtokvalue != NULL) && (string != NULL) && ( outValue != NULL) ){
        if( file ){
            while(fgets(lines, sizeof(lines), file)){
                token = strtok(lines, strtokvalue);

                while(token != NULL){
                    if(strcmp(token,string) == 0 ){
                        strncpy(outValue, strtok(NULL,strtokvalue),128);
                        break;
                    }
                    token = strtok(NULL,strtokvalue);
                }
            }

            fclose(file);
        }
        else{
            SWLOG_ERROR("file open failed %s\n",path);
        }
    }
    else{
        if (file) {
            fclose(file);
        }
        SWLOG_ERROR("Invalid Parameters %p %p %p",strtokvalue, string, outValue);
    }
}
