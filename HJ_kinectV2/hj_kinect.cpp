#include "hj_kinect.h"

#include <iostream>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

int minDepthBuffer;//�Ȃ�ƂȂ�


#define MAX_OBJECT = 100;//�Ȃ�ƂȂ�
int box_count[100] = {0};//�Ȃ�ȂƂ�
int NUM_OBJECT = 0;//�Ȃ�ƂȂ�


#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
			    }



// ������
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

// ���s
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

// �f�[�^�̍X�V����
void HJ_Kinect::update()
{
	if (COLOR & mode_sensor)	updateColorFrame();
	if (DEPTH & mode_sensor)	updateDepthFrame();
	if (BODYINDEX & mode_sensor)	updateBodyIndexFrame();
}

// �`�揈��
void HJ_Kinect::draw()
{
	if (COLOR & mode_sensor)	drawColor();
	if (DEPTH & mode_sensor)	drawDepth();
	if (BODYINDEX & mode_sensor)	drawBodyIndex();
}

//DefaultKinectSensor���擾
void HJ_Kinect::initKinect()
{
	// �f�t�H���g��Kinect���擾����
	ERROR_CHECK(::GetDefaultKinectSensor(&kinect));

	// Kinect���J��
	ERROR_CHECK(kinect->Open());

	BOOLEAN isOpen = false;
	ERROR_CHECK(kinect->get_IsOpen(&isOpen));
	if (!isOpen){
		throw std::runtime_error("Kinect���J���܂���");
	}
}

