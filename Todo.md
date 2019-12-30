<!--
 * @Author: your name
 * @Date: 2019-12-30 14:02:16
 * @LastEditTime: 2019-12-30 15:30:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /InfiniTAM/Todo.md
 -->
1. voxel类别修改
- "segFusion/InfiniTAM/InfiniTAM/ITMLib/Objects/Scene/ITMVoxelTypes.h"
- 新建一个Voxel类，含semantic有关属性
- 语义标签概率向量
- semantic_depth
2. 图片相关格式转换
- ITMUChar4Image rgb图像格式
- ITMShortImage depth图像格式
- 编辑格式转换函数
3. 定位方法修改
- 入口：ITMBasicEngine.tpp trackingController->Track(trackingState, view);
- segFusion/InfiniTAM/InfiniTAM/ITMLib/Trackers/Interface/ITMTracker.h
- 增加一个ITMSemanticTracker类
- 迭代(义icp《-》语义概率融合)
4. 地图融合方式修改
- 入口：ITMBasicEngine.tpp denseMapper->ProcessFrame(view, trackingState, scene, renderState_live);
```	
// allocation
sceneRecoEngine->AllocateSceneFromDepth(scene, view, trackingState, renderState, false, resetVisibleList);

// integration
sceneRecoEngine->IntegrateIntoScene(scene, view, trackingState, renderState);
```
- 实现语义标签的概率融合