#ifndef PROCESSIMAGE_H
#define PROCESSIMAGE_H

#include <QtGui>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
//#include <QButtonGroup>
#include <QRadioButton>

#include "IVideoFrame.h"
#include "IVideoStream.h"
#include "ICameraControl.h"
#include "IDeviceControl.h"

#include "../apitest/FrameRateCounter.h"
//#include "gpsimu.h"

#include <atomic>

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

class ProcessImage : public QWidget
{
    Q_OBJECT
public:
    ProcessImage(QWidget *parent=0);
    ~ProcessImage();

     void onGetStereoFrame(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames);
     void onDeviceException(int);//override

private:
    //QPainter *painter;
    QLabel *lblVideoLeft;
    QLabel *lblVideoRight;
    QLabel *lblSavedframe;

    QLabel *lblFramerate;
    QLabel *lblFramecount;

    QPushButton *btnRender;
    QPushButton *btnSaveframe;

    QLineEdit *RgainEdit;
    QLineEdit *GgainEdit;
    QLineEdit *BgainEdit;
    QLineEdit *exposureEdit;
    QLineEdit *recEdit;
    QLineEdit *ipEdit;

    QRadioButton *radioBtn1;
    QRadioButton *radioBtn2;
    QRadioButton *radioBtn3;

    QRadioButton *radioBtnExp100;
    QRadioButton *radioBtnExp200;
    QRadioButton *radioBtnExp400;
    QRadioButton *radioBtnExp800;
    QRadioButton *radioBtnExp1200;

    QTimer *timer;
    int rs;

    uchar * p;
    unsigned int len;
    bool bRenderPaused;
    bool bEnabled;

    bool bSaveframe;
    int saveframe_index;
    int saved_frames;
    int max_save;

    //TopGear::VideoFormat format;

    std::shared_ptr<TopGear::IVideoStream> camera;
    std::shared_ptr<TopGear::ICameraControl> cameraControl;
    std::shared_ptr<TopGear::IDeviceControl> ioControl;

    std::atomic<int> dropcount;
    std::unique_ptr<uchar[]> prgb1;
    std::unique_ptr<uchar[]> prgb2;

    FrameRateCounter fc;

    QCheckBox *checkGetGPSIMU;

    QCheckBox *checkSetAE;
    bool bSetAE;
    QRadioButton *radioBtnFPS30;
    QRadioButton *radioBtnFPS20;
    QRadioButton *radioBtnFPS15;
    QRadioButton *radioBtnFPS10;


    bool bGetGPSIMU;
    //GPSIMUInfo gpsimuinfo;
    QLabel *lblGPS;
    QLabel *lblACC;
    QLabel *lblGYR;
    QLabel *lblMAG;


signals:
    //void onstereoframe(void *, void *);
    void onstereoframe(TopGear::IVideoFramePtr master, TopGear::IVideoFramePtr slave);
    void ondevexception(int);

private slots:

    void display_error(QString err);

    void ontimer();
    void onBtnRenderClick();
    void onBtnSaveframeClick();
    void radioExpoChange();

    void onAESet(int state);
    void onFramerateSet();

    void onGainGet();
    void onGainSet();
    void onExposureGet();
    void onExposureSet();
    //void showstereoframe(unsigned char *rgb1, unsigned char *rgb2,unsigned int frameid);
    void showstereoframe(TopGear::IVideoFramePtr master, TopGear::IVideoFramePtr slave);
    void handledeviceexception(int);

    void onGetGPSIMUCheckChanged(int state);
    void addInfoToFrame(void *frame, unsigned char *info, int infolen);
    void getInfoFromFrame(void *frame, unsigned char *info, int infolen);


    void getGain(float *gainR, float *gainG, float *gainB);
    void setGain(float gainR, float gainG, float gainB);
};

#endif

