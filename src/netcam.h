#pragma once

#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv2\opencv.hpp>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <windows.h>

#include <HCNetSDK.h>
/*#include "PlayM4.h"*/
#include <plaympeg4.h>

#include <time.h>
#include <process.h>

#include <locale.h>
#include <string>
#include <zbar.h>
#include <sstream>

//#include "libxl.h"
#include <fstream> 
using namespace cv;
using namespace std;
//using namespace libxl;
using namespace zbar;

struct Result {
	int count;
	Rect rect_all[100]; //一副图里面出现超过100个二维码，几率很低
	string info[100];
	int thre_num;
	Rect window;
};


int InitExcel(void);
int WriteExcel(Result result,string read_cam_id);
void ReleaseExcel(void);

unsigned ShowVideo(void *param);

unsigned readCamera(void *param);
void releaseHK();
unsigned Timer(void *param);

extern CRITICAL_SECTION Picture_CS[];
extern std::list<Mat> Picture_List[];
extern bool stop;

void InitHK(void);
void on_mouse(int event, int x, int y, int flags, void *ustc);
Result MyScan(Mat flame, bool by_canny);

int ClearClient();
int InitClient();
int Qr_Data_Ser(string data);
int Qr_Contol_Ser();
#define C_out(a) EnterCriticalSection(&cout_CS); \
				cout a;\
				LeaveCriticalSection(&cout_CS);