#include "jpeg_function.h"


// 文件路径定义
const char* yuv_files[NUM_IMAGES] = {
    "/root/MPP_text/image0.yuv",
    "/root/MPP_text/image1.yuv", 
    "/root/MPP_text/image2.yuv",
    "/root/MPP_text/image3.yuv",
    "/root/MPP_text/image4.yuv"
};

const char* jpeg_files[NUM_IMAGES] = {
    "/root/MPP_text/output_mpp0.jpg",
    "/root/MPP_text/output_mpp1.jpg",
    "/root/MPP_text/output_mpp2.jpg",
    "/root/MPP_text/output_mpp3.jpg",
    "/root/MPP_text/output_mpp4.jpg"
};

const char* jpeg_sw_files[NUM_IMAGES] = {
    "/root/MPP_text/output_sw0.jpg",
    "/root/MPP_text/output_sw1.jpg",
    "/root/MPP_text/output_sw2.jpg",
    "/root/MPP_text/output_sw3.jpg",
    "/root/MPP_text/output_sw4.jpg"
};


void diagnose_save_failure(const char* filename, const void* data, size_t size) {
    printf("   === 保存失败诊断 ===\n");
    
    // 1. 检查磁盘空间
    struct statfs fs;
    if (statfs("/", &fs) == 0) {
        uint64_t free_bytes = fs.f_bavail * fs.f_bsize;
        printf("   磁盘可用空间: %.2f GB\n", (double)free_bytes / (1024 * 1024 * 1024));
        
        if (free_bytes < size) {
            printf("   ❌❌ 磁盘空间不足: 需要 %.2f MB, 可用 %.2f MB\n",
                   (double)size / (1024 * 1024), (double)free_bytes / (1024 * 1024));
        }
    }
    
    // 2. 检查文件权限
    char dir_path[256];
    strncpy(dir_path, filename, sizeof(dir_path));
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (access(dir_path, W_OK) != 0) {
            printf("   ❌❌ 目录无写权限: %s (%s)\n", dir_path, strerror(errno));
        }
    }
    
    // 3. 检查文件系统类型
    printf("   文件系统类型: 0x%lx\n", fs.f_type);
    if (fs.f_type == 0xEF53) { // ext4
        printf("   文件系统: ext4\n");
    } else if (fs.f_type == 0x5346544E) { // NTFS
        printf("   ⚠⚠⚠️ 文件系统: NTFS (可能有兼容性问题)\n");
    } else if (fs.f_type == 0x4d44) { // FAT
        printf("   ⚠⚠⚠️ 文件系统: FAT (可能有文件大小限制)\n");
        // FAT32文件大小限制为4GB
        if (size > 4 * 1024 * 1024 * 1024ULL) {
            printf("   ❌❌ 超过FAT32文件大小限制(4GB)\n");
        }
    }
    
    // 4. 尝试创建小文件测试
    printf("   测试文件创建能力...\n");
    char test_file[256];
    snprintf(test_file, sizeof(test_file), "%s.test", filename);
    
    const char* test_data = "test";
    int test_fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (test_fd < 0) {
        printf("   ❌❌ 无法创建测试文件: %s (%s)\n", test_file, strerror(errno));
    } else {
        ssize_t written = write(test_fd, test_data, strlen(test_data));
        close(test_fd);
        
        if (written == strlen(test_data)) {
            printf("   ✅ 测试文件创建成功\n\n");
            unlink(test_file); // 清理测试文件
        } else {
            printf("   ❌❌ 测试文件写入失败: %s (%s)\n\n", test_file, strerror(errno));
        }
    }
}

