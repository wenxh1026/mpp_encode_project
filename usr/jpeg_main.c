#define _POSIX_C_SOURCE 200809L  // 启用POSIX.1-2008特性
#define _XOPEN_SOURCE 700        // 启用X/Open 7特性
#include "jpeg_mpp.h"
#include "jpeg_function.h"

int main() {
    // 测试编码性能
    //MPP内存管理 + 硬件JPEG编码
    //printf("=== RK3562 JPEG编解码性能对比测试 ===\n");
    //printf("测试图像: %dx%d, 数量: %d, JPEG质量: %d\n\n", IMAGE_WIDTH, IMAGE_HEIGHT, NUM_IMAGES, JPEG_QUALITY);
    //printf("   进行MPP性能测试！\n");

    test_mpp_jpeg_encoding(); 
    //纯软件内存 + 硬件JPEG编码
    //test_software_jpeg_encoding(); 
    //纯软件JPEG编码（参考基准）
    test_pure_software_jpeg();  
    return 0;
}

