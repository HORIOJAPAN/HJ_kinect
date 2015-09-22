#include "hj_kinect.h"

void main()
{
	try {
		HJ_Kinect app;
		app.initialize();
		app.run();
	}
	catch (std::exception& ex){
		std::cout << ex.what() << std::endl;
		cv::waitKey(0);
	}
}