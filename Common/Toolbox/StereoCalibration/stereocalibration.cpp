#include "stereocalibration.h"
#include <QtWidgets/QMessageBox>
#include <QFileDialog>
#include <DeepCamAPI.h>
#include "ImageAnalysis.h"
#include <thread>
#include <mutex>
#include <future>
#include <QtGui>
#include <iomanip>
#include <sstream>
//#include <ctime>
#include <ILowlevelControl.h>


StereoCalibration::StereoCalibration(QWidget *parent)
	: QWidget(parent), bCapture(false), bAutoCapture(false),
	  imageAnalysis(std::bind(&TopGear::DeepCamAPI::WriteLog, &TopGear::DeepCamAPI::Instance(), TopGear::Level::Info, std::placeholders::_1))
{
    captureNumber = 1;
	ui.setupUi(this);

	captureChecklist.fill(false);

	auto &api = TopGear::DeepCamAPI::Instance();
	api.EnableLog(TopGear::LogType::Standard | TopGear::LogType::DailyFile);
	api.SetLogLevel(TopGear::Level::Info);

	client.reset(new RCClient(nullptr, true));

	InitConnections();
	

	if (!(ParseText(*ui.textRows, camParams.ChessboardDimension.second) && camParams.ChessboardDimension.second>2))
	{
		ui.labelInfo->setText("Value of chessboard rows is invalid!");
		return;
	}

	if (!(ParseText(*ui.textColumns, camParams.ChessboardDimension.first) && camParams.ChessboardDimension.first>2))
	{
		ui.labelInfo->setText("Value of chessboard columns is invalid!");
		return;
	}
}

StereoCalibration::~StereoCalibration()
{
	CloseCamera();
	if (saveCompletion.valid())
		saveCompletion.wait();
	if (calCompletion.valid())
		calCompletion.wait();
}

