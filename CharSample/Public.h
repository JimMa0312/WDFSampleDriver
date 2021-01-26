/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#include <initguid.h>

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_CharSample,
    0x6d04c778,0xe912,0x463b,0xb6,0x36,0x4e,0x22,0x13,0x4f,0x25,0xdd);
// {6d04c778-e912-463b-b636-4e22134f25dd}

#define CharSample_IOCTL_800 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)