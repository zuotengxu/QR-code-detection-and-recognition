#include<WINSOCK2.H>
#include<STDIO.H>
#include<iostream>
#include <string>
#include <iphlpapi.h>
#include <fstream> 
#include "netcam.h"
#include "serinum.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

using namespace std;

extern bool stop_tmp;
bool unnormal = false;
SeriNum seri;

bool GetMacByGetAdaptersInfo(std::string& macOUT, string& descripe);
string MacAdd = "unkonw";


SOCKET s_data;
SOCKET slisten;

fd_set ContolSet;
fd_set readSet;


string ser_ip;
string ser_port;
int InitDataSer() {

	s_data = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_data == INVALID_SOCKET)
	{
		cout << "data server socket error!" << endl;
		return -1;
	}
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(atoi(ser_port.c_str()));
	serAddr.sin_addr.S_un.S_addr = inet_addr(ser_ip.c_str());
	if (connect(s_data, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{  
		cout << "data server connect error!" << endl;
		closesocket(s_data);
		return -1;
	}
	//cout << "数据服务器连接成功，IP：" << ser_ip << " 端口："<< ser_port  << endl;

	return 0;

}

int InitContolSer() {
	ifstream config_file(string("config//") + "server_config");
	if (!config_file.is_open()) {
		cout << "open config file fail!" << endl;
		return -1;
	}

	string ser_port;
	string buf;
	getline(config_file, buf);
	getline(config_file, buf);
	getline(config_file, buf);
	getline(config_file, buf);

	getline(config_file, ser_port);
	config_file.close();
	//s_contol = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET)
	{
		printf("contol socket error !");
		return -1;
	}

	//绑定IP和端口  
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(ser_port.c_str()));
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("contol bind error !");
		return -1;
	}

	//开始监听  
	if (listen(slisten, 5) == SOCKET_ERROR)
	{
		printf("listen error !");
		return -1;
	}

	cout << "控制端监听端口：" << ser_port << endl;
	return 0;
}

int InitClient()
{
 
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		cout << "WSAStartup error!" << endl;
		return -1;
	}

	string describe;
	if (GetMacByGetAdaptersInfo(MacAdd, describe) == false)
		cout << "Get MacAdd fail" << endl;
	cout << "网卡: " << describe << endl;
	cout << "Mac地址: " << MacAdd << endl;
	cout << "上次流水号：" << seri.getValue() << endl;

	ifstream config_file(string("config//") + "server_config");
	if (config_file.is_open()) {
		string buf;
		getline(config_file, buf);
		getline(config_file, ser_ip);
		getline(config_file, ser_port);
		config_file.close();
		cout << "数据服务器ip：" << ser_ip << "  端口：" << ser_port << endl;
	}
	else {
		cout << "open config file fail!" << endl;
		return -1;
	}
	
	if (InitContolSer() != 0) {
		cout << "控制端监听失败" << endl;
		return -1;
	};
	
	FD_ZERO(&ContolSet);
	FD_SET(slisten, &ContolSet);
	FD_ZERO(&readSet);



}

int Qr_Data_Ser(string data) {
	const char * sendData;
	sendData = data.c_str();
	static Mat ImgSerError = imread("error");
	unnormal = false;
	if (InitDataSer() != 0)
		unnormal = true;
	else {
		int ret = send(s_data, sendData, strlen(sendData), 0);
		if (ret == SOCKET_ERROR)//发送失败。
		{
			cout << "Send error!  " << sendData << endl;
			unnormal = true;
		}
		closesocket(s_data);
	}
	if (unnormal) {
		imshow("警告", ImgSerError);
		waitKey(3000);
		cvDestroyWindow("警告");
	}else
	return 0;
}
int receive_data() {
	SOCKET s_contol;
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	s_contol = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
	if (s_contol == INVALID_SOCKET)
	{
		printf("accept error !");
		return -1;
	}

	char recData[1000];
	int ret = recv(s_contol, recData, 900, 0);
	if (ret <= 0) {
		cout << "receive data error!" << endl;
		closesocket(s_contol);
		return -1;
	}
	
	recData[ret] = 0x00;
	if (strcmp(recData, "stop") == 0) {
		cout << "Receive stop signal" << endl;
		stop_tmp = 1;

	}
	else if (strcmp(recData, "start") == 0) {
		cout << "Receive start signal" << endl;
		if (stop_tmp != 0) {
			seri.add();
			stop_tmp = 0;
		}
	}
	else {
		cout << "receive unkonw data" << endl;
	}
	string state = (stop_tmp == 0 ? "start" : "stop");
	string respond = "{\"SerialNum\":\"" + seri.getValue() + "\",\"state\":\"" + state + "\",\"mac\":\"" + MacAdd + "\"}";
	send(s_contol, respond.c_str(), respond.length(), 0);
	
	closesocket(s_contol);


}
int Qr_Contol_Ser()
{
	TIMEVAL time = { 1,0 };
	readSet = ContolSet;
	int nRetAll = select(0, &readSet, NULL, NULL, &time);

	if (nRetAll <= 0)
		return 0;
	if (FD_ISSET(slisten, &readSet)) 
		receive_data();
	
	return 0;
}


int ClearClient() {

	closesocket(slisten);
	closesocket(s_data);
	WSACleanup();
	return 0;
}


bool GetMacByGetAdaptersInfo(std::string& macOUT,string& descripe)
{
	bool ret = false;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
		return false;
	// Make an initial call to GetAdaptersInfo to get the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
		if (pAdapterInfo == NULL)
			return false;
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
	{
		for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter != NULL; pAdapter = pAdapter->Next)
		{
			// 确保是以太网
			//cout << pAdapter->Type << endl;
			//cout << pAdapter->Description << endl;
			if (pAdapter->Type != MIB_IF_TYPE_ETHERNET)
				continue;
			// 确保MAC地址的长度为 00-00-00-00-00-00
			if (pAdapter->AddressLength != 6)
				continue;
			char acMAC[32];
			sprintf(acMAC, "%02X-%02X-%02X-%02X-%02X-%02X",
				int(pAdapter->Address[0]),
				int(pAdapter->Address[1]),
				int(pAdapter->Address[2]),
				int(pAdapter->Address[3]),
				int(pAdapter->Address[4]),
				int(pAdapter->Address[5]));
			macOUT = acMAC;
			descripe = pAdapter->Description;
			//cout << macOUT << endl;
			ret = true;
			break;
		}
	}

	free(pAdapterInfo);
	return ret;
}