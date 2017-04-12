#include "FeatureDetection.h"
#include "MetaObject/Parameters/detail/TypedInputParameterPtrImpl.hpp"
#include "MetaObject/Parameters/detail/TypedParameterPtrImpl.hpp"

using namespace aq;
using namespace aq::Nodes;


bool GoodFeaturesToTrack::ProcessImpl()
{
    cv::cuda::GpuMat grey;
    if(input->GetChannels() != 1)
    {
        cv::cuda::cvtColor(input->GetGpuMat(Stream()), grey, cv::COLOR_BGR2GRAY, 0, Stream());
    }else
    {
        grey = input->GetGpuMat(Stream());
    }
    if(!detector || max_corners_param._modified || quality_level_param._modified || min_distance_param._modified || block_size_param._modified
        || use_harris_param._modified/* || harris_K_param._modified*/)
    {
        detector = cv::cuda::createGoodFeaturesToTrackDetector(input->GetDepth(), max_corners, quality_level, min_distance, block_size, use_harris);
        max_corners_param._modified = false;
        quality_level_param._modified  = false;
        min_distance_param._modified = false;
        block_size_param._modified = false;
        use_harris_param._modified = false;
    }
    cv::cuda::GpuMat keypoints;
    if(mask)
    {
        detector->detect(grey, keypoints, mask->GetGpuMat(Stream()), Stream());
    }
    else
    {
        detector->detect(grey, keypoints, cv::noArray(), Stream());
    }
    key_points_param.UpdateData(keypoints, input_param.GetTimestamp(), _ctx);
    num_corners_param.UpdateData(keypoints.cols, input_param.GetTimestamp(), _ctx);
    return true;
}



bool FastFeatureDetector::ProcessImpl()
{
    if(threshold_param._modified ||
        use_nonmax_suppression_param._modified ||
        fast_type_param._modified ||
        max_points_param._modified ||
        detector == nullptr)
    {
        detector = cv::cuda::FastFeatureDetector::create(threshold, use_nonmax_suppression, fast_type.getValue(), max_points);
    }
    cv::cuda::GpuMat keypoints;
    if(mask)
    {
        detector->detectAsync(input->GetGpuMat(Stream()), keypoints, mask->GetGpuMat(Stream()), Stream());
    }else
    {
        detector->detectAsync(input->GetGpuMat(Stream()), keypoints, cv::noArray(), Stream());
    }
    if(!keypoints.empty())
    {
        keypoints_param.UpdateData(keypoints, input_param.GetTimestamp(), _ctx);
    }
    return true;
}


/// *****************************************************************************************
/// *****************************************************************************************
/// *****************************************************************************************




bool ORBFeatureDetector::ProcessImpl()
{
    if(num_features_param._modified || scale_factor_param._modified ||
        num_levels_param._modified || edge_threshold_param._modified ||
        first_level_param._modified || WTA_K_param._modified || score_type_param._modified ||
        patch_size_param._modified || fast_threshold_param._modified || blur_for_descriptor_param._modified ||
        detector == nullptr)
    {
        detector = cv::cuda::ORB::create(num_features, scale_factor, num_levels, edge_threshold, first_level,
            WTA_K, score_type.getValue(), patch_size, fast_threshold, blur_for_descriptor);
        num_features_param._modified = false;
        scale_factor_param._modified = false;
        num_levels_param._modified = false;
        edge_threshold_param._modified = false;
        first_level_param._modified = false;
        WTA_K_param._modified = false;
        score_type_param._modified = false;
        patch_size_param._modified = false;
        fast_threshold_param._modified = false;
        blur_for_descriptor_param._modified = false;
    }
    cv::cuda::GpuMat keypoints;
    cv::cuda::GpuMat descriptors;
    if(mask)
    {
        detector->detectAndComputeAsync(input->GetGpuMat(Stream()), mask->GetGpuMat(Stream()), keypoints, descriptors, false, Stream());
    }else
    {
        detector->detectAndComputeAsync(input->GetGpuMat(Stream()), cv::noArray(), keypoints, descriptors, false, Stream());
    }
    keypoints_param.UpdateData(keypoints, input_param.GetTimestamp(), _ctx);
    descriptors_param.UpdateData(descriptors, input_param.GetTimestamp(), _ctx);
    return true;
}




bool CornerHarris::ProcessImpl()
{
    if(block_size_param._modified || sobel_aperature_size_param._modified || harris_free_parameter_param._modified || detector == nullptr)
    {
        detector = cv::cuda::createHarrisCorner(input->GetType(), block_size, sobel_aperature_size, harris_free_parameter);
    }
    cv::cuda::GpuMat score;
    detector->compute(input->GetGpuMat(Stream()), score, Stream());
    score_param.UpdateData(score, input_param.GetTimestamp(), _ctx);
    return true;
}



bool CornerMinEigenValue::ProcessImpl()
{
    if (block_size_param._modified || sobel_aperature_size_param._modified || harris_free_parameter_param._modified || detector == nullptr)
    {
        detector = cv::cuda::createMinEigenValCorner(input->GetType(), block_size, sobel_aperature_size, harris_free_parameter);
    }
    cv::cuda::GpuMat score;
    detector->compute(input->GetGpuMat(Stream()), score, Stream());
    score_param.UpdateData(score, input_param.GetTimestamp(), _ctx);
    return true;
}



MO_REGISTER_CLASS(GoodFeaturesToTrack)
MO_REGISTER_CLASS(ORBFeatureDetector)
MO_REGISTER_CLASS(FastFeatureDetector)

MO_REGISTER_CLASS(CornerHarris)