void StereoCalibration::InitConnections()
{
	
	connect(ui.cbUser, SIGNAL(stateChanged(int)), this, SLOT(OnUserTargetCheckChanged(int)));
	connect(ui.cbAE, SIGNAL(stateChanged(int)), this, SLOT(OnAEChanged(int)));
	connect(ui.buttonExGet, SIGNAL(clicked()), this, SLOT(OnExposureGet()));
	connect(ui.buttonExSet, SIGNAL(clicked()), this, SLOT(OnExposureSet()));

	connect(ui.buttonUpdate, &QPushButton::clicked, this, &StereoCalibration::UpdateParameters);
	
	connect(ui.buttonCapture, &QPushButton::clicked, [this]()
	{
		bCapture = true;
	});

	connect(ui.buttonCal, SIGNAL(clicked()), this, SLOT(OnCalibration()));

	connect(ui.buttonConnect, &QPushButton::clicked, [this]()
	{
		if (client->IsConnected())
		{
			client->Disconnect();
		}
		else
		{
			client->Connect(QStringLiteral("ws://") + ui.textIP->toPlainText() + QStringLiteral(":10080"));
		}

	});

	connect(client.get(), &RCClient::connected, [this]()
	{
		QIcon icon;
		icon.addFile(QStringLiteral(":/StereoCalibration/Resources/connect-64.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui.buttonConnect->setIcon(icon);
		ui.buttonConnect->setToolTip(QStringLiteral("Connected, click to disconnect"));
		ui.labelInfo->clear();
		ui.textIP->setReadOnly(true);
		TopGear::DeepCamAPI::Instance().WriteLog(TopGear::Level::Info,
			"Target generator on host " + ui.textIP->toPlainText().toStdString() + " connected");
	});

	connect(client.get(), &RCClient::closed, [this]()
	{
		QIcon icon;
		icon.addFile(QStringLiteral(":/StereoCalibration/Resources/disconnect-64.png"), QSize(), QIcon::Normal, QIcon::Off);
		ui.buttonConnect->setIcon(icon);
		ui.buttonConnect->setToolTip(QStringLiteral("Disconnected, click to connect"));
		ui.labelInfo->setText("Target generator disconnected!");
		ui.textIP->setReadOnly(false);
		TopGear::DeepCamAPI::Instance().WriteLog(TopGear::Level::Info,
			"Target generator on host " + ui.textIP->toPlainText().toStdString() + " disconnected");
	});

	qRegisterMetaType<std::shared_ptr<uint8_t>>("std::shared_ptr<uint8_t>");
	qRegisterMetaType<TopGear::VideoFormat>("TopGear::VideoFormat");
	connect(this, &StereoCalibration::onstereoframe, this, &StereoCalibration::ShowFrames, Qt::QueuedConnection);
	
	connect(this, SIGNAL(ondevexception(int)),
		this, SLOT(handledeviceexception(int)),
		Qt::QueuedConnection);

	qRegisterMetaType<TopGear::ImageAnalysis::CalibrationResult>("TopGear::ImageAnalysis::CalibrationResult");
	connect(this, &StereoCalibration::oncalcompleted, this, &StereoCalibration::OnCalibrationCompleted);

	connect(ui.toolFolder, &QToolButton::clicked, [this]()
	{
		auto dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
			"/home",
			QFileDialog::ShowDirsOnly
			| QFileDialog::DontResolveSymlinks);
		ui.lineEditPath->setText(dir);
	});

	connect(ui.lineEditPath, &QLineEdit::textChanged, [this]()
	{
		ui.lineEditPath->setToolTip(ui.lineEditPath->text());
	});

	connect(ui.buttonCamera, &QPushButton::clicked, [this]()
	{
		CloseCamera();
		ConnectCamera();
	});

	connect(ui.cbAC, &QCheckBox::stateChanged, [this](int checked)
	{
		captureChecklist.fill(false);
		if (checked)
		{
			captureNumber = 1;
			bAutoCapture = true;
			ui.buttonCapture->setEnabled(false);
			ui.buttonCal->setEnabled(false);
			ui.buttonUpdate->setEnabled(false);
			ui.buttonCamera->setEnabled(false);
		}
		else
		{
			bAutoCapture = false;
			ui.buttonCapture->setEnabled(true);
			ui.buttonCal->setEnabled(true);
			ui.buttonUpdate->setEnabled(true);
			ui.buttonCamera->setEnabled(true);
		}
	});
}

void StereoCalibration::OnStereoFrameArrival(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames)//this is called in sub-thread
{
	(void)sender;
	if (frames.size() != 2)
		return;
	if (prgb1 == nullptr || prgb2 == nullptr)
		return;

	auto format = frames[0]->GetFormat();
	auto w = format.Width;
	auto h = format.Height;

	auto gray1 = std::shared_ptr<uint8_t>(new uint8_t[w*h], std::default_delete<uint8_t[]>());
	auto gray2 = std::shared_ptr<uint8_t>(new uint8_t[w*h], std::default_delete<uint8_t[]>());

	uint8_t *raw1, *raw2;
	uint32_t stride;
	frames[0]->LockBuffer(&raw1, &stride);
	frames[1]->LockBuffer(&raw2, &stride);
	auto rt = std::thread(&TopGear::ImageAnalysis::ConvertRawGB12ToGray, &imageAnalysis,
		reinterpret_cast<uint16_t *>(raw2), gray2.get(), w, h, prgb2.get());
	imageAnalysis.ConvertRawGB12ToGray(reinterpret_cast<uint16_t *>(raw1), gray1.get(), w, h, prgb1.get());
	rt.join();
	frames[0]->UnlockBuffer();
	frames[1]->UnlockBuffer();
	emit onstereoframe(gray1, gray2, format);
}

void StereoCalibration::OnDeviceException(int err) 
{
	emit ondevexception(err);
}

void StereoCalibration::ConnectCamera()
{
	auto &api = TopGear::DeepCamAPI::Instance();
	api.WriteLog(TopGear::Level::Info, "Initialized");
	camera = api.CreateCamera(TopGear::Camera::CamaroDual);
	if (camera)
	{
		//auto ioControl = std::dynamic_pointer_cast<TopGear::IDeviceControl>(camera);
		//TopGear::PropertyData<std::string> info;
		std::string info;
		TopGear::IDeviceControl::GetProperty(camera, "DeviceInfo", info);
		auto cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
		if (cameraControl)
		{
			cameraControl->Flip(true, true);
			cameraControl->SetExposure(800);
		}
		TopGear::IVideoStream::RegisterFrameCallback(*camera,
			&StereoCalibration::OnStereoFrameArrival, this);
		//labeldevinfo->setText(QString("devinfo:%1").arg(item->GetDeviceInfo().c_str()));
		//Get optimized video format
		TopGear::VideoFormat format;
		auto index = camera->GetOptimizedFormatIndex(format);

		ui.labelCamera->setText(QString::fromStdString("Camera : " + info + " X2  Resolution: ") +
			QString::number(format.Width) + "x" + QString::number(format.Height));
		
		videoSize.setHeight(format.Height);
		videoSize.setWidth(format.Width);
		videoSize.scale(800, 600, Qt::KeepAspectRatio);

		camParams.UserRes.first = format.Width;
		camParams.UserRes.second = format.Height;
		zoomRatio = float(videoSize.width()) / format.Width;

		camera->SetCurrentFormat(index);
		camera->StartStream();

		ui.sharpLeft->clear();
		ui.sharpRight->clear();

		prgb1 = std::unique_ptr<uchar[]>(new uchar[format.Width*format.Height * 3]);
		prgb2 = std::unique_ptr<uchar[]>(new uchar[format.Width*format.Height * 3]);

		tolerance.setWidth(format.Width / 20);
		tolerance.setHeight(format.Height / 20);

		UpdateParameters();
	}
}

void StereoCalibration::CloseCamera()
{
	ui.labelCamera->setText("Camera : ");
	if (camera)
		camera->StopStream();
	if (image_result.valid())
		image_result.wait();
	camera.reset();
	prgb1.reset();
	prgb2.reset();
}

std::string StereoCalibration::GetPath() const
{
	auto path = ui.lineEditPath->text();
	if (path.isEmpty())
		return{};
	QDir dir(path);
	if (!dir.exists())
		dir.mkpath(".");
	return path.toStdString()+"/";
}

QPoint StereoCalibration::MarkTarget(QPainter& paint, std::vector<std::pair<float, float>>& corners, const QBrush& brush) const
{
	auto xpos = 0;
	auto ypos = 0;
	for (auto &point : corners)
	{
		xpos += point.first;
		ypos += point.second;
		//paint.drawEllipse(QPoint(point.first, point.second), 3, 3);
	}
	xpos /= corners.size();
	ypos /= corners.size();
	QPoint p(xpos, ypos);
	if (zoomRatio > 0)
	{
		xpos *= zoomRatio;
		ypos *= zoomRatio;
	}
	paint.setPen(QPen(brush, 3));
	paint.drawLine(xpos - CrossSize, ypos, xpos + CrossSize, ypos);
	paint.drawLine(xpos, ypos - CrossSize, xpos, ypos + CrossSize);
	return std::move(p);
}

void StereoCalibration::MarkView(QPainter& paint, QRect& rect, const QBrush& brush)
{
	if (!rect.isValid())
		return;
	paint.setPen(QPen(brush,3, Qt::DashLine));
	//if (zoomRatio > 0)
	//{
	//	rect.setSize(rect.size() * zoomRatio);
	//	rect.setTopLeft(rect.topLeft() * zoomRatio);
	//}
	paint.drawRect(rect);
	
	auto &positions = camFactor.GetCalLocations();
    for (auto i = 0u; i < positions.size(); ++i)
	{
		if (captureChecklist[i])
			paint.setPen(QPen(Qt::red, 3));
		else
			paint.setPen(QPen(brush, 3));
		if (i == 0)
		{
			auto center = rect.center();
			paint.drawLine(center.x() - CrossSize, center.y(), center.x() + CrossSize, center.y());
			paint.drawLine(center.x(), center.y() - CrossSize, center.x(), center.y() + CrossSize);
		}
		else
		{
			QPoint p(positions[i].first, positions[i].second);
			p *= zoomRatio;
			p += rect.topLeft();
			paint.drawEllipse(p, 3, 3);
		}
	}
}

void StereoCalibration::display_error(QString err)
{
    (void)err;
	//QMessageBox::warning(this,tr("error"), err,QMessageBox::Yes);
	//qDebug() << err;
}

QString float2QString(float number, int prec=2)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(prec) << number;
	return QString::fromStdString(ss.str());
}

