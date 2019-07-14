#include "netcam.h"

#define WALK 2

#define SHOW(a,b) {	namedWindow(a, 0);\
					imshow(a, b);\
					waitKey(1);\
					 }

string GetQRInBinImg(Mat img)
{
	string result;
	//int count =0;

	ImageScanner scanner;
	scanner.set_config(ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
	//stringstream aos;
	const void *raw = (&img)->data;
	// wrap image data
	Image image(img.cols, img.rows, "Y800", raw, img.cols * img.rows);
	// scan the image for barcodes
	int n = scanner.scan(image);

	Image::SymbolIterator symbol = image.symbol_begin();
	// extract results
	for (; symbol != image.symbol_end(); ++symbol) {
		//aos << symbol->get_data() << "\n";
		result = result + symbol->get_data() + '#';


									//count++;
	}
	//aos << count;
	image.set_data(NULL, 0);
	return result;
}

//main function
string GetQR(Mat img)
{
	Mat binImg;
	int thre;
	int thre_ostu;
	string result = GetQRInBinImg(img);
	if (!result.empty()) {
		//cout << "succ without thre" << endl;
		return result;
	}
	//-30到+30
	equalizeHist(img, binImg); //直方图均衡化，意义有待考究。如果图像的背景色彩占据了大部分的像素，那么扩展的只是背景的像素范围
	thre_ostu = threshold(img, binImg, 0, 255, cv::THRESH_OTSU);
	//cout << "thre_ostu:"<< thre_ostu << endl;
	result = GetQRInBinImg(binImg);
	if (!result.empty()) {
		//cout << "              succ in ori thre" << thre_ostu << endl;
		return result;
	}

	thre = thre_ostu;
	while (result.empty() && thre > 50 && thre > thre_ostu-30)
	{
		thre -= WALK;
		threshold(img, binImg, thre, 255, cv::THRESH_BINARY);
		result = GetQRInBinImg(binImg);
		//阈值步长设为20，步长越大，识别率越低，速度越快
	}
	if (!result.empty()) {
		//cout << thre_ostu << " " << thre << endl;
		return result;
	}

	thre = thre_ostu;
	while (result.empty() && thre<210 && thre < thre_ostu+40)
	{
		thre += WALK;//阈值步长设为20，步长越大，识别率越低，速度越快
		threshold(img, binImg, thre, 255, cv::THRESH_BINARY);
		result = GetQRInBinImg(binImg);
	}
	if (!result.empty()) {
		return result;
	}
	//cout<<thre<<endl;
	return result;
}

Mat pre_canny(Mat image) {

	Mat gray_x, gray_y;

	cv::resize(image.clone(), image, Size(), 0.5, 0.5);
	blur(image, image, Size(3, 3));
	Sobel(image, gray_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
	Sobel(image, gray_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT);

	addWeighted(gray_x, 0.5, gray_y, 0.5, 0, image);
	convertScaleAbs(image.clone(), image);

	equalizeHist(image, image);

	threshold(image, image, 240, 255, cv::THRESH_BINARY);

	cv::resize(image.clone(), image, Size(), 0.5, 0.5);
	threshold(image, image, 10, 255, cv::THRESH_BINARY);



	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));    //膨胀运算
	morphologyEx(image, image, MORPH_DILATE, element);

	element = getStructuringElement(MORPH_RECT, Size(5, 5));      //腐蚀
	morphologyEx(image, image, MORPH_ERODE, element);

	morphologyEx(image, image, MORPH_OPEN, element);
	morphologyEx(image, image, MORPH_OPEN, element);
	morphologyEx(image, image, MORPH_OPEN, element);
	morphologyEx(image, image, MORPH_OPEN, element);

	element = getStructuringElement(MORPH_RECT, Size(9, 9));    //膨胀运算
	morphologyEx(image, image, MORPH_DILATE, element);

	return image;


}
int just_QRcord(Mat image) {

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	int count = 0;
	blur(image.clone(), image, Size(3, 3)); //模糊，去除毛刺
	int thre = threshold(image.clone(), image, 50, 255, THRESH_OTSU);
	do {
		findContours(image.clone(), contours, hierarchy, CV_RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));
		int c = 0, ic = 0, area = 0;
		int parentIdx = -1;
		for (int i = 0; i < contours.size(); i++)
		{
			//hierarchy[i][2] != -1 表示有内层轮廓
			if (hierarchy[i][2] != -1 && ic == 0)
			{
				parentIdx = i;
				ic++;
			}
			else if (hierarchy[i][2] != -1)
			{
				ic++;
			}
			//最外面的清0
			else if (hierarchy[i][2] == -1)
			{
				ic = 0;
				parentIdx = -1;
			}
			//找到定位点信息
			if (ic >= 2)
			{
				count++;
				ic = 0;
				parentIdx = -1;
			}

		}

		//cout << count << endl;
		if (count >= 2) {
			//cout << count << endl;
			//imshow("catch", image);
			return 1;
		}
		thre -= 20;
		count = 0;
		threshold(image.clone(), image, thre, 255, THRESH_BINARY);
	} while (thre > 0);
	return 0;
}

