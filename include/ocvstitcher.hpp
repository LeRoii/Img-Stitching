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
#include "stitcherconfig.h"

using namespace std;
using namespace cv;
using namespace cv::detail;

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

float match_conf = 0.3f;
float conf_thresh = .7f;
float blend_strength = 0;

static int num_images = 4;

static void Stringsplit(string str, const char split, vector<string>& res)
{
    istringstream iss(str);	// 输入流
    string token;			// 接收缓冲区
    while (getline(iss, token, split))	// 以split为分隔符
    {
        res.push_back(token);
    }
}
class ocvStitcher
{
    public:
    ocvStitcher(int width, int height, int id, std::string cfgpath):
    m_imgWidth(width), m_imgHeight(height), m_id(id), m_cfgpath(cfgpath)
    {
        // auto gpu = cuda::getCudaEnabledDeviceCount();
        // if (gpu > 0)
        //     finder = makePtr<SurfFeaturesFinderGpu>();
        // else
        //     finder = makePtr<SurfFeaturesFinder>();

        finder = makePtr<SurfFeaturesFinder>();

        seam_work_aspect = min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));
        seam_work_aspect = 1;//min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));

        camK.reserve(num_images);
        corners.reserve(num_images);
        blenderMask.reserve(num_images);

        inputOk = false;
        outputOk = false;

        cameraK = Mat(Size(3,3), CV_32FC1);
        for(int i=0;i<num_images;i++)
            cameraR.push_back(Mat(Size(3,3), CV_32FC1));
        
        (initCamParams(cfgpath) == RET_OK) ? (presetParaOk = true) : (presetParaOk = false);

    }

    ~ocvStitcher()
    {

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

        for(int i=0;i<7*(paranum-1);i++)
            getline(fin,str);

        fin >> str;
        // cout << "params time:" << str << endl;
        spdlog::debug("params timestamp:{}", str);
        fin >> str;
        Stringsplit(str, ',', res);
        for(int i=0;i<3;i++)
        {
            for(int j=0;j<3;j++)
            {
                cameraK.at<float>(i,j) = stof(res[i*3+j]);
            }
        }
        cout<<cameraK<<endl;

        for(int i=0;i<4;i++)
        {
            fin >> str;
            cout<<str<<endl;
            res.clear();
            Stringsplit(str, ',', res);
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    cameraR[i].at<float>(mi,mj) = stof(res[mi*3+mj]);
                }
            }

            cout<<cameraR[i]<<endl;
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
        for(int mi=0;mi<3;mi++)
        {
            for(int mj=0;mj<3;mj++)
            {
                fout << cameras[0].K().at<double>(mi,mj) << ",";
                cameraK.at<float>(mi,mj) = cameras[0].K().at<double>(mi,mj);
            }
        }
        for(int idx=0;idx<num_images;idx++)
        {
            fout << "\n";
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    fout << cameras[idx].R.at<float>(mi,mj) << ",";
                    cameraR[idx].at<float>(mi,mj) = cameras[idx].R.at<float>(mi,mj);
                }
            }
        }
        fout << "\n";
        fout << warped_image_scale << "\n";

        return RET_OK;
    }

    int init(vector<Mat> &imgs, bool initMode)
    {
        if(initMode || !presetParaOk)
            return initAll(imgs);
        else
            return initSeam(imgs);
    }

    int initAll(vector<Mat> &imgs)
    {
        spdlog::info("***********stitcher {} init start**************", m_id);
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        vector<ImageFeatures> features(num_images);
        for(int i=0;i<num_images;i++)
        {
            std::vector<cv::Rect> rois = {cv::Rect(m_imgWidth/2, 0, m_imgWidth/2, m_imgHeight)};
            // if(i == num_images - 1)
            //     (*finder)(imgs[i], features[i], rois);
            // else
                (*finder)(imgs[i], features[i]);

            features[i].img_idx = i;
            spdlog::info("Features in image {} : {}", i, features[i].keypoints.size());

            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);
        }

        finder->collectGarbage();

        vector<MatchesInfo> pairwise_matches;
        matcher = makePtr<BestOf2NearestMatcher>(false, match_conf);

        (*matcher)(features, pairwise_matches);
        matcher->collectGarbage();

        estimator = makePtr<HomographyBasedEstimator>();

        if (!(*estimator)(features, pairwise_matches, cameras))
        {
            cout << "Homography estimation failed.\n";
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

        // Find median focal length

        spdlog::debug("***********after Camera parameters adjusting Find median focal length**************");

        // vector<double> focals;
        // for (size_t i = 0; i < cameras.size(); ++i)
        // {
        //     LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
        //     focals.push_back(cameras[i].focal);
        //     LOGLN("focal:"<<cameras[i].focal);
        // }

        // sort(focals.begin(), focals.end());
        // float warped_image_scale;
        // if (focals.size() % 2 == 1)
        //     warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
        // else
        //     warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;
        
        warped_image_scale = static_cast<float>(cameras[2].focal);

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
        
        for (size_t i = 0; i < cameras.size(); ++i)
        {
            // LOGLN("Initial camera intrinsics #" << i << ":\nK:\n" << cameras[i].K());
            cameras[i] = cameras[0];
            cameras[i].R = rmats[i];
            // LOGLN("camera R:" << i << "\n" << cameras[i].R);
            // LOGLN("after set Initial camera intrinsics #" << i+1 << ":\nK:\n" << cameras[i].K());
            // fout << "camera " << i << ":\n";
        }

        /*save camera parsms*/
        saveCameraParams();

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        // warper_creator = makePtr<cv::CylindricalWarperGpu>();
        warper_creator = makePtr<cv::SphericalWarperGpu>();
        warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        // vector<Size> sizes(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            // Mat_<float> K;
            cameras[i].K().convertTo(camK[i], CV_32F);

            // LOGLN("cameras[i] for sephere warp #" << i << "\n" << cameras[i].K());
            // LOGLN("cam K for sephere warp #" << i << "\n" << camK[i]);
            // LOGLN("cam R for sephere warp #" << i << "\n" << cameras[i].R);

            auto K = camK[i].clone();

            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = warper->warp(seamSizedImgs[i], K, cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            warper->warp(masks[i], K, cameras[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

            imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);

            LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
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

        spdlog::info("*************Compositing...");

        // seam_work_aspect is 1, use the same warper
        // warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            Rect roi = warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameras[i].R);
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

        /*mask black(0):overlap region, white(255):non overlap region*/
        // for(int i=0;i<4;i++)
        // {
        //     // blenderMask[i] = masks_warped[i].getMat(ACCESS_RW);
        //     masks_warped[i].copyTo(blenderMask[i]);
        //     // blenderMask[i].setTo(Scalar::all(255));
        // }

        // auto masksize = blenderMask[0].size();
        // blenderMask[0].setTo(255);
        // blenderMask[0](Rect(0,0,masksize.width*0.3,masksize.height)).setTo(0);

        // masksize = blenderMask[1].size();
        // blenderMask[1].setTo(0);
        // blenderMask[1](cv::Rect(masksize.width*0.3,0,masksize.width*0.3, masksize.height)).setTo(255);

        // masksize = blenderMask[2].size();
        // blenderMask[2].setTo(0);
        // blenderMask[2](cv::Rect(masksize.width*0.3,0,masksize.width*0.3, masksize.height)).setTo(255);

        // masksize = blenderMask[3].size();
        // blenderMask[3](cv::Rect(masksize.width*0.7,0,masksize.width*0.3, masksize.height)).setTo(0);

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            spdlog::info("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            warper->warp(img, camK[img_idx], cameras[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            warper->warp(mask, camK[img_idx], cameras[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

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
                spdlog::debug("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::debug("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
        imwrite(to_string(m_id)+"_ocv.png", result);

        return 0;
    }
    int initSeam(vector<Mat> &imgs)
    {
        spdlog::debug("***********init seam start**************");
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        // vector<ImageFeatures> features(num_images);
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
        warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            corners[i] = warper->warp(seamSizedImgs[i], cameraK, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            warper->warp(masks[i], cameraK, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

            // imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            // imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);

            // LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
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
        // warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            Rect roi = warper->warpRoi(Size(m_imgWidth,m_imgHeight), cameraK, cameraR[i]);
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
            spdlog::info("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            warper->warp(img, cameraK, cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            warper->warp(mask, cameraK, cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

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
                spdlog::debug("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::debug("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
        imwrite(to_string(m_id)+"_ocv.png", result);

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

        if(cnt++ == 500)
        {
            updateMask(imgs);
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
            warper->warp(img, cameraK, cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

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
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::debug("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }
            // LOGLN("before feed takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);
        result.convertTo(ret, CV_8U);

        spdlog::debug("stitcher process takes : {:03.3f} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
        // imwrite("ocvprocess.png", ret);
    }

    void updateMask(vector<Mat> &imgs)
    {
        auto t = getTickCount();
        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);
        vector<UMat> images_warped_f(num_images);
        vector<UMat> masks(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            corners[i] = warper->warp(imgs[i], cameraK, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
            masks[i].create(imgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
            warper->warp(masks[i], cameraK, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
        }

        seam_finder->find(images_warped_f, corners, masks_warped);

        Mat dilated_mask, seam_mask;
        Mat mask;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            mask.create(imgs[img_idx].size(), CV_8U);
            mask.setTo(Scalar::all(255));
            warper->warp(mask, cameraK, cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];
        }

        spdlog::debug("updateMask takes : {:03.2f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
    }


    public:
    Ptr<FeaturesFinder> finder;
    Ptr<FeaturesMatcher> matcher;
    Ptr<Estimator> estimator;
    vector<CameraParams> cameras;
    Ptr<WarperCreator> warper_creator;
    Ptr<detail::BundleAdjusterBase> adjuster;
    Ptr<ExposureCompensator> compensator;
    Ptr<SeamFinder> seam_finder;
    Ptr<RotationWarper> warper;
    vector<Mat_<float>> camK = vector<Mat_<float>>(num_images);
    vector<Point> corners = vector<Point>(num_images);
    vector<Mat> blenderMask = vector<Mat>(num_images);
    vector<Mat> compensatorMaskWarped = vector<Mat>(num_images);
    vector<Mat> seamSizedImgs = vector<Mat>(num_images);

    vector<Mat> cameraR;
    Mat cameraK;
    int m_imgWidth, m_imgHeight;
    double seam_work_aspect;
    Size dst_sz;
    vector<Size> sizes = vector<Size>(num_images);
    bool inputOk, outputOk;

    float warped_image_scale;
    int m_id;

    bool presetParaOk;
    std::string m_cfgpath;

};

#endif