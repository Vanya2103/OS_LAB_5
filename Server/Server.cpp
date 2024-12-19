﻿#include <iostream>
#include <windows.h>
#include <string>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

Employee* emps;
int empsNum;
HANDLE hSemaphore, hMutex;
int readerCount = 0;

DWORD WINAPI InstanceThread(LPVOID param) {
    HANDLE hPipe = (HANDLE)param;
    int empNum, fEmpNum;
    char command[20] = "";
    DWORD dwBytesRead, dwBytesWritten;
    BOOL fSuccess = FALSE;
    bool found = false;

    while (true) {
        fSuccess = ReadFile(hPipe, &command, sizeof(command), &dwBytesRead, NULL);
        if (!fSuccess || dwBytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                cout << "InstanceThread: клиент отключен." << endl;
            }
            else {
                cout << "InstanceThread: ошибка ReadFile, GLE: " << GetLastError() << endl;
            }
            break;
        }

        if (strcmp(command, "read") == 0) {
            WaitForSingleObject(hMutex, INFINITE);
            readerCount++;
            if (readerCount == 1) {
                WaitForSingleObject(hSemaphore, INFINITE);
            }
            ReleaseMutex(hMutex);

            fSuccess = ReadFile(hPipe, &empNum, sizeof(empNum), &dwBytesRead, NULL);
            if (!fSuccess || dwBytesRead == 0) {
                cout << "InstanceThread: ошибка ReadFile, GLE: " << GetLastError() << endl;
                break;
            }

            found = false;
            for (int i = 0; i < empsNum; i++) {
                if (emps[i].num == empNum) {
                    found = true;
                    fEmpNum = i;
                }
            }

            fSuccess = WriteFile(hPipe, &found, sizeof(bool), &dwBytesWritten, NULL);
            if (!fSuccess || dwBytesWritten == 0) {
                cout << "InstanceThread: ошибка WriteFile, GLE: " << GetLastError() << endl;
                break;
            }

            if (found) {
                fSuccess = WriteFile(hPipe, &emps[fEmpNum], sizeof(Employee), &dwBytesWritten, NULL);
                if (!fSuccess || dwBytesWritten == 0) {
                    cout << "InstanceThread: ошибка WriteFile, GLE: " << GetLastError() << endl;
                    break;
                }
            }

            WaitForSingleObject(hMutex, INFINITE);
            readerCount--;
            if (readerCount == 0) {
                ReleaseSemaphore(hSemaphore, 1, NULL);
            }
            ReleaseMutex(hMutex);
        }
        else if (strcmp(command, "write") == 0) {
            WaitForSingleObject(hSemaphore, INFINITE);

            fSuccess = ReadFile(hPipe, &empNum, sizeof(empNum), &dwBytesRead, NULL);
            if (!fSuccess || dwBytesRead == 0) {
                cout << "InstanceThread: ошибка ReadFile, GLE: " << GetLastError() << endl;
                break;
            }

            found = false;
            for (int i = 0; i < empsNum; i++) {
                if (emps[i].num == empNum) {
                    found = true;
                    fEmpNum = i;
                }
            }

            fSuccess = WriteFile(hPipe, &found, sizeof(bool), &dwBytesWritten, NULL);
            if (!fSuccess || dwBytesWritten == 0) {
                cout << "InstanceThread: ошибка WriteFile, GLE: " << GetLastError() << endl;
                break;
            }

            if (found) {
                fSuccess = WriteFile(hPipe, &emps[fEmpNum], sizeof(Employee), &dwBytesWritten, NULL);
                if (!fSuccess || dwBytesWritten == 0) {
                    cout << "InstanceThread: ошибка WriteFile, GLE: " << GetLastError() << endl;
                    break;
                }

                fSuccess = ReadFile(hPipe, &emps[fEmpNum], sizeof(Employee), &dwBytesRead, NULL);
                if (!fSuccess || dwBytesRead == 0) {
                    cout << "InstanceThread: ошибка ReadFile, GLE: " << GetLastError() << endl;
                    break;
                }
            }

            ReleaseSemaphore(hSemaphore, 1, NULL);
        }
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    cout << "Поток завершил свою работу." << endl;
    return 1;
}

int main() {
    setlocale(LC_ALL, "rus");

    PROCESS_INFORMATION pi;
    STARTUPINFOW si; // Используем Unicode STARTUPINFO
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);

    wchar_t commandLine[] = L"Client.exe";
    int kolofclients;

    cout << "Введите количество студентов: ";
    cin >> empsNum;
    emps = new Employee[empsNum];

    cout << "Введите данные о студенте (номер, имя, часы):" << endl;
    for (int i = 0; i < empsNum; i++) {
        cout << "Данные " << i + 1 << "-го студента:" << endl;
        cout << "номер: ";
        cin >> emps[i].num;
        cout << "имя: ";
        cin >> emps[i].name;
        cout << "часы: ";
        cin >> emps[i].hours;
    }

    cout << "Введите количество клиентов: ";
    cin >> kolofclients;

    hSemaphore = CreateSemaphoreW(NULL, 1, kolofclients, L"WriteSemaphore");
    hMutex = CreateMutexW(NULL, FALSE, L"SyncMutex");

    HANDLE hPipe;
    HANDLE* hThreads = new HANDLE[kolofclients];
    DWORD dwThreadId;

    for (int i = 0; i < kolofclients; i++) {
        hPipe = CreateNamedPipeW( // Используем Unicode CreateNamedPipeW
            L"\\\\.\\pipe\\pipe_name",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            kolofclients, 0, 0,
            INFINITE,
            NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            cout << "Не удалось создать трубу " << i + 1 << ", GLE: " << GetLastError() << endl;
            return -1;
        }

        if (!CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            cout << "Не удалось создать процесс " << i + 1 << "." << endl;
            return -1;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        if (ConnectNamedPipe(hPipe, NULL)) {
            cout << "Клиент " << i + 1 << " подключен." << endl;
            hThreads[i] = CreateThread(NULL, 0, InstanceThread, (LPVOID)hPipe, 0, &dwThreadId);
            if (hThreads[i] == 0) {
                cout << "Не удалось создать поток " << i + 1 << "." << endl;
                return -1;
            }
        }
        else {
            CloseHandle(hPipe);
        }
    }

    WaitForMultipleObjects(kolofclients, hThreads, TRUE, INFINITE);
    for (int i = 0; i < kolofclients; i++) {
        CloseHandle(hThreads[i]);
    }

    return 0;
}
