#ifndef _INC_HJ_KINECT
#define _INC_HJ_KINECT

#include <Kinect.h>
#include <opencv2/opencv.hpp>
#include <opencv2/opencv_lib.hpp>

#include "ComPtr.h"

using namespace std;
using namespace cv;

class HJ_Kinect
{
private:

	//Kinect全般用
	IKinectSensor* kinect = nullptr;

	//Color用変数
	IColorFrameReader* colorFrameReader = nullptr;
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;

	VideoWriter writer_color;
	Size videosize_color;

	//Depth用変数
	IDepthFrameReader* depthFrameReader;
	std::vector<UINT16> depthBuffer;

	UINT16 minDepthReliableDistance;
	UINT16 maxDepthReliableDistance;

	int depthWidth;
	int depthHeight;

	VideoWriter writer_depth;
	Size videosize_depth;

public:

	// 初期化
	void initialize();

	// 実行
	void run();

private:

	//Kinectの初期化
	void initKinect();
	//Colorの初期化
	void initColor();
	//Depthの初期化
	void initDepth();

	// データの更新処理
	void update();

	// カラーフレームの更新
	void updateColorFrame();
	// 深度フレームの更新
	void updateDepthFrame();

	// 描画系
	void draw();

};

#endif