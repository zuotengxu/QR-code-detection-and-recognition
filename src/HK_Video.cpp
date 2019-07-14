#include "netcam.h"
//--------------------------------------------

#define USECOLOR 0
#define LIST_SIZE 2

//int iPicNum = 0;//Set channel NO.
LONG nPort[50];
LONG UserID[50];
int port_list[50];
extern int Frame_Rate;
extern CRITICAL_SECTION cout_CS;
//����ص� ��ƵΪYUV����(YV12)����ƵΪPCM����


void CALLBACK DecCBFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nReserved1, long nReserved2)
{
	long lFrameType = pFrameInfo->nType;

	if (lFrameType == T_YV12)
	{
#if USECOLOR
		//int start = clock();
		static IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//�õ�ͼ���Y����  
		yv12toYUV(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//�õ�ȫ��RGBͼ��
		static IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);
		cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
		//int end = clock();
#else
		IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 1);
		memcpy(pImg->imageData, pBuf, pFrameInfo->nWidth*pFrameInfo->nHeight);
#endif
		int thre_num;
		for (int i = 0; i < 50; i++) {
			if (port_list[i] == nPort) {
				thre_num = i;
				break;
			}
		}
		
		Mat frametemp(pImg);
		
		EnterCriticalSection(&Picture_CS[thre_num]);
		
		(Picture_List[thre_num]).push_back(frametemp.clone());

		if ((Picture_List[thre_num]).size() > LIST_SIZE)
			(Picture_List[thre_num]).pop_front();
		
		LeaveCriticalSection(&Picture_CS[thre_num]);

#if USECOLOR
		//      cvReleaseImage(&pImgYCrCb);
		//      cvReleaseImage(&pImg);
#else
		cvReleaseImage(&pImg);
#endif
		//��ʱ��YV12��ʽ����Ƶ���ݣ�������pBuf�У�����fwrite(pBuf,nSize,1,Videofile);
		//fwrite(pBuf,nSize,1,fp);
	}
	//Sleep(500);
}


///ʵʱ���ص�
void CALLBACK fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
	DWORD dRet;
	string *config = (string *)pUser;

	int thre_num = atoi(config[4].c_str());

	
	//cout << "fRealDataCallBack" << config[0] << "  "<<config[4] << endl;
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD:    //ϵͳͷ
		if (!PlayM4_GetPort(&nPort[thre_num])) //��ȡ���ſ�δʹ�õ�ͨ����
		{
			break;
		}

		port_list[thre_num] = nPort[thre_num];

		if (dwBufSize > 0)
		{
			if (!PlayM4_OpenStream(nPort[thre_num], pBuffer, dwBufSize, 1024 * 1024))
			{
				dRet = PlayM4_GetLastError(nPort[thre_num]);
				break;
			}
			//���ý���ص����� ֻ���벻��ʾ
			if (!PlayM4_SetDecCallBack(nPort[thre_num], DecCBFun))
			{
				dRet = PlayM4_GetLastError(nPort[thre_num]);
				break;
			}
			
			//����Ƶ����
			if (!PlayM4_Play(nPort[thre_num], NULL))
			{
				dRet = PlayM4_GetLastError(nPort[thre_num]);
				break;
			}
    
		}
		break;

	case NET_DVR_STREAMDATA:   //��������
		if (dwBufSize > 0 && nPort[thre_num] != -1)
		{

			BOOL inData = PlayM4_InputData(nPort[thre_num], pBuffer, dwBufSize);
			while (!inData)
			{
				Sleep(10);
				inData = PlayM4_InputData(nPort[thre_num], pBuffer, dwBufSize);
				OutputDebugString(L"PlayM4_InputData failed \n");
			}
		}
		break;
	}
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG UserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RECONNECT:    //Ԥ��ʱ����
		printf("----------reconnect--------%d\n", time(NULL));
		break;
	default:
		cout << dwType << "  error" << time(NULL) << endl;
		break;
	}
}




