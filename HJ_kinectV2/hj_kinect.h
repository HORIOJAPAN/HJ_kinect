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

	//Kinect�S�ʗp
	IKinectSensor* kinect = nullptr;

	//Color�p�ϐ�
	IColorFrameReader* colorFrameReader = nullptr;
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;

	VideoWriter writer_color;
	Size videosize_color;

	//Depth�p�ϐ�
	IDepthFrameReader* depthFrameReader;
	std::vector<UINT16> depthBuffer;

	UINT16 minDepthReliableDistance;
	UINT16 maxDepthReliableDistance;

	int depthWidth;
	int depthHeight;

	VideoWriter writer_depth;
	Size videosize_depth;

public:

	// ������
	void initialize();

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

	// �`��n
	void draw();

};

#endif