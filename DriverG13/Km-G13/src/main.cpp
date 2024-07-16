#include <ntifs.h>

#ifndef BYTE
typedef unsigned char BYTE;
#endif

extern "C" {
    NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE INITIALIZEfunction);
    NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess,
        PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
}

void debug_print(PCSTR text) {
#ifdef DEBUG
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));
#else
    UNREFERENCED_PARAMETER(text);
#endif
}

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
    };

    // Simple XOR encryption (use a stronger method in production)
    void EncryptBuffer(PVOID buffer, SIZE_T size) {
        BYTE key = 0xAA;
        for (SIZE_T i = 0; i < size; ++i) {
            ((BYTE*)buffer)[i] ^= key;
        }
    }

    NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);
        debug_print("[+] device control called \n");

        NTSTATUS status = STATUS_UNSUCCESSFUL;
        PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

        auto request = reinterpret_cast<driver::Request*>(irp->AssociatedIrp.SystemBuffer);
        if (stack_irp == nullptr || request == nullptr) {
            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_INVALID_PARAMETER;
        }

        // Decrypt the request
        driver::EncryptBuffer(request, sizeof(driver::Request));

        static PEPROCESS target_process = nullptr;
        PEPROCESS current_process = PsGetCurrentProcess(); // Use PsGetCurrentProcess() to get the current process
        const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

        switch (control_code) {
        case driver::codes::attach:
            status = PsLookupProcessByProcessId(request->process_id, &target_process);
            if (NT_SUCCESS(status)) {
                debug_print("[+] Process attached successfully\n");
            }
            else {
                debug_print("[-] Failed to attach process\n");
            }
            break;
        case driver::codes::read:
            if (target_process != nullptr) {
                SIZE_T returnSize = 0;
                status = MmCopyVirtualMemory(target_process, request->target, current_process,
                    request->buffer, request->size, KernelMode, &returnSize);
                request->returnSize = returnSize;
                debug_print("[+] Memory read successfully\n");
            }
            else {
                status = STATUS_INVALID_HANDLE;
                debug_print("[-] Target process is null\n");
            }
            break;
        case driver::codes::write:
            if (target_process != nullptr) {
                SIZE_T returnSize = 0;
                status = MmCopyVirtualMemory(current_process, request->buffer, target_process,
                    request->target, request->size, KernelMode, &returnSize);
                request->returnSize = returnSize;
                debug_print("[+] Memory written successfully\n");
            }
            else {
                status = STATUS_INVALID_HANDLE;
                debug_print("[-] Target process is null\n");
            }
            break;
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            debug_print("[-] Invalid control code\n");
            break;
        }

        // Encrypt the response
        driver::EncryptBuffer(request, sizeof(driver::Request));

        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }
}

NTSTATUS Driver_main(PDRIVER_OBJECT driverObject, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    UNICODE_STRING device_Name = {};
    RtlInitUnicodeString(&device_Name, L"\\Device\\G13Driver");

    PDEVICE_OBJECT device_object = nullptr;
    NTSTATUS status = IoCreateDevice(driverObject, 0, &device_Name, FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
    if (status != STATUS_SUCCESS) {
        debug_print("[-] failed to create driver device. \n");
        return status;
    }
    debug_print("[+] driver device successfully created. \n");

    UNICODE_STRING symbolic_link = {};
    RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\G13Driver");

    status = IoCreateSymbolicLink(&symbolic_link, &device_Name);
    if (status != STATUS_SUCCESS) {
        debug_print("[-] Failed to establish symbolic link. \n");
        IoDeleteDevice(device_object);
        return status;
    }
    debug_print("[+] successfully established symbolic link. \n");

    SetFlag(device_object->Flags, DO_BUFFERED_IO);

    driverObject->MajorFunction[IRP_MJ_CREATE] = driver::create;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = driver::close;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

    ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);
    debug_print("[+] Driver initialized successfully. \n");

    return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);
    debug_print("[+] G13 kernel\n");

    UNICODE_STRING driver_Name = {};
    RtlInitUnicodeString(&driver_Name, L"\\Driver\\G13Driver");

    return IoCreateDriver(&driver_Name, Driver_main);
}