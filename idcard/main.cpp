#include "card.h"
#include "path.h"

int main(int argc, char** argv)
{
	Card card;												// 识别身份证号码类

	if (argc == 1)											// 如果没有参数 则默认相对路径
	{
		card.setPicFolderPath("./data/pic");				// 图片根目录 注意最后没有斜杠 '/'
		card.setTrainDataFolderPath("./data/trainData/");	// 训练数据目录 注意最后有斜杠 '/'
		card.setSavePath("./data/res/");					// 设置结果保存路径
		card.setTrain("FALSE");								// 设置 SVM 是否重新训练 TRUE / FALSE
		card.setDebug("FALse");								// 设置是否 DEBUG 模式 TRUE / FALSE
	}
	else if (argc == 6)
	{
		/// params 1: 待识别图片文件夹路径
		/// params 2: SVM 训练数据路径
		/// params 3: 设置结果保存路径
		/// params 4: 是否训练 SVM
		/// params 5: 设置 debug 模式
		card.setPicFolderPath(argv[1]);						// 不要有中文
		card.setTrainDataFolderPath(argv[2]);				// 不要有中文
		card.setSavePath(argv[3]);							// 设置结果保存路径
		card.setTrain(argv[4]);								// TRUE / FALSE
		card.setDebug(argv[5]);								// TRUE / FALSE
	}
	else
	{
		cout << "please check your dir" << endl;
		system("pause");
		return 0;
	}

	TickMeter totalT;										// 记录总时间
	totalT.reset();											// 置零

	string txt;												// 保存识别结果
	vector<string> files;									// 图片路径列表
	getFileNames(card.getPicFolderPath(), files);			// 获得路径列表
	cout << "------- IDENTIFY START -------" << endl;
	for (auto file : files)									// 遍历
	{
		totalT.start();										// 开始计时
		int res = card.identify(file, txt);					// 识别
		if (res == 0) {
			cout << "| result: " + txt + " | " + file << endl; // 输出号码
			if (card.is_DEBUG())
			{
				card.show(file);								// 显示并返回识别出的号码 调用 show 后会释放内存
			}
		}
		else if (res == 1)
			cout << "| error : no such picture | " + file << endl; // 提示图片为空
		else if (res == 2)
			cout << "| error : can not detect | " + file << endl; // 提示不能检测到号码
		else if (res == 3)
			cout << "| error : can not find number area | " + file << endl; // 号码不是18位
		else if (res == 4)
			cout << "| error : wrong number | " + file << endl; // 提示识别错误

		totalT.stop();										// 暂停计时
		if (card.is_DEBUG())
		{
			waitKey();											// 等待按键
			destroyAllWindows();								// 销毁所有窗口
		}
	}

	totalT.stop();											// 停止计时
	cout << "total time: " << totalT.getTimeSec() << " s" << endl;	// 总时间
	cout << "------- IDENTIFY  END -------" << endl;

	system("pause");
	return 0;
}