// 高精度时间测量
double get_us_time(void) {
    static int time_method = 0; // 0=auto, 1=gettimeofday, 2=clock_gettime, 3=clock
    
    if (time_method == 0) {
        // 自动检测最佳时间源
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0) {
            time_method = 1;
        }
        #ifdef CLOCK_MONOTONIC
        else {
            struct timespec ts;
            if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
                time_method = 2;
            } else {
                time_method = 3;
            }
        }
        #else
        else {
            time_method = 3;
        }
        #endif
    }
    
    // 使用选定的时间源
    switch (time_method) {
        case 1: {
            struct timeval tv;
            if (gettimeofday(&tv, NULL) == 0) {
                return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
            }
            break;
        }
        case 2: {
            #ifdef CLOCK_MONOTONIC
            struct timespec ts;
            if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
                return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
            }
            #endif
            break;
        }
        case 3:
        default:
            return (double)clock() * 1000000.0 / CLOCKS_PER_SEC;
    }
    
    // 如果所有方法都失败，返回0
    return 0;
}

// 读取YUV文件到内存
int read_yuv_file(const char* filename, unsigned char** buffer) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("   无法打开YUV文件 %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    if (file_size != YUV_SIZE) {
        printf("   YUV文件大小不匹配: 期望 %d, 实际 %ld\n", YUV_SIZE, file_size);
        close(fd);
        return -1;
    }

    *buffer = (unsigned char*)malloc(YUV_SIZE);
    if (!*buffer) {
        printf("   内存分配失败\n");
        close(fd); 
        return -1;
    }
    ssize_t bytes_read = read(fd, *buffer, YUV_SIZE);
    close(fd);
    
    return (bytes_read == YUV_SIZE) ? 0 : -1;
}

int readmpp_yuv_file(const char* filename, void** buffer) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("   无法打开YUV文件 %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    if (file_size != YUV_SIZE) {
        printf("   YUV文件大小不匹配: 期望 %d, 实际 %ld\n", YUV_SIZE, file_size);
        close(fd);
        return -1;
    }

    ssize_t bytes_read = read(fd, *buffer, YUV_SIZE);
    close(fd);
    
    return (bytes_read == YUV_SIZE) ? 0 : -1;
}

// 写入数据到文件
int write_data_to_file(const char* filename, const void* data, size_t size) {
    printf("     正在将jpeg格式写入进文件！！\n\n");
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("     正在打开文件！！\n\n");
    if (fd < 0) {
        printf("     无法创建输出文件 %s: %s\n", filename, strerror(errno));
        return -1;
    }else{
        printf("     打开成功！！\n");
    }
    printf("     准备写入的字节为%ld\n",size);
    ssize_t bytes_written = write(fd, data, size);
    if (fsync(fd) < 0) {
    printf("   ⚠⚠⚠️  fsync失败: %s (数据可能未完全持久化)\n", strerror(errno));
    // 不立即返回失败，因为数据可能已写入缓存
    }else{
        printf("     fsync成功！！\n");
    }  
    close(fd);
    printf("     写入的字节数为%ld\n",bytes_written);
    return (bytes_written == size) ? 0 : -1;
}

//MPP缓冲区状态函数
int check_buffer_state(MppBuffer buffer, int index) {
    printf("     检查缓冲区[%d]状态...\n", index);
    
    if (!buffer) {
        printf("   ❌❌ 缓冲区为空指针\n");
        return -1;
    }else{
        printf("     缓冲区正常！\n");
    }
    
    // 检查缓冲区指针
    void* ptr = mpp_buffer_get_ptr(buffer);
    printf("     缓冲区指针: %p\n", ptr);
    
    if (!ptr) {
        printf("     ❌❌ 无法获取缓冲区指针\n");
        return -1;
    }
    
    // 检查缓冲区信息
    MppBufferInfo info;
    if (mpp_buffer_info_get(buffer, &info) == MPP_OK) {
        printf("     缓冲区信息: fd=%d, size=%zu, type=%d\n", 
            info.fd, info.size, info.type);
        
        // 检查大小是否匹配
        if (info.size < YUV_SIZE) {
            printf("   ❌❌ 缓冲区太小: 需要 %d, 实际 %zu\n", YUV_SIZE, info.size);
            return -1;
        }
    } else {
        printf("   ⚠⚠⚠️ 无法获取缓冲区信息\n");
    }
    return 0;
}