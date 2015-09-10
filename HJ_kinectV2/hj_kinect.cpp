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
void HJ_Kinect::initialize( int sensor , int color , int depth)
{
	mode_sensor = COLOR | DEPTH;
	//mode_sensor = sensor;

	mode_color = SHOW | REC;
	//mode_color = color;

	mode_depth = SHOW | REC;
	//mode_depth = depth;

	initKinect();

	if (COLOR & mode_sensor)	initColor();
	if (DEPTH & mode_sensor)	initDepth();
}

//DefaultKinectSensorを取得
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
	if (REC & mode_color)
	{
		writer_color.open("unko_color.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_color, true);
		if (!writer_color.isOpened())	cout << "うんこ" << endl;
	}
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
	if (REC & mode_depth)
	{
		writer_depth.open("unko_depth.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_depth, false);
		if (!writer_depth.isOpened())	cout << "うんこ" << endl;
	}
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
	if (COLOR & mode_sensor)	updateColorFrame();
	if (DEPTH & mode_sensor)	updateDepthFrame();
}

// カラーフレームの更新
void HJ_Kinect::updateColorFrame()
{
	// フレームを取得する
	ComPtr<IColorFrame> colorFrame;
	auto ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
	if (ret != S_OK){
		return;
	}

	// BGRAの形式でデータを取得する
	ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
		colorBuffer.size(), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));

	// カラーデータを録画する
	colorImage = Mat(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
	cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRAからBGRへ変換

	resize(colorImage, colorImage, videosize_color); //半分に縮小
	flip(colorImage, colorImage, 1); // 左右反転

	if (REC & mode_color)	writer_color << colorImage;

	// colorFrame->Release();
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
	depthImage = Mat(depthHeight, depthWidth, CV_8UC1);

	// Depthデータを0-255のグレーデータにする
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
		//depthImage.data[i] = ~((depthBuffer[i] * 255) / 8000);
	}

	flip(depthImage, depthImage, 1); // 左右反転


	if( REC & mode_depth ) writer_depth << depthImage;

	// depthFrame->Release();
}

Point HJ_Kinect::minDepthPoint()
{
	int min_depth = maxDepthReliableDistance;
	int depthPointX = 0;
	int depthPointY = 0;

	// 指定した範囲内で最も深度の浅い点を変数に代入
	for (int index = depthHeight*depthWidth / 3; index < depthHeight*depthWidth * 2 / 3; index++){
		if (min_depth > depthBuffer[index] && depthBuffer[index] != 0){
			if (depthWidth / 3 < index % depthWidth && index % depthWidth < depthWidth * 2 / 3){
				min_depth = depthBuffer[index];
				depthPointX = index % depthWidth;
				depthPointY = index / depthWidth;
			}
		}
	}
	
	return Point(depthPointX, depthPointY);

}

void HJ_Kinect::draw()
{
	if (COLOR & mode_sensor)	drawColor();
	if (DEPTH & mode_sensor)	drawDepth();
}

void HJ_Kinect::drawColor()
{
	colorImage = Mat(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
	cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRAからBGRへ変換

	resize(colorImage, colorImage, videosize_color); //半分に縮小
	flip(colorImage, colorImage, 1); // 左右反転

	if (colorImage.empty())
	{
		cout << "うんこ" << endl;
		return;
	}
	if (SHOW & mode_color)	imshow("Color Image", colorImage);
}

void HJ_Kinect::drawDepth()
{
	stringstream ss;
	Point retPoint;

	retPoint = minDepthPoint();

	depthImage = Mat(depthHeight, depthWidth, CV_8UC1);
	// Depthデータを0-255のグレーデータにする
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
	}

	ss << depthBuffer[ retPoint.y * depthWidth + retPoint.x ] << "mm";
	cv::circle(depthImage, retPoint, 5, cv::Scalar(0, 0, 255), 1);
	flip(depthImage, depthImage, 1); // 左右反転
	cv::putText(depthImage, ss.str(), retPoint, 0, 1, cv::Scalar(0, 255, 255));

	if (SHOW & mode_depth)	cv::imshow("Depth Image", depthImage);
}