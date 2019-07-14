#include "netcam.h"
#include <direct.h>  
#include <io.h> 

//#define VIDEO
#define TIME_A clock_t __start = clock(); 

#define TIME_B clock_t __ends = clock();\
				cout << __ends - __start << endl;

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define CAM_NUM 8
unsigned Read_Picture(void* param);

unsigned Worker(void *param);
bool stop = false;
bool stop_tmp = false;
bool set_window_globle = false;
bool show_window = false;
bool all_haar = false;
int Frame_Rate = 7;  //5 - 1; 6 - 2; 7 - 4; 8 - 6;
CRITICAL_SECTION qrcode_CS;
CRITICAL_SECTION Picture_CS[50];
list<Mat> Picture_List[50];
CRITICAL_SECTION cout_CS;
int ReadConfig(string config[][6]) {

	ifstream config_file(string("config//") + "camer_config");
	string buf;
	if (!config_file.is_open()) {
		cout << "open config file fail!" << endl;
		return -1;
	}


	getline(config_file, buf);
	if (buf.c_str()[11] == '1') 
		set_window_globle = true;
	else
		set_window_globle = false;

	getline(config_file, buf);
	if (buf.c_str()[5] == '1')
		show_window = true;
	else
		show_window = false;

	getline(config_file, buf);
	if (buf.c_str()[9] == '1')
		all_haar = true;
	else
		all_haar = false;

	int i;
	for (i=0;i<CAM_NUM;i++) {

		if (!getline(config_file, buf))	break;	//读取一个空白行

		if (!getline(config_file, config[i][3])) break;		//ID
		if (config[i][3].length() == 0) break;
		if (!getline(config_file, config[i][0])) break;		//IP
		if (config[i][0].length() == 0) break;
		if (!getline(config_file, config[i][1])) break;		//username
		if (config[i][1].length() == 0) break;
		if (!getline(config_file, config[i][2])) break;		//passwd
		if (config[i][2].length() == 0) break;

		config[i][4] = to_string(i);						//thre_num

		}
	config_file.close();
	return i;
}


int main(int argc,char* argv[])
{

	InitializeCriticalSection(&cout_CS);
	InitExcel();
	InitHK();
	string config[50][6];
	int thre_num = ReadConfig(config);

	if(thre_num < 4)  //4个摄像头，每秒6帧
		Frame_Rate = 8;  //5 - 1; 6 - 2; 7 - 4; 8 - 6;
	else if(thre_num < 8)	//8个摄像头，每秒4帧
		Frame_Rate = 7;
	else					//8个以上，每秒2帧
		Frame_Rate = 6;

	for (int i=0; i<thre_num; i++) {
		_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Worker, (void*)config[i], 0, NULL);
		Sleep(1000);
	}

	InitClient();
	cout << "按 ESC 退出" << endl;
	cout << "等待 \"start\" 信号" << endl;

	while (!stop) {
		if (KEY_DOWN(27))
			stop = true;
		Qr_Contol_Ser();
		Sleep(100);
	}

	ReleaseExcel();
	ClearClient();
	Sleep(5000);
	NET_DVR_Cleanup();
	return 0;
}

unsigned Worker(void *param) {
	string *config = (string *)param;
	string cam_id = config[3];
	int thre_num = atoi(config[4].c_str());


	Result result;
	result.count = 0;
	result.thre_num = thre_num;
	bool set_window = set_window_globle;
	Rect act_window(0,0,0,0);
	InitializeCriticalSection(&Picture_CS[thre_num]);


#ifdef VIDEO
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&readCamera, (void *)config, 0, NULL);
#else
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Read_Picture, (void *)config, 0, NULL);
#endif
	clock_t __start,__end;
	int clock_num = 0;
	Mat frame;
	while (!stop)
	{


		result.count = 0;
		
		while (1) {
			EnterCriticalSection(&Picture_CS[thre_num]);
			if (Picture_List[thre_num].size() == 0) {
				LeaveCriticalSection(&Picture_CS[thre_num]);
				Sleep(100);
				continue;
			}
			break;
		}
		frame = (*(Picture_List[thre_num]).begin()).clone();
		(Picture_List[thre_num]).pop_front();
		LeaveCriticalSection(&Picture_CS[thre_num]);

		if (set_window) {
			char key;
			Mat tmp;
			do {
				tmp = frame.clone();
				namedWindow(to_string(thre_num) + " set_window", 0);
				imshow(to_string(thre_num) + " set_window", tmp);
				setMouseCallback(to_string(thre_num) + " set_window", on_mouse, &act_window);//调用回调函数
				waitKey(0);
				rectangle(tmp, act_window, Scalar(0, 255, 0), 13);
				imshow(to_string(thre_num) + " set_window", tmp);
				C_out(<< act_window << endl)
					key = (char)waitKey(0);
			} while (key != ' ');
			set_window = false;
			cvDestroyWindow((to_string(thre_num) + " set_window").c_str());
			result.window = act_window;
		}


		
		if (!stop_tmp) {

			
			bool by_canny;
			if (all_haar) {
				by_canny = false;
			}
			else
				by_canny = (frame.cols < 2560);

			//frame = imread("D:\\Desktop\\local.jpg", 0);
			by_canny = false;


			if(set_window_globle)
				result = MyScan(frame(act_window).clone(), by_canny);				//扫描二维码，result->count存放这一帧取到的二维码数量
			else
				result = MyScan(frame.clone(), by_canny);
			result.thre_num = thre_num;
			result.window = act_window;
			if (result.count > 0)
				WriteExcel(result, cam_id);
		}
		else {
			result.count = 0;
			result.info[99] = "just_pass";
			Sleep(100);
		}

		if (show_window && (thre_num == 0)) {
			rectangle(frame, act_window, Scalar(255, 0, 0), 13);
			for (int i = 0; result.info[99 - i] != "just_pass"; i++) {
				Rect bound = result.rect_all[99 - i];
				bound.x += result.window.x;
				bound.y += result.window.y;
				rectangle(frame, bound, Scalar(255, 255, 255), 5);
			
			}
			for (int i = 0; i < result.count; i++) {
				Rect bound = result.rect_all[i];
				bound.x += result.window.x;
				bound.y += result.window.y;
				rectangle(frame, bound, Scalar(0, 0, 0), 15);
			}
			namedWindow(cam_id, 0);
			imshow(cam_id, frame);
			waitKey(1);
		}

		if (show_window) {
			if (stop_tmp) {
				clock_num = 0;
			}else {
				if (clock_num == 0) {
					__start = clock();
				}
				clock_num++;
				if (clock_num > 30) {
					clock_num = 0;
					__end = clock();
					//C_out(<< cam_id << ": " << (__end - __start) / 30 << "毫秒处理一帧" << endl)
				}
			}
		}
	}

	return 0;

}