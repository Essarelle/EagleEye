#include "nodes/Node.h"

#include <external_includes/cv_highgui.hpp>
#include <external_includes/cv_core.hpp>
#include <opencv2/core/opengl.hpp>
#include <CudaUtils.hpp>
#include <ObjectDetection.hpp>

namespace EagleLib
{
    class QtImageDisplay: public Node
    {
        std::string prevName;
        cv::cuda::HostMem hostImage;
    public:
        QtImageDisplay();
        QtImageDisplay(boost::function<void(cv::Mat, Node*)> cpuCallback_);
        QtImageDisplay(boost::function<void(cv::cuda::GpuMat, Node*)> gpuCallback_);
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());
        void displayImage(cv::cuda::HostMem image);

    };
    class OGLImageDisplay: public Node
    {
        std::string prevName;
		BufferPool<cv::cuda::GpuMat, EventPolicy> bufferPool;
		
    public:
        OGLImageDisplay();

		void display();
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());

    };
    class KeyPointDisplay: public Node
    {
        ConstEventBuffer<std::pair<cv::cuda::HostMem, cv::cuda::HostMem>> hostData;
        int displayType;
    public:

        KeyPointDisplay();
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());
        cv::Mat uicallback();
        virtual void Serialize(ISimpleSerializer *pSerializer);
    };
    class FlowVectorDisplay: public Node
    {
        // First buffer is the image, second is a pair of the points to be used
        ConstEventBuffer<cv::cuda::HostMem[4]> hostData;
        void display(cv::cuda::GpuMat img, cv::cuda::GpuMat initial, cv::cuda::GpuMat final, cv::cuda::GpuMat mask, std::string& name, cv::cuda::Stream);
    public:
        std::string displayName;
        FlowVectorDisplay();
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());
        cv::Mat uicallback();
        virtual void Serialize(ISimpleSerializer *pSerializer);
    };

    class HistogramDisplay: public Node
    {

    public:
        ConstBuffer<cv::cuda::HostMem> histograms;
        void displayHistogram();
        HistogramDisplay();
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());
    };
    class DetectionDisplay: public Node
    {
    public:
        ConstBuffer<std::pair<cv::cuda::HostMem, std::vector<DetectedObject>>> hostData;
        void displayCallback();
        DetectionDisplay();
        virtual void Init(bool firstInit);
        virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat &img, cv::cuda::Stream& stream);
    };

}
