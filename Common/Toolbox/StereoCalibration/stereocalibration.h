#ifndef STEREOCALIBRATION_H
#define STEREOCALIBRATION_H

#include <QtWidgets/QWidget>
#include "ui_stereocalibration.h"
#include "IVideoFrame.h"
#include <atomic>
#include <IDeviceControl.h>
#include <ICameraControl.h>
#include <IVideoStream.h>
#include <mutex>
#include <future>
#include <ImageAnalysis.h>
#include <type_traits>

#include "echoclient.h"

class StereoCalibration : public QWidget
{
	Q_OBJECT

public:
	explicit StereoCalibration(QWidget *parent = nullptr);
	~StereoCalibration();

	void OnStereoFrameArrival(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames);
	void OnDeviceException(int);//override

private:
	std::unique_ptr<RCClient> client;
	Ui::StereoCalibrationClass ui;

	static const auto CrossSize = 10;

	TopGear::CameraParameters camParams;
	TopGear::CameraFactor camFactor;
	QSize videoSize;
	QSize tolerance;
	float zoomRatio = 0;
	QRect masterRect;
	QRect slaveRect;

	void InitConnections();
	void ConnectCamera();
	void CloseCamera();
	std::string GetPath() const;
	QPoint MarkTarget(QPainter &paint, std::vector<std::pair<float, float>> &corners, const QBrush& brush) const;
	void MarkView(QPainter &paint, QRect &rect, const QBrush& brush);

	void SaveImagesAsync(const std::string &name, std::shared_ptr<uint8_t> &data1, std::shared_ptr<uint8_t> &data2, int width, int height);

	std::unique_ptr<uchar[]> prgb1;
	std::unique_ptr<uchar[]> prgb2;

    std::atomic<int> captureNumber;
	std::array<bool, 13> captureChecklist;

	std::shared_ptr<TopGear::IVideoStream> camera;
	std::atomic<bool> bCapture;
	std::atomic<bool> bAutoCapture;
	std::mutex mtx;
	std::future<TopGear::ImageAnalysis::Result> image_result;
	std::future<void> calCompletion;
	std::future<void> saveCompletion;
	TopGear::ImageAnalysis imageAnalysis;

	template<class T>
	static bool ParseText(QPlainTextEdit &textEdit, T &var)
	{
		auto ok = false;
		T val {};
		if (std::is_floating_point<T>::value)
			val = textEdit.toPlainText().toFloat(&ok);
		else if (std::is_integral<T>::value)
			val = textEdit.toPlainText().toInt(&ok);
		if (ok)
			var = val;
		else
			return false;
		return true;
	}

	void SendToRemote() const;

signals:
	// ReSharper disable CppFunctionIsNotImplemented
	void onstereoframe(std::shared_ptr<uint8_t>, std::shared_ptr<uint8_t>, TopGear::VideoFormat);
	void ondevexception(int);
	void oncalcompleted(TopGear::ImageAnalysis::CalibrationResult);
	// ReSharper restore CppFunctionIsNotImplemented

private slots:
	void display_error(QString err);

	void UpdateParameters();
	void OnUserTargetCheckChanged(int checked) const;

	void OnAEChanged(int state) const;
	void OnCalibration();
	void OnCalibrationCompleted(TopGear::ImageAnalysis::CalibrationResult result) const;
	void OnExposureGet() const;
	void OnExposureSet() const;

	void ShowFrames(std::shared_ptr<uint8_t> gray1, std::shared_ptr<uint8_t> gray2, TopGear::VideoFormat format);
	void handledeviceexception(int);
};

#endif // STEREOCALIBRATION_H
