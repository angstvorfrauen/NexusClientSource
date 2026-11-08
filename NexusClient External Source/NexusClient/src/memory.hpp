#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <winternl.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

class MemoryManager {
private:
    DWORD processId = 0;
    HANDLE hProcess = nullptr;

    using NtReadVirtualMemoryFn = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    using NtWriteVirtualMemoryFn = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    NtReadVirtualMemoryFn pNtReadVirtualMemory = nullptr;
    NtWriteVirtualMemoryFn pNtWriteVirtualMemory = nullptr;

    void InitSyscalls() {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll) {
            pNtReadVirtualMemory = (NtReadVirtualMemoryFn)GetProcAddress(hNtdll, "NtReadVirtualMemory");
            pNtWriteVirtualMemory = (NtWriteVirtualMemoryFn)GetProcAddress(hNtdll, "NtWriteVirtualMemory");
        }
    }

public:
    MemoryManager() { InitSyscalls(); }
    ~MemoryManager() { CleanUp(); }
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    bool IsAttached() const { return hProcess != nullptr; }

    bool ReadBuffer(uintptr_t address, void* buffer, size_t size) {
        if (!hProcess || !pNtReadVirtualMemory || address == 0) return false;
        SIZE_T bytesRead = 0;
        NTSTATUS status = pNtReadVirtualMemory(hProcess, (PVOID)address, buffer, size, &bytesRead);
        return NT_SUCCESS(status) && bytesRead == size;
    }

    bool Attach(const std::wstring& processName) {
        CleanUp();
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (processName == pe.szExeFile) {
                    processId = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
        if (!processId) return false;
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
        return hProcess != nullptr;
    }

    uintptr_t GetModuleBaseAddress(const std::wstring& moduleName) {
        if (!processId) return 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32W me = { sizeof(MODULEENTRY32W) };
        uintptr_t base = 0;
        if (Module32FirstW(hSnapshot, &me)) {
            do {
                if (moduleName == me.szModule) {
                    base = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnapshot, &me));
        }
        CloseHandle(hSnapshot);
        return base;
    }

    template <typename T>
    T Read(uintptr_t address) {
        T buffer{};
        if (!ReadBuffer(address, &buffer, sizeof(T))) return buffer;
        return buffer;
    }

    template <typename T>
    bool Write(uintptr_t address, const T& value) {
        if (!hProcess || !pNtWriteVirtualMemory || address == 0) return false;
        SIZE_T bytesWritten = 0;
        NTSTATUS status = pNtWriteVirtualMemory(hProcess, (PVOID)address, (PVOID)&value, sizeof(T), &bytesWritten);
        return NT_SUCCESS(status) && bytesWritten == sizeof(T);
    }

    uintptr_t ResolvePointerChain(uintptr_t base, const std::vector<unsigned int>& offsets) {
        uintptr_t cur = base;
        for (size_t i = 0; i < offsets.size(); ++i) {
            cur = Read<uintptr_t>(cur);
            cur += offsets[i];
        }
        return cur;
    }

    void CleanUp() {
        if (hProcess) { CloseHandle(hProcess); hProcess = nullptr; }
        processId = 0;
    }
    bool AttachCurrent() {
        CleanUp();
        hProcess = GetCurrentProcess();
        return true;
    }
};