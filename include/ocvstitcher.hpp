#ifndef _OCVSTITCHER_HPP_
#define _OCVSTITCHER_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "opencv2/opencv_modules.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/stitching/detail/autocalib.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/timelapsers.hpp"
#include "opencv2/stitching/detail/camera.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/warpers.hpp"
#include "opencv2/stitching/warpers.hpp"

#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

using namespace std;
using namespace cv;
using namespace cv::detail;

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

//static int num_images = 4;

static std::mutex stmtx;

#if CENTRIC_STRUCT
// circle structure
// silver st
static std::string camParams[2] = {"487.808,0,320,0,487.808,180,0,0,1,0.358901,-0.00255477,-0.933372,-0.00515675,0.999976,-0.00471995,0.933361,0.00650716,0.358879,\
533.62,0,320,0,533.62,180,0,0,1,0.927762,-0.00397522,-0.373152,0.0130791,0.999675,0.0218688,0.372944,-0.0251695,0.927512,\
566.215,0,320,0,566.215,180,0,0,1,0.920505,0.0103068,0.390594,-0.0137577,0.999887,0.00603792,-0.390488,-0.0109316,0.920543,\
595.578,0,320,0,595.578,180,0,0,1,0.401075,0.014492,0.915931,0.00593535,0.999813,-0.0184182,-0.916026,0.0128234,0.400914,\
549.917",
"492.936,0,320,0,492.936,180,0,0,1,0.333934,0.00376102,-0.942589,-0.00390736,0.999989,0.00260578,0.942588,0.00281288,0.333945,\
531.666,0,320,0,531.666,180,0,0,1,0.920266,-0.00331873,-0.391279,-0.00138152,0.99993,-0.0117304,0.391291,0.0113356,0.920197,\
563.184,0,320,0,563.184,180,0,0,1,0.91131,0.00169592,0.411716,0.00588441,0.999836,-0.0171433,-0.411678,0.0180456,0.911151,\
605.804,0,320,0,605.804,180,0,0,1,0.386633,0.0122536,0.922152,-0.00720674,0.999921,-0.0102654,-0.922205,-0.00267676,0.386691,\
547.425"
};

//258 green st
// static std::string camParams[2] = {"5093.54,0,320,0,5093.54,180,0,0,1,0.990251,-0.0289489,-0.13625,-0.0162866,0.947392,-0.31966,0.138336,0.318763,0.937685,\
// 5062.47,0,320,0,5062.47,180,0,0,1,0.998381,-0.0367373,-0.043432,0.0209781,0.947459,-0.319187,0.0528762,0.317759,0.946696,\
// 4976.91,0,320,0,4976.91,180,0,0,1,0.998971,0.0103404,0.04415,0.00423479,0.948122,-0.317878,-0.0451466,0.317738,0.947103,\
// 4947.03,0,320,0,4947.03,180,0,0,1,0.989246,0.05499,0.135532,-0.0091451,0.948075,-0.317916,-0.145977,0.313257,0.938382,\
// 5019.69",
// "6157.41,0,320,0,6157.41,180,0,0,1,0.993221,-0.0372395,-0.110116,0.0193385,0.98703,-0.159369,0.114622,0.15616,0.981059,\
// 6149.73,0,320,0,6149.73,180,0,0,1,0.999114,0.0213781,-0.0362632,-0.0269055,0.986826,-0.159535,0.0323749,0.160369,0.986526,\
// 6150.96,0,320,0,6150.96,180,0,0,1,0.999231,0.0132883,0.036889,-0.00720142,0.987013,-0.160476,-0.0385424,0.160087,0.98635,\
// 6156.61,0,320,0,6156.61,180,0,0,1,0.993984,0.00266958,0.10949,0.0149602,0.987023,-0.159879,-0.108496,0.160555,0.981046,\
// 6153.79"
// };

