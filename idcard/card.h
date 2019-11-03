#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
using namespace std;
using namespace cv;
using namespace cv::ml;

class Card
{
public:
	Card();
	~Card();

	// 用户接口
	const int identify(const string path, string& txt);		// 识别
	void show(string winName, const int x = 0, const int y = 0); // 显示框选、识别的图片
	void setPicFolderPath(const string path);				// 设置图片文件夹路径
	void setTrainDataFolderPath(const string path);			// 设置 SVM 训练数据路径
	void setTrain(string _TRAIN);							// 设置是否训练 SVM
	void setDebug(string _Debug);							// 设置 Debug 模式
	bool is_DEBUG();										// 返回模式
	void setSavePath(const string path);					// 设置识别结果保存路径
	const string getPicFolderPath();						// 获取图片文件夹路径

private:
	// 身份证数据
	Mat idcard;												// 身份证原图
	Mat img;												// 过程图
	Mat idcardNumber;										// 保存号码截图
	vector<Mat> idcardNumbers;								// 保存切割成单字符的证件号
	vector<RotatedRect> rotatedRects;						// 检测到的连通域
	Ptr<SVM> svm;											// SVM 分类器
	vector<int> predictNumber;								// 保存号码识别结果
	string trainDataFolderPath;								// 数据文件夹路径
	string imgFolderPath;									// 待识别图片文件夹路径
	string imgPath;											// 图片路径	
	string savePath;										// 识别结果保存路径
	bool TRAIN = true;										// 是否训练 SVM
	bool DEBUG = false;										// 是否 DEBUG 模式
	map<int, string> mapPlace;								// 身份证前两位号码地点映射
	float time = 0;											// 识别时间

	// 实现函数
	void resize(const int toRows = 800, const int toCols = 800);// 原图比例缩放至预设大小
	void resize(Mat& img);									// 数字尺寸归一化
	void preDeal();											// 预处理：缩放、滤波、边缘、二值化
	const int detect();										// 检测连通域
	const int findNumber();									// 找出连通域中的号码区域
	void thin();											// 细化（可有可无 未实现）
	const int correct();									// 尾号校验（未实现）
	const int predict();									// 识别号码
	void twoPass();											// 两遍扫描法 标记连通域
	void release();											// 每次识别完一张身份证需要释放内存
};
