#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <QMainWindow>
#include <iostream>
#include <fstream>
#include <io.h>
#include <QFileDialog>
#include <QDebug>
#include <vector>
#include <QLabel>
#include <QVBoxLayout>
#include <QThread>

using namespace std;
using namespace cv;

#define CALIBRATERESULTFILE "CalibrateResult.txt"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_LoadImage_clicked();

    void on_pushButton_SaveResult_clicked();

    void on_pushButton_StartCalibrate_clicked();

    void on_pushButton_AppraiseCalibrate_clicked();

public:
    // QT图像 to openCV图像  和  openCV图像 to QT图像
    //QImage MatToQImage(Mat const& src);
    QImage MatToQImage(Mat &m);
    Mat QImageToMat(QImage const& src);

    void showCameraMatrix(Mat const& data);  // 显示内参矩阵
    void showDistCoeffs(Mat const& data);   // 显示畸变系数

private:
    Ui::MainWindow *ui;

    // 保存不同图片标定板上角点的三维坐标
    vector<vector<Point3f>> object_points;
    // 缓存每幅图像上检测到的角点
    vector<Point2f> image_points_buf;
    // 保存检测到的所有角点
    vector<vector<Point2f>> image_points_seq;
    // 相机内参数矩阵
    cv::Mat cameraMatrix;
    // 相机的畸变系数
    cv::Mat distCoeffs;
    // 每幅图像的平移向量
    vector<cv::Mat> tvecsMat;
    // 每幅图像的旋转向量
    vector<cv::Mat> rvecsMat;
    // 加载标定图片的文件夹
    QString m_strCalibrateFolder;
    // 保存标定结果的文件夹
    QString m_strSaveResultFolder;
    // 写入
    std::ofstream fout;
    // 图像数量
    int image_count = 0;
    // 每幅图像中角点的数量
    vector<int> point_counts;
};

#endif // MAINWINDOW_H

