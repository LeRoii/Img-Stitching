#include "imageProcess.h"


/* can define start */
#define sendPos_id 0x421
#define sendAngle_id 0x321

#define recvCmd1_id 0x221
#define recvCmd2_id 0x222

#define SERV_PORT 33332 
#define CANPORT "can0"
int can_socket_fd;
unsigned long nbytes;
/* can define end */
int angle_x;  //第一个目标中心坐标-角度
int angle_y;  //第一个目标中心坐标-角度



// int n_batch = 1;



cv::Mat imageProcessor::getROIimage(cv::Mat srcImg)
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

cv::Mat imageProcessor::ImageDetect(cv::Mat &img, std::vector<int> &detret)
{
    static int ii = 0;
    cv::Mat tmp = img.clone();
    static vector<int> lastret;
    std::vector<cv::Mat> batch_frame;
    std::vector<cv::Mat> batch_dnn_input;
    std::vector<std::string> classnames;

    batch_dnn_input.clear();
    batch_frame.clear();

    batch_frame.push_back(tmp);
    // batch_frame.push_back(tmp);
    // this will be resized to the net format
    batch_dnn_input.push_back(tmp);
    // batch_dnn_input.push_back(tmp);

    detNN.update(batch_dnn_input, n_batch);
    detret.clear();

    spdlog::debug("detNN ret size:{}", detNN.batchDetected.size());
    // detNN.draw(batch_frame);
    detNN.draw(batch_frame, detret, classnames);
    spdlog::debug("detNN draw_boxes okkkkk");
    spdlog::debug("detret size:{}\n",  detret.size());   //x,y,w,h,class,probality

    // if(ii==0)
    // {
    //     lastret = detret;
    // }

    // if(ii == 1)
    // {
        
    //     ii=0;
    // }
    // else  ii++;

    // for(int i=0;i<lastret.size()/6;i++){
    //         int x0 = lastret[6*i];
    //         int y0 = lastret[6*i+1];
    //         int x1 =x0+lastret[6*i+2];
    //         int y1 = y0 + lastret[6*i+3];
    //         cv::rectangle(tmp, cv::Point(lastret[6*i], lastret[6*i+1]), cv::Point(x1, y1), cv::Scalar(0,255,0), 2); 
    //     }

    // return tmp;
    return batch_frame.back();
}

void imageProcessor::ImageDetect(std::vector<cv::Mat> &imgs, std::vector<std::vector<int>> &detret)
{
    std::vector<cv::Mat> batch_frame;
    std::vector<cv::Mat> batch_dnn_input;
    std::vector<std::string> classnames;

    batch_dnn_input.clear();
    batch_frame.clear();
    for(auto &img:imgs)
    {
        batch_frame.push_back(img);
        batch_dnn_input.push_back(img.clone());
    }

    detNN.update(batch_dnn_input, n_batch);
    detret.clear();
    detNN.draw(batch_frame, detret, classnames);
    spdlog::debug("draw box fini, batch size:{}, detret size:{}", n_batch, detret.size());
    // printf("detret size:%d\n",  detret.size());   //x,y,w,h,class,probality

    return ;
}

void imageProcessor::cut_img(cv::Mat &src_img, std::vector<cv::Mat> &ceil_img)
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

cv::Mat imageProcessor::channel_process(cv::Mat R) {
    cv::Mat ret;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(1.5, cv::Size(5,5)); //0.01s
    clahe->apply(R, ret);
    return ret;
}

cv::Mat imageProcessor::SSR(cv::Mat input) {
    cv::Mat ret,temp;
    cv::cvtColor(input, temp, CV_BGR2YUV);
    std::vector<cv::Mat> img;
    cv::split(temp, img);
    cv::Mat Y = img[0];
    cv::Mat U = img[1];
    cv::Mat V = img[2];
    cv::Mat channels[3] = { channel_process(Y), U, V};
    cv::merge(channels, 3, ret);
    cv::cvtColor(ret, ret, CV_YUV2BGR);
    return ret;
}


