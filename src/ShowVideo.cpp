#include "netcam.h"

unsigned ShowVideo(void *param) {
	
	Mat frame;
	char key;
	Result *result = (Result*)param;
	int thre_num = (*result).thre_num;
	
	namedWindow(to_string(thre_num), 0);
	//list<Mat> thre_num_list = (Picture_List[thre_num]);
	while (!stop) {

		//Sleep(100);
		if (waitKey(60) == ' ')
			while ((char)waitKey(60) != ' ');


		while (1) {
			EnterCriticalSection(&Picture_CS[thre_num]);
			if ((Picture_List[thre_num]).size() == 0) {
				LeaveCriticalSection(&Picture_CS[thre_num]);
				Sleep(100);
				continue;
			}
			break;
		}

		frame = (*(Picture_List[thre_num]).begin()).clone();
		LeaveCriticalSection(&Picture_CS[thre_num]);

		//当有读取到二维码的时候，在二维码的位置上画一个框
		rectangle(frame, (*result).window, Scalar(255, 0, 0), 13);
		if ((*result).count > 0) {
			for (int i = 0; i<(*result).count; i++) {
				Rect bound = (*result).rect_all[i];
				bound.x += (*result).window.x;
				bound.y += (*result).window.y;
				rectangle(frame, bound, Scalar(0, 255, 0), 13);
			}
		}
		imshow(to_string(thre_num), frame);  //显示原始视频流

	}

	cvDestroyWindow(to_string(thre_num).c_str());
	
	return 0;
}

void on_mouse(int event, int x, int y, int flags, void *ustc)//event鼠标事件代号，x,y鼠标坐标，flags拖拽和键盘操作的代号  
{
	static Point pre_pt = (-1, -1);//初始坐标  
	static Point cur_pt = (-1, -1);//实时坐标  
	char temp[16];
	if (event == CV_EVENT_LBUTTONDOWN)//左键按下，读取初始坐标，并在图像上该点处划圆  
	{
		pre_pt = Point(x, y);
	}
	else if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON))//左键按下时，鼠标移动，则在图像上划矩形  
	{
		cur_pt = Point(x, y);
	}
	else if (event == CV_EVENT_LBUTTONUP)//左键松开，将在图像上划矩形  
	{
		cur_pt = Point(x, y);
		
		int width = abs(pre_pt.x - cur_pt.x);
		int height = abs(pre_pt.y - cur_pt.y);

		*(Rect*)ustc = Rect(min(cur_pt.x, pre_pt.x), min(cur_pt.y, pre_pt.y), width, height);
	}
}