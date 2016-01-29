#include "nodes/ImgProc/NonMaxSuppression.h"
#include <EagleLib/rcc/external_includes/cv_cudaarithm.hpp>

using namespace EagleLib;
using namespace EagleLib::Nodes;
void MinMax::Init(bool firstInit)
{
    updateParameter<double>("Min value", 0.0)->type = Parameters::Parameter::Output;
	updateParameter<double>("Max value", 0.0)->type = Parameters::Parameter::Output;
}

cv::cuda::GpuMat MinMax::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    stream.waitForCompletion();
	double minValue, maxValue;
    cv::cuda::minMax(img, &minValue, &maxValue);
	updateParameter(0, minValue);
	updateParameter(1, maxValue);
    return img;
}
void Threshold::Init(bool firstInit)
{
    if(firstInit)
    {
        addInputParameter<double>("Input max"); // 0
        addInputParameter<double>("Input min"); // 1
        updateParameter<double>("Replace Value", 255.0); // 2
		updateParameter<double>("Max", 0.0)->type =  Parameters::Parameter::Control; // 3
		updateParameter<double>("Min", 0.0)->type =  Parameters::Parameter::Control; // 4
		updateParameter<bool>("Two sided", false)->SetTooltip("If true, min and max are used to define a threshold range")->type = Parameters::Parameter::Control;
		updateParameter<bool>("Truncate", false)->SetTooltip("If true, threshold to threshold value instead of replace value or src value")->type = Parameters::Parameter::Control;
		updateParameter<bool>("Inverse", false)->SetTooltip("If true, inverse threshold is applied, ie values greater than max and less than min pass")->type = Parameters::Parameter::Control;
		updateParameter<bool>("Source value", true)->SetTooltip("If true, threshold to original source value")->type = Parameters::Parameter::Control;
		updateParameter<cv::cuda::GpuMat>("Mask", cv::cuda::GpuMat())->type = Parameters::Parameter::Output;
		updateParameter<double>("Input %", 0.9)->type = Parameters::Parameter::Control;
        updateParameter<bool>("Output mask", false);
    }

}

cv::cuda::GpuMat Threshold::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    cv::cuda::GpuMat mask, upperMask, lowerMask;
    bool inverse = getParameter<bool>(7)->Data();
	bool truncate = getParameter<bool>(6)->Data();
    bool sourceValue = *getParameter<bool>(8)->Data();
    double* inputMax = getParameter<double>(0)->Data();
    double inputRatio = *getParameter<double>(10)->Data();
    if(inputMax)
        updateParameter<double>(3, *inputMax * inputRatio);
    double* inputMin = getParameter<double>(1)->Data();
    if(inputMin)
        updateParameter<double>(4, *inputMin * inputRatio);


    if(getParameter<bool>(6)->Data())
    {
        // Two sided means max value will also be used to find an upper bound
        if(sourceValue)
        {
            if(inverse)
                cv::cuda::threshold(img, upperMask, *getParameter<double>(3)->Data(), *getParameter<double>(2)->Data(), 3, stream);
            else
                cv::cuda::threshold(img, upperMask, *getParameter<double>(3)->Data(), *getParameter<double>(2)->Data(), 4, stream);
        }else
        {
            if(inverse)
                cv::cuda::threshold(img, upperMask, *getParameter<double>(3)->Data(), *getParameter<double>(2)->Data(), 1, stream);
            else
                cv::cuda::threshold(img, upperMask, *getParameter<double>(3)->Data(), *getParameter<double>(2)->Data(), 0, stream);
        }
    }
    if(truncate)
    {
		cv::cuda::threshold(img, lowerMask, *getParameter<double>(4)->Data(), *getParameter<double>(2)->Data(), 2, stream);
    }else
    {
        if(sourceValue)
        {
            if(inverse)
				cv::cuda::threshold(img, lowerMask, *getParameter<double>(4)->Data(), 0.0, 4, stream);
            else
				cv::cuda::threshold(img, lowerMask, *getParameter<double>(4)->Data(), 0.0, 3, stream);
        }else
        {
            if(inverse)
				cv::cuda::threshold(img, lowerMask, *getParameter<double>(4)->Data(), *getParameter<double>(2)->Data(), 1, stream);
            else
				cv::cuda::threshold(img, lowerMask, *getParameter<double>(4)->Data(), *getParameter<double>(2)->Data(), 0, stream);
        }
    }
    if(upperMask.empty())
    {
        mask = lowerMask;
    }else
    {
        // Hopefully this preserves values instead of railing it to 255, but I can't be sure.
        cv::cuda::bitwise_and(upperMask, lowerMask, mask, cv::noArray(), stream);
    }
    mask.convertTo(mask, img.type(), stream);
    updateParameter<cv::cuda::GpuMat>("Mask", mask)->type = Parameters::Parameter::Output;
    if(*getParameter<bool>("Output mask")->Data())
        return mask;
	return img;
}

