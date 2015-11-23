#pragma once


#include "vlc/vlc.h"
#include <nodes/Node.h>
#include <Manager.h>
#ifdef PROJECT_INCLUDES_ && !defined(PROJECT_INCLUDES)
#define PROJECT_INCLUDES PROJECT_INCLUDES_
#endif
#ifdef PROJECT_LIB_DIRS_ && !defined(PROJECT_LIB_DIRS)
#define PROJECT_LIB_DIRS PROJECT_LIB_DIRS_
#endif



#include <mutex>
#include "ObjectInterfacePerModule.h"

#ifdef _DEBUG
RUNTIME_COMPILER_LINKLIBRARY("Qt5Cored.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Networkd.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Guid.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Widgetsd.lib");
#else
RUNTIME_COMPILER_LINKLIBRARY("Qt5Core.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Network.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Gui.lib");
RUNTIME_COMPILER_LINKLIBRARY("Qt5Widgets.lib");
#endif
RUNTIME_COMPILER_LINKLIBRARY("libvlccore.lib");
RUNTIME_COMPILER_LINKLIBRARY("libvlc.lib");
#ifdef __cplusplus
extern "C" {
#endif

	CV_EXPORTS IPerModuleInterface* GetModule();
	
#ifdef __cplusplus
}
#endif
SETUP_PROJECT_DEF

namespace EagleLib
{
	class vlcCamera : public Node
	{
		
		libvlc_instance_t* vlcInstance;
		libvlc_media_player_t* mp;
		libvlc_media_t* media;
	public:
		vlcCamera();
		~vlcCamera();
		void onSourceChange();
		virtual void Init(bool firstInit);
		virtual cv::cuda::GpuMat doProcess(cv::cuda::GpuMat& img, cv::cuda::Stream& stream);
		virtual bool SkipEmpty() const;
		ConstBuffer<cv::cuda::HostMem> h_dest;
		ConstBuffer<cv::cuda::GpuMat> d_dest;
		concurrent_queue<cv::cuda::HostMem*> imgQueue;
	};
}
/*
https://forum.videolan.org/viewtopic.php?t=87031
using namespace cv;


struct ctx
{
Mat* image;
HANDLE mutex;
uchar*    pixels;
};
// define output video resolution
#define VIDEO_WIDTH     1920
#define VIDEO_HEIGHT    1080

void *lock(void *data, void**p_pixels)
{
struct ctx *ctx = (struct ctx*)data;

WaitForSingleObject(ctx->mutex, INFINITE);

// pixel will be stored on image pixel space
*p_pixels = ctx->pixels;

return NULL;

}

void display(void *data, void *id){
(void) data;
assert(id == NULL);
}

void unlock(void *data, void *id, void *const *p_pixels){

// get back data structure
struct ctx *ctx = (struct ctx*)data;

// VLC just rendered the video, but we can also render stuff
// show rendered image
imshow("test", *ctx->image);

ReleaseMutex(ctx->mutex);
}

int main()
{
	// VLC pointers
	libvlc_instance_t *vlcInstance;
	libvlc_media_player_t *mp;
	libvlc_media_t *media;

	const char * const vlc_args[] = {
		"-I", "dummy", // Don't use any interface
		"--ignore-config", // Don't use VLC's config
		"--extraintf=logger", // Log anything
		"--verbose=2", // Be much more verbose then normal for debugging purpose
	};

	vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

	// Read a distant video stream
	//media = libvlc_media_new_location(vlcInstance, "rtsp://192.168.0.218:554/Streaming/Channels/1");
	// Read a local video file
	media = libvlc_media_new_path(vlcInstance, "d:\\a.mp4");

	mp = libvlc_media_player_new_from_media(media);

	libvlc_media_release(media);

	struct ctx* context = (struct ctx*)malloc(sizeof(*context));
	context->mutex = CreateMutex(NULL, FALSE, NULL);

	context->image = new Mat(VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC3);
	context->pixels = (unsigned char *)context->image->data;
	// show blank image
	imshow("test", *context->image);

	libvlc_video_set_callbacks(mp, lock, unlock, display, context);
	libvlc_video_set_format(mp, "RV24", VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH * 24 / 8); // pitch = width * BitsPerPixel / 8


	int ii = 0;
	int key = 0;
	while (key != 27)
	{
		ii++;
		if (ii > 5)
		{
			libvlc_media_player_play(mp);
		}
		float fps = libvlc_media_player_get_fps(mp);
		printf("fps:%f\r\n", fps);
		key = waitKey(100); // wait 100ms for Esc key
	}

	libvlc_media_player_stop(mp);


	return 0;
}
*/
