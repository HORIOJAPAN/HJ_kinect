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

	//Kinect�S�ʗp�ϐ�
	IKinectSensor* kinect = nullptr;
	unsigned char mode_sensor;

	//Color�p�ϐ�
	IColorFrameReader* colorFrameReader = nullptr;
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;
	Mat colorImage;

	VideoWriter writer_color;
	Size videosize_color;

	int mode_color;

	//Depth�p�ϐ�
	IDepthFrameReader* depthFrameReader;
	std::vector<UINT16> depthBuffer;

	UINT16 minDepthReliableDistance;
	UINT16 maxDepthReliableDistance;

	int depthWidth;
	int depthHeight;
	Mat depthImage;

	VideoWriter writer_depth;
	Size videosize_depth;

	int mode_depth;

public:
	//�f�o�b�O�p���[�h�w��
	enum { COLOR = 1 , DEPTH = 2};
	enum { SHOW = 1 ,REC = 2 };

	// ������
	void initialize( int sensor = 3 , int color = 3 , int depth = 3);

	// ���s
	void run();

private:

	//Kinect�̏�����
	void initKinect();
	//Color�̏�����
	void initColor();
	//Depth�̏�����
	void initDepth();

	// �f�[�^�̍X�V����
	void update();

	// �J���[�t���[���̍X�V
	void updateColorFrame();
	// �[�x�t���[���̍X�V
	void updateDepthFrame();

	//�[�x���ł��������_��Ԃ�
	Point minDepthPoint();

	// �`��n
	void draw();
	void drawColor();
	void drawDepth();
};

#endif