cv::Mat imageProcessor::processImage(std::vector<cv::Mat> &ceil_img) {
    std::vector<int> detret_left;
    std::vector<int> detret_right;
    std::vector<int> detret_all;
    char center_str[10]={0};
    detret_left.clear();
    detret_right.clear();
    cv::Mat roi_img1=ImageDetect(ceil_img[0], detret_left);
    cv::Mat roi_img2=ImageDetect(ceil_img[1], detret_right);


    detret_all.insert(detret_all.end(),detret_left.begin(),detret_left.end());
    detret_all.insert(detret_all.end(),detret_right.begin(),detret_right.end());

    // pCanSender->sendObjDetRet(detret_all);
    // canSend(detret_all);
    // canRecv();

    int w1 = roi_img1.cols; int h1 = roi_img1.rows;
    int w2 = roi_img2.cols; int h2 = roi_img2.rows;
    int width = w1 + w2; int height = max(h1, h2);

    cv::Mat  resultImg = cv::Mat(height, width, CV_8UC3, cv::Scalar::all(0));

    cv::Mat ROI_1 = resultImg(cv::Rect(0, 0, w1, h1));
    cv::Mat ROI_2 = resultImg(cv::Rect(w1, 0, w2, h2));
    roi_img1.copyTo(ROI_1);
    roi_img2.copyTo(ROI_2);
    if(detret_all.size()>=6){
        cv::Point p = cv::Point(detret_all[0]+detret_all[2]/2,detret_all[1]+detret_all[3]/2);
        sprintf(center_str,"%d, %d", angle_x/10, angle_y/10);
        cv::putText(resultImg, center_str, p, cv::FONT_HERSHEY_TRIPLEX, 0.4, cv::Scalar(0,255, 0), 1, CV_AA);
    }
    
    return resultImg;
}

cv::Mat imageProcessor::Process(cv::Mat &img){
    std::vector<cv::Mat>  ceil_img;
    cv::Mat  yolo_result;

    cut_img(img, ceil_img);

    yolo_result = processImage(ceil_img);
    // publishImage(yolo_result);
    return yolo_result;
}

cv::Mat imageProcessor::ProcessOnce(cv::Mat &img, std::vector<int> &detr){
    std::vector<cv::Mat>  ceil_img;
    cv::Mat  yolo_result;

    // std::vector<int> detret_all;
    char center_str[10]={0};
    cv::Mat ret = ImageDetect(img, detr);

    if(detr.size()>=6){
        cv::Point p = cv::Point(detr[0]+detr[2]/2,detr[1]+detr[3]/2);
        sprintf(center_str,"%d, %d", angle_x/10, angle_y/10);
        cv::putText(ret, center_str, p, cv::FONT_HERSHEY_TRIPLEX, 0.4, cv::Scalar(0,255, 0), 1, CV_AA);
    }

    // yolo_result = processImage(ceil_img);
    // publishImage(yolo_result);
    return ret;
}

cv::Mat imageProcessor::ProcessOnce(cv::Mat &img){
    std::vector<cv::Mat>  ceil_img;
    cv::Mat  yolo_result;

    std::vector<int> detr;
    char center_str[10]={0};
    cv::Mat ret = ImageDetect(img, detr);


    if(detr.size()>=6){
        cv::Point p = cv::Point(detr[0]+detr[2]/2,detr[1]+detr[3]/2);
        sprintf(center_str,"%d, %d", angle_x/10, angle_y/10);
        cv::putText(ret, center_str, p, cv::FONT_HERSHEY_TRIPLEX, 0.4, cv::Scalar(0,255, 0), 1, CV_AA);
    }

    // yolo_result = processImage(ceil_img);
    // publishImage(yolo_result);
    return ret;
}


void imageProcessor::publishImage(cv::Mat img)
{
    cv::Mat yuvImg;
    cv::resize(img, img, cv::Size(1280,720));

    cvtColor(img, yuvImg,CV_BGR2YUV_I420);

    // spdlog::warn("yuvImg size:{}", yuvImg.total()*yuvImg.elemSize());

    nvEncoder.encodeFrame(yuvImg.data); 
}

//Init here
imageProcessor::imageProcessor(std::string net, std::string canname, int batchsize):n_batch(batchsize) {
    pthread_t tid;
    // canInit();
    int n_classes = 80;
    float conf_thresh=0.8;
    detNN.init(net, n_classes, n_batch, conf_thresh);

    // pCanSender = new cansender(canname.c_str());

    // int ret = pthread_create(&tid, NULL, canRecv, NULL);  //为can接收程序创建线程
    // if (ret != 0)
    // {
    //    spdlog::warn("pthread_create error: error_code={}", ret);
    // }
   spdlog::debug("detNN init completed");
}