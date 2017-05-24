#include "flip.hpp"
#include "Aquila/nodes/NodeInfo.hpp"
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudawarping.hpp>
#include "opencv2/imgproc.hpp"

using namespace aq::Nodes;
bool Flip::ProcessImpl()
{
    auto state = input->getSyncState();
    if(state == input->DEVICE_UPDATED)
    {
        cv::cuda::GpuMat output;
        cv::cuda::flip(input->getGpuMat(Stream()), output, axis.getValue(), Stream());
        output_param.UpdateData(output, input_param.GetTimestamp(), _ctx);
        return true;
    }else
    {
        if(state == input->HOST_UPDATED)
        {
            cv::Mat flipped;
            cv::flip(input->getMat(Stream()), flipped, axis.getValue());
            output_param.UpdateData(flipped, input_param.GetTimestamp(), _ctx);
            return true;
        }else if(state == input->SYNCED)
        {
            cv::Mat h_flipped;
            cv::flip(input->getMat(Stream()), h_flipped, axis.getValue());
            cv::cuda::GpuMat d_flipped;
            cv::cuda::flip(input->getGpuMat(Stream()), d_flipped, axis.getValue(), Stream());
            output_param.UpdateData({h_flipped, d_flipped}, input_param.GetTimestamp(), _ctx);
            return true;
        }
    }
    return false;
}

MO_REGISTER_CLASS(Flip)


bool Rotate::ProcessImpl()
{
    cv::cuda::GpuMat rotated;
    auto size = input->GetSize();
    cv::Mat rotation = cv::getRotationMatrix2D({size.width / 2.0f, size.height / 2.0f}, angle_degrees, 1.0);
    cv::cuda::warpAffine(input->getGpuMat(Stream()), rotated , rotation, size, cv::INTER_CUBIC, cv::BORDER_REFLECT, cv::Scalar(), Stream());
    output_param.UpdateData(rotated, input_param.GetTimestamp(), _ctx);
    return true;
}

MO_REGISTER_CLASS(Rotate)
