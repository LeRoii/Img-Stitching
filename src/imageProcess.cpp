#include "imageProcess.h"


/* can define start */
#define sendPos 0x421
#define SERV_PORT 33332 
#define CANPORT "can1"
int can_socket_fd;
unsigned long nbytes;
/* can define end */

tk::dnn::Yolo3Detection detNN;

int n_batch = 1;
std::string net ="/home/nvidia/ssd/code/cameracap/cfg/yolo4_berkeley_fp16.rt" ; //yolo4_320_fp16.rt（44ms, double detect）, yolo4_berkeley_fp16.rt(64ms),  kitti_yolo4_int8.rt 


targetInfo sendData;

jetsonEncoder nvEncoder;



void canInit()
{
	struct sockaddr_can addr;
	struct ifreq ifrr;

	can_socket_fd = socket(PF_CAN,SOCK_RAW,CAN_RAW);
	strcpy(ifrr.ifr_name,CANPORT);
	ioctl(can_socket_fd,SIOCGIFINDEX,&ifrr);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifrr.ifr_ifindex;
	bind(can_socket_fd, (struct sockaddr *)&addr, sizeof(addr));
}

int canSend(std::vector<int> temp_data){
    int length = temp_data.size()/6;

    struct can_frame can_send_pos[length];


    for(int i=0;i<length;i++){ 
        can_send_pos[i].can_id = sendPos+i;
        can_send_pos[i].can_dlc = 8;
        can_send_pos[i].data[0]=temp_data[6*i];    //x高8位
        can_send_pos[i].data[1]=temp_data[6*i]>>8;   //x低8位
        can_send_pos[i].data[2]=temp_data[6*i+1];    //y高8位
        can_send_pos[i].data[3]=temp_data[6*i+1]>>8;   //y低8位
        can_send_pos[i].data[4]=temp_data[6*i+2];    //w高8位
        can_send_pos[i].data[5]=temp_data[6*i+2]>>8;   //w低8位
        can_send_pos[i].data[6]=temp_data[6*i+3];    //h高8位
        can_send_pos[i].data[7]=temp_data[6*i+3]>>8;   //h低8位
        nbytes = write(can_socket_fd, &can_send_pos[i], sizeof(can_send_pos[i]));        
    }
    
}

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

cv::Mat imageProcessor::ImageDetect(cv::Mat img, std::vector<int> &detret)
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

void imageProcessor::cut_img(cv::Mat src_img,std::vector<cv::Mat> &ceil_img)
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
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2, cv::Size(20, 20)); //0.01s
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


cv::Mat imageProcessor::processImage(std::vector<cv::Mat> ceil_img) {
    std::vector<int> detret_left;
    std::vector<int> detret_right;
    std::vector<int> detret_all;
    detret_left.clear();
    detret_right.clear();
    cv::Mat roi_img1=ImageDetect(ceil_img[0], detret_left);
    cv::Mat roi_img2=ImageDetect(ceil_img[1], detret_right);

    //=======get target bbox, and send it to server========//
    sendData.target_header=0xFFEEAABB;
    sendData.target_num = detret_left.size()/6 + detret_right.size()/6;
    if(detret_left.size()>=6 ){
        for(int i=0;i<detret_left.size()/6;i++){
            sendData.target_id[i]=i;
            sendData.target_x[i]=detret_left[6*i];
            sendData.target_y[i]=detret_left[6*i+1];
            sendData.target_w[i]=detret_left[6*i+2];
            sendData.target_h[i]=detret_left[6*i+3];
            sendData.target_class[i]=detret_left[6*i+4]; 
            sendData.target_prob[i]=detret_left[6*i+5];
        }
    }
    if(detret_right.size()>=6){
        for(int i=0; i< detret_right.size()/6;i++){
            sendData.target_id[i+detret_left.size()/6]=i+detret_left.size()/6;
            sendData.target_x[i+detret_left.size()/6]=detret_right[6*i]+760;
            sendData.target_y[i+detret_left.size()/6]=detret_right[6*i+1];
            sendData.target_w[i+detret_left.size()/6]=detret_right[6*i+2];
            sendData.target_h[i+detret_left.size()/6]=detret_right[6*i+3];
            sendData.target_class[i+detret_left.size()/6]=detret_right[6*i+4]; 
            sendData.target_prob[i+detret_left.size()/6]=detret_right[6*i+5];

        } 
    }

    nvEncoder.pubTargetData(sendData);

    detret_all.insert(detret_all.end(),detret_left.begin(),detret_left.end());
    detret_all.insert(detret_all.end(),detret_right.begin(),detret_right.end());

    canSend(detret_all);

    int w1 = roi_img1.cols; int h1 = roi_img1.rows;
    int w2 = roi_img2.cols; int h2 = roi_img2.rows;
    int width = w1 + w2; int height = max(h1, h2);

    cv::Mat  resultImg = cv::Mat(height, width, CV_8UC3, cv::Scalar::all(0));

    cv::Mat ROI_1 = resultImg(cv::Rect(0, 0, w1, h1));
    cv::Mat ROI_2 = resultImg(cv::Rect(w1, 0, w2, h2));
    roi_img1.copyTo(ROI_1);
    roi_img2.copyTo(ROI_2);
    return resultImg;
}



cv::Mat imageProcessor::Process(cv::Mat img){
    std::vector<cv::Mat>  ceil_img;
    cv::Mat  yolo_result;

    cut_img(img, ceil_img);

    yolo_result = processImage(ceil_img);
    // publishImage(yolo_result);
    return yolo_result;
}


void imageProcessor::publishImage(cv::Mat img)
{


    cv::Mat yuvImg;
    cv::resize(img, img, cv::Size(1920,720));
    cvtColor(img, yuvImg,CV_BGR2YUV_I420);

    nvEncoder.encodeFrame(yuvImg.data);

}

controlData imageProcessor::getCtlCommand(){
    controlData ctl_data;
    ctl_data = nvEncoder.getControlData();
    return ctl_data;
}
//Init here
imageProcessor::imageProcessor() {
    canInit();
    printf("can init ok!\n");
    int n_classes = 80;
    float conf_thresh=0.8;
    detNN.init(net, n_classes, n_batch, conf_thresh);
   printf("detNN init okkkkk\n");
}