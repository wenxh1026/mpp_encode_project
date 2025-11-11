#if !defined(JPEG_FUNCTION)
#define JPEG_FUNCTION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/vfs.h>   
#include <sys/statvfs.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_buffer.h>
// 启用必要的特性宏定义
#define _POSIX_C_SOURCE 200809L  // 启用POSIX.1-2008特性
#define _XOPEN_SOURCE 700        // 启用X/Open 7特性

// 图像参数定义
#define IMAGE_WIDTH   1920
#define IMAGE_HEIGHT  1080
#define YUV_SIZE      (IMAGE_WIDTH * IMAGE_HEIGHT * 3 / 2)  // NV12格式
#define NUM_IMAGES    5
#define JPEG_QUALITY  80
#define buffer_size   YUV_SIZE+1920*4
#define IMAGE_WIDTH_STRIDE 1920
#define IMAGE_HEIGHT_STRIDE 1080

extern const char* yuv_files[NUM_IMAGES];
extern const char* jpeg_files[NUM_IMAGES];
extern const char* jpeg_sw_files[NUM_IMAGES];

void diagnose_save_failure(const char* filename, const void* data, size_t size);
double get_us_time(void);
int read_yuv_file(const char* filename, unsigned char** buffer);
int readmpp_yuv_file(const char* filename, void** buffer);
int write_data_to_file(const char* filename, const void* data, size_t size);
int check_buffer_state(MppBuffer buffer, int index);

#endif // JPEG_FUNCTION