void StereoCalibration::UpdateParameters()
{
	if (!(ParseText(*ui.textf, camParams.LensFocus) && camParams.LensFocus>0))
	{
		ui.labelInfo->setText("Value of lens f is invalid!");
		return;
	}

	if (!(ParseText(*ui.textF, camParams.LensFSplash) && camParams.LensFSplash>0))
	{
		ui.labelInfo->setText("Value of lens F/ is invalid!");
		return;
	}

	if (!(ParseText(*ui.textSensorW, camParams.SensorSize.first) && camParams.SensorSize.first>0))
	{
		ui.labelInfo->setText("Value of sensor width is invalid!");
		return;
	}

	if (!(ParseText(*ui.textSensorH, camParams.SensorSize.second) && camParams.SensorSize.second>0))
	{
		ui.labelInfo->setText("Value of sensor height is invalid!");
		return;
	}
	
	if (!(ParseText(*ui.textPixel, camParams.PixelSize) && camParams.PixelSize>0))
	{
		ui.labelInfo->setText("Value of sensor pixel size is invalid!");
		return;
	}

	if (!(ParseText(*ui.textR, camParams.BlurRadius) && camParams.BlurRadius>0))
	{
		ui.labelInfo->setText("Value of blur radius is invalid!");
		return;
	}

	if (!(ParseText(*ui.textTarget, camParams.TargetDistance) && camParams.TargetDistance>0))
	{
		ui.labelInfo->setText("Value of target distance is invalid!");
		return;
	}

	if (!(ParseText(*ui.textShrink, camParams.ShrinkCoeff) && camParams.ShrinkCoeff>1))
	{
		ui.labelInfo->setText("Value of target distance is invalid!");
		return;
	}

	if (!(ParseText(*ui.textRows, camParams.ChessboardDimension.second) && camParams.ChessboardDimension.second>2))
	{
		ui.labelInfo->setText("Value of chessboard rows is invalid!");
		return;
	}

	if (!(ParseText(*ui.textColumns, camParams.ChessboardDimension.first) && camParams.ChessboardDimension.first>2))
	{
		ui.labelInfo->setText("Value of chessboard columns is invalid!");
		return;
	}

	if (!(ParseText(*ui.textBaseline, camParams.Baseline) && camParams.Baseline>0))
	{
		ui.labelInfo->setText("Value of chessboard columns is invalid!");
		return;
	}

	if (ui.cbUser->checkState() == Qt::Checked)
	{
		if (ParseText(*ui.textTargetSide, camParams.ChessboardSide) && camParams.ChessboardSide > 0)
			camParams.ChessboardSide /= 1000;
		else
		{
			ui.labelInfo->setText("Value of square side length is invalid!");
			return;
		}
	}
	else
	{
		camParams.ChessboardSide = -1;
	}

	try
	{
		camFactor.Update(camParams);
	}
	catch(const std::invalid_argument &ex)
	{
		ui.labelInfo->setText(ex.what());
		return;
	}

	auto dof = camFactor.GetDOF();
	ui.textDofMin->setPlainText(float2QString(dof.first));
	ui.textDofMax->setPlainText(float2QString(dof.second));
	float h, pan, tilt;
	camFactor.GetPosture(h, pan, tilt);

	ui.textOffset->setPlainText(float2QString(h));
	ui.textPan->setPlainText(float2QString(pan));
	ui.textTilt->setPlainText(float2QString(tilt));

	auto view = camFactor.GetStereoViewInPixel();
	auto deltaX = int((camParams.UserRes.first - view.first)*zoomRatio);
	masterRect = QRect(deltaX, 0, videoSize.width() - deltaX, videoSize.height());
	slaveRect = QRect(0, 0, videoSize.width() - deltaX, videoSize.height());
	float sq;
	std::pair<float, float> size;
	camFactor.GetChessboard(sq, size);

	ui.labelTargetSize->setText(float2QString(size.first * 1000, 0) + " x " +
		float2QString(size.second * 1000, 0) + " mm");

	if (ui.cbUser->checkState() != Qt::Checked)
		ui.textTargetSide->setPlainText(float2QString(sq * 1000, 0));

	ui.labelWarning->hide();
	ui.labelInfo->clear();

	SendToRemote();
}

