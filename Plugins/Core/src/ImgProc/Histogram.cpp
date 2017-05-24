#include "Histogram.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "Aquila/nodes/NodeInfo.hpp"
using namespace aq::Nodes;

bool HistogramRange::ProcessImpl()
{
    if(lower_bound_param._modified || upper_bound_param._modified || bins_param._modified)
    {
        updateLevels(input->GetDepth());
        lower_bound_param._modified = false;
        upper_bound_param._modified = false;
        bins_param._modified = false;
    }
    if(input->GetChannels() == 1 || input->GetChannels() == 4)
    {
        cv::cuda::GpuMat hist;
        cv::cuda::histRange(input->getGpuMat(Stream()), hist, levels.getGpuMat(Stream()), Stream());
        histogram_param.UpdateData(hist, input_param.GetTimestamp(), _ctx);
        return true;
    }
    return false;
}


void HistogramRange::updateLevels(int type)
{
    cv::Mat h_mat;
    if(type == CV_32F)
        h_mat = cv::Mat(1, bins, CV_32F);
    else
        h_mat = cv::Mat(1, bins, CV_32S);
    double step = (upper_bound - lower_bound) / double(bins);

    double val = lower_bound;
    for(int i = 0; i < bins; ++i, val += step)
    {
        if(type == CV_32F)
            h_mat.at<float>(i) = val;
        if(type == CV_8U)
            h_mat.at<int>(i) = val;
    }
    levels_param.UpdateData(h_mat);
}

bool Histogram::ProcessImpl()
{
    cv::cuda::GpuMat bins, hist;
    cv::cuda::histogram(input->getGpuMat(Stream()), bins, hist, min, max, Stream());
    histogram_param.UpdateData(hist, input_param.GetTimestamp(), _ctx);
    return true;
}

MO_REGISTER_CLASS(HistogramRange)
MO_REGISTER_CLASS(Histogram)