#else if DISTRIBUTED_STRUCT
//205 structure
static std::string camParams[2] = {"448.947,0,320,0,448.947,180,0,0,1,0.184235,0.0264681,-0.982526,-0.00994292,0.999636,0.0250646,0.982832,0.00515138,0.184431,\
460.455,0,320,0,460.455,180,0,0,1,0.950664,-0.0259506,-0.309136,0.0291518,0.999559,0.00573974,0.308851,-0.0144684,0.951,\
473.401,0,320,0,473.401,180,0,0,1,0.948512,0.0235424,0.315866,-0.0292872,0.99948,0.0134523,-0.315385,-0.0220105,0.948708,\
494.546,0,320,0,494.546,180,0,0,1,0.21866,0.00304694,0.975796,0.0086784,0.999949,-0.00506705,-0.975762,0.00957631,0.218623,\
466.928",
"527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
543.86"
};

#endif

static void Stringsplit(string str, const char split, vector<string>& res)
{
    istringstream iss(str);	// 输入流
    string token;			// 接收缓冲区
    res.clear();
    while (getline(iss, token, split))	// 以split为分隔符
    {
        res.push_back(token);
    }
}

static float radian2degree(float x)
{
    return x*180/M_PI;
}
 
// Calculates rotation matrix to euler angles
// The result is the same as MATLAB except the order
// of the euler angles ( x and z are swapped ).
static Vec3f rotationMatrixToEulerAngles(Mat &m)
{
 
    // assert(isRotationMatrix(R));
     Mat R;
     m.convertTo(R, CV_64FC1);
    float sy = sqrt(R.at<double>(0,0) * R.at<double>(0,0) +  R.at<double>(1,0) * R.at<double>(1,0) );
 
    bool singular = sy < 1e-6; // If
 
    float x, y, z;
    if (!singular)
    {
        x = atan2(R.at<double>(2,1) , R.at<double>(2,2));
        y = atan2(-R.at<double>(2,0), sy);
        z = atan2(R.at<double>(1,0), R.at<double>(0,0));
    }
    else
    {
        x = atan2(-R.at<double>(1,2), R.at<double>(1,1));
        y = atan2(-R.at<double>(2,0), sy);
        z = 0;
    }
    return Vec3f(radian2degree(x), radian2degree(y), radian2degree(z));
}
class ocvStitcher
{
    public:
    ocvStitcher(stStitcherCfg cfg):
    m_imgWidth(cfg.width), m_imgHeight(cfg.height), m_id(cfg.id),num_images(cfg.num_images), m_cfgpath(cfg.cfgPath), 
    match_conf(cfg.matchConf), conf_thresh(cfg.adjusterConf), blend_strength(cfg.blendStrength),
    cameraExThres(cfg.stitchercameraExThres), cameraInThres(cfg.stitchercameraInThres)
    {

        finder = makePtr<SurfFeaturesFinder>();

        seam_work_aspect = min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));
        // seam_work_aspect = 1;//min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));

        // camK.reserve(num_images);
        corners.reserve(num_images); 
        blenderMask.reserve(num_images);

        cameraK = Mat(Size(3,3), CV_32FC1);
        for(int i=0;i<num_images;i++)
        {
            cameraR.push_back(Mat(Size(3,3), CV_32FC1));
            camK.push_back(Mat(Size(3,3), CV_32FC1));
        }

        useDefaultCamParams();
