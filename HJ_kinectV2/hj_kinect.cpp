#include "hj_kinect.h"

#include <iostream>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

int minDepthBuffer;//なんとなく


#define MAX_OBJECT = 100;//なんとなく
int box_count[100] = {0};//なんなとく
int NUM_OBJECT = 0;//なんとなく


#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
			    }



// 初期化
void HJ_Kinect::initialize( int sensor , int color , int depth)
{
	mode_sensor = DEPTH;
	//mode_sensor = sensor;

	mode_color = SHOW | REC;
	//mode_color = color;

	mode_depth = SHOW | REC;
	//mode_depth = depth;

	mode_bodyindex = SHOW | REC;
	//mode_bodyindex = bodyindex;

	initKinect();

	if (COLOR & mode_sensor)	initColor();
	if (DEPTH & mode_sensor)	initDepth();
	if (BODYINDEX & mode_sensor)	initBodyIndex();
}

// 実行
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
	waitKey(0);
}

// データの更新処理
void HJ_Kinect::update()
{
	if (COLOR & mode_sensor)	updateColorFrame();
	if (DEPTH & mode_sensor)	updateDepthFrame();
	if (BODYINDEX & mode_sensor)	updateBodyIndexFrame();
}

// 描画処理
void HJ_Kinect::draw()
{
	if (COLOR & mode_sensor)	drawColor();
	if (DEPTH & mode_sensor)	drawDepth();
	if (BODYINDEX & mode_sensor)	drawBodyIndex();
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

void HJ_Kinect::initBodyIndex()
{
	// ボディインデックスリーダーを取得する
	ComPtr<IBodyIndexFrameSource> bodyIndexFrameSource;
	ERROR_CHECK(kinect->get_BodyIndexFrameSource(&bodyIndexFrameSource));
	ERROR_CHECK(bodyIndexFrameSource->OpenReader(&bodyIndexFrameReader));

	// ボディインデックスの解像度を取得する
	ComPtr<IFrameDescription> bodyIndexFrameDescription;
	ERROR_CHECK(bodyIndexFrameSource->get_FrameDescription(&bodyIndexFrameDescription));
	bodyIndexFrameDescription->get_Width(&BodyIndexWidth);
	bodyIndexFrameDescription->get_Height(&BodyIndexHeight);

	// バッファーを作成する
	bodyIndexBuffer.resize(BodyIndexWidth * BodyIndexHeight);

	// プレイヤーの色を設定する
	colors[0] = cv::Scalar(255, 0, 0);
	colors[1] = cv::Scalar(0, 255, 0);
	colors[2] = cv::Scalar(0, 0, 255);
	colors[3] = cv::Scalar(255, 255, 0);
	colors[4] = cv::Scalar(255, 0, 255);
	colors[5] = cv::Scalar(0, 255, 255);

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
		if (depthBuffer[i] > 1){

			depthImage.data[i] = 255;

		}
		else
		depthImage.data[i] = ~((depthBuffer[i] *255) / maxDepthReliableDistance);
		//depthImage.data[i] = ~((depthBuffer[i] * 255) / 8000);
	}

	flip(depthImage, depthImage, 1); // 左右反転


	if( REC & mode_depth ) writer_depth << depthImage;

	// depthFrame->Release();
}

void HJ_Kinect::updateBodyIndexFrame()
{
	// フレームを取得する
	ComPtr<IBodyIndexFrame> bodyIndexFrame;
	auto ret = bodyIndexFrameReader->AcquireLatestFrame(&bodyIndexFrame);
	if (ret != S_OK){
		return;
	}

	// データを取得する
	ERROR_CHECK(bodyIndexFrame->CopyFrameDataToArray(bodyIndexBuffer.size(), &bodyIndexBuffer[0]));

	// bodyIndexFrame->Release();
}

