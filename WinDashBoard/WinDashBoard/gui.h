#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <cstdlib>

int InitGUI(HINSTANCE hInstance, int nCmdShow);
// �޽����� ó���� �ݹ� �Լ�
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);