// #ifdef GENERATE_SO
//         useDefaultCamParams();
//         presetParaOk = true;
// #else
//         (initCamParams(m_cfgpath) == RET_OK) ? (presetParaOk = true) : (presetParaOk = false);
// #endif
        spdlog::debug("stitcher {} constructor completed!", m_id);
    }

    ~ocvStitcher()
    {

    }

    int verifyCamParams()
    {
        //compare cameras and camK & cameraR
        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameras[{}] R:",m_id, i);
        //     std::cout<<cameras[i].R<<endl;
        // }

        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameras[{}] angles:",m_id, i);
        //     Vec3f angles = rotationMatrixToEulerAngles(cameras[i].R);
        //     std::cout<<angles<<endl;
        // }
        
        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameraR[{}] matrix:",m_id, i);
        //     std::cout<<cameraR[i]<<endl;
        // }

        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameraR[{}] angles:",m_id, i);
        //     Vec3f angles = rotationMatrixToEulerAngles(cameraR[i]);
        //     std::cout<<angles<<endl;
        // }

        for(int i=0;i<num_images;i++)
        {
            Vec3f defaultAngle = rotationMatrixToEulerAngles(cameraR[i]);
            Vec3f estimatedAngle = rotationMatrixToEulerAngles(cameras[i].R);
            double diff = norm(defaultAngle, estimatedAngle);
            spdlog::debug("camera[{}] extrinsic diff:{}", i, diff);
            // std::cout<<"angle:"<<endl<<defaultAngle<<endl<<estimatedAngle<<endl;
            if(diff > cameraExThres)
            {
                spdlog::warn("environment is not suitable for calibration, init failed");
                return RET_ERR;
            }
            
            Vec2f defaultIntrinsic = Vec2f{camK[i].at<float>(0,0), camK[i].at<float>(1,1)};
            Vec2f estimatedIntrinsic = Vec2f{cameras[i].K().at<double>(0,0), cameras[i].K().at<double>(1,1)};
            // cout<<"instrinsic:"<<endl<<defaultIntrinsic<<endl<<estimatedIntrinsic<<endl;
            diff = norm(defaultIntrinsic, estimatedIntrinsic);
            spdlog::debug("camera[{}] intrinsic diff:{}", i, diff);
            if(diff > cameraInThres)
            {
                spdlog::warn("environment is not suitable for calibration, init failed");
                return RET_ERR;
            }
        }

        return RET_OK; 
        
    }

    int useDefaultCamParams()
    {
        vector<string> res;
        Stringsplit(camParams[m_id-1], ',', res);
        spdlog::debug("useDefaultCamParams,res size:{}", res.size());
        for(int i=0;i<num_images;i++)
        {
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    camK[i].at<float>(mi,mj) = stof(res[18*i+mi*3+mj]);
                    cameraR[i].at<float>(mi,mj) = stof(res[18*i+9+mi*3+mj]);
                }
            }

            // cout<<K[i]<<endl;
            // cout<<R[i]<<endl;
        }
        warped_image_scale = stof(res.back());

        return RET_OK;
    }

    int initCamParams(std::string cfgpath)
    {
        vector<string> res;
        string filename = cfgpath + "cameraparaout_" + to_string(m_id) + ".txt";
        std::ifstream fin(filename, std::ios::in);
        if(!fin.is_open())
        {
            spdlog::warn("no. {} can not open camerapara file, no preset parameters found, init all!", m_id);
            return RET_ERR;
        }
        std::string str;
        int linenum = 0;
        int paranum = 0;

        while(getline(fin,str))
        {
            linenum++;
            if (std::string::npos != str.find(":"))
            {
                paranum++;
            }
        }

        fin.clear();
        fin.seekg(0, ios::beg);

        spdlog::debug("linenum:{}, paranum:{}", linenum, paranum);
        // cout<<"linenum:"<<linenum<<",paranum:"<<paranum<<endl;
        if(linenum == 0)
        {
            spdlog::info("no. {} no preset parameters found, init all!", m_id);
            return RET_ERR;
        }

        for(int i=0;i<6*(paranum-1);i++)
            getline(fin,str);

        fin >> str;
        // cout << "params time:" << str << endl;
        spdlog::debug("params timestamp:{}", str);

        for(int i=0;i<num_images;i++)
        {
            fin >> str;
            Stringsplit(str, ',', res);
            if(res.size() != 18)
            {
                spdlog::warn("camera {} preset parameter incorrect, init all!", m_id);
                return RET_ERR;
            }

            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    camK[i].at<float>(mi,mj) = stof(res[mi*3+mj]);
                    cameraR[i].at<float>(mi,mj) = stof(res[9+mi*3+mj]);
                }
            }

            // cout<<camK[i]<<endl;
            // cout<<cameraR[i]<<endl;

        }
        fin >> str;
        warped_image_scale = stof(str);

        return RET_OK;
    }

    int saveCameraParams()
    {
        std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
        struct std::tm * ptm = std::localtime(&tt);
        // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';

        string filename = m_cfgpath + "cameraparaout_" + to_string(m_id) + ".txt";
        ofstream fout(filename, std::ofstream::out | std::ofstream::app);

        if(!fout.is_open())
        {
            spdlog::warn("no. {} can not open camerapara file, save camera para failed!", m_id);
            return RET_ERR;
        }

        fout << std::put_time(ptm,"%F-%H-%M-%S:\n");
        
        for(int idx=0;idx<num_images;idx++)
        {
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    fout << camK[idx].at<float>(mi,mj) << ",";
                    // cameraK.at<float>(mi,mj) = cameras[idx].K().at<double>(mi,mj);
                }
            }  
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    fout << cameraR[idx].at<float>(mi,mj) << ",";
                    // cameraR[idx].at<float>(mi,mj) = cameraR[idx].at<float>(mi,mj);
                }
            }
            fout << "\n";
        }
        fout << warped_image_scale << "\n";

        return RET_OK;
    }

    // init mode, 1:initall, 2:use default, init seam, 3:read cfg, init seam
    int init(vector<Mat> &imgs, int initMode)
    {
        // if(initMode || !presetParaOk)
        //     return initAll(imgs);
        // else
        //     return initSeam(imgs);

        if(enInitALL == initMode)
            return initAll(imgs);
        else if(enInitByDefault)
        {
            useDefaultCamParams();
            return initSeam(imgs);
        }
        else if(enInitByCfg)
        {
            if(RET_OK ==initCamParams(m_cfgpath))
                return initSeam(imgs);
            else
                return initAll(imgs);
        }
        else
        {
            spdlog::critical("invalid init mode, exit");
            return RET_ERR;
        }


    }

    int initAll(vector<Mat> &imgs)
    {
        spdlog::debug("***********stitcher {} init start**************", m_id);
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        vector<ImageFeatures> features(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);
        for(int i=0;i<num_images;i++)
        {
            // std::vector<cv::Rect> rois = {cv::Rect(m_imgWidth/2, 0, m_imgWidth/2, m_imgHeight)};
            // if(i == num_images - 1)
            //     (*finder)(imgs[i], features[i], rois);
            // else
                (*finder)(imgs[i], features[i]);

            features[i].img_idx = i;
            spdlog::debug("Features in image {} : {}", i, features[i].keypoints.size());

            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);
        }

        finder->collectGarbage();

        vector<MatchesInfo> pairwise_matches;
        matcher = makePtr<BestOf2NearestMatcher>(false, match_conf);

        (*matcher)(features, pairwise_matches);
        matcher->collectGarbage();

        // std::string save_graph_to = "11match.txt";
        // ofstream f(save_graph_to.c_str());
        // std::vector<String> img_names;
        // img_names.push_back("5.png");
        // img_names.push_back("6.png");
        // img_names.push_back("7.png");
        // img_names.push_back("8.png");
        // f << matchesGraphAsString(img_names, pairwise_matches, conf_thresh);

        estimator = makePtr<HomographyBasedEstimator>();

        if (!(*estimator)(features, pairwise_matches, cameras))
        {
            spdlog::critical("Homography estimation failed.");
            return -1;
        }

        // LOGLN("***********after estimator**************" << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        spdlog::debug("***********after estimator**************, {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        for (size_t i = 0; i < cameras.size(); ++i)
        {
            Mat R;
            cameras[i].R.convertTo(R, CV_32F);
            cameras[i].R = R;
            // LOGLN("Initial camera intrinsics #" << i << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
        }

        adjuster = makePtr<detail::BundleAdjusterRay>();
        adjuster->setConfThresh(conf_thresh);
        Mat_<uchar> refine_mask = Mat::zeros(3, 3, CV_8U);

        refine_mask(0,0) = 1;
        refine_mask(0,1) = 1;
        refine_mask(0,2) = 1;
        refine_mask(1,1) = 1;
        refine_mask(1,2) = 1;
        adjuster->setRefinementMask(refine_mask);
        if (!(*adjuster)(features, pairwise_matches, cameras))
        {
            spdlog::critical("Camera parameters adjusting failed.");
            return -1;
        }

        //  for (size_t i = 0; i < cameras.size(); ++i)
        // {
        //     LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
        // }

        // Find median focal length

        spdlog::debug("***********after Camera parameters adjusting Find median focal length**************");

        vector<double> focals;

        for (size_t i = 0; i < cameras.size(); ++i)
        {
            // LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
            focals.push_back(cameras[i].focal);
            // LOGLN("focal:"<<cameras[i].focal);
        }

        sort(focals.begin(), focals.end());
        float estimated_warped_image_scale;
        // float warped_image_scale;
        if (focals.size() % 2 == 1)
            estimated_warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
        else
            estimated_warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;
        
        // warped_image_scale = static_cast<float>(cameras[2].focal);

        /* takes about 0.1ms*/
        t = getTickCount();
        vector<Mat> rmats;
        for (size_t i = 0; i < cameras.size(); ++i)
            rmats.push_back(cameras[i].R.clone());
        waveCorrect(rmats, detail::WAVE_CORRECT_HORIZ);
        for (size_t i = 0; i < cameras.size(); ++i)
            cameras[i].R = rmats[i];

        // LOGLN("***********waveCorrect takes::**************" << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        spdlog::debug("***********waveCorrect takes::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        spdlog::debug("Warping images (auxiliary)... ");

        /*every camera use same intrinsic parameter, make no big difference*/
        
        // for (size_t i = 0; i < cameras.size(); ++i)
        // {
        //     // LOGLN("Initial camera intrinsics #" << i << ":\nK:\n" << cameras[i].K());
        //     // cameras[i] = cameras[0];
        //     // cameras[i].R = rmats[i];
        //     LOGLN("camera R:" << i << "\n" << cameras[i].R);
        //     LOGLN("after set Initial camera intrinsics #" << i+1 << ":\nK:\n" << cameras[i].K());
        //     LOGLN("default camera intrinsics #" << i+1 << ":\nK:\n" << camK[i]);
        //     // fout << "camera " << i << ":\n";
        // }

        // verify camera parameters
       if(RET_OK == verifyCamParams())
       {
           for(int i=0;i<num_images;i++)
           {
            cameraR[i] = cameras[i].R.clone();
            camK[i] = cameras[i].K().clone();
            warped_image_scale = estimated_warped_image_scale;
           }
       }
