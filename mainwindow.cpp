#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 渲染设置为硬件加速
    ui->label_showMat->setAttribute(Qt::WA_OpaquePaintEvent,true);
    ui->label_showMat->setAttribute(Qt::WA_NoSystemBackground,true);
    ui->label_showMat->setAutoFillBackground(false);

    cameraMatrix = cv::Mat(3,3,CV_32FC1, Scalar::all(0));
    distCoeffs = cv::Mat(1,5,CV_32FC1, Scalar::all(0));
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString folderPath =QFileDialog::getOpenFileName(nullptr, "选择文件", "/path/to/start", "Images (*.png *.jpg)");

void MainWindow::on_pushButton_LoadImage_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,QString(u8"选择标定图片文件夹"),tr(""),QFileDialog::ShowDirsOnly);
    if(!folderPath.isEmpty()) {
        //文件夹不为空
        m_strCalibrateFolder = folderPath;
    } else {
        qDebug()<< "未选择任何文件夹";
        return;
    }

    // 将加载的路径显示在界面
    ui->lineEdit_CalibrateImagePath->setText(folderPath);
    // 设置文字左对齐
    ui->lineEdit_SaveResultPath->setAlignment(Qt::AlignLeft);
}

void MainWindow::on_pushButton_SaveResult_clicked()
{
   QString folderPath = QFileDialog::getExistingDirectory(this,QString(u8"选择保存结果文件夹"),tr(""),QFileDialog::ShowDirsOnly);
   if(folderPath.isEmpty()) {
       //qDebug()<< "未选择任何文件夹";
       return;
   }

   m_strSaveResultFolder = folderPath;
   // 设置路径到界面
   ui->lineEdit_SaveResultPath->setText(folderPath);
   // 左对齐
   ui->lineEdit_SaveResultPath->setAlignment(Qt::AlignLeft);
}

void MainWindow::on_pushButton_StartCalibrate_clicked()
{
    // 保存标定结果的txt
    QString strResult = m_strSaveResultFolder + QString("/%1").arg(CALIBRATERESULTFILE);
    fout.open(strResult.toStdString().c_str());

    // 1、加载标定图片
    vector<QString> imageNames;
    QDir dir(m_strCalibrateFolder);
    QStringList fileNames = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    foreach(const QString& fileName, fileNames) {
        QString filePath = dir.filePath(fileName);
        imageNames.push_back(filePath);  // 将完整的路径添加到图片路径容器
    }

    // 2、分别对每张图片进行角点提取
    Size image_size;      // 图像尺寸
    Size board_size = Size(4,6);  // 标定板上每行、列的角点数
    int count = -1;  // 用于存储角点个数

    for(unsigned long long i = 0; i < imageNames.size(); i++) {
        image_count++;
        // 输出观察
        qDebug()<< "image_count = " << image_count;
        // 输出校验
        qDebug()<< "Check count = " << count;

        // 读取图片
        Mat imageInput = imread(imageNames[i].toStdString().c_str());

        if(image_count == 1) {
            // 读入第一张图片时获取图像宽高信息
            image_size.width = imageInput.cols;
            image_size.height = imageInput.rows;
        }

        // 提取角点
        if(0 == findChessboardCorners(imageInput, board_size, image_points_buf))
        {// 未发现角点信息/找不到角点
            qDebug()<< "未发现角点信息";
            return;
        }
        else {
         // 3、对每一张标定图像进行亚像素化处理
            Mat view_gray;
            // 将imageInput转为灰度图像

            cvtColor(imageInput, view_gray, COLOR_RGB2GRAY);
            // 亚像素精准化（对粗提取的角点进行精准化）
            find4QuadCornerSubpix(view_gray,image_points_buf,Size(6,8));
            image_points_seq.push_back(image_points_buf);  // 尾插，保存亚像素角点

         // 4、在棋盘格显示，并在界面刷新图片(显示找到的内角点绘制图片)
            // 在图像上显示角点位置
            drawChessboardCorners(imageInput, board_size, image_points_buf, true);
#if 0
            imshow("Camera Calibration", imageInput);  // 显示图片
            imwrite("Calibration" + to_string(image_count) + ".png", imageInput); // 写入图片
            waitKey(100);  // 暂停0.1s
#else
            QImage tmpImage = MatToQImage(imageInput);
            ui->label_showMat->setPixmap(QPixmap::fromImage(tmpImage.rgbSwapped()));
            ui->label_showMat->show();
            QThread::msleep(100);  // 延时0.1s
            QCoreApplication::processEvents();
#endif

            qDebug()<< "角点提取完成";
        }
    }

    //destroyAllWindows();

    // 5、相机标定
    Size square_size = Size(5,5);

    // 初始化标定板上角点的三维坐标
    int i, j, t;
    for(t = 0; t < image_count; t++) {
        // 图片个数
        vector<Point3f> tempPointSet;
        for(i = 0; i < board_size.height; i++) {
            for(j = 0; j < board_size.width; j++) {
                Point3f realPoint;
                // 假设标定板放在世界坐标系中，z=0的平面上
                realPoint.x = i * square_size.height;
                realPoint.y = j * square_size.width;
                realPoint.z = 0;
                tempPointSet.push_back(realPoint);
            }
        }
        object_points.push_back(tempPointSet);
    }

    // 初始化每幅图像上的角点数量，假定每幅图像中都可以看到完整的标定板
    for(i = 0; i < image_count; i++) {
        point_counts.push_back(board_size.width* board_size.height);
    }

    cv::calibrateCamera(object_points, image_points_seq,image_size,cameraMatrix,distCoeffs,rvecsMat,tvecsMat,0);
    qDebug()<< "标定完成!";

    // 6/7对应下面1/2
}

