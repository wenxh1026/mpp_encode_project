#include "jpeg_mpp.h"

// 使用MPP内存管理进行硬件JPEG编码
void test_mpp_jpeg_encoding(void) {
    printf("\n=== MPP内存管理 + 硬件JPEG编码测试 ===\n");
    
    MppCtx ctx;
    MppApi *mpi;
    MppBufferGroup group;
    MppEncCfg cfg;
    MPP_RET ret;
    
    double total_time = 0;
    int success_count = 0;

    // 初始化MPP JPEG编码器
    ret = mpp_create(&ctx, &mpi);
    if (ret != MPP_OK) {
        printf("   MPP创建失败: %d\n", ret);
        return;
    }else{
        printf("   MPP创建成功!   \n");
    }
    
    ret = mpp_init(ctx, MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
    if (ret != MPP_OK) {
        printf("   MPP初始化失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }else{
        printf("   MPP初始化成功!   \n");
    }
    
    // 配置编码参数
    mpp_enc_cfg_init(&cfg);
    mpp_enc_cfg_set_s32(cfg, "prep:width", IMAGE_WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:height", IMAGE_HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride",IMAGE_WIDTH_STRIDE);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride",IMAGE_HEIGHT_STRIDE);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);  // NV12格式
    mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", JPEG_QUALITY);    // JPEG质量

    ret=mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    // 创建MPP内存池
    ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        printf("   MPP内存池创建失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }else{
        printf("   MPP内存池创建成功！\n");
    }

    ret =mpp_buffer_group_limit_config(group,buffer_size,NUM_IMAGES);
    if (ret!=MPP_OK){
        printf("   MPP内存池限制失败，请重试！\n");
        return ;
    }else{
        printf("   MPP内存池限制成功！\n");
    }



    // 处理每张图像

    for (int i = 0; i < NUM_IMAGES; i++) {
        printf("   处理图像IMAGE(%d)...\n", i);
        // 分配MPP缓冲区
        MppBuffer buffer = NULL;
        MppBuffer buffer2 =NULL;
        MppPacket packet =NULL;
        MppFrame  frame =NULL;
        ret = mpp_buffer_get(group, &buffer, buffer_size);
        if (ret != MPP_OK || !buffer) {
            printf("   MPP缓冲区分配失败");
            printf("   问题是：ret=%d,buffer=%p",ret,buffer);
            continue;
        }else{
            printf("   MPP缓冲区分配成功!\n");
        }
        unsigned char* yuv_data = NULL;
        // 读取YUV数据到MPP缓冲区

        // 检查MPP缓冲区状态的函数
        
        // 将数据复制到MPP缓冲区
        double start_time = get_us_time();
        void* mpp_ptr = mpp_buffer_get_ptr(buffer);

        if (!mpp_ptr) {
            printf("   无法获取MPP缓冲区指针\n");
            mpp_buffer_put(buffer);
            continue;
        }else{
            printf("   成功获取MPP缓冲区指针\n");
            printf("   mpp_ptr的地址为：%p\n",mpp_ptr);
        }
        
        printf("   尝试内存同步\n");
        mpp_buffer_sync_begin(buffer);
        printf("   同步开始成功！！\n");
        //printf("     尝试开始拷贝\n");
        if (readmpp_yuv_file(yuv_files[i], &mpp_ptr) != 0) {
            printf("   YUV文件读取失败\n");
            mpp_buffer_put(buffer);
            continue;
        }else{
            printf("   YUV读取成功\n");
        }
                

        //printf("     拷贝成功！！\n");
        mpp_buffer_sync_end(buffer);
        printf("   同步结束成功！\n");

        // 准备MPP帧
        mpp_frame_init(&frame);
        mpp_frame_set_buffer(frame, buffer);
        mpp_frame_set_width(frame, IMAGE_WIDTH);
        mpp_frame_set_height(frame, IMAGE_HEIGHT);
        mpp_frame_set_hor_stride(frame, IMAGE_HEIGHT_STRIDE);
        mpp_frame_set_ver_stride(frame,IMAGE_WIDTH_STRIDE);
        mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
        printf("   准备MPP帧成功！\n");
 

        ret = mpp_buffer_get(group,&buffer2,buffer_size);
        if (ret != MPP_OK || !buffer2) {
            printf("   MPP_packet缓冲区分配失败\n");
            printf("   问题是：ret=%d,buffer2=%p",ret,buffer2);
            continue;
        }else{
            printf("   MPP_packet缓冲区分配成功!\n");
        }

        //给packet分配缓冲区
        void *mpp_ptr2=mpp_buffer_get_ptr(buffer2);
        if (!mpp_ptr2) {
            printf("   无法获取MPP缓冲区指针\n");
            mpp_buffer_put(buffer);
            continue;
        }else{
            printf("   成功获取MPP缓冲区指针\n");
            printf("   mpp_ptr2的地址为：%p\n",mpp_ptr2);
        }
        
        ret=mpp_packet_init(packet,mpp_ptr2,buffer_size);
        mpp_packet_init_with_buffer(&packet, buffer2);

        // JPEG编码
        //ret=mpi->encode(ctx,frame,packet);//目前暂未开发
        printf("   准备进行帧提取！\n");
        ret=mpi->encode_put_frame(ctx, frame);
        if (ret != MPP_OK) {
            printf("   获取帧失败: %d \n", ret);
            free(yuv_data);
            mpp_buffer_put(buffer);
            mpp_frame_deinit(&frame);
            continue;
        }else{
            printf("   获取帧成功！\n\n");
            
        }

        printf("   准备进行包提取！\n");
        ret=mpi->encode_get_packet(ctx,packet);
        if (ret != MPP_OK) {
            printf("   获取包失败: %d \n", ret);
            free(yuv_data);
            mpp_buffer_put(buffer);
            mpp_frame_deinit(&frame);
            continue;
        }else{
            printf("   获取包成功！\n\n");
            
        }

        void* jpeg_data = mpp_packet_get_data(packet);
        printf("    packet指针的位置在：%p",jpeg_data);
        size_t jpeg_size = mpp_packet_get_length(packet);

        printf("   此次JPEG图像大小为：%ld\n",jpeg_size);
        ret=write_data_to_file(jpeg_files[i], jpeg_data, jpeg_size);
        // 保存JPEG文件
        if (!ret) {
            success_count++;
            printf("   ✅ 保存成功\n\n\n");
        } else {
            printf("   ❌❌ 保存失败\n\n");
            diagnose_save_failure(jpeg_files[i], jpeg_data, jpeg_size);
        }
          
        double end_time = get_us_time();       
        total_time = (end_time - start_time)/1000;   // 转换为毫秒
        // 清理资源
        free(yuv_data);
        mpp_packet_deinit(&packet);
        mpp_frame_deinit(&frame);
        mpp_buffer_put(buffer);
        mpp_buffer_put(buffer2);

    }

    // 清理MPP资源
    ret=mpp_enc_cfg_deinit(cfg);
    if (ret!=MPP_OK)
    {
        printf("   编码配置释放失败\n");
    }else{
        printf("   编码配置释放成功！\n");
    }
    ret=mpp_buffer_group_put(group);
    if (ret!=MPP_OK)
    {
        printf("   缓冲组释放失败\n");
    }else{
        printf("   缓冲组释放成功！\n");
    }
    ret=mpp_destroy(ctx);
    if (ret!=MPP_OK)
    {
        printf("   MPP释放失败\n");
    }else{
        printf("   MPP释放成功！\n");
    }
    //打印最终结果
    printf("   成功编码: %d/%d 图像\n", success_count, NUM_IMAGES);
    printf("   总处理时间: %.2f ms\n", total_time);
    printf("   平均每图像处理时间: %.2f ms\n", total_time / NUM_IMAGES);
    printf("   理论帧率: %.1f FPS\n\n", 1000.0 / (total_time / NUM_IMAGES));
}
/*
// 使用纯软件内存管理进行硬件JPEG编码
void test_software_jpeg_encoding(void) {
    printf("\n=== 纯软件内存管理 + 硬件JPEG编码测试 ===\n");
   
    MppCtx ctx;
    MppApi *mpi;
    MppBufferGroup group;
    MppEncCfg cfg;
    MPP_RET ret;

    double total_time = 0;
    int success_count = 0;
    
    // 初始化MPP JPEG编码器
    ret = mpp_create(&ctx, &mpi);
    if (ret != MPP_OK) {
        printf("   MPP创建失败: %d\n", ret);
        return;
    }else{
        printf("   MPP创建成功!   \n");
    }
    
    ret = mpp_init(ctx, MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
    if (ret != MPP_OK) {
        printf("   MPP初始化失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }else{
        printf("   MPP初始化成功!   \n");
    }
    
    // 配置编码参数
    mpp_enc_cfg_init(&cfg);
    mpp_enc_cfg_set_s32(cfg, "prep:width", IMAGE_WIDTH);
    mpp_enc_cfg_set_s32(cfg, "prep:height", IMAGE_HEIGHT);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);  // NV12格式
    mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", JPEG_QUALITY);    // JPEG质量

    mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    // 创建MPP内存池
    ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        printf("   MPP内存池创建失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }else{
        printf("   MPP内存池创建成功！\n");
    }

    ret =mpp_buffer_group_limit_config(group,YUV_SIZE,NUM_IMAGES);
    if (ret!=MPP_OK){
        printf("   MPP内存池限制失败，请重试！\n");
        return ;
    }else{
        printf("   MPP内存池限制成功！\n");
    }


    MppBuffer buffer = NULL;
    MppBuffer buffer2 =NULL;

    // 处理每张图像

    for (int i = 0; i < NUM_IMAGES; i++) {
        printf("   处理图像IMAGE(%d)...", i);
        // 分配MPP缓冲区
        ret = mpp_buffer_get(group, &buffer, YUV_SIZE);
        if (ret != MPP_OK || !buffer) {
            printf("     MPP缓冲区分配失败");
            printf("     问题是：ret=%d,buffer=%p",ret,buffer);
            continue;
        }else{
            printf("     MPP缓冲区分配成功!\n");
        }
        
        
        double start_time = get_us_time();
        void* mpp_ptr = mpp_buffer_get_ptr(buffer);
        printf("     mpp_ptr的地址为：%p\n",mpp_ptr);
        if (!mpp_ptr) {
            printf("     无法获取MPP缓冲区指针\n");
            mpp_buffer_put(buffer);
            continue;
        }else{
            printf("     成功获取MPP缓冲区指针\n");
        }
        //读取数据
        unsigned char* yuv_data = NULL;
        if (read_yuv_file(yuv_files[i], &yuv_data) != 0) {
            printf("     YUV文件读取失败\n");
            mpp_buffer_put(buffer);
            continue;
        }else{
            printf("     YUV读取成功\n");
        }

        // 将数据复制到MPP缓冲区
        printf("     开始计时！\n");
        mpp_buffer_sync_begin(buffer);
        printf("     同步开始成功！！\n");
        memcpy(mpp_ptr, yuv_data, YUV_SIZE);
        printf("     拷贝成功！！\n");
        mpp_buffer_sync_end(buffer);
        printf("     同步结束成功！！\n");

        // 准备MPP帧
        MppFrame frame;
        mpp_frame_init(&frame);
        mpp_frame_set_buffer(frame, buffer);
        mpp_frame_set_width(frame, IMAGE_WIDTH);
        mpp_frame_set_height(frame, IMAGE_HEIGHT);
        mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
        printf("     准备MPP帧成功！\n");
 
        printf("     准备进入性能测试！！\n");

        MppPacket packet;
        ret = mpp_buffer_get(group,&buffer2,YUV_SIZE);
        if (ret != MPP_OK || !buffer2) {
            printf("     MPP_packet缓冲区分配失败\n");
            printf("     问题是：ret=%d,buffer2=%p",ret,buffer2);
            continue;
        }else{
            printf("     MPP_packet缓冲区分配成功!\n");
        }

        //给packet分配缓冲区
        void *mpp_ptr2=mpp_buffer_get_ptr(buffer2);
        ret=mpp_packet_init(packet,mpp_ptr2,YUV_SIZE);
        mpp_packet_init_with_buffer(&packet, buffer2);

        // JPEG编码
        ret=mpi->encode(ctx,frame,packet);
        //ret=mpi->encode_put_frame(ctx, frame);
        if (ret != MPP_OK) {
            printf("     编码失败: %d \n", ret);
            free(yuv_data);
            mpp_buffer_put(buffer);
            mpp_frame_deinit(&frame);
            continue;
        }else{
            printf("     编码成功！\n\n");
            
        }

        void* jpeg_data = mpp_packet_get_pos(packet);
        size_t jpeg_size = mpp_packet_get_length(packet);

        printf("     此次JPEG图像大小为：%ld\n",jpeg_size);
        ret=write_data_to_file(jpeg_sw_files[i], jpeg_data, jpeg_size);
        // 保存JPEG文件
        if (!ret) {
            success_count++;
            printf("     ✅ 保存成功\n\n\n");
        } else {
            printf("     ❌❌ 保存失败\n\n");
            diagnose_save_failure(jpeg_files[i], jpeg_data, jpeg_size);
        }
        
    
        double end_time = get_us_time();       
        total_time = (end_time - start_time)/1000;   // 转换为毫秒
        // 清理资源
        free(yuv_data);
        mpp_packet_deinit(&packet);
        mpp_frame_deinit(&frame);
        mpp_buffer_put(buffer);
        mpp_buffer_put(buffer2);

    }

    // 清理MPP资源
    ret=mpp_enc_cfg_deinit(cfg);
    if (ret!=MPP_OK)
    {
        printf("     编码配置释放失败\n");
    }else{
        printf("     编码配置释放成功！\n");
    }

    ret=mpp_buffer_group_put(group);
    if (ret!=MPP_OK)
    {
        printf("     缓冲组释放失败\n");
    }else{
        printf("     缓冲组释放成功！\n");
    }

    ret=mpp_destroy(ctx);
    if (ret!=MPP_OK)
    {
        printf("     MPP释放失败\n");
    }else{
        printf("     MPP释放成功！\n");
    }

    //打印最终结果
    printf("   成功编码: %d/%d 图像\n", success_count, NUM_IMAGES);
    printf("   总处理时间: %.2f ms\n", total_time);
    printf("   平均每图像处理时间: %.2f ms\n", total_time / NUM_IMAGES);
    printf("   理论帧率: %.1f FPS\n\n", 1000.0 / (total_time / NUM_IMAGES));
}
*/


// 使用软件JPEG编码（完全软件方案）
void test_pure_software_jpeg(void) {
    printf("\n=== 纯软件JPEG编码测试（参考基准）===\n");
    // 首先检查ffmpeg是否可用
    printf("   检查ffmpeg是否可用...\n");
    int ffmpeg_check = system("which ffmpeg > /dev/null 2>&1");
    if (ffmpeg_check != 0) {
        printf("   ❌ ffmpeg未安装或不在PATH中\n");
        printf("   请安装ffmpeg: sudo apt install ffmpeg\n");
        return;
    }
    printf("   ✅ ffmpeg可用\n");

    // 注意：这里使用系统工具作为参考，实际应用中可以使用libjpeg等库
    double total_time = 0;
    int success_count = 0;

    for (int i = 0; i < NUM_IMAGES; i++) {
        printf("   处理图像 %d...\n", i);
        
        // 读取YUV数据
        unsigned char* yuv_data = NULL;
        yuv_data = (unsigned char*)malloc(YUV_SIZE);
        if (!yuv_data) {
            printf("   内存分配失败\n");
        }else{
            printf("   内存分配成功！\n");
        }

        if (read_yuv_file(yuv_files[i], &yuv_data) != 0) {
            printf("   YUV文件读取失败\n");
            continue;
        }else{
            printf("   读取成功！\n");
        }
        
        double start_time = get_us_time();
        printf("   开始计时！\n");
        // 使用系统工具进行JPEG编码（模拟软件编码）
        char cmd[512];
        snprintf(cmd, sizeof(cmd), 
                 "ffmpeg -f rawvideo -pix_fmt nv12 -s %dx%d -i - -qscale %d -y %s 2>/dev/null",
                 IMAGE_WIDTH, IMAGE_HEIGHT, JPEG_QUALITY, jpeg_sw_files[i]);
        printf("   执行命令: %s\n", cmd);
        FILE* ffmpeg = popen(cmd, "w");
        if (!ffmpeg) {
            printf("   无法启动ffmpeg\n");
            free(yuv_data);
            continue;
        }else{
            printf("   ffmpeg打开成功！\n");
        }
        
        size_t bytes_written = fwrite(yuv_data, 1, YUV_SIZE, ffmpeg);
        printf("   写入数据: %zu/%d 字节\n", bytes_written, YUV_SIZE);
        if (bytes_written != YUV_SIZE) {
            printf("   ⚠️ 数据写入不完整\n");
        }else{
            printf("   写入成功！\n");
        }
        
        int ret = pclose(ffmpeg);
        double end_time = get_us_time();
        
        if (ret == 0) {
            success_count++;
            printf(" ✅ 成功\n");
        } else {
            printf(" ❌❌ 失败\n");
        }
        
        total_time = (end_time - start_time) / 1000.0;  // 转换为毫秒
        free(yuv_data);
    }
    
    printf("   成功编码: %d/%d 图像\n", success_count, NUM_IMAGES);
    printf("   总处理时间: %.2f ms\n", total_time);
    printf("   平均每图像处理时间: %.2f ms\n", total_time / NUM_IMAGES);
    printf("   理论帧率: %.1f FPS\n\n", 1000.0 / (total_time / NUM_IMAGES));
}


/*
// JPEG解码测试（MPP硬件解码）
void test_mpp_jpeg_decoding(void) {
    printf("\n=== MPP硬件JPEG解码测试 ===\n");
    
    MppCtx ctx;
    MppApi *mpi;
    MppBufferGroup group;
    MPP_RET ret;
    
    double total_time = 0;
    int success_count = 0;
    
    // 初始化MPP JPEG解码器
    ret = mpp_create(&ctx, &mpi);
    if (ret != MPP_OK) {
        printf("   MPP创建失败: %d\n", ret);
        return;
    }
    
    ret = mpp_init(ctx, MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG);
    if (ret != MPP_OK) {
        printf("   MPP初始化失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }
    
    // 创建MPP内存池
    ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        printf("   MPP内存池创建失败: %d\n", ret);
        mpp_destroy(ctx);
        return;
    }
    
    // 处理每张JPEG图像
    for (int i = 0; i < NUM_IMAGES; i++) {
        printf("   解码图像 %d...", i);
        
        // 读取JPEG文件
        FILE* jpeg_file = fopen(jpeg_files[i], "rb");
        if (!jpeg_file) {
            printf("   无法打开JPEG文件\n");
            continue;
        }
        // 开始性能测量
        double start_time = get_us_time();
        fseek(jpeg_file, 0, SEEK_END);
        long jpeg_size = ftell(jpeg_file);
        fseek(jpeg_file, 0, SEEK_SET);
        
        unsigned char* jpeg_data = (unsigned char*)malloc(jpeg_size);
        if (!jpeg_data) {
            printf("   内存分配失败\n");
            fclose(jpeg_file);
            continue;
        }
        
        fread(jpeg_data, 1, jpeg_size, jpeg_file);
        fclose(jpeg_file);
        

        
        // 准备MPP包
        MppPacket packet;
        mpp_packet_init(&packet, jpeg_data, jpeg_size);
        mpp_packet_set_pos(packet, jpeg_data);
        mpp_packet_set_length(packet, jpeg_size);
        
        // 发送包到解码器
        ret = mpi->decode_put_packet(ctx, packet);
        if (ret != MPP_OK) {
            printf("   发送包失败: %d\n", ret);
            free(jpeg_data);
            mpp_packet_deinit(&packet);
            continue;
        }
        
        // 获取解码后的帧
        MppFrame frame;
        ret = mpi->decode_get_frame(ctx, &frame);
        if (ret != MPP_OK || !frame) {
            printf("   获取帧失败: %d\n", ret);
            free(jpeg_data);
            mpp_packet_deinit(&packet);
            continue;
        }
        
        // 检查帧状态
        if (mpp_frame_get_errinfo(frame)) {
            printf("   解码错误\n");
            free(jpeg_data);
            mpp_packet_deinit(&packet);
            mpp_frame_deinit(&frame);
            continue;
        }
        
        // 获取解码后的YUV数据
        void* yuv_data = mpp_frame_get_buffer(frame);
        size_t yuv_size = mpp_frame_get_buf_size(frame);
        
        if (yuv_data && yuv_size > 0) {
            // 保存解码后的YUV数据（可选）
            char out_filename[64];
            snprintf(out_filename, sizeof(out_filename), "/mnt/sdcard/decoded%d.yuv", i);
            
            if (write_data_to_file(out_filename, yuv_data, yuv_size) == 0) {
                success_count++;
                printf(" ✅ 成功\n");
            } else {
                printf(" ❌❌ 保存失败\n");
            }
        } else {
            printf("   无效的解码数据\n");
        }
        
        double end_time = get_us_time();
        total_time += (end_time - start_time) / 1000.0;  // 转换为毫秒
        
        // 清理资源
        free(jpeg_data);
        mpp_packet_deinit(&packet);
        mpp_frame_deinit(&frame);
    }
    
    // 清理MPP资源
    mpp_buffer_group_put(group);
    mpp_destroy(ctx);
    
    printf("   成功解码: %d/%d 图像\n", success_count, NUM_IMAGES);
    printf("   总处理时间: %.2f ms\n", total_time);
    printf("   平均每图像处理时间: %.2f ms\n", total_time / NUM_IMAGES);
    printf("   理论帧率: %.1f FPS\n\n", 1000.0 / (total_time / NUM_IMAGES));
}
*/