
#include <QtGui>
#include <QMessageBox>
#include <QHBoxLayout>
//#include <cstdio>
//#include <cstdlib>
//#include <ctime>
//#include <memory>
#ifdef _WIN32
#include <winsock.h>
#include <omp.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif

#include "DeepCamAPI.h"
#include "IVideoStream.h"
//#include "IVideoFrame.h"
//#include "ICameraControl.h"
//#include "ILowlevelControl.h"
//#include "IDeviceControl.h"
//#include "IMultiVideoStream.h"
//#include "VideoFormat.h"
#include <iostream>

#include "processimage.h"



#define RESYNC_NUM 900

inline int fast_abs(int value)
{
    uint32_t temp = value >> 31;     // make a mask of the sign bit
    value ^= temp;                   // toggle the bits if value is negative
    value += temp & 1;               // add one if value was negative
    return value;
}

const float WEIGHT_B = 0.1140f;
const float WEIGHT_G = 0.5870f;
const float WEIGHT_R = 0.2989f;
const uint16_t BAYER_MAX = 0xFF;

//AR 0134
#define Bayer(x,y)  ((unsigned short)(raw_16[(x) + w *(y)] >> 4) & 0x00FF)
//#define Bayer(x,y)  raw_16[(x) + w *(y)]
#define Grey(x,y)	grey[((x) + w *(y))*3] = grey[((x) + w *(y))*3 +1] = grey[((x) + w *(y))*3 +2]