void HJ_Kinect::initColor()
{
	// �J���[���[�_�[���擾����
	ComPtr<IColorFrameSource> colorFrameSource;
	ERROR_CHECK(kinect->get_ColorFrameSource(&colorFrameSource));
	ERROR_CHECK(colorFrameSource->OpenReader(&colorFrameReader));

	// �J���[�摜�̃T�C�Y���擾����
	ComPtr<IFrameDescription> colorFrameDescription;
	ERROR_CHECK(colorFrameSource->CreateFrameDescription(
		ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription));
	ERROR_CHECK(colorFrameDescription->get_Width(&colorWidth));
	ERROR_CHECK(colorFrameDescription->get_Height(&colorHeight));
	ERROR_CHECK(colorFrameDescription->get_BytesPerPixel(&colorBytesPerPixel));

	// �o�b�t�@�[���쐬����
	colorBuffer.resize(colorWidth * colorHeight * colorBytesPerPixel);
	cout << "[" << colorWidth << "," << colorHeight << "," << colorBytesPerPixel << "]" << endl;

	
	//VideoWriter��������
	videosize_color = Size(colorWidth / 2, colorHeight / 2);
	if (REC & mode_color)
	{
		writer_color.open("unko_color.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_color, true);
		if (!writer_color.isOpened())	cout << "����" << endl;
	}
}

void HJ_Kinect::initDepth()
{
	// Depth���[�_�[���擾����
	ComPtr<IDepthFrameSource> depthFrameSource;
	ERROR_CHECK(kinect->get_DepthFrameSource(&depthFrameSource));
	ERROR_CHECK(depthFrameSource->OpenReader(&depthFrameReader));

	// Depth�摜�̃T�C�Y���擾����
	ComPtr<IFrameDescription> depthFrameDescription;
	ERROR_CHECK(depthFrameSource->get_FrameDescription(&depthFrameDescription));
	ERROR_CHECK(depthFrameDescription->get_Width(&depthWidth));
	ERROR_CHECK(depthFrameDescription->get_Height(&depthHeight));

	// Depth�̍ő�l�A�ŏ��l���擾����
	ERROR_CHECK(depthFrameSource->get_DepthMinReliableDistance(&minDepthReliableDistance));
	ERROR_CHECK(depthFrameSource->get_DepthMaxReliableDistance(&maxDepthReliableDistance));

	std::cout << "Depth�f�[�^�̕�   : " << depthWidth << std::endl;
	std::cout << "Depth�f�[�^�̍��� : " << depthHeight << std::endl;

	std::cout << "Depth�ŏ��l       : " << minDepthReliableDistance << std::endl;
	std::cout << "Depth�ő�l       : " << maxDepthReliableDistance << std::endl;

	// �o�b�t�@�[���쐬����
	depthBuffer.resize(depthWidth * depthHeight);

	//VideoWriter��������
	videosize_depth = Size(depthWidth, depthHeight);
	if (REC & mode_depth)
	{
		writer_depth.open("unko_depth.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_depth, false);
		if (!writer_depth.isOpened())	cout << "����" << endl;
	}
}

void HJ_Kinect::initBodyIndex()
{
	// �{�f�B�C���f�b�N�X���[�_�[���擾����
	ComPtr<IBodyIndexFrameSource> bodyIndexFrameSource;
	ERROR_CHECK(kinect->get_BodyIndexFrameSource(&bodyIndexFrameSource));
	ERROR_CHECK(bodyIndexFrameSource->OpenReader(&bodyIndexFrameReader));

	// �{�f�B�C���f�b�N�X�̉𑜓x���擾����
	ComPtr<IFrameDescription> bodyIndexFrameDescription;
	ERROR_CHECK(bodyIndexFrameSource->get_FrameDescription(&bodyIndexFrameDescription));
	bodyIndexFrameDescription->get_Width(&BodyIndexWidth);
	bodyIndexFrameDescription->get_Height(&BodyIndexHeight);

	// �o�b�t�@�[���쐬����
	bodyIndexBuffer.resize(BodyIndexWidth * BodyIndexHeight);

	// �v���C���[�̐F��ݒ肷��
	colors[0] = cv::Scalar(255, 0, 0);
	colors[1] = cv::Scalar(0, 255, 0);
	colors[2] = cv::Scalar(0, 0, 255);
	colors[3] = cv::Scalar(255, 255, 0);
	colors[4] = cv::Scalar(255, 0, 255);
	colors[5] = cv::Scalar(0, 255, 255);

}

// �J���[�t���[���̍X�V
void HJ_Kinect::updateColorFrame()
{
	// �t���[�����擾����
	ComPtr<IColorFrame> colorFrame;
	auto ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
	if (ret != S_OK){
		return;
	}

	// BGRA�̌`���Ńf�[�^���擾����
	ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
		colorBuffer.size(), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));

	// �J���[�f�[�^��^�悷��
	colorImage = Mat(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
	cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRA����BGR�֕ϊ�

	resize(colorImage, colorImage, videosize_color); //�����ɏk��
	flip(colorImage, colorImage, 1); // ���E���]

	if (REC & mode_color)	writer_color << colorImage;

	// colorFrame->Release();
}

void HJ_Kinect::updateDepthFrame()
{
	// Depth�t���[�����擾����
	ComPtr<IDepthFrame> depthFrame;
	auto ret = depthFrameReader->AcquireLatestFrame(&depthFrame);
	if (ret != S_OK){
		return;
	}

	// �f�[�^���擾����
	ERROR_CHECK(depthFrame->CopyFrameDataToArray(depthBuffer.size(), &depthBuffer[0]));

	// Depth�f�[�^��\������
	depthImage = Mat(depthHeight, depthWidth, CV_8UC1);

	// Depth�f�[�^��0-255�̃O���[�f�[�^�ɂ���
	for (int i = 0; i < depthImage.total(); ++i){
		if (depthBuffer[i] > 1){

			depthImage.data[i] = 255;

		}
		else
		depthImage.data[i] = ~((depthBuffer[i] *255) / maxDepthReliableDistance);
		//depthImage.data[i] = ~((depthBuffer[i] * 255) / 8000);
	}

	flip(depthImage, depthImage, 1); // ���E���]


	if( REC & mode_depth ) writer_depth << depthImage;

	// depthFrame->Release();
}

void HJ_Kinect::updateBodyIndexFrame()
{
	// �t���[�����擾����
	ComPtr<IBodyIndexFrame> bodyIndexFrame;
	auto ret = bodyIndexFrameReader->AcquireLatestFrame(&bodyIndexFrame);
	if (ret != S_OK){
		return;
	}

	// �f�[�^���擾����
	ERROR_CHECK(bodyIndexFrame->CopyFrameDataToArray(bodyIndexBuffer.size(), &bodyIndexBuffer[0]));

	// bodyIndexFrame->Release();
}

Point HJ_Kinect::minDepthPoint()
{
	int min_depth = maxDepthReliableDistance;
	int depthPointX = 0;
	int depthPointY = 0;
	int min_num = 0;
	
	int flag = 0;//���������Ȃ��t���b�O

	NUM_OBJECT = 1;//�ŋߖT�̃I�u�W�F�N�g�}���ς�

	for (int i = 0 ; i < NUM_OBJECT ; i ++ ){//�Ȃ�ƂȂ�
		box_count[i] = 0;
	}

	// �w�肵���͈͓��ōł��[�x�̐󂢓_��ϐ��ɑ��
	for (int index = depthHeight*depthWidth * 2 / 5; index < depthHeight*depthWidth * 4 / 5 ; index++){
		if (min_depth > depthBuffer[index]&& depthBuffer[index] != 0){

			if (depthWidth * 2 / 5 < index % depthWidth && index % depthWidth < depthWidth * 4 / 5 ){//�w��͈͓���T��
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


	for (int i = (int(min_num / depthWidth) * depthWidth); i < ( int(min_num / depthWidth) * depthWidth ) + depthWidth; i++)//�Ȃ�ƂȂ������������Ȃ�����
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


	minDepthBuffer = min_depth ;//�Ȃ�ƂȂ�
	return Point(depthPointX, depthPointY);

}

void HJ_Kinect::drawColor()
{
	colorImage = Mat(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
	cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRA����BGR�֕ϊ�

	resize(colorImage, colorImage, videosize_color); //�����ɏk��
	flip(colorImage, colorImage, 1); // ���E���]

	if (colorImage.empty())
	{
		cout << "����" << endl;
		return;
	}
	if (SHOW & mode_color)	imshow("Color Image", colorImage);
}

void HJ_Kinect::drawDepth()
{
	stringstream ss;
	Point retPoint;

	uchar minDepthBuffer2;//�Ȃ�ƂȂ�uchar

	Mat color_depth(depthImage.cols, depthImage.rows, CV_8UC3);

	retPoint = minDepthPoint();

	depthImage = Mat(depthHeight, depthWidth, CV_8UC1);
	// Depth�f�[�^��0-255�̃O���[�f�[�^�ɂ���
	minDepthBuffer2 = ~((minDepthBuffer * 255) / maxDepthReliableDistance);
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
		if (depthImage.data[i] < (minDepthBuffer2-50)){//�Ȃ�ƂȂ�
			depthImage.data[i] = 0;
		}
	}

	//Canny(depthImage, depthImage, 50, 100);
	/*
	vector<vector<Point>> contours;
	findContours(depthImage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
	// ���o���ꂽ�֊s����΂ŕ`��
	for (auto contour = contours.begin(); contour != contours.end(); contour++){
		cv::polylines(depthImage, *contour, true, cv::Scalar(0, 255, 0), 2);
	}
	

	//�֊s�̐�
	int roiCnt = 0;

	//�֊s�̃J�E���g   
	int i = 0;

	for (auto contour = contours.begin(); contour != contours.end(); contour++){

		std::vector< cv::Point > approx;

		//�֊s�𒼐��ߎ�����
		cv::approxPolyDP(cv::Mat(*contour), approx, 0.01 * cv::arcLength(*contour, true), true);

		// �ߎ��̖ʐς����ȏ�Ȃ�擾
		double area = cv::contourArea(approx);

		if (area > 1000.0){
			//�ň͂ޏꍇ            
			cv::polylines(depthImage, approx, true, cv::Scalar(255, 0, 0), 2);
			std::stringstream sst;
			sst << "area : " << area;
			cv::putText(depthImage, sst.str(), approx[0], CV_FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 128, 0));

			//�֊s�ɗאڂ����`�̎擾
			cv::Rect brect = cv::boundingRect(cv::Mat(approx).reshape(2));
			//roi[roiCnt] = cv::Mat(depthImage, brect);

			//���͉摜�ɕ\������ꍇ
			cv::drawContours(depthImage, contours, i, CV_RGB(0, 0, 255), 4);

			//�\��
			//cv::imshow("label" + std::to_string(roiCnt + 1), roi[roiCnt]);

			roiCnt++;

			//�O�̂��ߗ֊s���J�E���g
			if (roiCnt == 99)
			{
				break;
			}
		}

		i++;
	}*/








	ss << depthBuffer[ retPoint.y * depthWidth + retPoint.x ] << "mm";
	printf("�ċߖT����%d",box_count[0]);
	printf("X=%lf\n", ((box_count[0]* depthBuffer[retPoint.y * depthWidth + retPoint.x] * tan(35*M_PI/180.0)) / 256));
	printf("�I�u�W�F�N�g���́@%d �ł������܂��B\n", NUM_OBJECT);
	
	for (int i = 0; i < NUM_OBJECT ; i ++ )//�Ȃ�ƂȂ�
	{
		printf("�I�u�W�F�N�g�@%d�@�ɂ���\n",(i+1));
		printf("����%d", box_count[i+1]);
		printf("X=%lf\n", ((box_count[i+1] * depthBuffer[retPoint.y * depthWidth + retPoint.x] * tan(35 * M_PI / 180.0)) / 256));
	}

	cv::circle(depthImage, retPoint, 5, cv::Scalar(0, 0, 255), 1);

	cvtColor(depthImage, color_depth, CV_GRAY2BGR);
	color_depth.at<Vec3b>(hasikko[0]) = Vec3b(200, 0, 200);
	cv::circle(color_depth, hasikko[0], 10, cv::Scalar(0, 200, 0), 3, 4);
	color_depth.at<Vec3b>(hasikko[1]) = Vec3b(200, 0, 200);
	cv::circle(color_depth, hasikko[1], 10, cv::Scalar(0, 0, 200), 3, 4);

	//flip(depthImage, depthImage, 1); // ���E���]
	//cv::putText(depthImage, ss.str(), retPoint, 0, 1, cv::Scalar(0, 255, 255));


	flip(color_depth, color_depth, 1); // ���E���]
	cv::putText(color_depth, ss.str(), retPoint, 0, 1, cv::Scalar(0, 255, 255));

	



	if (SHOW & mode_depth)	cv::imshow("Depth Image", color_depth);
}

void HJ_Kinect::drawBodyIndex()
{
	// �{�f�B�C���f�b�N�X���J���[�f�[�^�ɕϊ����ĕ\������
	cv::Mat bodyIndexImage(BodyIndexHeight, BodyIndexWidth, CV_8UC4);

	for (int i = 0; i < BodyIndexWidth * BodyIndexHeight; ++i){
		int index = i * 4;
		// �l�������255�ȊO
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