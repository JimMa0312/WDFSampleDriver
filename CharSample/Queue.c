/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, CharSampleQueueInitialize)
#endif

NTSTATUS
CharSampleQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    //初始化IO队列配置结构体为默认，设置IO请求分发处理方式为串行
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchSequential
        );

    //设置EvtIODeviceControl函数，处理应用程序的DeiceIoControl函数的调用
    queueConfig.EvtIoDeviceControl = CharSampleEvtIoDeviceControl;

    //根据队列配置，创建IO队列
    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
CharSampleEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    PVOID buffer;
    CHAR n, c[] = "零一二三四五六七八九";

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION,
        TRACE_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
        Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

    switch (IoControlCode)
    {
    case CharSample_IOCTL_800:
        //检查输入队列的有效性
        //如果输入队列的有效数据长度为0，或者输出缓存长度小于一个wchar长度，则判断为错误
        if (InputBufferLength == 0 || OutputBufferLength < 2)
        {
            //回馈请求为参数违法
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        }
        else
        {
            //获取输入缓冲区地址Buffer
            //申请1个字节
            status = WdfRequestRetrieveInputBuffer(Request, 1, &buffer, NULL);
            if (!NT_SUCCESS(status))
            {
                WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
                break;
            }

            //此时Buffer指针指向的是输入缓冲区地址
            n = *(CHAR*)buffer;
            if ((n >= '0') && (n <= '9'))
            {
                //如果为数字则处理为汉字

                n -= '0';

                status = WdfRequestRetrieveOutputBuffer(Request, 2, &buffer, NULL);

                if (!NT_SUCCESS(status))
                {
                    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
                    break;
                }

                //此时Buffer指针指向的是输出缓存区地址
                //输出：从中文数组c中取出对应的汉字数字，并拷贝到缓冲区
                strncpy((PCHAR)buffer, &c[n * 2], 2);

                //完成I/O请求，驱动程序传给应用的数据长度为2字节
                WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 2);
            }
            else
            {
                WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            }
        }
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        WdfRequestCompleteWithInformation(Request, status, 0);
        break;
    }

    return;
}

VOID
CharSampleEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}