#ifdef DEV_MODE
        /*save camera parsms*/
        saveCameraParams();
#endif

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        spdlog::debug("warped_image_scale:{},seam_work_aspect:{}", warped_image_scale, seam_work_aspect);
        // warper_creator = makePtr<cv::CylindricalWarperGpu>();
        warper_creator = makePtr<cv::SphericalWarperGpu>();
        seamfinder_warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        // vector<Size> sizes(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            // Mat_<float> K;
            camK[i].convertTo(camK[i], CV_32F);

            // LOGLN("cameras[i] for sephere warp #" << i << "\n" << cameras[i].K());
            // LOGLN("cam K for sephere warp #" << i << "\n" << camK[i]);
            // LOGLN("cam R for sephere warp #" << i << "\n" << cameras[i].R);

            auto K = camK[i].clone();

            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
#ifdef DEV_MODE
            imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);
#endif
            // LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        for(int i=0;i<num_images;i++)
        {
            // blenderMask[i] = masks_warped[i].getMat(ACCESS_RW);
            masks_warped[i].copyTo(blenderMask[i]);
            // blenderMask[i].setTo(Scalar::all(255));
        }

        t = getTickCount();
        vector<UMat> images_warped_f(num_images);
        for (int i = 0; i < num_images; ++i)
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
        
        // compensator = ExposureCompensator::createDefault(ExposureCompensator::GAIN_BLOCKS);

        // compensator->feed(corners, images_warped, masks_warped);
        seam_finder = makePtr<detail::GraphCutSeamFinder>(GraphCutSeamFinderBase::COST_COLOR);
        // seam_finder = makePtr<detail::NoSeamFinder>();
        seam_finder->find(images_warped_f, corners, masks_warped);

        // for (int i = 0; i < num_images; ++i)
        // {
        //     LOGLN("corners:" << i << ":\n" << corners[i]);
        //     imwrite(std::to_string(i)+"-seamfinder-masks_warped.png", masks_warped[i]);
        // }

        spdlog::debug("***********seam_finder takes:::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        // Release unused memory
        seamSizedImgs.clear();
        images_warped.clear();
        images_warped_f.clear();
        masks.clear();

        spdlog::debug("*************Compositing...");

        // seam_work_aspect is 1, use the same warper
        blender_warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            // LOGLN("Update corners and sizes camK i #" << i << ":\nK:\n" << camK[i]);
            Rect roi = blender_warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameraR[i]);
            // Rect roi = warper->warpRoi(sz, K, cameras[i].R);
            corners[i] = roi.tl();
            sizes[i] = roi.size();

            // LOGLN("****:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;
        Mat dilated_mask, seam_mask;
        Mat mask, maskwarped;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            spdlog::debug("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, maskwarped);

            // imwrite("maskwarped.png", mask_warped);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();
            // mask.release();

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            // imwrite(std::to_string(img_idx)+"-dilated_mask.png", dilated_mask);
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            // mask_warped = seam_mask & mask_warped;
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];

            // imwrite("blendmask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-blenderMask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
