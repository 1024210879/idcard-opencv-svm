# idcard-
OpenCV实现第二代身份证号码识别，形态学、轮廓检测身份证号码位置，SVM识别数字。已设计好 Card 类供调用，具体调用方法见 main.cpp。数据均来自于百度搜索。

# requirement
OpenCV 4.0.0

# usage
## 1. .cpp 源码编译
VS2017 打开 idcard.sln，Ctrl+F5

## 2. .bat 批处理使用
```
idcard.exe ./data/pic ./data/trainData/ ./data/res/ FALSE FALSE
pause
```
* params0: idcard.exe
* params1: 包含 待识别图片   的文件夹  注意结尾没有 "/"  __QAQ__
* params2: 包含 SVM 训练数据 的文件夹  注意结尾包含 "/"  __QAQ__
* params3: 包含 识别结果保存 的文件夹  注意结尾包含 "/"  __QAQ__
* params4: 运行时是否训练 SVM：TRUE/FALSE
* params5: 运行模式是否DEBUG（该模式下会显示处理过程）：TRUE/FALSE

## 3. SVM 训练数据
* 路径./data/trainData/
* 为数字 0-9 以及罗马数字 X 分别建立文件夹 0-10，文件夹 10 中存放 X 的图片
* 每张训练图片尺寸为 28 * 28
* 训练图片前景白色，背景黑色
* SVM 模型路径 ./data/trainData/svm.xml
![dirs.jpg]
(images/dirs.jpg)

# 效果图
![001.jpg]
(images/001.jpg)
![002.jpg]
(images/002.jpg)
![003.jpg]
(images/003.jpg)
![004.jpg]
(images/004.jpg)
![005.jpg]
(images/005.jpg)
![006.jpg]
(images/006.jpg)
![007.jpg]
(images/007.jpg)
