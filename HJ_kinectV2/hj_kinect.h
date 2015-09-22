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

	//Kinect全般用変数
	IKinectSensor* kinect;
	unsigned char mode_sensor;//利用するセンサーの種類

	//Color用変数
	IColorFrameReader* colorFrameReader;
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;
	Mat colorImage;

	VideoWriter writer_color;//録画用オブジェクト
	Size videosize_color;//録画時の画像サイズ

	int mode_color;

	//Depth用変数
	IDepthFrameReader* depthFrameReader;
	std::vector<UINT16> depthBuffer;

	UINT16 minDepthReliableDistance;
	UINT16 maxDepthReliableDistance;

	int depthWidth;
	int depthHeight;
	Mat depthImage;

	Point hasikko[2];

	VideoWriter writer_depth;//録画用オブジェクト
	Size videosize_depth;//録画時の画像サイズ

	int mode_depth;

	//BodyIndex用変数
	IBodyIndexFrameReader* bodyIndexFrameReader;
	int BodyIndexWidth;
	int BodyIndexHeight;
	std::vector<BYTE> bodyIndexBuffer;

	cv::Scalar colors[6];

	int mode_bodyindex;

public:
	//デバッグ用モード指定
	enum { COLOR = 1 , DEPTH = 2 , BODYINDEX = 4 };
	enum { SHOW = 1 ,REC = 2 };

	// 初期化
	void initialize( int sensor = 3 , int color = 3 , int depth = 3);

	// 実行
	void run();

private:

	//Kinectの初期化
	void initKinect();
	//Colorの初期化
	void initColor();
	//Depthの初期化
	void initDepth();
	//BodyIndexの初期化
	void initBodyIndex();

	// データの更新処理
	void update();

	// カラーフレームの更新
	void updateColorFrame();
	// 深度フレームの更新
	void updateDepthFrame();
	// BodyIndexフレームの更新
	void updateBodyIndexFrame();

	//深度が最も小さい点を返す
	Point minDepthPoint();

	// 描画系
	void draw();
	void drawColor();
	void drawDepth();
	void drawBodyIndex();
};

#endif