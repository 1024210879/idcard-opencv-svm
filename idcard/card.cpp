#include "card.h"
#include "path.h"

/**************************** public ****************************/

Card::Card()
{
	// 身份证号前两位数字
	vector<int> key = {
		11, 12, 13, 14, 15,
		21, 22, 23,
		31, 32, 33, 34, 35, 36, 37,
		41, 42, 43,
		44, 45, 46,
		51, 52, 53, 54, 50,
		61, 62, 63, 64, 65,
		71, 81, 82
	};
	vector<string> value = {
		"北京","天津","河北","山西","内蒙古",
		"辽宁","吉林","黑龙江",
		"上海","江苏","浙江","安徽","福建","江西","山东",
		"河南","湖北","湖南",
		"广东","广西","湖南",
		"四川","贵州","云南","西藏","重庆",
		"陕西","甘肃","青海","宁夏","新疆",
		"台湾","香港","澳门"
	};
	for (int i = 0; i < key.size(); i++)
		mapPlace[key[i]] = value[i];
}

Card::~Card()
{
	idcard.release();
	img.release();
	map<int, string>().swap(mapPlace);
}

const int Card::identify(string path, string& txt)
{
	release();										// 识别前释放之前的内存
	txt.clear();									// 赋值前清空
	time = 0;										// 重置识别时间

	imgPath = path;									// 保存图片路径
	img = imread(imgPath);							// 读图
	if (img.empty()) 								// 保证传入的图像非空
		return 1;
	idcard = img.clone();							// 保存一份原图
	resize();										// 缩放大小
	img = idcard.clone();							// 过程图

	TickMeter _t;									// 记录运行时间
	_t.reset();										// 重置时间
	_t.start();										// 开始计时

	preDeal();										// 预处理
	if (detect()) {									// 检测
		_t.stop();
		time = _t.getTimeSec();						// 获得识别耗时
		return 2;									// 返回无法检测
	}
	if (findNumber()) {								// 找号码
		_t.stop();
		time = _t.getTimeSec();						// 获得识别耗时
		return 3;									// 返回无法找到号码
	}
	if (predict() || correct()) {					// 识别号码
		_t.stop();
		time = _t.getTimeSec();						// 获得识别耗时
		return 4;									// 返回识别错误
	}
	_t.stop();										// 停止计时
	time = _t.getTimeSec();							// 获得识别耗时

	for (auto x : predictNumber)					// 转换为字符串
		txt += x == 10 ? "X" : to_string(x);
	idcard(Rect(10, 10, 380, 40)) = Scalar(80, 80, 80);// 绘制灰色矩形背景
	putText(idcard, txt, Point(20, 40), 1, 2, Scalar(0, 255, 0), 2);// 绘制数字

	static int cnt = 0;								// 静态变量 用于编号
	imwrite(savePath + to_string(cnt++) + ".jpg", idcard);	// 保存识别结果
	return 0;
}

void Card::show(string winName, const int x, const int y)
{
	if (idcard.empty())
		return;

	winName += (" | time: " + to_string(time) + " s");
	namedWindow(winName);
	moveWindow(winName, x, y);
	imshow(winName, idcard);

	//waitKey();										// 等待按键
	//destroyWindow(winName);							// 销毁窗口
	release();										// 释放内存
}

void Card::setPicFolderPath(const string path)
{
	imgFolderPath = path;
}

void Card::setTrainDataFolderPath(const string path)
{
	trainDataFolderPath = path;
}

