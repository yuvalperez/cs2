#pragma once
#ifndef MEM_H
#define MEM_H

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>

namespace driver {
    namespace codes {
        constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2455, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2456, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2457, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    }

    struct Request {
        HANDLE process_id;
        PVOID target;
        PVOID buffer;
        SIZE_T size;
        SIZE_T returnSize;
        ULONG mouse_event_flags;
    };

    // Encrypted communication function
    inline void EncryptBuffer(PVOID buffer, SIZE_T size) {
        // Simple XOR encryption example (use a stronger method in production)
        BYTE key = 0xAA;
        for (SIZE_T i = 0; i < size; ++i) {
            ((BYTE*)buffer)[i] ^= key;
        }
    }

    inline bool attach_to_process(HANDLE driver_handle, DWORD pid) {
        Request r;
        r.process_id = reinterpret_cast<HANDLE>(pid);
        EncryptBuffer(&r, sizeof(r));  // Encrypt request
        bool result = DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        EncryptBuffer(&r, sizeof(r));  // Decrypt response
        return result;
    }

    template <class T>
    T read_memory(HANDLE driver_handle, uintptr_t addr) {
        T temp = {};
        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = &temp;
        r.size = sizeof(T);
        EncryptBuffer(&r, sizeof(r));  // Encrypt request
        DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        EncryptBuffer(&r, sizeof(r));  // Decrypt response
        return temp;
    }

    template <class T>
    void write_memory(HANDLE driver_handle, uintptr_t addr, const T& value) {
        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = const_cast<T*>(&value);
        r.size = sizeof(T);
        EncryptBuffer(&r, sizeof(r));  // Encrypt request
        DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        EncryptBuffer(&r, sizeof(r));  // Decrypt response
    }
}

class AdvancedMemoryTool {
private:
    DWORD processId;
    HANDLE driverHandle;

    DWORD GetProcessIdByName(const wchar_t* processName) const {
        DWORD pid = 0;
        bool found = false;

        while (!found) {
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to create snapshot" << std::endl;
                return 0;
            }

            PROCESSENTRY32W entry;
            entry.dwSize = sizeof(entry);

            if (Process32FirstW(snapshot, &entry)) {
                do {
                    if (!_wcsicmp(entry.szExeFile, processName)) {
                        pid = entry.th32ProcessID;
                        std::wcout << L"Found process: " << entry.szExeFile << L" with PID: " << entry.th32ProcessID << std::endl;
                        found = true;
                        break;
                    }
                } while (Process32NextW(snapshot, &entry));
            }

            CloseHandle(snapshot);
        }
        return pid;
    }

public:
    explicit AdvancedMemoryTool(const wchar_t* processName) {
        processId = GetProcessIdByName(processName);
        driverHandle = CreateFile("\\\\.\\G13Driver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (driverHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open driver" << std::endl;
            std::cin.get();
            exit(1);
        }

        if (!driver::attach_to_process(driverHandle, processId)) {
            std::cerr << "Failed to attach to process" << std::endl;
            std::cin.get();
            exit(1);
        }
    }

    DWORD GetProcessId() const {
        return processId;
    }

    uintptr_t GetModuleBaseAddress(const wchar_t* moduleName) const {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        MODULEENTRY32W entry;
        entry.dwSize = sizeof(entry);

        if (Module32FirstW(snapshot, &entry)) {
            do {
                if (!_wcsicmp(entry.szModule, moduleName)) {
                    CloseHandle(snapshot);
                    return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                }
            } while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        return 0;
    }

    template<typename T>
    T ReadMemory(uintptr_t address) const {
        return driver::read_memory<T>(driverHandle, address);
    }

    bool ReadString(uintptr_t address, char* buffer, size_t size) const {
        driver::Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = buffer;
        r.size = size;
        driver::EncryptBuffer(&r, sizeof(r));  // Encrypt request
        bool result = DeviceIoControl(driverHandle, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        driver::EncryptBuffer(&r, sizeof(r));  // Decrypt response
        return result;
    }

    std::string ReadString(uintptr_t address, size_t maxLength = 128) const {
        std::vector<char> buffer(maxLength, '\0');
        if (ReadString(address, buffer.data(), maxLength)) {
            return std::string(buffer.data());
        }
        return "";
    }

    template<typename T>
    bool WriteMemory(uintptr_t address, const T& value) const {
        driver::write_memory<T>(driverHandle, address, value);
        return true;
    }

    uintptr_t FindDMAAddy(uintptr_t ptr, const std::vector<unsigned int>& offsets) const {
        uintptr_t addr = ptr;
        for (unsigned int offset : offsets) {
            addr = ReadMemory<uintptr_t>(addr);
            addr += offset;
        }
        return addr;
    }

    ~AdvancedMemoryTool() {
        if (driverHandle) {
            CloseHandle(driverHandle);
        }
    }
};

#endif // MEM_H