#include "nodes/ImgProc/DisplayHelpers.h"
//#include "DisplayHelpers.cuh"
using namespace EagleLib;
#include <external_includes/cv_cudaarithm.hpp>
#include <external_includes/cv_highgui.hpp>

#ifdef _MSC_VER

#else
RUNTIME_COMPILER_LINKLIBRARY("-lopencv_cudaarithm")
#endif

void
AutoScale::Init(bool firstInit)
{
    Node::Init(firstInit);
}

cv::cuda::GpuMat
AutoScale::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    std::vector<cv::cuda::GpuMat> channels;
    cv::cuda::split(img,channels);
    for(size_t i = 0; i < channels.size(); ++i)
    {
        double minVal, maxVal;
        cv::cuda::minMax(channels[i], &minVal, &maxVal);
        double scaleFactor = 255.0 / (maxVal - minVal);
        channels[i].convertTo(channels[0], CV_8U, scaleFactor, minVal*scaleFactor);
        updateParameter<double>("Min-" + boost::lexical_cast<std::string>(i), minVal, Parameters::Parameter::State);
		updateParameter<double>("Max-" + boost::lexical_cast<std::string>(i), maxVal, Parameters::Parameter::State);
    }
    cv::cuda::merge(channels,img);
    return img;
}

void
Colormap::Init(bool firstInit)
{
    Node::Init(firstInit);
	rescale = true;
    updateParameter("Colormapping scheme", int(0));
	updateParameter<bool>("Rescale", &rescale);
}

cv::cuda::GpuMat
Colormap::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    if(img.channels() != 1)
    {
		NODE_LOG(warning) << "Non-monochrome image! Has " + boost::lexical_cast<std::string>(img.channels()) + " channels";
        return img;
    }
	if (rescale)
	{
		double min, max;
		cv::cuda::minMax(img, &min, &max);
		mapper.setMapping(ColorScale(50, 255 / 25, true), ColorScale(50 / 3, 255 / 25, true), ColorScale(0, 255 / 25, true), min, max);
		rescale = false;
	}
	mapper.colormap_image(img, color_mapped_image, stream); 
	return color_mapped_image;
}


QtColormapDisplay::QtColormapDisplay():
    Colormap()
{
    nodeName = "QtColormapDisplay";
    treeName = nodeName;
    fullTreeName = treeName;
}

void QtColormapDisplayCallback(int status, void* data)
{
    QtColormapDisplay* node = static_cast<QtColormapDisplay*>(data);
	Parameters::UI::UiCallbackService::Instance()->post(boost::bind(&QtColormapDisplay::display, node));
}

void QtColormapDisplay::display()
{

}

void QtColormapDisplay::Init(bool firstInit)
{
    Colormap::Init(firstInit);
}
cv::cuda::GpuMat QtColormapDisplay::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    if(img.channels() != 1)
    {
		NODE_LOG(warning) << "Non-monochrome image! Has " + boost::lexical_cast<std::string>(img.channels()) + " channels";
        return img;
    }
	Colormap::doProcess(img, stream);
	//color_mapped_image.download()

    return img;
}

void Normalize::Init(bool firstInit)
{
    Node::Init(firstInit);
	Parameters::EnumParameter param;
    param.addEnum(ENUM(CV_MINMAX));
    param.addEnum(ENUM(cv::NORM_L2));
    param.addEnum(ENUM(cv::NORM_L1));
    param.addEnum(ENUM(cv::NORM_INF));
    updateParameter("Norm type", param);
    updateParameter<double>("Alpha", 0);
    updateParameter<double>("Beta", 1);
    if(firstInit)
    {
        addInputParameter<cv::cuda::GpuMat>("Mask");
    }
}

cv::cuda::GpuMat Normalize::doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream)
{
    TIME
    cv::cuda::GpuMat normalized = *normalizedBuf.getFront();
    cv::cuda::GpuMat* mask = getParameter<cv::cuda::GpuMat>(3)->Data();
    cv::cuda::normalize(img,normalized,
            *getParameter<double>(1)->Data(),
            *getParameter<double>(2)->Data(),
            getParameter<Parameters::EnumParameter>(0)->Data()->getValue(), img.type(),
            mask == NULL ? cv::noArray(): *mask,
            stream);
    return normalized;
}

NODE_DEFAULT_CONSTRUCTOR_IMPL(AutoScale)
NODE_DEFAULT_CONSTRUCTOR_IMPL(Colormap)
NODE_DEFAULT_CONSTRUCTOR_IMPL(Normalize)
REGISTERCLASS(QtColormapDisplay)