#ifdef DEV_MODE
        imwrite(to_string(m_id)+"_ocv.png", result);
#endif

        return 0;
    }
    int initSeam(vector<Mat> &imgs)
    {
        spdlog::debug("***********init seam start**************");
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        // vector<ImageFeatures> features(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);
        for(int i=0;i<num_images;i++)
        {
            // (*finder)(imgs[i], features[i]);
            // features[i].img_idx = i;
            // LOGLN("Features in image #" << i+1 << ": " << features[i].keypoints.size());

            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);
        }

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        // warper_creator = makePtr<cv::CylindricalWarperGpu>();
        warper_creator = makePtr<cv::SphericalWarperGpu>();
        seamfinder_warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            auto K = camK[i].clone();
            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

            // imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            // imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);

            // LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
            // spdlog::warn("images_warped channels::{}", images_warped[i].channels());
        }

        t = getTickCount();
        vector<UMat> images_warped_f(num_images);
        for (int i = 0; i < num_images; ++i)
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
        
        compensator = ExposureCompensator::createDefault(ExposureCompensator::GAIN_BLOCKS);
        compensator->feed(corners, images_warped, masks_warped);
        seam_finder = makePtr<detail::GraphCutSeamFinder>(GraphCutSeamFinderBase::COST_COLOR);
        // seam_finder = makePtr<detail::NoSeamFinder>();
        seam_finder->find(images_warped_f, corners, masks_warped);

        // for (int i = 0; i < num_images; ++i)
        // {
        //     LOGLN("corners:" << i << ":\n" << corners[i]);
        //     imwrite(std::to_string(i)+"-seamfinder-masks_warped.png", masks_warped[i]);
        // }

        spdlog::debug("***********seam_finder takes:::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        // Release unused memory
        seamSizedImgs.clear();
        images_warped.clear();
        images_warped_f.clear();
        masks.clear();

        spdlog::debug("*************Compositing*************.");

        // seam_work_aspect is 1, use the same warper
        blender_warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            Rect roi = blender_warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameraR[i]);
            // Rect roi = warper->warpRoi(sz, K, cameras[i].R);
            corners[i] = roi.tl();
            sizes[i] = roi.size();

            // LOGLN("****:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;
        Mat dilated_mask, seam_mask;
        Mat mask;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            spdlog::debug("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, compensatorMaskWarped[img_idx]);

            // imwrite("maskwarped.png", mask_warped);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();
            // mask.release();

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            // imwrite(std::to_string(img_idx)+"-dilated_mask.png", dilated_mask);
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            // mask_warped = seam_mask & mask_warped;
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];

            // imwrite("blendmask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-blenderMask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