int canny_rect(Mat image,Rect rect_all[]) {
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//CV_RETR_EXTERNAL外围 CV_RETR_LIST全部
	findContours(image.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);//检测所有轮廓  

	int idx = 0;
	int i = 0;


	int img_area = image.rows*image.cols;
	int len_min = image.rows / 18;
	//cout << hierarchy.size() << endl;
	//if(hierarchy.size() !=0)
	//i< contours.size();
	//for (; idx >= 0 && i <20; idx = hierarchy[idx][0])
	for (; idx < contours.size() && i <100; idx++)
	{
		Rect rect_on = boundingRect(contours[idx]);

		//&&rect.area()<(img_area/20)
		//(img_area / 400)<rect_on.area() &&
		if (rect_on.width >= len_min
			&& rect_on.height >= len_min
			&& rect_on.area()<(img_area / 2)
			&& ((sum(image(rect_on)) / 255)[0] / rect_on.area() > 0.3))
		{


			rect_on.x *= 4;
			rect_on.y *= 4;
			rect_on.width *= 4;
			rect_on.height *= 4;
			//cv::resize(image.clone(), image, Size(), 4, 4);
			rect_all[i] = rect_on;
			i++;
		}

	}
	vector<Vec4i>().swap(hierarchy);
	vector<vector<Point>>().swap(contours);
	return i;

}



int pre_haar(Mat flame, Rect rect_all[]) {

	CascadeClassifier face_cascade;
	String face_cascade_name = "1.xml";
	if (!face_cascade.load(face_cascade_name)) {
		cout << "--(!)Error loading face cascade" << endl;
		return 0;
	}
	std::vector<Rect> faces;
	Mat flame2;
	cv::resize(flame.clone(), flame2, Size(), 0.5, 0.5);
	int qr_min = flame2.rows / 40;
	int qr_max = flame2.rows / 1;
	
	face_cascade.detectMultiScale(flame2.clone(), faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(qr_min, qr_min), Size(qr_max, qr_max));

	int exp_rate;
	Rect flame_rect(0, 0, flame.cols, flame.rows);

	for (size_t i = 0; i < faces.size() && i < 100; i++) {

		rect_all[i] = faces[i];

		rect_all[i].x *= 2;
		rect_all[i].y *= 2;
		rect_all[i].width *= 2;
		rect_all[i].height *= 2;

		exp_rate = rect_all[i].width / 8;
		rect_all[i] -= Point(exp_rate, exp_rate);
		rect_all[i] += Size(exp_rate * 2, exp_rate * 2);
		rect_all[i] = rect_all[i] & flame_rect;
	}

	return faces.size();

}
Result MyScan(Mat flame,bool by_canny) {
	//MORPH_OPEN 
	//Mat image = flame.clone();
	Rect rect_all[100];
	int i;
	Result result;
	result.count = 0;


	if (by_canny) {
		Mat image_canny;
		image_canny = pre_canny(flame.clone());
		i = canny_rect(image_canny.clone(), rect_all);

	}
	else {
		i = pre_haar(flame.clone(), rect_all);
	}

	//**************************
	int just_pass = 0;
	for (i--; i >= 0; i--) {
		//rectangle(image, rect_all[i], Scalar(255, 255, 255), 5);
		if (just_QRcord(flame(rect_all[i]).clone())) {

			result.rect_all[99 - just_pass] = rect_all[i];
			result.info[99 - just_pass] = "";
			just_pass++;
			//rectangle(image, rect_all[i], Scalar(255, 255, 255), 15);
			string info = GetQR(flame(rect_all[i]).clone());
			if (!info.empty()) {
				size_t pos = info.find_first_of('#');
				size_t str_size = info.size();
				while (pos != std::string::npos)
				{
					std::string x = info.substr(0, pos);
					result.info[result.count] = x;
					result.rect_all[result.count] = rect_all[i];
					result.count++;

					info = info.substr(pos + 1, str_size);
					pos = info.find_first_of('#');
				}
			}
		}

	}
	result.info[99 - just_pass] = "just_pass";
	//SHOW("123", image);

	return result;
}


void cv_range_0to255(IplImage *src, IplImage *dst)//两个参数都是"64F, 2"  
{
	double m, M;
	double scale;
	double shift;

	cvMinMaxLoc(src, &m, &M, NULL, NULL);
	scale = 255 / (M - m);
	shift = -m * scale;
	cvConvertScale(src, dst, scale, shift);
}







//equalizeHist(image,image); //直方图均衡化
//blur(image, image, Size(3, 3));    //均值滤波
//threshold(image, image, 200, 255, cv::THRESH_BINARY);
// medianBlur(image, result, 7); 中值滤波，去点点