void MainWindow::on_pushButton_AppraiseCalibrate_clicked()
{
    // 1、对标定结果进行评价
    qDebug() << "开始评价标定结果.....";

    double total_err = 0.0;  // 所有图像的平均误差的总和
    double err = 0.0;  // 每幅图像的平均误差
    vector<Point2f> image_points2;  // 保存重新计算得到的投影点
    qDebug()<< "每幅图像的标定误差: ";
    fout << "每幅图像的标定误差: \n";
    for(int i = 0; i < image_count; i++) {
        vector<Point3f> tempPointSet = object_points[i];
        // 通过得到的摄像机内外参数，对空间的三维点进行重新投影计算，得到新的三维投影点
        projectPoints(tempPointSet, rvecsMat[i], tvecsMat[i], cameraMatrix, distCoeffs, image_points2);

        // 计算新的投影点和旧的投影点之间的误差
        vector<Point2f> tempImagePoint = image_points_seq[i];  // 原先的旧二维点
        Mat tempImagePointMat = Mat(1, tempImagePoint.size(), CV_32FC2);
        Mat image_points2Mat = Mat(1, image_points2.size(), CV_32FC2);
        for(unsigned long long j = 0; j < tempPointSet.size(); j++) {
            // j对应二维点的个数
            image_points2Mat.at<Vec2f>(0,j) = Vec2f(image_points2[j].x, image_points2[j].y);
            tempImagePointMat.at<Vec2f>(0,j) = Vec2f(tempImagePoint[j].x,tempImagePoint[j].y);
        }

        err = norm(image_points2Mat, tempImagePointMat, NORM_L2);
        total_err += err /= point_counts[i];
        //qDebug()<< "第" << i + 1 << "幅图像的平均误差: " << err << "像素";
        qDebug()<< "第" << i + 1 << "幅图像的平均误差: " << err << "像素";
        fout << "第" << i + 1 << "幅图像的平均误差: " << err << "像素" << endl;
    }

    qDebug()<< "总体平均误差: " << total_err / image_count << "像素";
    fout << "总体平均误差：" << total_err / image_count << "像素" << endl << endl;
    qDebug() << "评价完成！";

    // 2、查看标定结果并保存
    qDebug()<< "开始保存定标结果………………";
    Mat rotation_matrix = Mat(3, 3, CV_32FC1, Scalar::all(0)); /* 保存每幅图像的旋转矩阵 */
    fout << "相机内参数矩阵：" << endl;
    showCameraMatrix(cameraMatrix);
    fout << cameraMatrix << endl << endl;
    fout << "畸变系数：\n";
    showDistCoeffs(distCoeffs);
    fout << distCoeffs << endl;
    for (int i = 0; i < image_count; i++)
    {
        fout << "第" << i + 1 << "幅图像的旋转向量：" << endl;
        fout << rvecsMat[i] << endl;
        /* 将旋转向量转换为相对应的旋转矩阵 */
        Rodrigues(rvecsMat[i], rotation_matrix);
        fout << "第" << i + 1 << "幅图像的旋转矩阵：" << endl;
        fout << rotation_matrix << endl;
        fout << "第" << i + 1 << "幅图像的平移向量：" << endl;
        fout << tvecsMat[i] << endl << endl;
    }
    qDebug()<< "完成保存!";
    fout << endl;
}


QImage MainWindow::MatToQImage(Mat &m) //Mat转QImage
{
    //判断m的类型，可能是CV_8UC1  CV_8UC2  CV_8UC3  CV_8UC4
    switch(m.type())
    { //QIamge 构造函数, ((const uchar *data, 宽(列),高(行), 一行共多少个（字节）通道，宽度*字节数，宏参数)
    case CV_8UC1:
    {
        QImage img((uchar *)m.data,m.cols,m.rows,m.cols * 1,QImage::Format_Grayscale8);
        return img;
    }
        break;
    case CV_8UC3:   //一个像素点由三个字节组成
    {
        //cvtColor(m,m,COLOR_BGR2RGB); BGR转RGB
        QImage img((uchar *)m.data,m.cols,m.rows,m.cols * 3,QImage::Format_RGB888);
        return img.rgbSwapped(); //opencv是BGR  Qt默认是RGB  所以RGB顺序转换
    }
        break;
    case CV_8UC4:
    {
        QImage img((uchar *)m.data,m.cols,m.rows,m.cols * 4,QImage::Format_RGBA8888);
        return img;
    }
        break;
    default:
    {
        QImage img; //如果遇到一个图片均不属于这三种，返回一个空的图片
        return img;
    }
    }
}

Mat MainWindow::QImageToMat(QImage const& src)
{
    Mat tmp(src.height(),src.width(),CV_8UC4,(uchar*)src.bits(),src.bytesPerLine());
    Mat result;
    cvtColor(tmp,result,COLOR_RGBA2BGR);
    return result;
}

void MainWindow::showCameraMatrix(const Mat &data)
{
   std::ostringstream ss;
   ss << data;
   std::string strMatrix = ss.str();

   QVBoxLayout* layout = new QVBoxLayout(ui->groupBox_CameraInParam);
   QLabel* label = new QLabel();
   label->setText(QString::fromStdString(strMatrix));
   label->setAlignment(Qt::AlignCenter);
   layout->addWidget(label);
}

void MainWindow::showDistCoeffs(const Mat &data)
{
    std::ostringstream ss;
    ss << data;
    std::string strMatrix = ss.str();

    QVBoxLayout* layout = new QVBoxLayout(ui->groupBox_DistortionParam);
    QLabel* label = new QLabel();
    label->setText(QString::fromStdString(strMatrix));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    layout->addWidget(label);
}

