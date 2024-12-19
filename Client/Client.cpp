#include <iostream>
#include <windows.h>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

int main() {
    setlocale(LC_ALL, "rus");

    char command[20];
    int data;
    Employee emp;
    DWORD dw;
    bool b;

    HANDLE hSemaphore = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, L"WriteSemaphore");
    HANDLE hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"SyncMutex");
    HANDLE hServer = CreateFileW(
        L"\\\\.\\pipe\\pipe_name",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hServer == INVALID_HANDLE_VALUE) {
        cout << "������ ��������" << endl;
        return 1;
    }

    while (true) {
        cout << endl << "������� ������� (read, write ��� exit): ";
        cin >> command;

        if (strcmp(command, "exit") == 0) {
            cout << "���������� ������" << endl;
            break;
        }

        if (!WriteFile(hServer, &command, sizeof(command), &dw, NULL)) {
            cerr << "������ ������" << endl;
            return 1;
        }

        if (strcmp(command, "read") == 0) {
            cout << "������� ����� �������� ��� ������: ";
            cin >> data;

            if (!WriteFile(hServer, &data, sizeof(data), &dw, NULL)) {
                cerr << "������ ������" << endl;
                return 1;
            }

            if (!ReadFile(hServer, &b, sizeof(bool), &dw, NULL)) {
                cerr << "������ ������" << endl;
                return 1;
            }

            if (b) {
                if (!ReadFile(hServer, &emp, sizeof(Employee), &dw, NULL)) {
                    cerr << "������ ������" << endl;
                    return 1;
                }

                cout << "������ �������:" << endl;
                cout << "�����: " << emp.num << endl;
                cout << "���: " << emp.name << endl;
                cout << "����: " << emp.hours << endl;
            }
            else {
                cout << "������� � ����� ������� �� ������." << endl;
            }
        }
        else if (strcmp(command, "write") == 0) {
            cout << "������� ����� �������� ��� ������: ";
            cin >> data;

            if (!WriteFile(hServer, &data, sizeof(data), &dw, NULL)) {
                cerr << "������ ������" << endl;
                return 1;
            }

            if (!ReadFile(hServer, &b, sizeof(bool), &dw, NULL)) {
                cerr << "������ ������" << endl;
                return 1;
            }

            if (b) {
                if (!ReadFile(hServer, &emp, sizeof(Employee), &dw, NULL)) {
                    cerr << "������ ������" << endl;
                    return 1;
                }

                cout << "������ �������:" << endl;
                cout << "�����: " << emp.num << endl;
                cout << "���: " << emp.name << endl;
                cout << "����: " << emp.hours << endl;

                cout << "������� ����� ������:" << endl;
                cout << "���: ";
                cin >> emp.name;
                cout << "����: ";
                cin >> emp.hours;

                if (!WriteFile(hServer, &emp, sizeof(Employee), &dw, NULL)) {
                    cerr << "������ ������" << endl;
                    return 1;
                }
            }
            else {
                cout << "������� � ����� ������� �� ������." << endl;
            }
        }
    }

    CloseHandle(hServer);
    return 0;
}