void StereoCalibration::SendToRemote() const
{
	if (!client->IsConnected())
		return;
	std::stringstream ss;
	ss << "{ \"columns\":" << camParams.ChessboardDimension.first + 1;
	ss << ", \"rows\":" << camParams.ChessboardDimension.second + 1;
	ss << ", \"side\":" << ui.textTargetSide->toPlainText().toStdString() << " }";
	client->Send(QString::fromStdString(ss.str()));
	TopGear::DeepCamAPI::Instance().WriteLog(TopGear::Level::Info,
		"Send JSON message "+ ss.str() + " to " + ui.textIP->toPlainText().toStdString());
}

void StereoCalibration::OnUserTargetCheckChanged(int checked) const
{
	if (checked)
	{
		ui.textTargetSide->setReadOnly(false);
	}
	else
	{
		ui.textTargetSide->setReadOnly(true);
	}
}

void StereoCalibration::OnAEChanged(int checked) const
{
	auto lowlevel = std::dynamic_pointer_cast<TopGear::ILowlevelControl>(camera);
	if (lowlevel == nullptr)
		return;
	lowlevel->SetRegister(0x3100, checked ? 0x13 : 0x00);
}

void StereoCalibration::OnCalibration()
{
	ui.buttonCal->setText("Calibrating...");
	ui.buttonCal->setEnabled(false);
	QApplication::processEvents();	//Update UI changes
	calCompletion = std::async([this]() {
		float sq;
		std::pair<float, float> size;
		camFactor.GetChessboard(sq, size);
		auto result = imageAnalysis.StereoCalibrate(camParams, GetPath());
		emit oncalcompleted(result);
	});
}

