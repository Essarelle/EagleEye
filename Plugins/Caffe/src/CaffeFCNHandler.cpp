#include "CaffeFCNHandler.hpp"
#include "helpers.hpp"
#include "CaffeNetHandlerInfo.hpp"
#include "Caffe.h"
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudawarping.hpp"

using namespace aq::Caffe;

std::map<int, int> FCNHandler::CanHandleNetwork(const caffe::Net<float>& net)
{
    const std::vector<int>& out_idx = net.output_blob_indices();
    auto layer_names = net.layer_names();
    auto layers = net.layers();
    std::map<int, int> output;
    for(int i = 0; i < layer_names.size(); ++i)
    {
        std::vector<int> top_ids = net.top_ids(i);
        for(auto id : top_ids)
        {
            if(std::find(out_idx.begin(), out_idx.end(), id) != out_idx.end())
            {
                // Layer(i) outputs from network
                std::string type = layers[i]->type();
                if(type == "Softmax" || type == "Convolution" || type == "Crop")
                {
                    const std::vector<boost::shared_ptr<caffe::Blob<float> > >& blobs = net.blobs();
                    const std::vector<int>& shape = blobs[id]->shape();
                    if(shape.size() == 4)
                    {
                        if(shape[2] > 1 && shape[3] > 1)
                            output[id] = 10;
                    }
                }
            }
        }
    }
    return output;
}

void FCNHandler::HandleOutput(const caffe::Net<float>& net, boost::optional<mo::time_t> timestamp,  const std::vector<cv::Rect>& bounding_boxes, cv::Size input_image_size, const std::vector<DetectedObject2d>& objs)
{
    auto blob = net.blob_by_name(output_blob_name);
    if(!blob)
        return;
    cv::cuda::GpuMat label, confidence;
    aq::Caffe::argMax(blob.get(), label, confidence, _ctx->GetStream());


    cv::cuda::GpuMat resized_label, resized_confidence;
    cv::cuda::resize(label, resized_label, input_image_size, 0, 0, cv::INTER_NEAREST, _ctx->GetStream());
    cv::cuda::resize(confidence, resized_confidence, input_image_size, 0, 0, cv::INTER_NEAREST, _ctx->GetStream());

    cv::cuda::GpuMat mask;
    cv::cuda::threshold(resized_confidence, mask, min_confidence, 255, cv::THRESH_BINARY_INV, _ctx->GetStream());
    resized_label.setTo(cv::Scalar::all(0), mask, _ctx->GetStream());


    label_param.UpdateData(resized_label, timestamp, _ctx);
    confidence_param.UpdateData(resized_confidence, timestamp, _ctx);
}

MO_REGISTER_CLASS(FCNHandler)

std::map<int, int> FCNSingleClassHandler::CanHandleNetwork(const caffe::Net<float>& net)
{
    const std::vector<int>& out_idx = net.output_blob_indices();
    auto layer_names = net.layer_names();
    auto layers = net.layers();
    std::map<int, int> output;
    for(int i = 0; i < layer_names.size(); ++i)
    {
        std::vector<int> top_ids = net.top_ids(i);
        for(auto id : top_ids)
        {
            if(std::find(out_idx.begin(), out_idx.end(), id) != out_idx.end())
            {
                // Layer(i) outputs from network
                std::string type = layers[i]->type();
                if(type == "Softmax" || type == "Convolution" || type == "Crop")
                {
                    const std::vector<boost::shared_ptr<caffe::Blob<float> > >& blobs = net.blobs();
                    const std::vector<int>& shape = blobs[id]->shape();
                    if(shape.size() == 4)
                    {
                        if(shape[2] > 1 && shape[3] > 1)
                            output[id] = 10;
                    }
                }
            }
        }
    }
    return output;
}

void FCNSingleClassHandler::HandleOutput(const caffe::Net<float>& net, boost::optional<mo::time_t> timestamp,  const std::vector<cv::Rect>& bounding_boxes, cv::Size input_image_size, const std::vector<DetectedObject2d>& objs)
{
    auto blob = net.blob_by_name(output_blob_name);
    if(blob)
    {
        blob->gpu_data();
        //blob->cpu_data();
        auto wrapped = aq::Nodes::CaffeBase::WrapBlob(*blob);
        if(class_index < blob->channels() && blob->num())
        {
            const cv::cuda::GpuMat& confidence = wrapped[0].GetGpuMat(_ctx->GetStream(), class_index);
            //cv::Mat dbg = wrapped[0].GetMat(_ctx->GetStream(), class_index);
            cv::cuda::GpuMat confidence_out;
            cv::cuda::threshold(confidence, confidence_out, min_confidence, 255, cv::THRESH_BINARY, _ctx->GetStream());
            cv::cuda::GpuMat mask_out;
            confidence_out.convertTo(mask_out, CV_8UC1, _ctx->GetStream());
            label_param.UpdateData(mask_out, timestamp, _ctx);
            confidence_param.UpdateData(confidence, timestamp, _ctx);
        }
    }
}

MO_REGISTER_CLASS(FCNSingleClassHandler)


