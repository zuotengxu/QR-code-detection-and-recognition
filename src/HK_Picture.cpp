#include "netcam.h"

#define BUFFER_SIZE 4000
#define PICT_LIST_SIZE 5

LONG Init_Picture_catch(string *config) {
	LONG lUserID;

	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	lUserID = NET_DVR_Login_V30((char*)config[0].data(), 8000, (char*)config[1].data(), (char*)config[2].data(), &struDeviceInfo);


	if (lUserID < 0)
	{
		//NET_DVR_Cleanup();
		return -1;
	}

	//_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Read_Picture, NULL, 0, NULL);
	return lUserID;
}
LONG login_picture(string *config, int thre_num) {

	LONG lUserID;
	int err_count = 0;

	while ((lUserID = Init_Picture_catch(config)) == -1) {
		cout << config[3] << ": " << config[0] << "  login error!\t\t"<< err_count << endl;
		err_count++;
		if (err_count == 20) {
			Sleep(0);
		}
		Sleep(5000);
	}
	return lUserID;
}
unsigned Read_Picture(void* param)
{
	string *config = (string *)param;
	int thre_num = atoi(config[4].c_str());
	
	LONG lUserID = login_picture(config, thre_num);
	

	cout << " <camerid>: " << config[3] << "\t<ip>: " << config[0] << "\t<username>: " << config[1] << endl;



	LPNET_DVR_JPEGPARA lpJpegPara = new NET_DVR_JPEGPARA{ 0xff,0 };
	char *sJpegPicBuffer = new char[3 * BUFFER_SIZE * BUFFER_SIZE];
	DWORD dwPicSize = BUFFER_SIZE * BUFFER_SIZE * 3;
	DWORD lpSizeReturned = 0;

	IplImage* pImg = cvCreateImage(cvSize(BUFFER_SIZE, BUFFER_SIZE), 8, 3);
	clock_t __start, __end;
	int err_count = 0;
	while (!stop) {

		__start = clock();

		while (!NET_DVR_CaptureJPEGPicture_NEW(lUserID, 1, lpJpegPara, sJpegPicBuffer, dwPicSize, &lpSizeReturned)) {
			cout << " <camerid>: " << config[3]  << "\tReadJpeg error " << err_count << "\terror code: " <<NET_DVR_GetLastError()<< endl;
			err_count++;
			Sleep(2000);
			if (err_count > 20) {
				cout << " <camerid>: " << config[3] << "\t多次读取JPEG失败，尝试重新登陆" << endl;
				NET_DVR_Logout(lUserID);
				lUserID = login_picture(config, thre_num);
				cout << " <camerid>: " << config[3] << "\t<ip>: " << config[0] << "\t<username>: " << config[1] << endl;
			}
		}
		err_count = 0;
		memcpy(pImg->imageData, sJpegPicBuffer, lpSizeReturned);
		Mat img = imdecode(Mat(pImg), 1).clone();
		cvtColor(img.clone(), img, CV_BGR2GRAY);
		EnterCriticalSection(&Picture_CS[thre_num]);
		(Picture_List[thre_num]).push_back(img.clone());
		if ((Picture_List[thre_num]).size() > PICT_LIST_SIZE) {
			(Picture_List[thre_num]).pop_front();
		}
		LeaveCriticalSection(&Picture_CS[thre_num]);


		__end = clock();
		//cout << (__end - __start) << "毫秒获取一帧" << endl;


	}

	cvReleaseImage(&pImg);
	delete[] lpJpegPara;
	delete[] sJpegPicBuffer;

	NET_DVR_Logout(lUserID);
	return 0;
}