void StereoCalibration::OnCalibrationCompleted(TopGear::ImageAnalysis::CalibrationResult result) const
{
	ui.buttonCal->setText("Calibrate");
	ui.buttonCal->setEnabled(true);
	QApplication::processEvents();	//Update UI changes
	QString msg = result.Completed ? "Calibration Completed! \n" : "Calibration failed! \n";
	if (result.Completed)
		msg += "\n " + QString::number(result.Pairs) + " pairs of image used" +
		"\n RMS error = " + QString::number(result.RMS) +
		"\n Epipolar error = " + QString::number(result.EpipolarError) +
		"\n ROI = (" + QString::number(result.ROI[0]) + ", " +
		QString::number(result.ROI[1]) + ", " +
		QString::number(result.ROI[2]) + ", " +
		QString::number(result.ROI[3]) + ")";
	QMessageBox msgBox(QMessageBox::Information, "Info", msg,
		QMessageBox::Ok, const_cast<StereoCalibration *>(this));
	msgBox.exec();
}

void StereoCalibration::OnExposureGet() const
{
	auto cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
	if (cameraControl == nullptr)
		return;
	unsigned short shutter;
	cameraControl->GetExposure(shutter);
	ui.textEx->setPlainText(QString::number(shutter, 10));
}

void StereoCalibration::OnExposureSet() const
{
	auto cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
	if (cameraControl == nullptr)
		return;
    unsigned short shutter = 0;
	ParseText(*ui.textEx, shutter);
	cameraControl->SetExposure(shutter);
}

void StereoCalibration::handledeviceexception(int err)
{
    (void)err;
	CloseCamera();
	QMessageBox msgBox;
	msgBox.setText("----Device Lost!!!!!");
	msgBox.exec();
}

void StereoCalibration::SaveImagesAsync(const std::string &name, std::shared_ptr<uint8_t> &data1, std::shared_ptr<uint8_t> &data2, int width, int height)
{
	if (saveCompletion.valid())
		saveCompletion.wait();
	saveCompletion = std::async([this](std::string f, std::shared_ptr<uint8_t> p1, std::shared_ptr<uint8_t> p2, int w, int h)
	{
		imageAnalysis.WriteImage(p1.get(), w, h, f + "L", 1);
		imageAnalysis.WriteImage(p2.get(), w, h, f + "R", 1);
	}, name, data1, data2, width, height);
}

