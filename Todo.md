<!--
 * @Author: your name
 * @Date: 2019-12-30 14:02:16
 * @LastEditTime : 2020-01-04 00:24:31
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /InfiniTAM/Todo.md
 -->
<!-- TOC -->

- [**测试原工程**](#测试原工程)
    - [1. 数据输入](#1-数据输入)
        - [1.1. 本地文件流](#11-本地文件流)
        - [1.2. openni设备视频流读取](#12-openni设备视频流读取)
        - [1.3. 视频流的操作都封装到基类imageSource里了，用getImages函数获取图像帧](#13-视频流的操作都封装到基类imagesource里了用getimages函数获取图像帧)
        - [1.4. 另适配多种相机设备：LibUVCEngine、RealSenseEngine、RealSense2Engine、Kinect2Engine、PicoFlexxEngine](#14-另适配多种相机设备libuvcenginerealsenseenginerealsense2enginekinect2enginepicoflexxengine)
- [**todo**](#todo)
    - [1. voxel类别修改](#1-voxel类别修改)
    - [2. 图片相关格式转换](#2-图片相关格式转换)
        - [2.1 扩展imageFileReader类](#21-扩展imagefilereader类)
        - [2.2 增加一个depthProvider类，以便独立封装深度恢复算法](#22-增加一个depthprovider类以便独立封装深度恢复算法)
        - [2.3 lable矩阵的读取](#23-lable矩阵的读取)
    - [3. 定位方法修改](#3-定位方法修改)
    - [4. 地图融合方式修改](#4-地图融合方式修改)
- [下一阶段：加入深度融合](#下一阶段加入深度融合)
    - [1.参考论文](#1参考论文)
        - [1.1. CNNSLAM](#11-cnnslam)
        - [1.2. MLM(CPU实时单目稠密建图)](#12-mlmcpu实时单目稠密建图)
    - [2.想法](#2想法)
    - [3.todo](#3todo)

<!-- /TOC -->

 
## **测试原工程**
>> 先找数据集在infiniTAM跑起来
### 1. 数据输入
<font color="#dd0000">**视频流读取(设备URL)或文件读取(pnm或png格式)**</font><br />

#### 1.1. 本地文件流
    ```
        if ((imageSource == NULL) && (filename2 != NULL))
        {
            printf("using rgb images: %s\nusing depth images: %s\n", filename1, filename2);
            if (filename_imu == NULL)
            {
                ImageMaskPathGenerator pathGenerator(filename1, filename2);
                imageSource = new ImageFileReader<ImageMaskPathGenerator>(calibFile, pathGenerator);#该函数还有个默认参数(0)——开始帧的序号
            }
            else
            {
                printf("using imu data: %s\n", filename_imu);
                imageSource = new RawFileReader(calibFile, filename1, filename2, Vector2i(320, 240), 0.5f);
                imuSource = new IMUSourceEngine(filename_imu);
            }

            if (imageSource->getDepthImageSize().x == 0)
            {
                delete imageSource;
                if (imuSource != NULL) delete imuSource;
                imuSource = NULL;
                imageSource = NULL;
            }
        }

        if ((imageSource == NULL) && (filename1 != NULL) && (filename_imu == NULL))
        {
            imageSource = new InputSource::FFMPEGReader(calibFile, filename1, filename2);
            if (imageSource->getDepthImageSize().x == 0)
            {
                delete imageSource;
                imageSource = NULL;
            }
        }
    ```

- ffmpeg 视频流读取
    ```
    ps：函数调用成功之后处理过的AVFormatContext结构体。
    file：打开的视音频流的URL。
    fmt：强制指定AVFormatContext中AVInputFormat的。这个参数一般情况下可以设置为NULL，这样FFmpeg可以自动检测AVInputFormat。
    
    int avformat_open_input(AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options);
    ```
    入口：InputSource::FFMPEGReader(calibFile, filename1, filename2);其中，filename1和filename2是两个设备URL

- 文件流读取 **png格式或pnm(ppm\pgm\pbm|黑白、灰度、彩色)格式**
    ```
    #fileName为 文件目录+currentNo
	FILE *f = fopen(fileName, "rb");
	if (f == NULL) return false;
	
    FormatType type = pnm_readheader(f, &xsize, &ysize, &binary);    
	if ((type != RGB_8u)&&(type != RGBA_8u)) {
		fclose(f);
		f = fopen(fileName, "rb");
		type = png_readheader(f, xsize, ysize, pngData);
		if ((type != RGB_8u)&&(type != RGBA_8u)) {//=====
			fclose(f);
			return false;
		}
		usepng = true;
	}
    ...
    
	short *data = new short[xsize*ysize];
	if (usepng) {
		if (!png_readdata(f, xsize, ysize, pngData, data)) { fclose(f); delete[] data; return false; }
	} else if (binary) {
		if (!pnm_readdata_binary(f, xsize, ysize, type, data)) { fclose(f); delete[] data; return false; }
	} else {
		if (!pnm_readdata_ascii(f, xsize, ysize, type, data)) { fclose(f); delete[] data; return false; }
	}
	fclose(f);
    ```

    ```
    //读取文件格式、size等信息
    static FormatType pnm_readheader(FILE *f, int *xsize, int *ysize, bool *binary)
    {
        char tmp[1024];
        FormatType type = FORMAT_UNKNOWN;
        int xs = 0, ys = 0, max_i = 0;
        bool isBinary = true;

        /* read identifier */
        if (fscanf(f, "%[^ \n\t]", tmp) != 1) return type;
        if (!strcmp(tmp, pgm_id)) type = MONO_8u;
        else if (!strcmp(tmp, pgm_ascii_id)) { type = MONO_8u; isBinary = false; }
        else if (!strcmp(tmp, ppm_id)) type = RGB_8u;
        else if (!strcmp(tmp, ppm_ascii_id)) { type = RGB_8u; isBinary = false; }
        else return type;

        /* read size */
        if (!fscanf(f, "%i", &xs)) return FORMAT_UNKNOWN;
        if (!fscanf(f, "%i", &ys)) return FORMAT_UNKNOWN;

        if (!fscanf(f, "%i", &max_i)) return FORMAT_UNKNOWN;
        if (max_i < 0) return FORMAT_UNKNOWN;
        else if (max_i <= (1 << 8)) {}
        else if ((max_i <= (1 << 15)) && (type == MONO_8u)) type = MONO_16s;
        else if ((max_i <= (1 << 16)) && (type == MONO_8u)) type = MONO_16u;
        else return FORMAT_UNKNOWN;
        fgetc(f);

        if (xsize) *xsize = xs;
        if (ysize) *ysize = ys;
        if (binary) *binary = isBinary;

        return type;
    }
    ```
    
#### 1.2. openni设备视频流读取
- ![openni基础操作](/home/xyt/segFusion/InfiniTAM/pics_md/openni.png)
- 图像读取入口：
    ```
    其中第一个参数为标定txt文件路径，第二个参数为设备URL(char*类型)，第三个参数指是否用设备内部的标定参数(布尔值)。
    imageSource = new OpenNIEngine(calibFile, filename1, useInternalCalibration);
    ```
#### 1.3. 视频流的操作都封装到基类imageSource里了，用getImages函数获取图像帧
    ```
        void UIEngine::ProcessFrame()
    {
        if (!imageSource->hasMoreImages()) return;
        imageSource->getImages(inputRGBImage, inputRawDepthImage);
        ...
    }
    ```
#### 1.4. 另适配多种相机设备：LibUVCEngine、RealSenseEngine、RealSense2Engine、Kinect2Engine、PicoFlexxEngine

## **todo** 
### 1. voxel类别修改
- "segFusion/InfiniTAM/InfiniTAM/ITMLib/Objects/Scene/ITMVoxelTypes.h"
- 新建一个Voxel类，含semantic有关属性
- 语义标签概率向量
- semantic_depth
### 2. 图片相关格式转换
- ITMUChar4Image rgb图像格式
- ITMShortImage depth图像格式
- 编辑格式转换函数
#### 2.1 扩展imageFileReader类
- 改getImage函数
- rgbd数据：用opencv读入cv::mat类型，完成格式转换cv::mat->ITMImage
- 双目数据：读入左右视图，深度恢复后赋值个depth图像
#### 2.2 增加一个depthProvider类，以便独立封装深度恢复算法
- computeDepthFromStereo函数，输入为左右图像，标定参数，输出左图深度图
- libelas  
elas输入为：uint8_t* I1_,uint8_t* I2_,float* D1,float* D2,const int32_t* dims
ITM-》cvmat(rgb转grey)-》uint8_t*->ITM  
- 编辑ITMChar4Image->cvmat|cvmat->uint8_t*|float*->ITMShortImage
- <font color="#dd0000">ORUtils::Image类里的GetData成员函数可获得T*格式图像数据，而且得到的数据非const可修改
memcpy函数即可实现各类转换
</font><br />
- cv::Mat的一些格式定义
```
typedef Mat_< uchar > 	cv::Mat1b //note uchar uint8_t
typedef Mat_< double > 	cv::Mat1d
typedef Mat_< float > 	cv::Mat1f
typedef Mat_< int > 	cv::Mat1i
typedef Mat_< short > 	cv::Mat1s
typedef Mat_< ushort > 	cv::Mat1w
typedef Mat_< Vec2b > 	cv::Mat2b
typedef Mat_< Vec2d > 	cv::Mat2d
```
- 修改文件间引用关系  
```
INCLUDE(${PROJECT_SOURCE_DIR}/cmake/SetCUDAAppTarget.cmake)
```  
在*.cmake文件里执行ADD_EXECUTABLE  
SOURCE_GROUP(name [FILES src1 src2 ...])--define a grouping for sources files in IDE project generation  
cmake语法中link_directories()增加链接库的目录，link_libraries全局路径/相对路径(目录下查找)  
FIND_PACKAGE()调用CMAKE_MODULE_PATH下的Find<name>.cmake模块  cmake自带了一些（cmake --help-module-list）

- 高级的写法
```
add_library(viso2 ${LIBVISO2_SRC_FILES})
set(Viso2_LIBS viso2 CACHE INTERNAL "")
```
#### 2.3 lable矩阵的读取

### 3. 定位方法修改
```
ITMTrackingState::TrackingResult ITMBasicEngine<TVoxel,TIndex>::ProcessFrame(ITMUChar4Image *rgbImage, ITMShortImage *rawDepthImage, ITMIMUMeasurement *imuMeasurement)
```
- 入口：ITMBasicEngine.tpp trackingController->Track(trackingState, view);//view存了当前帧的rgb图和depth图
- segFusion/InfiniTAM/InfiniTAM/ITMLib/Trackers/Interface/ITMTracker.h
- 增加一个ITMSemanticTracker类
- 迭代(义icp《-》语义概率融合)
### 4. 地图融合方式修改
- 入口：ITMBasicEngine.tpp denseMapper->ProcessFrame(view, trackingState, scene, renderState_live);
    ```	
    // allocation
    sceneRecoEngine->AllocateSceneFromDepth(scene, view, trackingState, renderState, false, resetVisibleList);

    // integration
    sceneRecoEngine->IntegrateIntoScene(scene, view, trackingState, renderState);
    ```
- 实现语义标签的概率融合

## 下一阶段：加入深度融合
### 1.参考论文
#### 1.1. CNNSLAM
- 直接法估计位姿
- 关键帧 CNN深度估计+语义分割
- 关键帧间深度估计结果融合(当前keyframe结果融合与最近keyframe结果融合)，D和U(uncertainty)用于后端优化  
![公式](/home/xyt/segFusion/InfiniTAM/pics_md/CNNSLAM_part1.png)  
![公式](/home/xyt/segFusion/InfiniTAM/pics_md/CNNSLAM_part2.png)  
![公式](/home/xyt/segFusion/InfiniTAM/pics_md/CNNSLAM_part3.png)  
- framewise立体匹配结果 融入 深度估计结果，更新关键帧深度  
![公式](/home/xyt/segFusion/InfiniTAM/pics_md/CNNSLAM_part4.png)
- 基于D和T，融合语义分割结果

#### 1.2. MLM(CPU实时单目稠密建图)
- 基于半直接法的初始位姿估计
- 基于初始位姿估计，多分辨率立体匹配(quadtree)获得全图深度
- holefilling 补全
- Triangulation and Rasterization 获得全像素深度 
- 平滑quadtree形式深度图
- 后端优化 光度误差和深度误差 更新D和pose

### 2.想法
1.利用pose 对比双目立体匹配和两个左图立体匹配的深度估计误差，融到后端优化,同时优化深度和位姿和语义<-local_bundle_ajustment  
pose<->深度图 pose<->语义图  
前端是语义icp+贝叶斯

### 3.todo 
- 查阅双目稠密建图文献，寻找是否有相关的深度refinement方法

