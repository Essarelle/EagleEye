#pragma once
#include "Node.h"
#include "Root.h"

namespace EagleLib
{
    class RootThreaded: public Node// :public Root
	{
		RootThreaded();
		~RootThreaded();
        cv::cuda::GpuMat doProcess(cv::cuda::GpuMat& img, cv::cuda::Stream& stream = cv::cuda::Stream::Null());

		boost::shared_ptr<boost::thread> _thread;

	};
}