Point HJ_Kinect::minDepthPoint()
{
	int min_depth = maxDepthReliableDistance;
	int depthPointX = 0;
	int depthPointY = 0;
	int min_num = 0;
	
	int flag = 0;//くそきたないフラッグ

	NUM_OBJECT = 1;//最近傍のオブジェクト挿入済み

	for (int i = 0 ; i < NUM_OBJECT ; i ++ ){//なんとなく
		box_count[i] = 0;
	}

	// 指定した範囲内で最も深度の浅い点を変数に代入
	for (int index = depthHeight*depthWidth * 2 / 5; index < depthHeight*depthWidth * 4 / 5 ; index++){
		if (min_depth > depthBuffer[index]&& depthBuffer[index] != 0){

			if (depthWidth * 2 / 5 < index % depthWidth && index % depthWidth < depthWidth * 4 / 5 ){//指定範囲内を探す
				min_depth = depthBuffer[index];
				min_num = index;
				depthPointX = index % depthWidth;
				depthPointY = index / depthWidth;
			}
			
		}
	}

	for (int i = min_num; i / depthWidth == min_num / depthWidth; i++)
	{
		
	
		if (depthBuffer[i] > min_depth + 200)
		{
			hasikko[0].x = i % depthWidth;
			hasikko[0].y = i / depthWidth;
			break;
		}
			

		box_count[0]++;
	}

	for (int i = min_num; i / depthWidth == min_num / depthWidth; i--)
	{

		if (i<0)
			break;

		if (depthBuffer[i] > min_depth + 200)
		{
			hasikko[1].x = i % depthWidth;
			hasikko[1].y = i / depthWidth;
			break;
		}

		box_count[0]++;
	}


	for (int i = (int(min_num / depthWidth) * depthWidth); i < ( int(min_num / depthWidth) * depthWidth ) + depthWidth; i++)//なんとなくくっそきたない処理
	{
		if (depthBuffer[i] > min_depth + 200)
		{
			flag = 0;
			box_count[NUM_OBJECT]++;
		}
		else if (flag == 0){
			NUM_OBJECT++;
			flag = 1;
		}
	}


	minDepthBuffer = min_depth ;//なんとなく
	return Point(depthPointX, depthPointY);

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

	uchar minDepthBuffer2;//なんとなくuchar

	Mat color_depth(depthImage.cols, depthImage.rows, CV_8UC3);

	retPoint = minDepthPoint();

	depthImage = Mat(depthHeight, depthWidth, CV_8UC1);
	// Depthデータを0-255のグレーデータにする
	minDepthBuffer2 = ~((minDepthBuffer * 255) / maxDepthReliableDistance);
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
		if (depthImage.data[i] < (minDepthBuffer2-50)){//なんとなく
			depthImage.data[i] = 0;
		}
	}

	ss << depthBuffer[ retPoint.y * depthWidth + retPoint.x ] << "mm";
	printf("再近傍幅＝%d",box_count[0]);
	printf("X=%lf\n", ((box_count[0]* depthBuffer[retPoint.y * depthWidth + retPoint.x] * tan(35*M_PI/180.0)) / 256));
	printf("オブジェクト数は　%d でございます。\n", NUM_OBJECT);
	
	for (int i = 0; i < NUM_OBJECT ; i ++ )//なんとなく
	{
		printf("オブジェクト　%d　について\n",(i+1));
		printf("幅＝%d", box_count[i+1]);
		printf("X=%lf\n", ((box_count[i+1] * depthBuffer[retPoint.y * depthWidth + retPoint.x] * tan(35 * M_PI / 180.0)) / 256));
	}

	cv::circle(depthImage, retPoint, 5, cv::Scalar(0, 0, 255), 1);

	cvtColor(depthImage, color_depth, CV_GRAY2BGR);
	color_depth.at<Vec3b>(hasikko[0]) = Vec3b(200, 0, 200);
	cv::circle(color_depth, hasikko[0], 10, cv::Scalar(0, 200, 0), 3, 4);
	color_depth.at<Vec3b>(hasikko[1]) = Vec3b(200, 0, 200);
	cv::circle(color_depth, hasikko[1], 10, cv::Scalar(0, 0, 200), 3, 4);

	//flip(depthImage, depthImage, 1); // 左右反転
	//cv::putText(depthImage, ss.str(), retPoint, 0, 1, cv::Scalar(0, 255, 255));


	flip(color_depth, color_depth, 1); // 左右反転
	cv::putText(color_depth, ss.str(), retPoint, 0, 1, cv::Scalar(0, 255, 255));

	if (SHOW & mode_depth)	cv::imshow("Depth Image", color_depth);
}

void HJ_Kinect::drawBodyIndex()
{
	// ボディインデックスをカラーデータに変換して表示する
	cv::Mat bodyIndexImage(BodyIndexHeight, BodyIndexWidth, CV_8UC4);

	for (int i = 0; i < BodyIndexWidth * BodyIndexHeight; ++i){
		int index = i * 4;
		// 人がいれば255以外
		if (bodyIndexBuffer[i] != 255){
			auto color = colors[bodyIndexBuffer[i]];
			bodyIndexImage.data[index + 0] = color[0];
			bodyIndexImage.data[index + 1] = color[1];
			bodyIndexImage.data[index + 2] = color[2];
		}
		else{
			bodyIndexImage.data[index + 0] = 0;
			bodyIndexImage.data[index + 1] = 0;
			bodyIndexImage.data[index + 2] = 0;
		}
	}

	if (SHOW & mode_depth)	imshow("BodyIndex Image", bodyIndexImage);
}