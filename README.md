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
- [ ] API 40%
- [ ] stitch all  
- [ ] 最远检测距离，使用原图测试   
- [ ] 是否要加qt？？  
- [ ] 增加方位指示    
- [ ] 原始图像crop, crop之后的检测效果比原图的好   
8个一起拼无法初始化，bundle adjustment的时候会报错，因为第8张图和第1张，第7张都有公共区域  
7个一起拼效果不好，接缝明显  
暂时还是4个一起拼  


## Release note
11.18.   现在可以输出1080和960分辨率下的矫正图像  
11.22.   点击全景画面可以出现鼠标所在位置的对应相机的原始图像   