#ifdef DEV_MODE
        imwrite(to_string(m_id)+"_ocv.png", result);
#endif

        return 0;
    }

    void process(vector<Mat> &imgs, Mat &ret)
    {
        spdlog::debug("stitcher {} process start", m_id);
        auto app_start_time = getTickCount();

        

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;

        static int cnt = 0;

        if(cnt++ == 200)
        {
            stmtx.lock();
            updateMask(imgs);
            stmtx.unlock();
            cnt = 0;
            spdlog::debug("***********updateMask***********");
        }

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            // LOGLN("Compositing image #" << img_idx+1);
            auto t = getTickCount();
            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            stmtx.lock();
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);
            stmtx.unlock();
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            // LOGLN("process:::"<<"id:"<<m_id<< "  index::"<< img_idx<<"  K:"<<cameraK<<"cameraR[img_idx]:  "<< cameraR[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, compensatorMaskWarped[img_idx]);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                // Size dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }
            // LOGLN("before feed takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
            // LOGLN("****:" << "id:"<<m_id<<"  idx:"<< img_idx << ":\n corners  " << corners[img_idx] << "\n size:" << sizes[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);
        result.convertTo(ret, CV_8U);

        spdlog::debug("stitcher {} process takes : {:03.3f} ms", m_id, ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
        // imwrite("ocvprocess.png", ret);

        
    }

    void updateMask(vector<Mat> &imgs)
    {
        auto t = getTickCount();
        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);
        vector<UMat> images_warped_f(num_images);
        vector<UMat> masks(num_images);
        vector<Point> seamfinder_corners = vector<Point>(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);

            auto K = camK[i].clone();
            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            seamfinder_corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
        }

        seam_finder->find(images_warped_f, seamfinder_corners, masks_warped);

        Mat dilated_mask, seam_mask;
        Mat mask;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            mask.create(imgs[img_idx].size(), CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];
        }

        spdlog::debug("updateMask takes : {:03.2f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
    }


    public:
    short int num_images;
    Ptr<FeaturesFinder> finder;
    Ptr<FeaturesMatcher> matcher;
    Ptr<Estimator> estimator;
    vector<CameraParams> cameras;
    Ptr<WarperCreator> warper_creator;
    Ptr<detail::BundleAdjusterBase> adjuster;
    Ptr<ExposureCompensator> compensator;
    Ptr<SeamFinder> seam_finder;
    Ptr<RotationWarper> blender_warper, seamfinder_warper;
    vector<Mat_<float>> camK;// = vector<Mat_<float>>(num_images);
    vector<Point> corners = vector<Point>(num_images);
    vector<Mat> blenderMask = vector<Mat>(num_images);
    vector<Mat> compensatorMaskWarped = vector<Mat>(num_images);
    // vector<Mat> seamSizedImgs = vector<Mat>(num_images);

    vector<Mat> cameraR;
    Mat cameraK;
    int m_imgWidth, m_imgHeight;
    double seam_work_aspect;
    Size dst_sz;
    vector<Size> sizes = vector<Size>(num_images);

    float warped_image_scale;
    short int m_id;

    bool presetParaOk;
    std::string m_cfgpath;

    float match_conf, conf_thresh, blend_strength, cameraExThres, cameraInThres;

    // Ptr<Blender> blender;

};

#endif