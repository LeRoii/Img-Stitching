# Img-Stitching using opencv

## Requirement
- opencv >= 3.4.0  
- opencv contrib >= 3.4.0  
- cuda >= 10.2 (if you want real time stitching)  

## USE
```
mkdir build
cd build
cmake ..
make
./master
```

## TDL and memo
- [x] API 100%  初版完成
- [ ] stitch all  
- [x] 最远检测距离，使用原图测试   
      拼接之后的图像分辨率太低无法检测较远距离的目标，最多不超过300m，使用原图可以达到300m   
- [ ] 是否要加qt？？  
- [ ] 增加方位指示    
- [x] 原始图像crop, crop之后的检测效果比原图的好 
- [ ] 修改cansender为canmanager  
8个一起拼无法初始化，bundle adjustment的时候会报错，因为第8张图和第1张，第7张都有公共区域  
7个一起拼效果不好，接缝明显  
暂时还是4个一起拼  
- [x] 相机参数也写成配置文件？  
暂时好像不需要
- [x] 针对不同相机的处理方式用宏定义来区分，避免两套代码  
- [ ] udp_publisher是一个全局变量，需要优化
- [ ] 硬件绑定



## Release note
11.18.   
- 现在可以输出1080和960分辨率下的矫正图像   

11.22.  
- 点击全景画面可以出现鼠标所在位置的对应相机的原始图像  

12.03.   
- 增加发送can消息的类cansender，可以发送特定格式的目标检测结果，后续改为canmanager，增加接受消息的功能
- API接口增加设置模型路径和相机配置文件路径的功能   
- API接口增加设置相机输出图像尺寸的接口，这样在更换相机时可以避免重新编译库文件  
- 增加dev分支，新功能开发在dev上进行，保证master分支相对稳定，无bug  

12.07.
- API增加读取yaml配置文件的接口，可以设置相机分辨率，模型位置，相机配置文件的位置和can接口  

12.09.  
- 源码部分增加stitcher.yaml，可以通过配置文件设置相机分辨率，拼接分辨率，模型位置和相机配置文件的位置  

12.21.
- 增加对imx390相机的支持，增加相机内参和畸变参数  

12.22.
- 增加宏定义用于处理不同型号相机的读图和拼接  

12.23.
- 使用gpu处理读图之后的去畸变、crop和resize操作，从输出的log看能加速10ms左右，但是总体提升不大，主要瓶颈还是在相机读图    

12.24.
- 尝试deepstream，确实可以同时处理8路视频（包括解码和检测），不过样例中处理的是同一个视频文件，不涉及读实时相机画面，有同时处理30路视频的样例，不过xavier上跑不起来。用deepstream试着同时读多路相机画面，只有4路的时候已经非常卡，帧率降到15，比自己写的程序效果还差。deepstream似乎更擅长处理视频编解码和神经网络推理，对实时读相机视频流没有很大帮助，并且代码写得非常复杂，使用起来成本巨大。

12.27.
- 库文件增加版本号1.0.0

01.04.
- 目前同时运行8个camshow，延时大概在150ms左右，与camera_v4l2_cuda延时相近，主要改进一个是显示部分用NvEglRenderer替代opnecv，另一个是在NvBufferTransform时保持输入与输出分辨率一致，后一个因素对延迟的影响很大，之前`NvBufferTransform`输出的分辨率是输入的一半，延迟大概在50-60ms

01.06.
- 延迟问题解决，效果与camera_v4l2_cuda相同，gpu去畸变有问题，多线程时同一个render会出现不同相机的画面，暂时用cpu去畸变
- 增加nvrenderer  

01.16.
- api改用nvrenderer显示

02.14.
- 增加replay功能，只支持图片replay   
- nvrender增加egl模式与opencv模式，显示接口还是改用nvrender，一般使用egl，opencv用于开发和debug   
- 增加nvcam去畸变开关，不做畸变矫正可以显著提高速度

02.16.
- 修改了多线程拼接的方式   