void bayer_copy_grey(uint16_t *raw_16, uint8_t *grey, int x, int y, int w)
{
    float val = 0;

    val = Bayer(x, y + 1) * WEIGHT_B +
        Bayer(x, y) * WEIGHT_G +
        Bayer(x + 1, y) * WEIGHT_R;
    Grey(x, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

    val = Bayer(x, y + 1) * WEIGHT_B +
        ((Bayer(x, y) + Bayer(x + 1, y + 1)) >> 1) * WEIGHT_G +
        Bayer(x + 1, y) * WEIGHT_R;
    Grey(x + 1, y) = Grey(x, y + 1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

    val = Bayer(x, y + 1) * WEIGHT_B +
        Bayer(x + 1, y + 1) * WEIGHT_G +
        Bayer(x + 1, y) * WEIGHT_R;
    Grey(x + 1, y + 1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);
}


void bayer_bilinear_grey(uint16_t *raw_16, uint8_t *grey, int x, int y, int w)
{
    float val = 0;

    val = ((Bayer(x, y - 1) + Bayer(x, y + 1)) >> 1) * WEIGHT_B +
        Bayer(x, y) * WEIGHT_G +
        ((Bayer(x - 1, y) + Bayer(x + 1, y)) >> 1) * WEIGHT_R;
    Grey(x, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

    val = ((Bayer(x, y - 1) + Bayer(x + 2, y + 1) + Bayer(x + 2, y - 1) + Bayer(x, y + 1)) >> 2) * WEIGHT_B +
        ((Bayer(x, y) + Bayer(x + 2, y) + Bayer(x, y) + Bayer(x + 2, y)) >> 2) * WEIGHT_G +
        Bayer(x + 1, y)* WEIGHT_R;
    Grey(x+1, y) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

    val = Bayer(x, y + 1) * WEIGHT_B +
        ((Bayer(x, y) + Bayer(x + 1, y + 1) + Bayer(x - 1, y + 1) + Bayer(x, y + 2)) >> 2)* WEIGHT_G +
        ((Bayer(x - 1, y) + Bayer(x + 1, y) + Bayer(x - 1, y + 2) + Bayer(x + 1, y + 2)) >> 2)* WEIGHT_R;
    Grey(x, y+1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);

    val = ((Bayer(x, y + 1) + Bayer(x + 2, y + 1)) >> 1) * WEIGHT_B +
        Bayer(x + 1, y + 1)* WEIGHT_G +
        ((Bayer(x + 1, y) + Bayer(x + 1, y + 2)) >> 1)* WEIGHT_R;
    Grey(x+1, y+1) = (val > BAYER_MAX) ? BAYER_MAX : uint8_t(val);
}

void RawToGrey(uint8_t *raw, uint8_t *grey, int w, int h)
{
	auto i =0,j = 0;
    #pragma omp parallel for shared(raw,grey) private(i,j)
    for (j = 0; j < h; j += 2)
    {
        for (i = 0; i < w; i += 2)
        {

            if (i == 0 || j == 0 || i == w - 2 || j == h - 2)
                bayer_copy_grey(reinterpret_cast<uint16_t *>(raw), grey, i, j, w);
            else
                bayer_bilinear_grey(reinterpret_cast<uint16_t *>(raw), grey, i, j, w);
        }
    }
}

float Gradient(uint8_t *pixel, int x,int y, uint32_t stride, float &max)
{
    max = 0;
    float g = 0;
    auto current = pixel[y*stride + x*3];
    for (auto i = -1; i <= 1; ++i)
        for (auto j = -1; j <= 1;++j)
        {
            if (i == 0 && j == 0)
                continue;
            auto val = float(fast_abs((int)current - (int)pixel[(y + i)*stride + (x + j)*3]));
            if (fast_abs(i)+ fast_abs(j)>1)
                val *= 0.707107f;
            if (val > max)
                max = val;
            g += val;
        }
    return g;
}

float ProcessImage::Sharpness(std::unique_ptr<uint8_t[]> pixel,int w,int h)
{
	std::unique_lock<std::mutex> ul(ev_mutex);
    constexpr auto lamda1 = 0.6f;
    constexpr auto lamda2 = 1 - lamda1;
    float sum1 = 0;
    float sum2 = 0;
	auto i=0,j=0;
    #pragma omp parallel for shared(pixel) private(i,j)
    for (i = 1; i < h - 1; ++i)
        for (j = 1; j < w - 1;++j)
        {
            float max = 0;
            auto g = Gradient(pixel.get(), j, i, w*3, max);
            sum1 += g;
            sum2 += max;
        }
    auto s = (sum1*lamda1+sum2*lamda2)/((h - 2)*(w - 2))*10;
    qDebug("P:  %f", s);
	labelMag->setText(QString::number(s));
	//OutputDebugStringW(L"Frame Arrival\n");
	//std::cout << "P: " << s << std::endl;
    return s;
}

void ProcessImage::Init()
{
    setWindowTitle(tr("Camera View"));

    labelfps = new QLabel();
    labelfps->setText("fps:0");

    QLabel *labeldevinfo = new QLabel();


    QLabel *labelReg = new QLabel();
    labelReg->setText("register(addr/val):");

    regaddrEdit = new QLineEdit();
    regaddrEdit->setMaximumWidth(100);
    regvalEdit = new QLineEdit();
    regvalEdit->setMaximumWidth(100);

    QPushButton *regGetBtn = new QPushButton();
    regGetBtn->setMaximumWidth(100);
    regGetBtn->setText("get");
    connect(regGetBtn,SIGNAL(clicked()), this, SLOT(onRegGet()));
    QPushButton *regSetBtn = new QPushButton();
    regSetBtn->setMaximumWidth(100);
    regSetBtn->setText("set");
    connect(regSetBtn,SIGNAL(clicked()), this, SLOT(onRegSet()));

    QPushButton *gpiohBtn = new QPushButton();
    gpiohBtn->setText("TriggerGPIO_H");
    connect(gpiohBtn,SIGNAL(clicked()), this, SLOT(onSetGPIOHigh()));
    QPushButton *gpiolBtn = new QPushButton();
    gpiolBtn->setText("TriggerGPIO_L");
    connect(gpiolBtn,SIGNAL(clicked()), this, SLOT(onSetGPIOLow()));

    QHBoxLayout *hLayout1 = new QHBoxLayout();
    hLayout1->addWidget(labelReg);
    hLayout1->addWidget(regaddrEdit);
    hLayout1->addWidget(regvalEdit);
    hLayout1->addWidget(regGetBtn);
    hLayout1->addWidget(regSetBtn);
    hLayout1->addSpacing(100);
    hLayout1->addWidget(labelfps);
    hLayout1->addWidget(labeldevinfo);
    hLayout1->addWidget(gpiohBtn);
    hLayout1->addWidget(gpiolBtn);

    QLabel *labelNMag = new QLabel();
    labelMag = new QLabel();
    labelNMag->setText("Sharpness:");
    labelMag->setText("0");
    hLayout1->addWidget(labelNMag);
    hLayout1->addWidget(labelMag);

    hLayout1->addStretch();


    QHBoxLayout *hLayout3 = new QHBoxLayout();
    QLabel *lable2 = new QLabel();
    lable2->setText("RGB gain(0x):");
    hLayout3->addWidget(lable2);
    RgainEdit = new QLineEdit();
    RgainEdit->setMaximumWidth(100);
    hLayout3->addWidget(RgainEdit);
    GgainEdit = new QLineEdit();
    GgainEdit->setMaximumWidth(100);
    hLayout3->addWidget(GgainEdit);
    BgainEdit = new QLineEdit();
    BgainEdit->setMaximumWidth(100);
    hLayout3->addWidget(BgainEdit);

    QPushButton *gainGetBtn = new QPushButton();
    gainGetBtn->setText("get");
    connect(gainGetBtn,SIGNAL(clicked()), this, SLOT(onGainGet()));
    hLayout3->addWidget(gainGetBtn);
    QPushButton *gainSetBtn = new QPushButton();
    gainSetBtn->setText("set");
    connect(gainSetBtn,SIGNAL(clicked()), this, SLOT(onGainSet()));
    hLayout3->addWidget(gainSetBtn);


    QLabel *lableexposure = new QLabel();
    lableexposure->setText("Exposure(Decimal):");
    hLayout3->addWidget(lableexposure);
    exposureEdit = new QLineEdit();
    exposureEdit->setMaximumWidth(100);
    hLayout3->addWidget(exposureEdit);
    QPushButton *exposureGetbtn = new QPushButton();
    exposureGetbtn->setText("get");
    connect(exposureGetbtn,SIGNAL(clicked()), this, SLOT(onExposureGet()));
    hLayout3->addWidget(exposureGetbtn);
    QPushButton *exposureSetbtn = new QPushButton();
    exposureSetbtn->setText("set");
    connect(exposureSetbtn,SIGNAL(clicked()), this, SLOT(onExposureSet()));
    hLayout3->addWidget(exposureSetbtn);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->addLayout(hLayout1);
    vLayout->addLayout(hLayout3);

    labelVideo = new QLabel();//video display
    vLayout->addWidget(labelVideo);//video

    setLayout(vLayout);

    timer = new QTimer(this);
}

//void OnFrameCB(std::vector<TopGear::IVideoFrameRef> &frames)
//{
//    if (frames.size() == 0)
//        return;
//    auto frame = frames[0];
//    uint8_t *pData;		//Frame data
//    uint32_t stride;	//Actual stride of frame
//    frame->LockBuffer(&pData, &stride);		//Lock memory
//    std::cerr << static_cast<int>(frame->GetTimestamp().tv_usec) << std::endl;
//    //Do something...
//    frame->UnlockBuffer();	//Unlock memory
//}

//////////////////////////////////////
///
///

ProcessImage::ProcessImage(QWidget *parent)
    :QWidget(parent), dropcount(0)
{
    bRenderPaused = false;
    bEnabled = false;
    //file_index = 0;

    Init();
    fc.Reset();

    //Open

    qRegisterMetaType<TopGear::IVideoFramePtr>("TopGear::IVideoFramePtr");
    connect(this, SIGNAL(onvideoframe(TopGear::IVideoFramePtr)),
            this, SLOT(showvideoframe(TopGear::IVideoFramePtr)),
            Qt::QueuedConnection );
    connect(this, SIGNAL(ondevexception(int)),
            this, SLOT(handledevexception(int)),
            Qt::QueuedConnection );

//    auto uvcDevices = TopGear::Linux::DeviceFactory<TopGear::Linux::DiscernibleVCDevice>::EnumerateDevices();
//    for (auto dev : uvcDevices)
//    {
//        std::cerr << dev->GetFriendlyName() << std::endl;
//        std::cerr << dev->GetSymbolicLink() << std::endl;
//        camera = TopGear::Linux::CameraFactory<TopGear::Camaro>::CreateInstance(dev);
//        ioControl = std::dynamic_pointer_cast<TopGear::IDeviceControl>(camera);
//        cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
//        if (camera && ioControl && cameraControl)
//        {
//            std::cerr << ioControl->QueryDeviceInfo() << std::endl;
//            //Register callback function for frame arrival
//            TopGear::IVideoStream::RegisterFrameCallback(*camera,
//                                                         &ProcessImage::onGetVideoFrames,this);
//            //Is master camaro
//            if (ioControl->QueryDeviceRole() == 0)
//            {
//                TopGear::VideoFormat format;
//                //Get optimized video format
//                auto index = camera->GetOptimizedFormatIndex(format);
//                //Start streaming with selected format index
//                camera->StartStream(index);
//                break;
//            }
//            else
//                camera.reset();
//        }
//    }

//    auto devices = TopGear::Linux::DeviceFactory<TopGear::Linux::StandardVCDevice>::EnumerateDevices();
//    for (auto item : devices)
//    {
//        //Show some basic info about device
//        std::cout << item->GetFriendlyName() << std::endl;
//        std::cout << item->GetSymbolicLink() << std::endl;
//        std::cout << item->GetDeviceInfo() << std::endl;
//        camera = TopGear::Linux::CameraFactory<TopGear::StandardUVC>::CreateInstance(item);
//        if (camera)
//        {
//            TopGear::IVideoStream::RegisterFrameCallback(*camera,
//                   &ProcessImage::onGetVideoFrames,this);
//            //labeldevinfo->setText(QString("devinfo:%1").arg(item->GetDeviceInfo().c_str()));
//            TopGear::VideoFormat format;
//            //Get optimized video format
//            auto index = camera->GetOptimizedFormatIndex(format);
//            auto formats = camera->GetAllFormats();
//            camera->StartStream(21);
//            break;
//        }
//    }

    TopGear::DeepCamAPI::Initialize();
    camera = TopGear::DeepCamAPI::CreateCamera(TopGear::Camera::Camaro);
    if (camera)
    {
        ioControl = std::dynamic_pointer_cast<TopGear::IDeviceControl>(camera);
        cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
        lowlevelControl = std::dynamic_pointer_cast<TopGear::ILowlevelControl>(camera);
        TopGear::IVideoStream::RegisterFrameCallback(*camera,
               &ProcessImage::onGetVideoFrames,this);
        //labeldevinfo->setText(QString("devinfo:%1").arg(item->GetDeviceInfo().c_str()));
        TopGear::VideoFormat format;
        //Get optimized video format
        auto index = camera->GetOptimizedFormatIndex(format);
        //auto formats = camera->GetAllFormats();
        camera->SetCurrentFormat(index);
        cameraControl->Flip(true,false);
        camera->StartStream();
    }


    connect(timer,SIGNAL(timeout()),this,SLOT(ontimer()));
    timer->start(500);
}

ProcessImage::~ProcessImage()
{
    if (timer)
        timer->stop();
    if (camera)
        camera->StopStream();
	if (ev_thread.joinable())
		ev_thread.join();
}

void ProcessImage::display_error(QString err)
{
    //QMessageBox::warning(this,tr("error"), err,QMessageBox::Yes);
    qDebug() <<err;
}

void ProcessImage::onDeviceException(int err)
{
    emit ondevexception(err);
}

void ProcessImage::onGetVideoFrames(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames)//this is called in sub-thread
{
    (void)sender;
    if (frames.size()==0)
        return;
    //qDebug("frameidx:  %d",frames[0]->GetFrameIdx());
    emit onvideoframe(frames[0]);
}


void ProcessImage::handledevexception(int)
{
    if (camera)
        camera->StopStream();
    QMessageBox msgBox;
    msgBox.setText("Device Lost!");
    msgBox.exec();
}

void ProcessImage::showvideoframe(TopGear::IVideoFramePtr vf)
{
    static int lastIdx = -1;
    //TopGear::IVideoFrameRef vf = *reinterpret_cast<TopGear::IVideoFrameRef *>(p);
    auto currentIdx = vf->GetFrameIdx();

    if (lastIdx==-1)
    {
        lastIdx=currentIdx;
        dropcount = 0;
    }
    else
    {
        if (++lastIdx >= RESYNC_NUM)
            lastIdx = 0;
        if (currentIdx>lastIdx)
        {
            dropcount += currentIdx-lastIdx;
            lastIdx = currentIdx;
        }
        else if (currentIdx<lastIdx)
        {
            dropcount += currentIdx-lastIdx;
            lastIdx = currentIdx;
        }
        else if (currentIdx<lastIdx)
        {
            dropcount.fetch_add(RESYNC_NUM - lastIdx + currentIdx);
            lastIdx = currentIdx;
        }
    }


    //qDebug("frame %d.%03d", vf->tv.tv_sec, vf->tv.tv_usec/1000);

    fc.NewFrame();

    uint32_t w,h;
    auto format = camera->GetCurrentFormat();
    h = format.Height;
    w = format.Width;
    //vf->QueryActualSize(w,h);
    //std::unique_ptr<uchar[]> prgb(new uchar[w*h*3]);
    if (prgb==nullptr)
        prgb.reset(new uchar[w*h*3]);
    unsigned char *pdata;
    uint32_t stride;
    vf->LockBuffer(&pdata,&stride);
    RawToGrey(pdata,prgb.get(),w,h);
//    if ((currentIdx & 0xf) == 0)
//        qDebug("P:  %f", Sharpness(prgb.get(),w,h));
    //convert_raw_to_rgb_buffer(pdata, prgb.get(), false , w,h);
    vf->UnlockBuffer();

    std::unique_ptr<QImage> frame(new QImage(prgb.get(),w,h, QImage::Format_RGB888));
    labelVideo->setPixmap(QPixmap::fromImage(*frame, Qt::AutoColor));

    if (ev_thread.joinable())
    {
		std::unique_lock<std::mutex> ul(ev_mutex, std::try_to_lock);
		if (ul)
		{
			ev_thread.join();
			ev_thread = std::thread(&ProcessImage::Sharpness, this, std::move(prgb), w, h);
		}
    }
	else
        ev_thread = std::thread(&ProcessImage::Sharpness, this, std::move(prgb),w,h);
}


void ProcessImage::ontimer()
{
    //labelfps->setText("fps:"+QString::number((int)fc.GetFrameRate()));
	auto drops = dropcount.load(std::memory_order_relaxed);
    dropcount = 0;
    labelfps->setText(QString("fps:%1  framedrops:%2").arg(int(fc.GetFrameRate())).arg(drops));
}

void ProcessImage::onSetGPIOHigh()
{
    if (ioControl)
        //ioControl->SetSensorTrigger(1);
        ioControl->SetControl("Trigger", TopGear::PropertyData<uint8_t>(1));
}

void ProcessImage::onSetGPIOLow()
{
    if (ioControl)
        //ioControl->SetSensorTrigger(0);
        ioControl->SetControl("Trigger", TopGear::PropertyData<uint8_t>(0));
}

void ProcessImage::onRegGet()
{
    if (lowlevelControl ==nullptr)
        return;
    unsigned short regaddr,regval;
    bool ok;
    regaddr = regaddrEdit->text().toInt(&ok,16);
    if (!ok)
        return;
    lowlevelControl->GetRegister(regaddr,regval);
    regvalEdit->setText(QString::number(regval,16));
}

void ProcessImage::onRegSet()
{
    if (lowlevelControl ==nullptr)
        return;
    unsigned short regaddr,regval;
    bool ok;
    regaddr = regaddrEdit->text().toInt(&ok,16);
    if (!ok)
        return;
    regval = regvalEdit->text().toInt(&ok,16);
    if (!ok)
        return;
    lowlevelControl->SetRegister(regaddr,regval);
}
void ProcessImage::onGainGet()
{
    if (cameraControl == nullptr)
        return;

    unsigned short gainR=0, gainG=0, gainB=0;
    cameraControl->GetGain(gainR,gainG,gainB);

    RgainEdit->setText(QString::number(gainR,16));
    GgainEdit->setText(QString::number(gainG,16));
    BgainEdit->setText(QString::number(gainB,16));
}

void ProcessImage::onGainSet()
{
    if (cameraControl == nullptr)
        return;
    unsigned short gainR, gainG, gainB;
    bool ok;
    gainR = RgainEdit->text().toInt(&ok, 16);
    gainG = GgainEdit->text().toInt(&ok, 16);
    gainB = BgainEdit->text().toInt(&ok, 16);

    cameraControl->SetGain(gainR,gainG,gainB);
}

void ProcessImage::onExposureGet()
{
    if (cameraControl == nullptr)
        return;
    unsigned short shutter;
    cameraControl->GetExposure(shutter);
    exposureEdit->setText(QString::number(shutter,10));
}

void ProcessImage::onExposureSet()
{
    if (cameraControl == nullptr)
        return;
    unsigned short shutter;
    bool ok;
    shutter = exposureEdit->text().toInt(&ok, 10);
    cameraControl->SetExposure(shutter);
}


#if 0
/*yuv格式转换为rgb格式*/
int ProcessImage::convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
 unsigned int in, out = 0;
 unsigned int pixel_16;
 unsigned char pixel_24[3];
 unsigned int pixel32;
 int y0, u, y1, v;
 for(in = 0; in < width * height * 2; in += 4) {
  pixel_16 =
   yuv[in + 3] << 24 |
   yuv[in + 2] << 16 |
   yuv[in + 1] <<  8 |
   yuv[in + 0];
  y0 = (pixel_16 & 0x000000ff);
  u  = (pixel_16 & 0x0000ff00) >>  8;
  y1 = (pixel_16 & 0x00ff0000) >> 16;
  v  = (pixel_16 & 0xff000000) >> 24;
  pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
  pixel_24[0] = (pixel32 & 0x000000ff);
  pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
  pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
  rgb[out++] = pixel_24[0];
  rgb[out++] = pixel_24[1];
  rgb[out++] = pixel_24[2];
  pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
  pixel_24[0] = (pixel32 & 0x000000ff);
  pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
  pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
  rgb[out++] = pixel_24[0];
  rgb[out++] = pixel_24[1];
  rgb[out++] = pixel_24[2];
 }
 return 0;
}

int ProcessImage::convert_yuv_to_rgb_pixel(int y, int u, int v)
{
 unsigned int pixel32 = 0;
 unsigned char *pixel = (unsigned char *)&pixel32;
 int r, g, b;
 r = y + (1.370705 * (v-128));
 g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
 b = y + (1.732446 * (u-128));
 if(r > 255) r = 255;
 if(g > 255) g = 255;
 if(b > 255) b = 255;
 if(r < 0) r = 0;
 if(g < 0) g = 0;
 if(b < 0) b = 0;
 pixel[0] = r * 220 / 256;
 pixel[1] = g * 220 / 256;
 pixel[2] = b * 220 / 256;
 return pixel32;
}
/*yuv格式转换为rgb格式*/
#endif

