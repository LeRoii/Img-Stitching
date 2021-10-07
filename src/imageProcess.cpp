#include "imageProcess.h"



tk::dnn::Yolo3Detection detNN;

int n_batch = 1;
std::string net ="/home/nvidia/ssd/code/cameracap/cfg/yolo4_berkeley_fp16.rt" ; //yolo4_320_fp16.rt（44ms, double detect）, yolo4_berkeley_fp16.rt(64ms),  kitti_yolo4_int8.rt 

std::vector<int> detret;

jetsonEncoder nvEncoder;



cv::Mat imagePorcessor::getROIimage(cv::Mat srcImg)
{

    std::cout<<"image_width: "<<srcImg.cols<<"   image_height: "<<srcImg.rows<<std::endl;
    int width = srcImg.cols;
    int height = 2* srcImg.rows;

    cv::Mat desImg(height, width, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat imageROI;
    imageROI = desImg(cv::Rect(0, 0, srcImg.cols, srcImg.rows));
    // cv::Mat mask = cv::imread(file, 0);
    cv::Mat mask(srcImg.size(),CV_8UC1);
    cv::cvtColor(srcImg,mask,CV_RGB2GRAY);
    srcImg.copyTo(imageROI, mask);
    return desImg;
}

cv::Mat imagePorcessor::ImageDetect(cv::Mat img)
{
    std::vector<cv::Mat> batch_frame;
    std::vector<cv::Mat> batch_dnn_input;
    std::vector<std::string> classnames;

    batch_dnn_input.clear();
    batch_frame.clear();

    batch_frame.push_back(img);
    // this will be resized to the net format
    batch_dnn_input.push_back(img.clone());

    auto start = std::chrono::steady_clock::now();

    detNN.update(batch_dnn_input, n_batch);
    detret.clear();
    // detNN.draw(batch_frame);
    detNN.draw(batch_frame, detret, classnames);
    printf("detNN draw_boxes okkkkk\n");
    printf("detret size:%d\n",  detret.size());   //x,y,w,h,class,probality

    return batch_frame.back();
}

void imagePorcessor::cut_img(cv::Mat src_img,std::vector<cv::Mat> &ceil_img)
{  
    // int t = m * n; 
    int height = src_img.rows;  
    int width  = src_img.cols;  
    cv::Mat roi_img,tmp_img;  

    cv::Rect rect1(0,0,width/2,height);  
    src_img(rect1).copyTo(tmp_img);
    ceil_img.push_back(tmp_img);

    cv::Rect rect2(width/2,0,width/2,height);  
    src_img(rect2).copyTo(roi_img);
    ceil_img.push_back(roi_img);
}  

cv::Mat imagePorcessor::channel_process(cv::Mat R) {
    cv::Mat ret;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2, cv::Size(20, 20)); //0.01s
    clahe->apply(R, ret);
    return ret;
}

cv::Mat imagePorcessor::SSR(cv::Mat input) {
    cv::Mat ret;
    std::vector<cv::Mat> img;
    cv::split(input, img);
    cv::Mat B = img[0];
    cv::Mat G = img[1];
    cv::Mat R = img[2];
    cv::Mat channels[3] = { channel_process(B), channel_process(G), channel_process(R)};
    cv::merge(channels, 3, ret);
    return ret;
}


cv::Mat imagePorcessor::processImage(std::vector<cv::Mat> ceil_img) {
    cv::Mat roi_img1=ImageDetect(ceil_img[0]);
    cv::Mat roi_img2=ImageDetect(ceil_img[1]);

    int w1 = roi_img1.cols; int h1 = roi_img1.rows;
    int w2 = roi_img2.cols; int h2 = roi_img2.rows;
    int width = w1 + w2; int height = max(h1, h2);

    cv::Mat  resultImg = cv::Mat(height, width, CV_8UC3, cv::Scalar::all(0));

    cv::Mat ROI_1 = resultImg(cv::Rect(0, 0, w1, h1));
    cv::Mat ROI_2 = resultImg(cv::Rect(w1, 0, w2, h2));
    roi_img1.copyTo(ROI_1);
    roi_img2.copyTo(ROI_2);
    // resultImg =  resultImg(cv::Rect(0, 0, resultImg.cols, resultImg.rows/2));
    return resultImg;
}

imagePorcessor::imagePorcessor() {
    int n_classes = 80;
    float conf_thresh=0.8;
    detNN.init(net, n_classes, n_batch, conf_thresh);
   printf("detNN init okkkkk\n");
}

cv::Mat imagePorcessor::Process(cv::Mat img){
    std::vector<cv::Mat>  ceil_img;
    cv::Mat  yolo_result;

    cut_img(img, ceil_img);

    yolo_result = processImage(ceil_img);
    // publishImage(yolo_result);
    return yolo_result;
}


void imagePorcessor::publishImage(cv::Mat img)
{

    targetInfo sendData;
    cv::Mat yuvImg;
    cv::resize(img, img, cv::Size(1920,720));
    cvtColor(img, yuvImg,CV_BGR2YUV_I420);

    for(int i=0;i<sizeof(detret)/6;i++){
        sendData.target_header=0xFFEEAABB;
        sendData.target_num = sizeof(detret)/6;
        sendData.target_id[i]=i;
        sendData.target_x[i]=detret[i];
        sendData.target_y[i]=detret[i+1];
        sendData.target_w[i]=detret[i+2];
        sendData.target_h[i]=detret[i+3];
        sendData.target_velocity[i]=detret[i+4];;      

    }
    // for(int i=0;i<20;i++){
    //     sendData.target_header=0xFFEEAABB;
    //     sendData.target_id[i]=i;
    //     sendData.target_x[i]=10*i;
    //     sendData.target_y[i]=15*i;
    //     sendData.target_w[i]=5*i;
    //     sendData.target_h[i]=3*i;
    //     sendData.target_velocity[i]=11.5*i;      
    // }
    nvEncoder.pubTargetData(sendData);
    nvEncoder.encodeFrame(yuvImg.data);

}