#include "hj_kinect.h"

#include <iostream>
#include <sstream>

#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
			    }

// 初期化
void HJ_Kinect::initialize()
{
	initKinect();
	initColor();
	initDepth();
}

void HJ_Kinect::initKinect()
{
	// デフォルトのKinectを取得する
	ERROR_CHECK(::GetDefaultKinectSensor(&kinect));

	// Kinectを開く
	ERROR_CHECK(kinect->Open());

	BOOLEAN isOpen = false;
	ERROR_CHECK(kinect->get_IsOpen(&isOpen));
	if (!isOpen){
		throw std::runtime_error("Kinectが開けません");
	}
}

void HJ_Kinect::initColor()
{
	// カラーリーダーを取得する
	ComPtr<IColorFrameSource> colorFrameSource;
	ERROR_CHECK(kinect->get_ColorFrameSource(&colorFrameSource));
	ERROR_CHECK(colorFrameSource->OpenReader(&colorFrameReader));

	// カラー画像のサイズを取得する
	ComPtr<IFrameDescription> colorFrameDescription;
	ERROR_CHECK(colorFrameSource->CreateFrameDescription(
		ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription));
	ERROR_CHECK(colorFrameDescription->get_Width(&colorWidth));
	ERROR_CHECK(colorFrameDescription->get_Height(&colorHeight));
	ERROR_CHECK(colorFrameDescription->get_BytesPerPixel(&colorBytesPerPixel));

	// バッファーを作成する
	colorBuffer.resize(colorWidth * colorHeight * colorBytesPerPixel);
	cout << "[" << colorWidth << "," << colorHeight << "," << colorBytesPerPixel << "]" << endl;

	//VideoWriterを初期化
	videosize_color = Size(colorWidth / 2, colorHeight / 2);
	writer_color.open("unko_color.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_color, true);

	if (!writer_color.isOpened())	cout << "うんこ" << endl;
}

void HJ_Kinect::initDepth()
{
	// Depthリーダーを取得する
	ComPtr<IDepthFrameSource> depthFrameSource;
	ERROR_CHECK(kinect->get_DepthFrameSource(&depthFrameSource));
	ERROR_CHECK(depthFrameSource->OpenReader(&depthFrameReader));

	// Depth画像のサイズを取得する
	ComPtr<IFrameDescription> depthFrameDescription;
	ERROR_CHECK(depthFrameSource->get_FrameDescription(&depthFrameDescription));
	ERROR_CHECK(depthFrameDescription->get_Width(&depthWidth));
	ERROR_CHECK(depthFrameDescription->get_Height(&depthHeight));

	// Depthの最大値、最小値を取得する
	ERROR_CHECK(depthFrameSource->get_DepthMinReliableDistance(&minDepthReliableDistance));
	ERROR_CHECK(depthFrameSource->get_DepthMaxReliableDistance(&maxDepthReliableDistance));

	std::cout << "Depthデータの幅   : " << depthWidth << std::endl;
	std::cout << "Depthデータの高さ : " << depthHeight << std::endl;

	std::cout << "Depth最小値       : " << minDepthReliableDistance << std::endl;
	std::cout << "Depth最大値       : " << maxDepthReliableDistance << std::endl;

	// バッファーを作成する
	depthBuffer.resize(depthWidth * depthHeight);

	//VideoWriterを初期化
	videosize_depth = Size(depthWidth, depthHeight);
	writer_depth.open("unko_depth.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_depth, false);

	if (!writer_depth.isOpened())	cout << "うんこ" << endl;
}

void HJ_Kinect::run()
{
	while (1) {
		update();
		draw();

		auto key = cv::waitKey(1);
		if (key == 'q'){
			break;
		}
	}
}


// データの更新処理
void HJ_Kinect::update()
{
	updateColorFrame();
	updateDepthFrame();
}

// カラーフレームの更新
void HJ_Kinect::updateColorFrame()
{
	// フレームを取得する
	ComPtr<IColorFrame> colorFrame;
	auto ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
	if (ret == S_OK){
		// BGRAの形式でデータを取得する
		ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
			colorBuffer.size(), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));


		// カラーデータを表示,録画する
		Mat colorImage(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
		cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRAからBGRへ変換

		resize(colorImage, colorImage, videosize_color); //半分に縮小
		flip(colorImage, colorImage, 1); // 左右反転

		writer_color << colorImage;
		imshow("Color Image", colorImage);

		// スマートポインタを使ってない場合は、自分でフレームを解放する
		// colorFrame->Release();
	}
}

void HJ_Kinect::updateDepthFrame()
{
	// Depthフレームを取得する
	ComPtr<IDepthFrame> depthFrame;
	auto ret = depthFrameReader->AcquireLatestFrame(&depthFrame);
	if (ret != S_OK){
		return;
	}

	// データを取得する
	ERROR_CHECK(depthFrame->CopyFrameDataToArray(depthBuffer.size(), &depthBuffer[0]));

	// Depthデータを表示する
	cv::Mat depthImage(depthHeight, depthWidth, CV_8UC1);

	// Depthデータを0-255のグレーデータにする
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
		//depthImage.data[i] = ~((depthBuffer[i] * 255) / 8000);
	}

	flip(depthImage, depthImage, 1); // 左右反転

	writer_depth << depthImage;

	cv::imshow("Depth Image", depthImage);

	// 自動解放を使わない場合には、フレームを解放する
	// depthFrame->Release();
}

void HJ_Kinect::draw()
{
}