int GetMaxResolu(string ability) {
	size_t pos;
	size_t pos_r;
	string tmpstr;
	string reso_num;
	int max_reso = 0;
	int tmp_reso =0;
	int jiojio, jiojio2, w, h;
	int max_reso_deg;
	string a, b;
	while ((pos = ability.find_first_of('V')) != string::npos) {
		tmpstr = ability.substr(pos-1, strlen("<VideoResolutionEntry>"));
		ability = ability.substr(pos + 1, string::npos);
		if (tmpstr != "<VideoResolutionEntry>") {
			continue;
		}

		while ((pos = ability.find_first_of('I')) != string::npos) {
			tmpstr = ability.substr(pos-1, strlen("<Index>"));
			ability = ability.substr(pos + 1, string::npos);
			if (tmpstr != "<Index>") {
				continue;
			}

			pos_r = ability.find_first_of('<');
			reso_num = ability.substr(strlen("ndex>"), pos_r- strlen("ndex>"));


			pos = ability.find_first_of('*');
			jiojio = ((ability.substr(0, pos)).find_last_of('>')) + 1;
			jiojio2 = ((ability.substr(pos, string::npos)).find_first_of('<')) + pos;

			a = ability.substr(jiojio, pos - jiojio);
			b = ability.substr(pos + 1, jiojio2 - pos - 1);
			w = atoi(a.c_str());
			h = atoi(b.c_str());
			tmp_reso = w*h;

			if (max_reso < tmp_reso) {
				max_reso = tmp_reso;
				max_reso_deg = atoi(reso_num.c_str());
			}

			



			break;
		}
		
	} 
	return max_reso_deg;
}
unsigned readCamera(void *param)
{
	//---------------------------------------
	// ��ʼ��
	string *config = (string *)param;
	int thre_num = atoi(config[4].c_str());
	string cam_id = config[3];
	LONG lRealPlayHandle;
	//---------------------------------------
	// ��ȡ����̨���ھ��
	//HMODULE hKernel32 = GetModuleHandle((LPCWSTR)"kernel32");
	//GetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

	//---------------------------------------
	// ע���豸
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;


	//UserID = NET_DVR_Login_V30("192.168.0.66", 8000, "admin", "gdut709p40", &struDeviceInfo);

	UserID[thre_num] = NET_DVR_Login_V30((char*)config[0].data(), 8000, (char*)config[1].data(), (char*)config[2].data(), &struDeviceInfo);

	
	if (UserID[thre_num] < 0)
	{
		C_out(<< " <camerid>: " << config[3] << "\t: " << config[0] << "  login error! " << NET_DVR_GetLastError() << endl)
		return -1;
	}

	//��ȡ������,֧�ֵ����ֱ���
	char pOutBuf[20000] = {0};
	bool ret = NET_DVR_GetDeviceAbility(UserID[thre_num], DEVICE_ENCODE_ALL_ABILITY_V20, NULL,0, pOutBuf, 20000);
	if (!ret)
	{	
		C_out(<< " <camerid>: " << cam_id << " " << "Get GetDeviceAbility error: " << NET_DVR_GetLastError() << endl)
	}
	else {
		string ability(pOutBuf);
		int max_reso = GetMaxResolu(ability);


		//---------------------------------------
		//���������
		DWORD dwReturnLen;
		bool iRet;
		NET_DVR_COMPRESSIONCFG_V30 struParams = { 0 };
		iRet = NET_DVR_GetDVRConfig(UserID[thre_num], NET_DVR_GET_COMPRESSCFG_V30, 1, \
			&struParams, sizeof(NET_DVR_COMPRESSIONCFG_V30), &dwReturnLen);
		if (!iRet)
		{
			C_out(<< " <camerid>: " << cam_id << " " << "Get DVRConfig error: " << NET_DVR_GetLastError() << endl)
		}else {
			struParams.struNormHighRecordPara.wIntervalFrameI = 20;   //I֡���,0xfffe �Զ�����Դһ��
			struParams.struNormHighRecordPara.dwVideoFrameRate = Frame_Rate;   //֡��  5-1; 6-2; 7-4; 8-6;
			struParams.struNormHighRecordPara.byIntervalBPFrame = 2; // 2-��P֡
			struParams.struNormHighRecordPara.byResolution = max_reso; // 0xff-Auto(ʹ�õ�ǰ�����ֱ���)

			iRet = NET_DVR_SetDVRConfig(UserID[thre_num], NET_DVR_SET_COMPRESSCFG_V30, 1, \
				&struParams, sizeof(NET_DVR_COMPRESSIONCFG_V30));
			if (!iRet)
			{
				C_out(<< " <camerid>: " << cam_id << " " << "Set DVRConfig error: " << NET_DVR_GetLastError() << endl)

			}
		}
	}
	//cvNamedWindow("IPCamera");
	//---------------------------------------
	//����Ԥ�������ûص������� 
	NET_DVR_CLIENTINFO ClientInfo;
	ClientInfo.lChannel = 1;        //Channel number �豸ͨ����
	ClientInfo.hPlayWnd = NULL;     //����Ϊ�գ��豸SDK������ֻȡ��
	ClientInfo.lLinkMode = 0;       //Main Stream
	ClientInfo.sMultiCastIP = NULL;

	
	lRealPlayHandle = NET_DVR_RealPlay_V30(UserID[thre_num], &ClientInfo, fRealDataCallBack, (void*)config, TRUE);
	if (lRealPlayHandle<0)
	{
		C_out(<< " <camerid>: " << config[3] << ": " << config[0] << "  RealPlay error! " << NET_DVR_GetLastError() << endl)
		NET_DVR_Logout(UserID[thre_num]);
		return -1;
	}
	C_out( << " <camerid>: " << config[3] << "\t<ip>: " << config[0] << "\t<username>: " << config[1] << "\tInit succ"<< endl)
	
	while (!stop) {
		Sleep(1000);
	}
	NET_DVR_Logout(UserID[thre_num]);
	return 0;
	//---------------------------------------
	
}

void InitHK(void) {
	NET_DVR_Init();
	//��������ʱ��������ʱ��
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
	NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);
	for (int i = 0; i < 50; i++) {
		nPort[i] = -1;
		port_list[i] = -1234;
	}

}
/*
int InitialHK()
{
HANDLE hThread;
unsigned threadID;
Mat frame1;
namedWindow("video", 0);
InitializeCriticalSection(&g_cs_frameList);
hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&readCamera, NULL, 0, &threadID);
printf("Thread readCamera start��\n");
while (!stop) {
EnterCriticalSection(&g_cs_frameList);
if (g_frameList.size())
{

(*g_frameList.begin()).copyTo(frame1);
g_frameList.pop_front();
imshow("video", frame1);
}
LeaveCriticalSection(&g_cs_frameList);
if ((char)waitKey(30) == 27)
stop = true;
}
cvDestroyWindow("video");
g_frameList.clear();
getchar();
return 0;
}

*/