void StereoCalibration::ShowFrames(std::shared_ptr<uint8_t> gray1, std::shared_ptr<uint8_t> gray2, TopGear::VideoFormat format)
{
	static std::vector<std::pair<float, float>> masterCorners;
	static std::vector<std::pair<float, float>> slaveCorners;
	static auto masterCount = true;

	auto w = format.Width;
	auto h = format.Height;

	if (bCapture.exchange(false))
	{
		time_t rawtime;
		std::time(&rawtime);
		auto timeinfo = std::localtime(&rawtime);
		char buffer[80]{};
		std::strftime(buffer, 80, "%y%m%d-%H%M%S-", timeinfo);
		std::string filename(buffer);
		filename = GetPath() + filename;
		SaveImagesAsync(filename, gray1, gray2, w, h);
	}

	std::unique_lock<std::mutex> ul(mtx, std::try_to_lock);
	if (ul)
	{
		if (image_result.valid())
		{
			auto result = image_result.get();

			if (masterCount)
			{
				masterCorners = std::move(result.Corners);
				ui.sharpLeft->setText("Sharpness: " + QString::number(result.Sharpness));
			}
			else
			{
				slaveCorners = std::move(result.Corners);
				ui.sharpRight->setText("Sharpness: " + QString::number(result.Sharpness));
			}
			masterCount = !masterCount;
		}
		ul.unlock();
		if (ui.cbAF->checkState() == Qt::Checked)
		{
			image_result = std::async([this](std::shared_ptr<uchar> pixel, int width, int height, std::pair<int,int> dimension)
			{
				std::lock_guard<std::mutex> lock(mtx);
				return imageAnalysis.Process(pixel, width, height, dimension.first, dimension.second, ui.cbFastCheck->checkState());
			}, masterCount ? gray1 : gray2, w, h, camParams.ChessboardDimension);
		}
		else
		{
			ui.sharpLeft->clear();
			ui.sharpRight->clear();
			masterCorners.clear();
			slaveCorners.clear();
		}
	}

	auto pixmap1(QPixmap::fromImage(QImage(prgb1.get(), w, h, QImage::Format_RGB888), Qt::AutoColor).scaled(videoSize));
	auto pixmap2(QPixmap::fromImage(QImage(prgb2.get(), w, h, QImage::Format_RGB888), Qt::AutoColor).scaled(videoSize));
	/*auto pixmap1(QPixmap::fromImage(QImage(gray1.get(), w, h, QImage::Format_Grayscale8), Qt::MonoOnly).scaled(videoSize));
	auto pixmap2(QPixmap::fromImage(QImage(gray2.get(), w, h, QImage::Format_Grayscale8), Qt::MonoOnly).scaled(videoSize));*/

	auto sync = std::async([&, this]() {
		auto result = false;
		QPainter paint;
		paint.begin(&pixmap2);
		MarkView(paint, slaveRect, Qt::green);
		if (slaveCorners.size() > 0)
		{
			MarkTarget(paint, slaveCorners, Qt::blue);
			result = true;
		}
		paint.end();
		return result;
	});

	QPoint targetLocation;
	QPainter paint;
	paint.begin(&pixmap1);
	MarkView(paint, masterRect, Qt::green);
	if (masterCorners.size() > 0)
	{
		targetLocation = MarkTarget(paint, masterCorners, Qt::blue);
	}
	paint.end();

	if (sync.get() && masterCorners.size()>0 && bAutoCapture)
	{
		auto offsetX = w - camFactor.GetStereoViewInPixel().first;
		auto offsetY = h - camFactor.GetStereoViewInPixel().second;
		auto &positions = camFactor.GetCalLocations();
		auto allDone = true;
        for (auto i = 0u; i < positions.size();++i)
		{
			if (captureChecklist[i])
				continue;
			allDone = false;
			if (TopGear::fast_abs(targetLocation.x() - (positions[i].first+offsetX))<tolerance.width() &&
				TopGear::fast_abs(targetLocation.y() - (positions[i].second+offsetY))<tolerance.height())
			{
				captureChecklist[i] = true;
				//Save
				auto filename = GetPath() + std::to_string(captureNumber.fetch_add(1));
				SaveImagesAsync(filename, gray1, gray2, w, h);
				break;
			}
		}
		if (allDone)
		{
			ui.cbAC->setCheckState(Qt::Unchecked);
			QApplication::processEvents();	//Update UI changes
		}
	}



	ui.videoLeft->setPixmap(pixmap1);
	ui.videoRight->setPixmap(pixmap2);
}

