#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "downloadUtil.h"
#include "rdkv_cdl_log_wrapper.h"

int doHttpFileDownload(void *in_curl, FileDwnl_t *pfile_dwnl, MtlsAuth_t *auth, unsigned int max_dwnl_speed, char *dnl_start_pos, int *out_httpCode );
int doAuthHttpFileDownload(void *in_curl, FileDwnl_t *pfile_dwnl, int *out_httpCode);
void *doCurlInit(void);
void doStopDownload(void *curl);

int main(int argc, char **argv) {
    void *handle;
    void *(*fnptr_doCurlInit)(void);
    void (*fnptr_doStopDownload)(void *);
    int (*fnptr_doHttpFileDownload)(void *in_curl, FileDwnl_t *pfile_dwnl, MtlsAuth_t *auth, unsigned int max_dwnl_speed, char *dnl_start_pos, int *out_httpCode );
    int (*fnptr_doAuthHttpFileDownload)(void *in_curl, FileDwnl_t *pfile_dwnl, int *out_httpCode);
    char *error;
    void *Curl_obj = NULL;
    int ret = 0;
    int httpcode = 0;
    FileDwnl_t file_dwnl;
    DownloadData DwnLoc;
    char *ptr = NULL;
    
    if (argc < 2) {
        printf("Less number of argument. Atleast 1 argument need to pass.\n");
	exit(EXIT_FAILURE);
    }
    
    log_init();
    
    memset(&file_dwnl, '\0', sizeof(file_dwnl));
    file_dwnl.hashData = NULL;
    file_dwnl.pPostFields = NULL;
    //file_dwnl.pPostFields = "mac=11:22:33";
    
    handle = dlopen("libdwnlutil.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    *(void **) (&fnptr_doCurlInit) = dlsym(handle, "doCurlInit");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }
    Curl_obj = (*fnptr_doCurlInit)();
    printf("doCurlInit returned %p\n", Curl_obj);

    if (1 == atoi(argv[1]) || 5 == atoi(argv[1])) { //1: xconf download
	*(file_dwnl.pathname) = 0;
	file_dwnl.chunk_dwnl_retry_time = 0;
	//file_dwnl.sslverify = true;
	int arg = atoi(argv[1]);
	if (arg == 1) {
	    strcpy(file_dwnl.url, "https://mockxconf:50052/firmwareupdate/getfirmwaredata");//Add mcok container url
	} else {
	    printf("Test case to check http 404 error\n");
	    strcpy(file_dwnl.url, "https://mockxconf:50052/firmwareupdate404/getfirmwaredata");//Add mcok container url to get http 404 error
	}
	printf("URL=%s\n", file_dwnl.url);
	DwnLoc.pvOut = NULL;
        DwnLoc.datasize = 0;
        DwnLoc.memsize = 512;
        ptr = malloc( 1000 );
        DwnLoc.pvOut = ptr;
	file_dwnl.pDlData = &DwnLoc;
        *(void **) (&fnptr_doHttpFileDownload) = dlsym(handle, "doHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doHttpFileDownload)(Curl_obj, &file_dwnl, NULL, 0, NULL, &httpcode);
    }else if (2 == atoi(argv[1])) { //File Download
	strcpy(file_dwnl.pathname,"/opt/firmware_test.bin");
	file_dwnl.chunk_dwnl_retry_time = 0;
	strcpy(file_dwnl.url, "https://mockxconf:50052/getfirmwarefile/firmware_test.bin");//Add mcok container url
	file_dwnl.pDlData = NULL;
        *(void **) (&fnptr_doHttpFileDownload) = dlsym(handle, "doHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doHttpFileDownload)(Curl_obj, &file_dwnl, NULL, 0, NULL, &httpcode);
    }else if (3 == atoi(argv[1])) { //File Download with 404 error
	strcpy(file_dwnl.pathname,"/opt/firmware404_test.bin");
	file_dwnl.chunk_dwnl_retry_time = 0;
	strcpy(file_dwnl.url, "https://mockxconf:50052/getfirmwarefile/firmware404_test.bin");//Add mcok container url
	file_dwnl.pDlData = NULL;
        *(void **) (&fnptr_doHttpFileDownload) = dlsym(handle, "doHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doHttpFileDownload)(Curl_obj, &file_dwnl, NULL, 0, NULL, &httpcode);
    }else if (4 == atoi(argv[1])) { //Throttle Download
	strcpy(file_dwnl.pathname,"/opt/firmware_test.bin");
	file_dwnl.chunk_dwnl_retry_time = 0;
	strcpy(file_dwnl.url, "https://mockxconf:50052/getfirmwarefile/firmware_test.bin");//Add mcok container url
	file_dwnl.pDlData = NULL;
	unsigned int max_dwnl_speed = 1 * 1024 * 1024;
        *(void **) (&fnptr_doHttpFileDownload) = dlsym(handle, "doHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doHttpFileDownload)(Curl_obj, &file_dwnl, NULL, max_dwnl_speed, NULL, &httpcode);
    }else if (6 == atoi(argv[1])) { //Chunk Download
	strcpy(file_dwnl.pathname,"/opt/firmware_test.bin");
	file_dwnl.chunk_dwnl_retry_time = 0;
	strcpy(file_dwnl.url, "https://mockxconf:50052/getfirmwarefile/firmware_test.bin");//Add mcok container url
	file_dwnl.pDlData = NULL;
	unsigned int max_dwnl_speed = 1 * 1024 * 1024;
        *(void **) (&fnptr_doHttpFileDownload) = dlsym(handle, "doHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doHttpFileDownload)(Curl_obj, &file_dwnl, NULL, max_dwnl_speed, "2-", &httpcode);
    }else if (7 == atoi(argv[1])) { //Codebig Download
	strcpy(file_dwnl.pathname,"/opt/firmware_test.bin");
	file_dwnl.chunk_dwnl_retry_time = 0;
	strcpy(file_dwnl.url, "https://mockxconf:50052/getfirmwarefile/firmware_test.bin");//Add mcok container url
	file_dwnl.pDlData = NULL;
        *(void **) (&fnptr_doAuthHttpFileDownload) = dlsym(handle, "doAuthHttpFileDownload");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }
        ret = (*fnptr_doAuthHttpFileDownload)(Curl_obj, &file_dwnl, &httpcode);
    }else{
        printf("Invalid argumenr pass.\n");
    }
    *(void **) (&fnptr_doStopDownload) = dlsym(handle, "doStopDownload");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }
    (*fnptr_doStopDownload)(Curl_obj);
    dlclose(handle);
    log_exit();
    printf("App exit with ret=%d\n", ret);
    return ret;
}
