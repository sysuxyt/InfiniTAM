/*
 * @Author: your name
 * @Date: 2020-01-03 16:39:53
 * @LastEditTime : 2020-01-03 20:51:56
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /InfiniTAM/InfiniTAM/InputSource/DepthProvider.h
 */
#include "elas.h"
#include "../ITMLib/Utils/ITMImageTypes.h"


class DepthProvider{

public:
    void computeDepthFromStereo(const ITMUChar4Image &left_image, 
                                const ITMUChar4Image &right_image, 
                                ITMShortImage* depth_out){

        cv::Mat3b *left_rgb, *right_rgb;//opencv格式 rgb图像
        cv::Mat1b *left_gray, *right_gray;//opencv格式 灰度图像
        uint8_t *left_data, *right_data;//elas.process()的输入

        int32_t width = left_image.noDims[0];
        int32_t height = left_image.noDims[1];        

        float *D1_data = (float*)malloc(width*height*sizeof(float));//elas输出的左视图深度
        float *D2_data = (float*)malloc(width*height*sizeof(float));//elas输出的右视图深度

        //rgb图像转为灰度图像
        ItmToCv(left_image, left_rgb);
        ItmToCv(right_image, right_rgb);
        cv::cvtColor(*left_rgb, *left_gray, cv::COLOR_RGB2GRAY);
        cv::cvtColor(*right_rgb, *right_gray, cv::COLOR_RGB2GRAY);

        //将mat1b格式数据转为uint8_t*
        memcpy(left_data, left_gray->data, width * height * sizeof(uint8_t));
        memcpy(right_data, right_gray->data, width * height * sizeof(uint8_t));

        //用elas库计算深度
        Elas::parameters param;
        param.postprocess_only_left = false;
        Elas elas(param);
        
        const int32_t dims[3] = {width, height, width};
        elas.process(left_data, right_data, D1_data, D2_data, dims);
        
        //整理ITMUChar4Image格式深度图
    	ORUtils::Vector2<int> newSize(width, height);
        depth_out->ChangeDims(newSize);
        FloatDepthmapToShort(D1_data, depth_out);                            
    
    }

    //ITMChar4Image->CVMAT
    void ItmToCv(const ITMUChar4Image &itm, cv::Mat3b *out_mat) {
        // TODO(andrei): Suport resizing the matrix, if necessary.
        const Vector4u *itm_data = itm.GetData(MemoryDeviceType::MEMORYDEVICE_CPU);
        for (int i = 0; i < itm.noDims[1]; ++i) {
            for (int j = 0; j < itm.noDims[0]; ++j) {
            out_mat->at<cv::Vec3b>(i, j) = cv::Vec3b(
                itm_data[i * itm.noDims[0] + j].b,
                itm_data[i * itm.noDims[0] + j].g,
                itm_data[i * itm.noDims[0] + j].r
            );
            }
        }
    }

    //elas库深度图输出为float*格式，需转为ITMShortImage格式
    void FloatDepthmapToShort(const float *pixels, ITMShortImage *out_itm) {
        int16_t *itm_data = out_itm->GetData(MemoryDeviceType::MEMORYDEVICE_CPU);
        const int kMetersToMillimeters = 1000;
        for (int i = 0; i < out_itm.noDims[1]; ++i) {
            for (int j = 0; j < out_itm.noDims[0]; ++j) {
            itm_data[i * out_itm.noDims[0] + j] = static_cast<int16_t>(
                pixels[i * out_itm.noDims[0] + j] * kMetersToMillimeters
            );
            }
        }
    }  

};