void NonMaxSuppression::Init(bool firstInit)
{
    if(firstInit)
    {
        updateParameter<int>("Size", 3);
        addInputParameter<cv::cuda::GpuMat>("Mask");
    }
}

cv::cuda::GpuMat NonMaxSuppression::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    // initialise the block mask and destination
    const int M = img.rows;
    const int N = img.cols;
    cv::Mat src(img);
    int sz = *getParameter<int>("Size")->Data();

    cv::Mat mask;
    if(auto maskParam = getParameter<cv::cuda::GpuMat>(1)->Data())
        if(!maskParam->empty())
            maskParam->download(mask, stream);
    stream.waitForCompletion();
    const bool masked = !mask.empty();
    cv::Mat block = 255*cv::Mat_<uint8_t>::ones(cv::Size(2*sz+1,2*sz+1));
    cv::Mat dst = cv::Mat_<uint8_t>::zeros(src.size());

    // iterate over image blocks
    for (int m = 0; m < M; m+=sz+1) {
        for (int n = 0; n < N; n+=sz+1) {
            cv::Point  ijmax;
            double vcmax, vnmax;

            // get the maximal candidate within the block
            cv::Range ic(m, cv::min(m+sz+1,M));
            cv::Range jc(n, cv::min(n+sz+1,N));

            cv::minMaxLoc(src(ic,jc), NULL, &vcmax, NULL, &ijmax, masked ? mask(ic,jc) : cv::noArray());


            cv::Point cc = ijmax + cv::Point(jc.start,ic.start);

            // search the neighbours centered around the candidate for the true maxima
            cv::Range in(cv::max(cc.y-sz,0), cv::min(cc.y+sz+1,M));
            cv::Range jn(cv::max(cc.x-sz,0), cv::min(cc.x+sz+1,N));

            // mask out the block whose maxima we already know
            cv::Mat_<uint8_t> blockmask;
            block(cv::Range(0,in.size()), cv::Range(0,jn.size())).copyTo(blockmask);
            cv::Range iis(ic.start-in.start, cv::min(ic.start-in.start+sz+1, in.size()));
            cv::Range jis(jc.start-jn.start, cv::min(jc.start-jn.start+sz+1, jn.size()));
            blockmask(iis, jis) = cv::Mat_<uint8_t>::zeros(cv::Size(jis.size(),iis.size()));

            cv::minMaxLoc(src(in,jn), NULL, &vnmax, NULL, &ijmax, masked ? mask(in,jn).mul(blockmask) : blockmask);
            //cv::Point cn = ijmax + cv::Point(jn.start, in.start);

            // if the block centre is also the neighbour centre, then it's a local maxima
            if (vcmax > vnmax) {
                dst.at<uint8_t>(cc.y, cc.x) = 255;
            }
        }
    }
    cv::cuda::GpuMat maxMask;
    maxMask.upload(dst, stream);
    updateParameter("Output Mask", maxMask);
    return maxMask;
}

NODE_DEFAULT_CONSTRUCTOR_IMPL(NonMaxSuppression, Image, Processing)
NODE_DEFAULT_CONSTRUCTOR_IMPL(MinMax, Image, Processing)
NODE_DEFAULT_CONSTRUCTOR_IMPL(Threshold, Image, Processing)
