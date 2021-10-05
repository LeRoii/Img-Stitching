#ifndef _OCVSTITCHER_HPP_
#define _OCVSTITCHER_HPP_

#include <iostream>
#include <fstream>
#include <string>
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

using namespace std;
using namespace cv;
using namespace cv::detail;

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

float match_conf = 0.3f;
float conf_thresh = .7f;
float blend_strength = 0;

int num_images = 4;
class ocvStitcher
{
    public:
    ocvStitcher(int width, int height):m_imgWidth(width), m_imgHeight(height)
    {
        // auto gpu = cuda::getCudaEnabledDeviceCount();
        // if (gpu > 0)
        //     finder = makePtr<SurfFeaturesFinderGpu>();
        // else
        //     finder = makePtr<SurfFeaturesFinder>();

        finder = makePtr<SurfFeaturesFinder>();

        seam_work_aspect = min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));

        camK.reserve(num_images);
        corners.reserve(num_images);
        blenderMask.reserve(num_images);
    }

    ~ocvStitcher()
    {

    }

    int init(vector<Mat> &imgs)
    {
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        vector<ImageFeatures> features(num_images);
        for(int i=0;i<num_images;i++)
        {
            (*finder)(imgs[i], features[i]);
            features[i].img_idx = i;
            LOGLN("Features in image #" << i+1 << ": " << features[i].keypoints.size());

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

        for (size_t i = 0; i < cameras.size(); ++i)
        {
            Mat R;
            cameras[i].R.convertTo(R, CV_32F);
            cameras[i].R = R;
            LOGLN("Initial camera intrinsics #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
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
            cout << "Camera parameters adjusting failed.\n";
            return -1;
        }

        // Find median focal length

        vector<double> focals;
        for (size_t i = 0; i < cameras.size(); ++i)
        {
            LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
            focals.push_back(cameras[i].focal);
        }

        sort(focals.begin(), focals.end());
        float warped_image_scale;
        if (focals.size() % 2 == 1)
            warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
        else
            warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;

        vector<Mat> rmats;
        for (size_t i = 0; i < cameras.size(); ++i)
            rmats.push_back(cameras[i].R.clone());
        waveCorrect(rmats, detail::WAVE_CORRECT_HORIZ);
        for (size_t i = 0; i < cameras.size(); ++i)
            cameras[i].R = rmats[i];

        LOGLN("Warping images (auxiliary)... ");

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        warper_creator = makePtr<cv::SphericalWarperGpu>();
        warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));


        vector<UMat> images_warped(num_images);
        // vector<Size> sizes(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            // Mat_<float> K;
            cameras[i].K().convertTo(camK[i], CV_32F);

            LOGLN("camK[i #" << i << ":\nK:\n" << camK[i]);

            auto K = camK[i].clone();

            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = warper->warp(seamSizedImgs[i], K, cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            warper->warp(masks[i], K, cameras[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

        }

        vector<UMat> images_warped_f(num_images);
        for (int i = 0; i < num_images; ++i)
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
        
        compensator = ExposureCompensator::createDefault(ExposureCompensator::GAIN_BLOCKS);

        compensator->feed(corners, images_warped, masks_warped);
        seam_finder = makePtr<detail::GraphCutSeamFinder>(GraphCutSeamFinderBase::COST_COLOR);
        seam_finder->find(images_warped_f, corners, masks_warped);

        // Release unused memory
        seamSizedImgs.clear();
        images_warped.clear();
        images_warped_f.clear();
        masks.clear();

        LOGLN("Compositing...");

        // seam_work_aspect is 1, use the same warper
        // warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            LOGLN("camK[1111i #" << i << ":\nK:\n" << camK[i]);
            Rect roi = warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameras[i].R);
            // Rect roi = warper->warpRoi(sz, K, cameras[i].R);
            corners[i] = roi.tl();
            sizes[i] = roi.size();
        }

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;
        Mat dilated_mask, seam_mask;
        Mat mask, maskwarped;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            LOGLN("Compositing image #" << img_idx+1);

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
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            // mask_warped = seam_mask & mask_warped;
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];
            // imwrite("blendmask.png", blenderMask[img_idx]);

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    LOGLN("Multi-band blender, number of bands: " << mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        LOGLN("init ok takes : " << ((getTickCount() - app_start_time) / getTickFrequency()) * 1000 << " ms");
        imwrite("ocv.png", result);

        return 0;
    }

    void process(vector<Mat> &imgs, Mat &ret)
    {
        
        auto app_start_time = getTickCount();

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            LOGLN("Compositing image #" << img_idx+1);
            auto t = getTickCount();
            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            warper->warp(img, camK[img_idx], cameras[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);

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
                    LOGLN("Multi-band blender, number of bands: " << mb->numBands());
                }
                blender->prepare(corners, sizes);
            }
            LOGLN("before feed takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        LOGLN("process takes : " << ((getTickCount() - app_start_time) / getTickFrequency()) * 1000 << " ms");

        result.convertTo(ret, CV_8U);
        imwrite("ocvprocess.png", ret);
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
    int m_imgWidth, m_imgHeight;
    double seam_work_aspect;
    Size dst_sz;
    vector<Size> sizes = vector<Size>(num_images);

};

#endif