void Card::setTrain(string _TRAIN)
{
	transform(_TRAIN.begin(), _TRAIN.end(), _TRAIN.begin(), ::tolower);
	TRAIN = _TRAIN == "true" ? true : false;

	if (TRAIN)										// 是否训练
	{
		TickMeter trainT;
		trainT.reset();
		trainT.start();

		cout << "------- TRAIN SVM START -------" << endl;
		Mat trainImages;							// 训练数据
		vector<int> trainLabels;					// 训练标签

		svm = SVM::create();
		svm->setType(SVM::C_SVC);
		svm->setKernel(SVM::LINEAR);
		svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6));

		int classes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };	// 10 代表 X
		vector<string> files;

		for (auto x : classes)						// 遍历每个分类文件夹
		{
			getFileNames(trainDataFolderPath + to_string(x), files);// 获取该分类文件夹的所有图片
			for (auto file : files)					// 读取该类图片
			{
				img = imread(file, 0);				// 读取灰度图
				if (img.empty()) continue;			// 判断是否为空
				threshold(img, img, 10, 255, THRESH_OTSU); // 二值化
				img.convertTo(img, CV_32FC1);		// 转换为 float 类型单通道图像
				trainImages.push_back(img.reshape(0, 1));// 图片转化为一行 加入训练数据中
				trainLabels.push_back(x);			// 保存对应图片标签
			}
			files.clear();							// 遍历下一个文件夹前清空
		}
		img.release();								// 释放 img 占用的内存

		// 训练
		svm->train(trainImages, ROW_SAMPLE, trainLabels);// 训练模型
		svm->save(trainDataFolderPath + "svm.xml");	//保存模型
		svm->clear();
		trainT.stop();
		cout << "train time: " << trainT.getTimeSec() << " s" << endl;
		cout << "------- TRAIN SVM  END  -------" << endl;
	}
}

void Card::setDebug(string _Debug)
{
	transform(_Debug.begin(), _Debug.end(), _Debug.begin(), ::tolower);
	DEBUG = _Debug == "true" ? true : false;
}

void Card::setSavePath(const string path)
{
	savePath = path;
}

const string Card::getPicFolderPath()
{
	return imgFolderPath;
}

bool Card::is_DEBUG()
{
	return this->DEBUG;
}

/**************************** private ***************************/
// 使原图尺寸缩放至 toRows * toCols 左右
void Card::resize(const int toRows, const int toCols)
{
	int lr = 0, lc = 0;								// 记录迭代时较小的 row col
	int hr = idcard.rows, hc = idcard.cols;			// 记录迭代时较大的 row col	
	int mr = (lr + hr) >> 1, mc = (lc + hc) >> 1;	// 记录迭代时二分点 row col
	int size = mr * mc;								// 当前二分点尺寸
	int toSize = toRows * toCols;					// 目标尺寸
	int sub = toSize - size;						// 当前尺寸与目标尺寸差

	// 使尺寸差小于两倍长宽和
	int delta = (toCols + toRows) << 1;				// 允许缩放至目标尺寸的误差
	while (abs(sub) > delta)						// 差大于误差时继续循环
	{
		// 原图大于目标图
		if (sub < 0) { hr = mr; hc = mc; }			// 降低较大的 row col
		// 原图小于目标图
		else { hr += mr; hc += mc; lr = mr; lc = mc; }// 提升较小的 row col
		mr = (hr + lr) >> 1;						// 更新二分点 row
		mc = (hc + lc) >> 1;						// 更新二分点 col
		size = mr * mc;								// 更新当前尺寸
		sub = toSize - size;						// 更新当前差值
	}
	cv::resize(idcard, idcard, Size(mc, mr));		// OpenCV 双线性插值缩放
}

void Card::resize(Mat& _img)
{
	int k = _img.rows - _img.cols;						// 高 - 宽
	if (k > 0) {										// 如果宽 < 高 用黑色填充左右部分
		copyMakeBorder(_img, _img, 0, 0, k/2, k - k/2, BORDER_CONSTANT, 0);
	}
	else {												// 用黑色填充上下部分
		k = -k;
		copyMakeBorder(_img, _img, k/2, k - k/2, 0, 0, BORDER_CONSTANT, 0);	
	}
	cv::resize(_img, _img, Size(28, 28));				// 归一化到 28 * 28
}

