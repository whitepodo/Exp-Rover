#include <string>
#include <vector>

#include "gui.h"
#include "command.h"
#include "Resource.h"
#include "mqtt.h"
#include "mqtt_video_receiver.h"

HWND hListBox;
HFONT hFont;
HWND hPictureBox;
HBITMAP hCurrentBitmap = nullptr;
PROCESS_INFORMATION g_pyProcess = {};
bool g_isRunning = false;

int InitGUI(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"DASHBOARD";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"MQTT DASHBOARD", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 620,
        nullptr, nullptr, hInstance, nullptr
    );

    // �ؽ�Ʈ �Է�â
    HWND hCarNameEdit = CreateWindow(
        L"EDIT", nullptr,
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        25, 25, 200, 25,
        hwnd, (HMENU)4001, hInstance, nullptr
    );

    // ���� ��ư
    HWND hConnectBtn = CreateWindow(
        L"BUTTON", L"����",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        240, 25, 100, 25,
        hwnd, (HMENU)4002, hInstance, nullptr
    );

    // ���� â
    hPictureBox = CreateWindow(
        L"STATIC", nullptr,
        WS_VISIBLE | WS_CHILD | SS_BITMAP | WS_BORDER,
        25, 75, 500, 375,
        hwnd, nullptr, hInstance, nullptr
    );

    CreateButtonControls(hwnd, hInstance);

    HWND hLogGroup = CreateWindow(
        L"BUTTON", nullptr,
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        555, 20, 500, 550,
        hwnd, nullptr, hInstance, nullptr
    );

    // �α� â
    hListBox = CreateWindow(
        L"LISTBOX", nullptr,
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_NOTIFY | LBS_NOSEL | WS_DISABLED,
        565, 55, 480, 510,
        hwnd, (HMENU)3000, hInstance, nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // IsDialogMessage ���� (�׽�Ʈ)
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 4002: { // ���� ��ư
            wchar_t buffer[64];
            GetWindowText(GetDlgItem(hwnd, 4001), buffer, 64);
            std::wstring carName(buffer);
            if (!carName.empty()) {
                std::string nameStr(carName.begin(), carName.end());
                std::string motorTopic = nameStr + "/motor";
                std::string videoTopic = nameStr + "/video";
                AddLogMsg((L"DEBUG | Setting motor topic: " + ConvertToWString(motorTopic)).c_str());
                StartVideoReceiver(videoTopic);
                SetMotorTopic(motorTopic);
                SetFocus(hwnd);
                AddLogMsg((L"INFO | " + carName + L"�� �����").c_str());
            }
            else {
                MessageBox(hwnd, L"������ �� �̸��� �Է��ϼ���", L"�Է� ����", MB_ICONWARNING);
            }
            break;
        }
        case ID_UP:
        case ID_DOWN:
        case ID_LEFT:
        case ID_RIGHT:
        case ID_SPACE:
        case ID_CTRL:
        case ID_ALT:
            AddLogMsg((L"DEBUG | WM_COMMAND received for button ID: " + std::to_wstring(LOWORD(wParam))).c_str());
            HandleButtonCommand(hwnd, wParam);
            break;
        }
        break;
    case WM_KEYDOWN:
        // �ؽ�Ʈ �Է�â�� ��Ŀ���� ������ Ű �Է� ����
        if (GetFocus() != GetDlgItem(hwnd, 4001)) {
            SetFocus(hwnd);
            AddLogMsg((L"DEBUG | WM_KEYDOWN received with wParam: " + std::to_wstring(wParam) + L", Focus: " + (GetFocus() == hwnd ? L"Main Window" : L"Other")).c_str());
            HandleKeyDown(hwnd, wParam);
        }
        else {
            AddLogMsg(L"DEBUG | WM_KEYDOWN ignored (Textbox has focus)");
        }
        break;
    case WM_LBUTTONDOWN:
        if (GetFocus() != GetDlgItem(hwnd, 4001)) {
            SetFocus(hwnd);
            AddLogMsg(L"DEBUG | WM_LBUTTONDOWN: Focus set to main window");
        }
        break;
    case WM_DESTROY:
        if (g_isRunning) {
            TerminateProcess(g_pyProcess.hProcess, 0);
            CloseHandle(g_pyProcess.hProcess);
            CloseHandle(g_pyProcess.hThread);
        }
        if (hCurrentBitmap) {
            DeleteObject(hCurrentBitmap);
            hCurrentBitmap = nullptr;
        }
        CleanupMQTT();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}