#pragma once
#include "../../RuntimeObjectSystem/RuntimeLinkLibrary.h"
#include <opencv2/core.hpp>
#if _WIN32
#if _DEBUG
RUNTIME_COMPILER_LINKLIBRARY("opencv_core300d.lib")
#else
RUNTIME_COMPILER_LINKLIBRARY("opencv_core300.lib")
#endif
#else
RUNTIME_COMPILER_LINKLIBRARY("-lopencv_core")
#endif
