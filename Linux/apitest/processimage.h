#ifndef PROCESSIMAGE_H
#define PROCESSIMAGE_H

#include <QtGui>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>

#include <atomic>
#include <future>

#include "FrameRateCounter.h"
#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"
#include "ILowlevelControl.h"

//#include "ImageAnalysis.h"

class ProcessImage : public QWidget
{
    Q_OBJECT

public:
	explicit ProcessImage(QWidget *parent = nullptr);
    ~ProcessImage();

private:
    QLabel * labelVideo ,*labelfps, *labelMag;

    QPushButton *btn;
    QPushButton *btn2;
    QPushButton *testBtn;
    QPushButton *pngBtn;
    QCheckBox *savecheckbox;

    QLineEdit *RgainEdit;
    QLineEdit *GgainEdit;
    QLineEdit *BgainEdit;
    QLineEdit *exposureEdit;
    QLineEdit *recEdit;

    QLineEdit *regaddrEdit;
    QLineEdit *regvalEdit;

    QRadioButton *radioBtn1;
    QRadioButton *radioBtn2;
    QRadioButton *radioBtn3;

    QTimer *timer;
    int rs;

    FrameRateCounter fc;

    uchar * p;
    unsigned int len;

    std::shared_ptr<TopGear::IVideoStream> camera;
    std::shared_ptr<TopGear::ICameraControl> cameraControl;
    std::shared_ptr<TopGear::IDeviceControl> ioControl;
    std::shared_ptr<TopGear::ILowlevelControl> lowlevelControl;

    bool bRenderPaused;
    bool bEnabled;

    std::atomic<int> dropcount;
    std::unique_ptr<uchar[]> prgb;
	std::mutex ev_mutex;
    //std::future<TopGear::ImageAnalysis::Result> image_result;

    void Init();
    void onGetVideoFrames(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames);
    void onDeviceException(int); //override
signals:
    void onvideoframe(TopGear::IVideoFramePtr vf);
    void ondevexception(int err);
private slots:
    void showvideoframe(TopGear::IVideoFramePtr vf);
    void handledevexception(int);
    void ontimer();
    void display_error(QString err);
    void onGainGet();
    void onGainSet();
    void onExposureGet();
    void onExposureSet();
    void onRegGet();
    void onRegSet();
    void onSetGPIOHigh();
    void onSetGPIOLow();
};

extern int convert_raw_to_rgb_buffer(unsigned char *raw,
                                     unsigned char *rgb,
                                     bool isGradientBasedInter,
                                     int w,int h);

#endif // PROCESSIMAGE_H
