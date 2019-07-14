#include <iostream>
#include <string>
#include <fstream> 
#include <time.h>
using namespace std; 
class SeriNum {
public:
	SeriNum() {

		string buf;
		ifstream Seri_file("SerialNum");
		if (!Seri_file.is_open()) {
			cout << "获取上次流水号失败!" << endl;
			backNum = 0;
			date = "00000000";
		}
		else {
			getline(Seri_file, buf);
			date = buf;
			getline(Seri_file, buf);
			backNum = atoi(buf.c_str());
			Seri_file.close();

			if (date.length() != 8 || backNum < 0 || backNum > 9999) {
				cout << "流水号记录文件出错!" << endl;
				backNum = 0;
				date = "00000000";
			}
		}

	};
	void add() {
		if (date == "00000000") {
			cout << "流水号记录文件出错!" << endl;
			return;
		}

		time_t t1;
		time(&t1);
		char DateBuf[64];
		strftime(DateBuf, sizeof(DateBuf), "%Y%m%d", localtime(&t1));
		if (date != string(DateBuf)) {
			date = string(DateBuf);
			backNum = 0;
		}
		else
			backNum++;


		ofstream Serio("SerialNum");
		Serio << date << endl;
		Serio << backNum;
		Serio.close();
		

	};
	string getValue() {
		string value;
		if (backNum < 10)
			value = date + "000" + to_string(backNum);
		else if (backNum < 100)
			value = date + "00" + to_string(backNum);
		else if (backNum < 1000)
			value = date + "0" + to_string(backNum);
		else
			value = date + to_string(backNum);
		return value;
	};
private:
	string date;
	int backNum;

};

#pragma once
