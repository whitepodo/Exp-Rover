#include <iostream>
#include <thread>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <string>

#include "base64.h"
#include "command.h"
#include "mqtt/async_client.h"
#include "mqtt_video_receiver.h"

extern HWND hPictureBox;
extern HBITMAP hCurrentBitmap;

namespace mqtt {
	const std::string message::EMPTY_STR;
}

const std::string SERVER_ADDRESS = "tcp://54.180.119.169";
const std::string CLIENT_ID = "WinVideoReceiverClient";
//const std::string VIDEO_TOPIC = "rpi/video";

// Callback �Լ�: �޽��� ���� �� ȣ��
void OnVideoMessage(const std::string& base64) {
	std::cout << "[MQTT] ���� �޽��� ����: " << base64.length() << " ����Ʈ" << std::endl;
	
	std::vector<uchar> jpg_bytes = base64_decode(base64);
	cv::Mat image = cv::imdecode(jpg_bytes, cv::IMREAD_COLOR);

	if (image.empty()) {
		AddLogMsg(L"ERROR | ���ڵ�  ���� (�� �̹���)");
		//std::cerr << "[ERROR] ���ڵ� ���� (�� �̹���)" << std::endl;
		return;
	}

	// 180�� ȸ�� (ī�޶� �Ųٷ� �޸� ���)
	cv::rotate(image, image, cv::ROTATE_180);

	// 320x240 �� 500x375 (4:3 ���� ����)
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(500, 375), 0, 0, cv::INTER_LINEAR);

	HBITMAP hNewBitmap = MatToHBITMAP(resized);
	if (hNewBitmap) {
		// ���� ��Ʈ�� ����
		if (hCurrentBitmap) {
			DeleteObject(hCurrentBitmap);
		}
		hCurrentBitmap = hNewBitmap;

		// GUI�� ǥ��
		SendMessage(hPictureBox, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hNewBitmap);
	}

	std::cout << "[MQTT] �̹��� ���ڵ� ����: " << image.cols << "x" << image.rows << std::endl;

	// TODO: WinAPI�� ȭ�鿡 ���
}

void StartVideoReceiver(const std::string& topic) {
	std::thread([topic] {
		mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

		// �޽��� ���� �� ������ �ݹ� ���
		class callback : public virtual mqtt::callback {
		public:
			void message_arrived(mqtt::const_message_ptr msg) override {
				OnVideoMessage(msg->to_string());
			}
		};

		callback cb;
		client.set_callback(cb);

		try {
			client.connect()->wait();
			client.subscribe(topic, 1)->wait();

			AddLogMsg(L"MQTT | ���� ���� ����");
			//std::cout << "[MQTT] ���� ���� ����" << std::endl;

			// ������ ����
			while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		catch (const mqtt::exception& err) {
			std::string errorMsg = std::string("MQTT | ERROR | ") + err.what();
			std::wstring wMsg = ConvertToWString(errorMsg);

			AddLogMsg(wMsg.c_str());
			//std::cerr << "[MQTT ERROR] " << err.what() << std::endl;
		}
	}).detach();
}

HBITMAP MatToHBITMAP(const cv::Mat& mat) {
	if (mat.empty()) return nullptr;

	cv::Mat bgr;
	if (mat.channels() == 1)
		cv::cvtColor(mat, bgr, cv::COLOR_GRAY2BGR);
	else if (mat.channels() == 3)
		bgr = mat;
	else
		return nullptr;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = bgr.cols;
	bmi.bmiHeader.biHeight = -bgr.rows;  // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;
	HDC hdc = GetDC(nullptr);
	HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
	ReleaseDC(nullptr, hdc);

	if (bits) {
		memcpy(bits, bgr.data, bgr.cols * bgr.rows * 3);
	}
	return hBitmap;
}