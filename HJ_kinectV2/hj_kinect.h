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
	IKinectSensor* kinect;
	unsigned char mode_sensor;//���p����Z���T�[�̎��

	//Color�p�ϐ�
	IColorFrameReader* colorFrameReader;
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;
	Mat colorImage;

	VideoWriter writer_color;//�^��p�I�u�W�F�N�g
	Size videosize_color;//�^�掞�̉摜�T�C�Y

	int mode_color;

	//Depth�p�ϐ�
	IDepthFrameReader* depthFrameReader;
	std::vector<UINT16> depthBuffer;

	UINT16 minDepthReliableDistance;
	UINT16 maxDepthReliableDistance;

	int depthWidth;
	int depthHeight;
	Mat depthImage;

	Point hasikko[2];

	VideoWriter writer_depth;//�^��p�I�u�W�F�N�g
	Size videosize_depth;//�^�掞�̉摜�T�C�Y

	int mode_depth;

	//BodyIndex�p�ϐ�
	IBodyIndexFrameReader* bodyIndexFrameReader;
	int BodyIndexWidth;
	int BodyIndexHeight;
	std::vector<BYTE> bodyIndexBuffer;

	cv::Scalar colors[6];

	int mode_bodyindex;

public:
	//�f�o�b�O�p���[�h�w��
	enum { COLOR = 1 , DEPTH = 2 , BODYINDEX = 4 };
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
	//BodyIndex�̏�����
	void initBodyIndex();

	// �f�[�^�̍X�V����
	void update();

	// �J���[�t���[���̍X�V
	void updateColorFrame();
	// �[�x�t���[���̍X�V
	void updateDepthFrame();
	// BodyIndex�t���[���̍X�V
	void updateBodyIndexFrame();

	//�[�x���ł��������_��Ԃ�
	Point minDepthPoint();

	// �`��n
	void draw();
	void drawColor();
	void drawDepth();
	void drawBodyIndex();
};

#endif