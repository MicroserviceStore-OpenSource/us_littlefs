/*
 * @file
 *
 * @brief Microservice API static library source file. This source file/library
 *        runs in the caller execution.
 *
 ******************************************************************************/

/********************************* INCLUDES ***********************************/

#include "us-littlefs.h"

#include "us_Internal.h"

#include "uService.h"

/***************************** MACRO DEFINITIONS ******************************/

/***************************** TYPE DEFINITIONS *******************************/
typedef struct
{
    struct
    {
        uint32_t initialised        : 1;
    } flags;

    /*
    * The "Execution Index" is a system wide enumaration by the Microservice Runtime
    * to interact with the Microservice.
    */
    uint32_t execIndex;
} uS_UserLibSettings;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
PRIVATE uS_UserLibSettings userLibSettings;

PRIVATE const char usUID[SYS_EXEC_NAME_MAX_LENGTH] = "LITTLEFS";

/***************************** PRIVATE FUNCTIONS *******************************/

/***************************** PUBLIC FUNCTIONS *******************************/
#define INITIALISE_FUNCTIONEXPAND(a, b, c) a##b##c
#define INITIALISE_FUNCTION(name) INITIALISE_FUNCTIONEXPAND(us_, name, _Initialise)
SysStatus INITIALISE_FUNCTION(USERVICE_NAME_NONSTR)(void)
{
    /* Get the Microservice Index to interact with the Microservice */
    return uService_Initialise(usUID, &userLibSettings.execIndex);
}

int us_littlefs_format(void)
{
    const uint32_t timeoutInMs = 60000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_format;
        request.header.length = sizeof(usRequestPackage);
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.format.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_mount(void)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_mount;
        request.header.length = sizeof(usRequestPackage);
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.mount.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_unmount(void)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_unmount;
        request.header.length = sizeof(usRequestPackage);
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.unmount.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_file_open(const char* path, int flags)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;
    uint32_t i;

    {
        request.header.operation = usOp_file_open;
        request.header.length = sizeof(usRequestPackage);

        for (i = 0; i < (US_LITTLEFS_MAX_PATH - 1); i++)
        {
            request.payload.file_open.path[i] = path[i];
            if (path[i] == '\0')
            {
                break;
            }
        }
        request.payload.file_open.path[US_LITTLEFS_MAX_PATH - 1] = '\0';

        request.payload.file_open.flags = flags;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.file_open.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_file_write(const void* buffer, uint32_t size)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;
    uint32_t i;
    const uint8_t* src = (const uint8_t*)buffer;

    if (size > US_LITTLEFS_MAX_DATA)
    {
        return LFS_ERR_INVAL;
    }

    {
        request.header.operation = usOp_file_write;
        request.header.length = sizeof(usRequestPackage);

        for (i = 0; i < size; i++)
        {
            request.payload.file_write.buffer[i] = src[i];
        }
        request.payload.file_write.size = size;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.file_write.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_file_read(void* buffer, uint32_t size)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;
    uint32_t i;
    uint8_t* dst = (uint8_t*)buffer;

    if (size > US_LITTLEFS_MAX_DATA)
    {
        return LFS_ERR_INVAL;
    }

    {
        request.header.operation = usOp_file_read;
        request.header.length = sizeof(usRequestPackage);

        request.payload.file_read.size = size;
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        int result = response.payload.file_read.result;

        if (result > 0)
        {
            uint32_t copyLen = (uint32_t)result;
            if (copyLen > size)
            {
                copyLen = size;
            }
            for (i = 0; i < copyLen; i++)
            {
                dst[i] = response.payload.file_read.buffer[i];
            }
        }

        return result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_file_close(void)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;

    {
        request.header.operation = usOp_file_close;
        request.header.length = sizeof(usRequestPackage);
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.file_close.result;
    }

    return LFS_ERR_IO;
}

int us_littlefs_remove(const char* path)
{
    const uint32_t timeoutInMs = 10000;
    SysStatus retVal;
    usResponsePackage response;
    usRequestPackage request;
    uint32_t i;

    {
        request.header.operation = usOp_remove;
        request.header.length = sizeof(usRequestPackage);

        for (i = 0; i < (US_LITTLEFS_MAX_PATH - 1); i++)
        {
            request.payload.remove.path[i] = path[i];
            if (path[i] == '\0')
            {
                break;
            }
        }
        request.payload.remove.path[US_LITTLEFS_MAX_PATH - 1] = '\0';
    };

    retVal = uService_RequestBlocker(userLibSettings.execIndex, (uServicePackage*)&request, (uServicePackage*)&response, timeoutInMs);

    if (retVal == SysStatus_Success && response.header.status == usStatus_Success)
    {
        return response.payload.remove.result;
    }

    return LFS_ERR_IO;
}