void Card::preDeal()
{
	if (DEBUG)
	{
		imshow("0_src", idcard);
	}
	bilateralFilter(idcard, img, 7, 10, 5);				// 双边滤波
	idcard = img.clone();								// 保存双边滤波后的原图
	if (DEBUG)
	{
		imshow("predeal_0_bilateraFilter", img);
	}

	// 灰度化
	//vector<Mat> m;										// 存储分离的 BGR 通道
	//cv::split(idcard, m);								// 通道分离
	//img = m[2].clone();									// 保留 R 通道
	//vector<Mat>().swap(m);								// 清空 m 占用的内存
	cvtColor(img, img, COLOR_BGR2GRAY);
	if (DEBUG)
	{
		imshow("predeal_1_gray", img);
	}

	// 形态学边缘检测
	Mat tmp = img.clone();								// 临时存储空间
	morphologyEx(										// 形态学运算
		img,											// 输入图像
		tmp,											// 输出图像
		MORPH_CLOSE,									// 指定闭运算 连通破碎区域
		getStructuringElement(MORPH_RECT, Size(7, 7))	// 获取结构核
	);
	img = tmp - img;									// 作差

	if (DEBUG)
	{
		imshow("predeal_2_close", tmp);
		imshow("predeal_3_gray - close", img);
	}

	tmp.release();										// 释放临时空间

	// 二值化
	threshold(img, img, 0, 255, THRESH_OTSU);			// 二值化
	if (DEBUG)
	{
		imshow("predeal_4_threshold_otsu", img);
	}
}

const int  Card::detect()
{
	// 闭运算 形成连通区域
	morphologyEx(										// 形态学运算
		img,											// 输入图像
		img,											// 输出图像
		MORPH_CLOSE,									// 指定闭运算 连通号码区域
		getStructuringElement(MORPH_RECT, Size(21, 13))	// 获取结构核
	);

	if (DEBUG)
	{
		imshow("detect_0_close", img);
	}

	// 查找轮廓
	vector<vector<Point>> contours;						// 保存轮廓点
	vector<Vec4i> hierarchy;							// 轮廓的层级关系
	findContours(										// 查找轮廓
		img,											// findContours 会改变输入图像
		contours,										// 输出轮廓
		hierarchy,										// 层级关系 此处只需一个占位
		RETR_TREE,										// 树形式保存
		CHAIN_APPROX_NONE								// 不做近似
	);	
	img.release();										// 清空 img 占用的内存
	vector<Vec4i>().swap(hierarchy);					// 清空 hierarchy 占用的内存

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(idcard.size(), CV_8UC1);
		for (int dbg_i = 0; dbg_i < contours.size(); dbg_i++)
			drawContours(dbg_img, contours, dbg_i, 255, 1, 8);
		imshow("detect_1_find_contours", dbg_img);
	}

	// 筛选轮廓
	vector<vector<Point>> contours_number;				// 保存可能的号码区域
	for (auto itr = contours.begin(); itr != contours.end(); itr++)	
		if (itr->size() > 400)							// 保留轮廓点数多于 400 的
			contours_number.push_back(*itr);			// 保存该轮廓
	vector<vector<Point>>().swap(contours);				// 释放 contours 占用的内存

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(idcard.size(), CV_8UC1);
		for (int dbg_i = 0; dbg_i < contours_number.size(); dbg_i++)
			drawContours(dbg_img, contours_number, dbg_i, 255, 1, 8);
		imshow("detect_2_filter_contours", dbg_img);
	}

	// 最小包围矩形框
	int i = 0;
	for (auto itr = contours_number.begin(); itr != contours_number.end(); itr++, i++)
	{
		RotatedRect rotatedRect = minAreaRect(*itr);	// 从点集求出最小包围旋转矩形
		const float width = rotatedRect.size.width;		// x 轴逆时针旋转得到的第一条边定义为宽
		const float height = rotatedRect.size.height;	// 另一条边定义为高
		const float k = height / width;					// 高比宽
		if (											// 筛选
			width < 15 || height < 15					// 边长过小
			|| (0.1 < k && k < 10)						// 宽高比在一定范围（不是很长的矩形）
		)
			continue;

		rotatedRects.push_back(rotatedRect);			// 保存
	}
	vector<vector<Point>>().swap(contours_number);		// 释放 contours_number 占用的内存

	if (rotatedRects.empty()) return 2;					// 如果未检测到符合条件的连通域直接返回 2

	if (DEBUG)
	{
		Point2f dbg_p[4];
		Mat dbg_img = idcard.clone();
		for (auto dbg_rotatedRect : rotatedRects)
		{
			dbg_rotatedRect.points(dbg_p);
			for (int dbg_i = 0; dbg_i < 4; dbg_i++)
				line(dbg_img, dbg_p[dbg_i], dbg_p[(dbg_i + 1) % 4], Scalar(0, 0, 255), 2, 8);
		}
		imshow("detect_3_filter_rotated_rect", dbg_img);
	}

	return 0;
}

