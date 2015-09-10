#include "hj_kinect.h"

#include <iostream>
#include <sstream>

#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
			    }

// ������
void HJ_Kinect::initialize()
{
	initKinect();
	initColor();
	initDepth();
}

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
	writer_color.open("unko_color.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_color, true);

	if (!writer_color.isOpened())	cout << "����" << endl;
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
	writer_depth.open("unko_depth.avi", CV_FOURCC_MACRO('M', 'J', 'P', 'G'), 30.0, videosize_depth, false);

	if (!writer_depth.isOpened())	cout << "����" << endl;
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


// �f�[�^�̍X�V����
void HJ_Kinect::update()
{
	updateColorFrame();
	updateDepthFrame();
}

// �J���[�t���[���̍X�V
void HJ_Kinect::updateColorFrame()
{
	// �t���[�����擾����
	ComPtr<IColorFrame> colorFrame;
	auto ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
	if (ret == S_OK){
		// BGRA�̌`���Ńf�[�^���擾����
		ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
			colorBuffer.size(), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));


		// �J���[�f�[�^��\��,�^�悷��
		Mat colorImage(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
		cvtColor(colorImage, colorImage, CV_BGRA2BGR);//BGRA����BGR�֕ϊ�

		resize(colorImage, colorImage, videosize_color); //�����ɏk��
		flip(colorImage, colorImage, 1); // ���E���]

		writer_color << colorImage;
		imshow("Color Image", colorImage);

		// �X�}�[�g�|�C���^���g���ĂȂ��ꍇ�́A�����Ńt���[�����������
		// colorFrame->Release();
	}
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
	cv::Mat depthImage(depthHeight, depthWidth, CV_8UC1);

	// Depth�f�[�^��0-255�̃O���[�f�[�^�ɂ���
	for (int i = 0; i < depthImage.total(); ++i){
		depthImage.data[i] = ~((depthBuffer[i] * 255) / maxDepthReliableDistance);
		//depthImage.data[i] = ~((depthBuffer[i] * 255) / 8000);
	}

	flip(depthImage, depthImage, 1); // ���E���]

	writer_depth << depthImage;

	cv::imshow("Depth Image", depthImage);

	// ����������g��Ȃ��ꍇ�ɂ́A�t���[�����������
	// depthFrame->Release();
}

void HJ_Kinect::draw()
{
}