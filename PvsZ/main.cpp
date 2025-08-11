#include <Windows.h>
#include <TlHelp32.h>
#include "resource.h"
#include <iostream>
#include <commctrl.h>  // ��� InitCommonControlsEx
#pragma comment(lib, "comctl32.lib")




DWORD GetModuleBaseAddress(DWORD procID, const wchar_t* modName) {
    DWORD baseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procID);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(snapshot, &modEntry)) {
            do {
                if (_wcsicmp(modEntry.szModule, modName) == 0) {
                    baseAddress = (DWORD)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &modEntry));
        }
        CloseHandle(snapshot);
    }
    return baseAddress;
}

#define BUTTON_ID 0x01
#define CHECKBOX_ID 0x02



LPCWSTR windowBitmapPath = L"P_vs_Z.bmp";
LPCWSTR buttonBitmapPath = L"buttonImage.bmp";
LPCWSTR ClassName = L"MyClassName";

HWND hAddressLabel = NULL;
HWND hProcessIDLabel = NULL;

int backgroundImg[2] = { 451, 278 };
int newValue = 200;
DWORD address = 0;
DWORD procID = 0;

// ����� � ������ �� ����� PID/������, ������������ ����� ����� ��������� �����
DWORD WINAPI MemoryThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;

    HWND hwndGame = FindWindowA(NULL, "Plants vs. Zombies");
    if (!hwndGame) return 1;

    GetWindowThreadProcessId(hwndGame, &procID);

    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
    if (!handle) return 0;

    DWORD baseAddress = GetModuleBaseAddress(procID, L"popcapgame1.exe");
    if (!baseAddress) {
        CloseHandle(handle);
        return 0;
    }

    DWORD firstValue = 0;
    DWORD firstAddr = baseAddress + 0x331C50;
    if (!ReadProcessMemory(handle, (LPCVOID)firstAddr, &firstValue, sizeof(firstValue), NULL)) {
        CloseHandle(handle);
        return 0;
    }

    DWORD secondValue = 0;
    DWORD secondAddr = firstValue + 0x868;
    if (!ReadProcessMemory(handle, (LPCVOID)secondAddr, &secondValue, sizeof(secondValue), NULL)) {
        CloseHandle(handle);
        return 0;
    }

    address = secondValue + 0x5578;

    // ��������� ����� ������� (��������� � ��� �������� WM_SETTEXT � ������� �����)
    wchar_t addrText[256];
    swprintf(addrText, 256, L"�����: 0x%X", address);
    SetWindowTextW(hAddressLabel, addrText);

    wchar_t pidText[256];
    swprintf(pidText, 256, L"ID ��������: %u", procID);
    SetWindowTextW(hProcessIDLabel, pidText);

    CloseHandle(handle);
    return 0;
}

LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // ������ UI �������� �������
        HWND closeButton = CreateWindow(L"button", NULL,
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_BITMAP,
            backgroundImg[0] - 18, 0, 18, 16,
            hWnd, (HMENU)BUTTON_ID, GetModuleHandle(NULL), NULL);
        HBITMAP closeBitmap = (HBITMAP)LoadImage(NULL, buttonBitmapPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        if (closeBitmap) SendMessage(closeButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)closeBitmap);

        CreateWindow(L"BUTTON", L"Caps Mode", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            60, 50, 120, 20, hWnd, (HMENU)CHECKBOX_ID, GetModuleHandle(NULL), NULL);

        CreateWindow(L"STATIC", L"by Talo9", WS_CHILD | WS_VISIBLE,
            (backgroundImg[0] / 2) - 50, backgroundImg[1] - 40, 70, 20,
            hWnd, NULL, GetModuleHandle(NULL), NULL);

        hAddressLabel = CreateWindow(L"STATIC", L"�����: (��������)", WS_CHILD | WS_VISIBLE,
            60, 160, 300, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);

        hProcessIDLabel = CreateWindow(L"STATIC", L"ID ��������: (��������)", WS_CHILD | WS_VISIBLE,
            60, 190, 300, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);

        HWND hButton = CreateWindowEx(
            0,                // WS_EX_ �����
            L"BUTTON",         // ����� ��������
            L"DFD",     // �����
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // �����
            10, 10, 100, 30,  // ������� � ������
            hWnd,             // ������������ ����
            (HMENU)1001,      // ID ��������
            GetModuleHandle(NULL),        // ��������� ����������
            NULL              // ���. ������
        );

        // ������, ����� ����� �������, ��������� ����� ��� ��������������� ������ PID/������
        CreateThread(NULL, 0, MemoryThread, hWnd, 0, NULL);

        // � ��������� ������, ������� ����� ������ � ������ (���� ����� ������)
        SetTimer(hWnd, 1, 50, NULL); // 50 ms
    } break;

    case WM_TIMER:
        if (wParam == 1) {
            if (address && procID) {
                // ��������� ������� ������ ��� ������, ����� ����� ���������
                HANDLE hProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, procID);
                if (hProc) {
                    int value = 0;
                    ReadProcessMemory(hProc, (LPCVOID)address, &value, sizeof(value), NULL);
                    //CloseHandle(hProc);
                    if ((GetKeyState(VK_CAPITAL) & 0x0001) && (GetAsyncKeyState(VK_TAB) & 0x8000)) {
                        newValue = value + 200;
                        if (WriteProcessMemory(hProc, (LPVOID)address, &newValue, sizeof(newValue), 0)) {

                            // ������� �����, ����� ������ ����������
                            wchar_t tmp[128];
                            //swprintf(tmp, 128, L"�����: 0x%X (������� %d) (%d)", address, newValue, value);
                            swprintf(tmp, 128, L"(������� %d) ", newValue);
                            SetWindowTextW(hAddressLabel, tmp);
                        }
                        else {
                            SetWindowTextW(hAddressLabel, L"������ ������");
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case BUTTON_ID:
            DestroyWindow(hWnd);
            break;
        case CHECKBOX_ID:
            if (HIWORD(wParam) == BN_CLICKED) {
                LRESULT state = SendDlgItemMessage(hWnd, CHECKBOX_ID, BM_GETCHECK, 0, 0);
                MessageBox(hWnd, state == BST_CHECKED ? L"������� �������" : L"������� ��������", L"Info", MB_OK);
            }
            break;
        }
        break;

    case WM_NCHITTEST:
        if (DefWindowProc(hWnd, msg, wParam, lParam) == HTCLIENT) return HTCAPTION;
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int nCmdLine) {

    // ������������� Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    HBRUSH brush = (HBRUSH)CreatePatternBrush((HBITMAP)LoadImage(NULL, windowBitmapPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));

    WNDCLASSEX windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = hInst;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hbrBackground = brush;
    windowClass.lpszClassName = ClassName;



    windowClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    windowClass.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));


    if (!RegisterClassEx(&windowClass)) return EXIT_FAILURE;

    HWND windowHandle = CreateWindowEx(WS_EX_LAYERED, ClassName, NULL, WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, backgroundImg[0], backgroundImg[1],
        NULL, NULL, hInst, NULL);

    if (!windowHandle) {
        MessageBox(NULL, L"Error", L"���-�� ����� �� ��� ��� �������� ����", NULL);
        return EXIT_FAILURE;
    }

    SetLayeredWindowAttributes(windowHandle, RGB(0, 255, 255), 0, LWA_COLORKEY);
    ShowWindow(windowHandle, nCmdLine);
    UpdateWindow(windowHandle);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
