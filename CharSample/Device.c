/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, CharSampleCreateDevice)
#endif

NTSTATUS
CharSampleCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    //�����豸��û�ж������Ժ��豸����Ļ��������ṹ
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);

    if (NT_SUCCESS(status)) {
        
        //�����豸GUID�ӿ�
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_CharSample,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            //��ʼ��I/O���Ͷ���
            status = CharSampleQueueInitialize(device);
        }
    }

    return status;
}
