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
// This function extracts the current hardware serial number string
std::string GetCurrentSerialNumber() {
    // Declare variables
    DWORD dwSize = 0;
    char buffer[1024];
    std::string serialNumber;

    // Open the device handle to the device
    HANDLE hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        // Could not open device
        return serialNumber;
    }

    // Query for the size of the serial number buffer
    DeviceIoControl(hDevice, MY_DEVICE_IOCTL, NULL, 0, &dwSize, sizeof(DWORD), NULL, NULL);

    // Allocate a buffer of the required size
    char* lpBuffer = new char[dwSize];
    if (!lpBuffer) {
        // Could not allocate buffer
        CloseHandle(hDevice);
        return serialNumber;
    }

    // Query the serial number buffer
    DeviceIoControl(hDevice, MY_DEVICE_IOCTL, NULL, 0, lpBuffer, dwSize, &dwSize, NULL);

    // Get the serial number
    serialNumber = lpBuffer;

    // Free the buffer
    delete[] lpBuffer;

    // Close the device handle
    CloseHandle(hDevice);

    // Return the serial number
    return serialNumber;
}

// This function spoofs the serial number of the current hardware
bool SpoofSerialNumber(std::string serialNumber) {

    HANDLE hDevice;
    DWORD dwBytesRead;
    DWORD dwBytesWritten;
    char inputBuffer[1024];
    DWORD dwBytes;

    // Copy the serial number to the input buffer
    strcpy_s(inputBuffer, serialNumber.c_str());

    // Open the device handle to the device
    hDevice = CreateFile(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        // Could not open device
        return false;
    }

    // Execute the spoof control code
    DeviceIoControl(hDevice, MY_IOCTL_SPOOF, inputBuffer, serialNumber.length() + 1, NULL, 0, &dwBytes, (LPOVERLAPPED)NULL);

    // Close the device handle
    CloseHandle(hDevice);

    // Return success
    return true;
}

// Main entry point
int main(int argc, char* argv[]) {
    // Find the device name
    std::wstring deviceName = FindDeviceName();
    if (deviceName.empty()) {
        std::cerr << "Error: Could not find device name." << std::endl;
        return 1;
    }

    // Get the current serial number
    std::string currentSerial = GetCurrentSerialNumber();
    if (!currentSerial.empty()) {
        // Spoof the serial number
        SpoofSerialNumber(currentSerial);

        // Display success message
        std::cout << "Serial number spoofed successfully." << std::endl;
        std::cout << "Old HWID: " << currentSerial << std::endl;
        std::cout << "New HWID: " << GetCurrentSerialNumber() << std::endl;
    }
    return 0;
}
