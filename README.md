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
8个一起拼无法初始化，bundle adjustment的时候会报错，因为第8张图和第1张，第7张都有公共区域  
7个一起拼效果不好，接缝明显  
暂时还是4个一起拼  



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