const int Card::findNumber()
{
	// 记录下来的所有旋转矩形按照中心坐标 x 进行从右到左排序
	sort(rotatedRects.begin(), rotatedRects.end(), [](RotatedRect a, RotatedRect b) {
		return a.center.x > b.center.x;
	});

	// 透视变换设置
	const int toCols = 504, toRows = 28;				// 最终希望的号码截图大小
	const int offset = 7;								// 多截出来一圈
	vector<vector<Point>> contours;						// 保存数字轮廓

	// 遍历每个旋转矩形 是否是号码区域
	for (auto itr = rotatedRects.begin(); itr != rotatedRects.end(); itr++)
	{
		// 保存旋转矩形顶点
		Point2f p[4];
		itr->points(p);

		// 对 p 顶点进行排序
		sort(p, p + 4, [](Point2f a, Point2f b) {			// 按照 x 从小到大排序
			return a.x < b.x;
		});
		if (p[0].y > p[1].y) swap(p[0], p[1]);
		if (p[2].y < p[3].y) swap(p[2], p[3]);
		if (abs(p[0].y - p[1].y) > 60)						// 需要横放 
			return 3;

		// 对该旋转矩形透视变换
		Point2f pSrc[4] = {									// 透视变换 4 个源点
			Point2f(p[0].x - offset, p[0].y - offset), Point2f(p[3].x + offset, p[3].y - offset),
			Point2f(p[1].x - offset, p[1].y + offset), Point2f(p[2].x + offset, p[2].y + offset)
		};
		Point2f pDst[4] = {									// 透视变换 4 个目标点
			Point2f(0, 0),		Point2f(toCols, 0),
			Point2f(0, toRows),	Point2f(toCols, toRows)
		};
		warpPerspective(									// 透视变换函数
			idcard,											// 输入图像
			img,											// 输出图像
			getPerspectiveTransform(pSrc, pDst),			// 求透视变换矩阵
			Size(toCols, toRows)							// 输出图像大小
		);

		// 判断是否包含18个从左到右排列的字符（连通域）
		cvtColor(img, img, COLOR_BGR2GRAY);					// 灰度化
		idcardNumber = img.clone();							// 保存可能的区域 用于识别

		// 作差 边缘检测
		Mat tmp = img.clone();								// 临时存储空间
		morphologyEx(										// 形态学运算
			img,											// 输入图像
			tmp,											// 输出图像
			MORPH_CLOSE,									// 指定闭运算 连通破碎区域
			getStructuringElement(MORPH_RECT, Size(7, 7))	// 获取 7 * 7结构核
		);
		img = tmp - img;									// 作差

		blur(img, img, Size(3, 3));							// 3 * 3 均值滤波
		threshold(img, img, 0, 255, THRESH_OTSU);			// otsu 二值化

		// 闭运算 连通断裂的笔画
		morphologyEx(										// 形态学运算
			img,											// 输入图像
			img,											// 输出图像
			MORPH_CLOSE,									// 指定闭运算 连通破碎区域
			getStructuringElement(MORPH_RECT, Size(3, 7))	// 获取结构核
		);

		findContours(img, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);	// 查找外层轮廓

		if (contours.size() == 18)							// 轮廓数量 18 个说明是号码区域
		{
			// 在 idcard 上框出号码区域
			for (int i = 0; i < 4; i++)
				line(idcard, p[i], p[(i + 1) % 4], Scalar(0, 255, 0), 2, 8);

			if (DEBUG)
			{
				imshow("find_number_0_number_rotated_rect", idcard);
			}
			break;
		}
	}
	vector<RotatedRect>().swap(rotatedRects);				// 清空 rotatedRects 内存
	if (contours.size() != 18)								// 所有连通域的字符数量都不对 则返回无法找到号码区域
	{
		idcardNumber.release();
		return 3;
	}

	// 号码区域轮廓 从左到右排序
	sort(contours.begin(), contours.end(), [](vector<Point> a, vector<Point> b) {
		return boundingRect(a).br().x < boundingRect(b).br().x;
	});

	// 从左到右依次保存每个数字
	vector<Mat> mv;
	for (auto itr = contours.begin(); itr != contours.end(); itr++)
	{
		Rect rect = boundingRect(*itr);						// 求包围矩形
		Mat tmp = idcardNumber(Rect(rect)).clone();			// 提取出该数字
		threshold(tmp, tmp, 0, 255, THRESH_OTSU | THRESH_BINARY_INV); // OTSU 二值化 
		resize(tmp);										// 尺寸归一化 28 * 28
		idcardNumbers.push_back(tmp);						// 保存该数字

		//static int cnt = 0;									// 用于增量训练 保存新产生的单个数字图片
		//imwrite("E:/大四上/idcard/x64/Debug/data/trainData/" + to_string(cnt++) + ".jpg", tmp);
	}

	if (DEBUG)
	{
		Mat dbg_img = Mat::zeros(img.size(), CV_8UC3);
		int dbg_b = 60, dbg_g = 20, dbg_r = 100;
		Scalar dbg_color(dbg_b, dbg_g, dbg_r);
		for (int dbg_i = 0; dbg_i < contours.size(); dbg_i++)
		{
			drawContours(dbg_img, contours, dbg_i, dbg_color, 1, 8);
			dbg_b = (dbg_b + 20) % 256;
			dbg_g = (dbg_g + 40) % 256;
			dbg_r = (dbg_r + 80) % 256;
			dbg_color = Scalar(dbg_b, dbg_g, dbg_r);
		}
		Mat dbg_dst = Mat::zeros(Size(img.cols, img.rows * 3), CV_8UC3);
		merge(vector<Mat>({ idcardNumber, idcardNumber, idcardNumber }), idcardNumber);
		idcardNumber.copyTo(dbg_dst(Rect(0, 0, img.cols, img.rows)));
		dbg_img.copyTo(dbg_dst(Rect(0, img.rows, img.cols, img.rows)));

		dbg_img = Mat::zeros(Size(32 * 18, 28), CV_8UC1);
		for (int dbg_i = 0; dbg_i < idcardNumbers.size(); dbg_i++)
			idcardNumbers[dbg_i].copyTo(dbg_img(Rect(dbg_i * 32, 0, 28, 28)));
		cv::resize(dbg_img, dbg_img, Size(img.cols, img.rows));

		merge(vector<Mat>({ dbg_img, dbg_img, dbg_img }), dbg_img);
		dbg_img.copyTo(dbg_dst(Rect(0, img.rows * 2, img.cols, img.rows)));

		imshow("find_number_1_single_number", dbg_dst);
		dbg_dst.release();
		dbg_img.release();
		waitKey(1);
	}

	idcardNumber.release();									// 释放 idcardNumber 占用的内存
	return 0;
}

