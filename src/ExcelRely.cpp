#include "netcam.h"
#include "serinum.h"
//excel相关***********************

extern CRITICAL_SECTION qrcode_CS;
extern CRITICAL_SECTION cout_CS;
extern bool stop_tmp;
extern SeriNum seri;
string qrcode_list[100];
int qrcode_detect[100] = { 0 };
int qrcode_occ[100] = { 0 };
string qrcode_camid[100];

extern string MacAdd;
//int time_col = 1;
//int qrcode_col = 2;
//int cam_id = 3;
//int row = 2;
//Sheet* sheet;
//Book* book;
//string ws2s(const wstring& ws) {
//	size_t convertedChars = 0;
//	string curLocale = setlocale(LC_ALL, NULL); //curLocale="C"
//	setlocale(LC_ALL, "chs");
//	const wchar_t* wcs = ws.c_str();
//	size_t dByteNum = sizeof(wchar_t)*ws.size() + 1;
//	//cout << "ws.size():" << ws.size() << endl;            //5
//
//	char* dest = new char[dByteNum];
//	wcstombs_s(&convertedChars, dest, dByteNum, wcs, _TRUNCATE);
//	//cout << "convertedChars:" << convertedChars << endl; //8
//	string result = dest;
//	delete[] dest;
//	setlocale(LC_ALL, curLocale.c_str());
//	return result;
//}

//wstring s2ws(const string& s) {
//	size_t convertedChars = 0;
//	string curLocale = setlocale(LC_ALL, NULL);   //curLocale="C"
//	setlocale(LC_ALL, "chs");
//	const char* source = s.c_str();
//	size_t charNum = sizeof(char)*s.size() + 1;
//	//cout << "s.size():" << s.size() << endl;   //7
//
//	wchar_t* dest = new wchar_t[charNum];
//	mbstowcs_s(&convertedChars, dest, charNum, source, _TRUNCATE);
//	//cout << "s2ws_convertedChars:" << convertedChars << endl; //6
//	wstring result = dest;
//	delete[] dest;
//	setlocale(LC_ALL, curLocale.c_str());
//	return result;
//}

int InitExcel(void) {


	InitializeCriticalSection(&qrcode_CS);
	_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Timer, NULL, 0, NULL);

	//book = xlCreateBook();
	//if (!book) {
	//	std::cout << "xlCreateBook fail!" << endl;
	//	return -1;
	//}
	//if (book->load(L"capture.xls")) {
	//	sheet = book->getSheet(0);
	//}
	//else {
	//	sheet = book->addSheet(L"Sheet1");
	//	cout << "CreateBook new excel!" << endl;
	//}
	//if (!sheet) {
	//	cout << "getSheet fail!" << endl;
	//	return -1;
	//}

	//while (sheet->cellType(row, qrcode_col) != CELLTYPE_EMPTY) {
	//	row++;
	//}

	//cout << "write to excel row No." << row << endl;

	return 0;
}

int WriteExcel(Result result,string real_cam_id) {

	//Excel相关，为了让数据不重复保存
	EnterCriticalSection(&qrcode_CS);

	if (stop_tmp) {
		LeaveCriticalSection(&qrcode_CS);
		return 0;
	}
	int old_data = 0;
	for (int i = 0; i<result.count; i++) {
		for (int j = 0; j<100; j++)
			if (qrcode_occ[j] == 1)
				if (!strcmp(result.info[i].c_str(), qrcode_list[j].c_str())) {
					//if(!strcmp(real_cam_id.c_str(), qrcode_camid[j].c_str())){
						qrcode_detect[j] = 0;
						old_data = 1;
						break;
				}

		if (!old_data) {
			C_out( << "<camer_id> "<<real_cam_id<<"\t<QR_info> "<<result.info[i] << endl)
			int k;
			for (k = 0; k < 100; k++) {
				if (qrcode_occ[k] == 0) {
					break;
				}

			}
			if (k >= 100) {
				k = 0;
			}
			qrcode_list[k] = result.info[i];
			qrcode_detect[k] = 0;
			qrcode_occ[k] = 1;
			qrcode_camid[k] = real_cam_id;
			time_t t1;
			time(&t1);
			char DateBuf[64];
			strftime(DateBuf, sizeof(DateBuf), "%Y%m%d%H%M%S", localtime(&t1));
			string str_time(DateBuf);

			//sheet->writeStr(row, time_col, s2ws(str_time).c_str());
			//sheet->writeStr(row, qrcode_col, s2ws(qrcode_list[k]).c_str());
			//sheet->writeStr(row, cam_id, s2ws(real_cam_id).c_str());
			//row++;
			//if (!book->save(L"capture.xls")) {
			//	cout << "File save fail." << endl;
			//}

			string qr_tcp_msg;
			qr_tcp_msg = "{\"SerialNum\":\"" +seri.getValue() +"\",\"cameraID\":\"" + real_cam_id + "\",\"mac\":\"" + MacAdd + "\",\"qrcode\":\"" + qrcode_list[k] + "\",\"time\":\"" + str_time + "\"}";
			Qr_Data_Ser(qr_tcp_msg);


		}
		old_data = 0;

	}
	LeaveCriticalSection(&qrcode_CS);
	return 0;
}
void ReleaseExcel(void) {
	//book->release();
	return;
}
unsigned Timer(void *param)
{
	while (!stop) {

		if (stop_tmp) {
			EnterCriticalSection(&qrcode_CS);
			for (int i = 0; i < 100; i++) {
				qrcode_occ[i] = 0;
				qrcode_detect[i] = 0;
			}
			LeaveCriticalSection(&qrcode_CS);
			Sleep(1000);
			continue;
		}

		EnterCriticalSection(&qrcode_CS);
		for (int i = 0; i < 100; i++) {
			if (qrcode_occ[i] == 1) {
				qrcode_detect[i]++;
				if (qrcode_detect[i] >= 20) {
					//cout << qrcode_list[i] << "  ：" << "out of " << 20 << "s" << endl;
					qrcode_occ[i] = 0;
					qrcode_detect[i] = 0;
				}
			}
		}
		LeaveCriticalSection(&qrcode_CS);
		Sleep(1000);
	}
	return 0;
}
