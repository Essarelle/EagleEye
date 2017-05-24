#include "super_resolution.h"

using namespace aq;
using namespace aq::Nodes;

my_frame_source::my_frame_source()
{
    current_source = nullptr;
    current_stream = nullptr;
}

void my_frame_source::nextFrame(cv::OutputArray frame)
{
    if(current_source && current_stream)
        frame.getGpuMatRef() = current_source->getGpuMatMutable(*current_stream);
}

void my_frame_source::reset()
{
    current_source = nullptr;
    current_stream = nullptr;
}

void my_frame_source::input_frame(SyncedMemory& image, cv::cuda::Stream& stream)
{
    current_source = &image;
    current_stream = &stream;
}



bool super_resolution::ProcessImpl()
{
    if(scale_param._modified)
    {
        super_res->setScale(scale);
        scale_param._modified = false;
    }
    if(iterations_param._modified)
    {
        super_res->setIterations(iterations);
        iterations_param._modified = false;
    }
    if(tau_param._modified)
    {
        super_res->setTau(tau);
        tau_param._modified = false;
    }
    if(lambda_param._modified)
    {
        super_res->setLabmda(lambda);
        lambda_param._modified = false;
    }
    if(alpha_param._modified)
    {
        super_res->setAlpha(alpha);
        alpha_param._modified = false;
    }
    if(kernel_size_param._modified)
    {
        super_res->setKernelSize(kernel_size);
        kernel_size_param._modified = false;
    }
    if(blur_size_param._modified)
    {
        super_res->setBlurKernelSize(blur_size);
        blur_size_param._modified = false;
    }
    if(blur_sigma_param._modified)
    {
        super_res->setBlurSigma(blur_sigma);
        blur_sigma_param._modified = false;
    }
    if(temporal_radius_param._modified)
    {
        super_res->setTemporalAreaRadius(temporal_radius);
        temporal_radius_param._modified = false;
    }
    cv::cuda::GpuMat result;
    
    //frame_source->input_frame(*input, Stream());
    
    return true;
}


//MO_REGISTER_CLASS(super_resolution)