const int Card::predict()
{
	svm = SVM::load(trainDataFolderPath + "svm.xml");// 读取模型

	// 逐个识别
	for (auto itr = idcardNumbers.begin(); itr != idcardNumbers.end(); itr++)
	{
		itr->convertTo(img, CV_32FC1);				// 转换为 float 类型单通道图片
		float res = svm->predict(img.reshape(0, 1));// 预测结果
		predictNumber.push_back(res);				// 保存预测结果
	}

	img.release();
	svm->clear();

	return correct() ? 1 : 0;
}

const int Card::correct()
{
	int sum = 0;
	vector<int> W = { 7, 9, 10, 5, 8, 4, 2, 1, 6, 3, 7, 9, 10, 5, 8, 4, 2, 1 };	// 权重
	for (int i = 0; i < W.size(); i++)							// 加权和
		sum += predictNumber[i] * W[i];
	sum %= 11;													// 取余 11

	if (
		sum == 1												// 如果余数为 1
		|| (sum == 10 && predictNumber.back() == 10)			// 或者 (10 且最后一位 X)
		|| mapPlace.find(predictNumber[0]*10 + predictNumber[1]) != mapPlace.end() // 如果前两位正确
	)	
		return 0;
	else
		return 1;
}

void Card::release()
{
	idcard.release();
	img.release();
	idcardNumber.release();
	imgPath.clear();
	vector<RotatedRect>().swap(rotatedRects);
	vector<int>().swap(predictNumber);
	vector<Mat>().swap(idcardNumbers);
}
