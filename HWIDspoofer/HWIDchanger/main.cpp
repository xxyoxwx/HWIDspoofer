#include <string>
#include <Windows.h>
#include <winioctl.h>
#include <aclapi.h>
#include <sys/stat.h>
#include <iostream>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <cfgmgr32.h>


#define MY_DEVICE_IOCTL 0x800
#define MY_IOCTL_SPOOF CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define DEVICE_NAME L"\\\\.\\COM1"
#define MAX_DEVICE_LEN 128

std::wstring FindDeviceName() {
    std::wstring deviceName;
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (deviceInfoSet != INVALID_HANDLE_VALUE) {
    # TO COE KEKE
        SP_DEVINFO_DATA deviceInfoData = {};
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        DWORD index = 0;
        while (SetupDiEnumDeviceInfo(deviceInfoSet, index++, &deviceInfoData)) {
            WCHAR instanceID[MAX_DEVICE_ID_LEN];
            if (CM_Get_Device_ID(deviceInfoData.DevInst, instanceID, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS) {
                HKEY hDeviceKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (hDeviceKey != INVALID_HANDLE_VALUE) {
                    WCHAR deviceNameBuffer[MAX_PATH];
                    DWORD bufferSize = sizeof(deviceNameBuffer);
                    if (RegQueryValueEx(hDeviceKey, L"PortName", NULL, NULL, (LPBYTE)deviceNameBuffer, &bufferSize) == ERROR_SUCCESS) {
                        deviceName = deviceNameBuffer;
                        break;
                    }
                    RegCloseKey(hDeviceKey);
                }
            }
        }
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
    return deviceName;
}

std::string GetCurrentSerialNumber() {

    DWORD dwSize = 0;
    char buffer[1024];
    std::string serialNumber;


    HANDLE hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {

        return serialNumber;
    }


    DeviceIoControl(hDevice, MY_DEVICE_IOCTL, NULL, 0, &dwSize, sizeof(DWORD), NULL, NULL);


    char* lpBuffer = new char[dwSize];
    if (!lpBuffer) {
        // Could not allocate buffer
        CloseHandle(hDevice);
        return serialNumber;
    }

    DeviceIoControl(hDevice, MY_DEVICE_IOCTL, NULL, 0, lpBuffer, dwSize, &dwSize, NULL);


    serialNumber = lpBuffer;


    delete[] lpBuffer;


    CloseHandle(hDevice);


    return serialNumber;
}


bool SpoofSerialNumber(std::string serialNumber) {

    HANDLE hDevice;
    DWORD dwBytesRead;
    DWORD dwBytesWritten;
    char inputBuffer[1024];
    DWORD dwBytes;


    strcpy_s(inputBuffer, serialNumber.c_str());

    hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {

        return false;
    }


    DeviceIoControl(hDevice, MY_IOCTL_SPOOF, inputBuffer, serialNumber.length() + 1, NULL, 0, &dwBytes, (LPOVERLAPPED)NULL);


    CloseHandle(hDevice);


    return true;
}

int main(int argc, char* argv[]) {

    std::wstring deviceName = FindDeviceName();
    if (deviceName.empty()) {
        std::cerr << "Error: Could not find device name." << std::endl;
        return 1;
    }

  
    std::string currentSerial = GetCurrentSerialNumber();
    if (!currentSerial.empty()) {

        SpoofSerialNumber(currentSerial);


        std::cout << "Serial number spoofed successfully." << std::endl;
        std::cout << "Old HWID: " << currentSerial << std::endl;
        std::cout << "New HWID: " << GetCurrentSerialNumber() << std::endl;
    }
    return 0;
}
