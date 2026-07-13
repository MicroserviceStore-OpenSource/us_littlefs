#define LOG_ERROR_ENABLED   1
#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED    1
#include "uService.h"

#include "us-littlefs.h"

#include "us_Internal.h"

#ifndef CFG_US_MAX_NUM_OF_SESSION
#define CFG_US_MAX_NUM_OF_SESSION   1       /* Let us allow one session at a time */
#endif

/*
 * Block device geometry. These are aligned with the littlefs configuration
 * and the eMR NV Storage geometry. The actual block count is derived at
 * run-time from the NV Storage size.
 */
#ifndef CFG_US_LFS_READ_SIZE
#define CFG_US_LFS_READ_SIZE        16
#endif

#ifndef CFG_US_LFS_PROG_SIZE
#define CFG_US_LFS_PROG_SIZE        16
#endif

#ifndef CFG_US_LFS_BLOCK_SIZE
#define CFG_US_LFS_BLOCK_SIZE       4096
#endif

#ifndef CFG_US_LFS_CACHE_SIZE
#define CFG_US_LFS_CACHE_SIZE       16
#endif

#ifndef CFG_US_LFS_LOOKAHEAD_SIZE
#define CFG_US_LFS_LOOKAHEAD_SIZE   16
#endif

#ifndef CFG_US_LFS_BLOCK_CYCLES
#define CFG_US_LFS_BLOCK_CYCLES     500
#endif

/*
 * NV Storage APIs provided by the eMR ("NV Storage" Universal Resource).
 */
extern SysStatus nvmem_getsize(uint32_t* storageSize);
extern SysStatus nvmem_read(uint32_t offset, uint32_t length, uint8_t* buffer);
extern SysStatus nvmem_write(uint32_t offset, uint32_t length, uint8_t* buffer);
extern SysStatus nvmem_erase(uint32_t offset, uint32_t length);
extern SysStatus nvmem_clear(void);

typedef struct
{
    struct
    {
        uint32_t mounted    : 1;
        uint32_t fileOpen   : 1;
        uint32_t sessionOwned : 1;
    } flags;

    uint8_t ownerID;

    lfs_t lfs;
    lfs_file_t file;

    struct lfs_config cfg;

    uint8_t readBuffer[CFG_US_LFS_CACHE_SIZE];
    uint8_t progBuffer[CFG_US_LFS_CACHE_SIZE];
    uint8_t lookaheadBuffer[CFG_US_LFS_LOOKAHEAD_SIZE];
    uint8_t fileBuffer[CFG_US_LFS_CACHE_SIZE];

    struct lfs_file_config fileCfg;
} usServiceContext;

PRIVATE void startService(void);
PRIVATE void processRequest(uint8_t senderID, usRequestPackage* request);
PRIVATE void sendError(uint8_t receiverID, uint16_t operation, uint8_t status);

PRIVATE int bd_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size);
PRIVATE int bd_prog(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size);
PRIVATE int bd_erase(const struct lfs_config* c, lfs_block_t block);
PRIVATE int bd_sync(const struct lfs_config* c);
PRIVATE void initConfig(void);

PRIVATE usServiceContext ctx;

int main()
{
    SysStatus retVal;

    uService_PrintIntro();

    SYS_INITIALISE_IPC_MESSAGEBOX(retVal, CFG_US_MAX_NUM_OF_SESSION);
    if (retVal != SysStatus_Success)
    {
        LOG_ERROR("Failed to initialise MessageBox. Error : %d", retVal);
    }
    else
    {
        initConfig();
        startService();
    }

    LOG_ERROR("Exiting the Microservice...");
    Sys_Exit();
}

PRIVATE int bd_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    uint32_t address = ((uint32_t)block * c->block_size) + (uint32_t)off;
    SysStatus retVal = nvmem_read(address, (uint32_t)size, (uint8_t*)buffer);
    if (retVal != SysStatus_Success)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

PRIVATE int bd_prog(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    uint32_t address = ((uint32_t)block * c->block_size) + (uint32_t)off;
    SysStatus retVal = nvmem_write(address, (uint32_t)size, (uint8_t*)buffer);
    if (retVal != SysStatus_Success)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

PRIVATE int bd_erase(const struct lfs_config* c, lfs_block_t block)
{
    uint32_t address = (uint32_t)block * c->block_size;
    SysStatus retVal = nvmem_erase(address, (uint32_t)c->block_size);
    if (retVal != SysStatus_Success)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

PRIVATE int bd_sync(const struct lfs_config* c)
{
    (void)c;
    return LFS_ERR_OK;
}

PRIVATE void initConfig(void)
{
    uint32_t storageSize = 0;
    uint32_t blockCount;

    (void)nvmem_getsize(&storageSize);

    if (storageSize == 0)
    {
        blockCount = 0;
    }
    else
    {
        blockCount = storageSize / CFG_US_LFS_BLOCK_SIZE;
    }

    ctx.cfg.context = (void*)0;

    ctx.cfg.read  = bd_read;
    ctx.cfg.prog  = bd_prog;
    ctx.cfg.erase = bd_erase;
    ctx.cfg.sync  = bd_sync;

    ctx.cfg.read_size      = CFG_US_LFS_READ_SIZE;
    ctx.cfg.prog_size      = CFG_US_LFS_PROG_SIZE;
    ctx.cfg.block_size     = CFG_US_LFS_BLOCK_SIZE;
    ctx.cfg.block_count    = blockCount;
    ctx.cfg.block_cycles   = CFG_US_LFS_BLOCK_CYCLES;
    ctx.cfg.cache_size     = CFG_US_LFS_CACHE_SIZE;
    ctx.cfg.lookahead_size = CFG_US_LFS_LOOKAHEAD_SIZE;

    ctx.cfg.read_buffer      = ctx.readBuffer;
    ctx.cfg.prog_buffer      = ctx.progBuffer;
    ctx.cfg.lookahead_buffer = ctx.lookaheadBuffer;

    ctx.cfg.name_max = 0;
    ctx.cfg.file_max = 0;
    ctx.cfg.attr_max = 0;
    ctx.cfg.metadata_max = 0;

    ctx.fileCfg.buffer = ctx.fileBuffer;
    ctx.fileCfg.attrs = (struct lfs_attr*)0;
    ctx.fileCfg.attr_count = 0;
}

PRIVATE void startService(void)
{
    usRequestPackage request;

    uint32_t sequenceNo; (void)sequenceNo;
    usStatus responseStatus;
    uint8_t senderID = 0xFF;

    while (true)
    {
        bool dataReceived = false;
        uint32_t receivedLen = 0;
        responseStatus = usStatus_Success;

        (void)Sys_IsMessageReceived(&dataReceived, &receivedLen, &sequenceNo);
        if (!dataReceived || receivedLen == 0)
        {
            /* Sleep until receive an IPC message */
            Sys_WaitForEvent(SysEvent_IPCMessage, 0);

            continue;
        }

        if (receivedLen <= USERVICE_PACKAGE_HEADER_SIZE)
        {
            responseStatus = usStatus_InvalidParam_UnsufficientSize;
            LOG_PRINTF(" > Unsufficint Mandatory Received Length (%d)/(%d)",
                receivedLen, USERVICE_PACKAGE_HEADER_SIZE);
        }

        /* Get the message */
        (void)Sys_ReceiveMessage(&senderID, (uint8_t*)&request, receivedLen, &sequenceNo);

        /* Do not process the message if there was an error */
        if (responseStatus != usStatus_Success)
        {
            sendError(senderID, request.header.operation, responseStatus);
            continue;
        }

        /* Process the request */
        processRequest(senderID, &request);
    }
}

PRIVATE void processRequest(uint8_t senderID, usRequestPackage* request)
{
    SysStatus retVal = SysStatus_Success; (void)retVal;
    usResponsePackage response;
    uint32_t sequenceNo;

    response.header = request->header;

    /*
     * Session ownership check. Only the owner may operate on the mounted
     * filesystem. The session is acquired at mount and released at unmount.
     */
    if (ctx.flags.sessionOwned && request->header.operation != usOp_mount)
    {
        if (senderID != ctx.ownerID)
        {
            sendError(senderID, request->header.operation, usStatus_InvalidSession);
            return;
        }
    }

    switch (request->header.operation)
    {
        case usOp_format:
        {
            int result = lfs_format(&ctx.lfs, &ctx.cfg);
            response.payload.format.result = result;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_mount:
        {
            int result;

            if (ctx.flags.sessionOwned && senderID != ctx.ownerID)
            {
                sendError(senderID, request->header.operation, usStatus_NoSessionSlotAvailable);
                return;
            }

            result = lfs_mount(&ctx.lfs, &ctx.cfg);
            response.payload.mount.result = result;

            if (result == LFS_ERR_OK)
            {
                ctx.flags.mounted = 1;
                ctx.flags.sessionOwned = 1;
                ctx.ownerID = senderID;
            }

            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_unmount:
        {
            int result;

            if (!ctx.flags.mounted)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            result = lfs_unmount(&ctx.lfs);
            response.payload.unmount.result = result;

            if (result == LFS_ERR_OK)
            {
                ctx.flags.mounted = 0;
                ctx.flags.fileOpen = 0;
                ctx.flags.sessionOwned = 0;
                ctx.ownerID = 0xFF;
            }

            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_file_open:
        {
            int result;

            if (!ctx.flags.mounted)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            if (ctx.flags.fileOpen)
            {
                LOG_ERROR("A file is already open");
                sendError(senderID, request->header.operation, usStatus_NoSessionSlotAvailable);
                return;
            }

            request->payload.file_open.path[US_LITTLEFS_MAX_PATH - 1] = '\0';

            result = lfs_file_opencfg(&ctx.lfs, &ctx.file,
                request->payload.file_open.path,
                request->payload.file_open.flags, &ctx.fileCfg);

            response.payload.file_open.result = result;

            if (result == LFS_ERR_OK)
            {
                ctx.flags.fileOpen = 1;
            }

            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_file_write:
        {
            lfs_ssize_t result;

            if (!ctx.flags.mounted || !ctx.flags.fileOpen)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            if (request->payload.file_write.size > US_LITTLEFS_MAX_DATA)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidParam_SizeExceedAllowed);
                return;
            }

            result = lfs_file_write(&ctx.lfs, &ctx.file,
                request->payload.file_write.buffer,
                (lfs_size_t)request->payload.file_write.size);

            response.payload.file_write.result = (int)result;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_file_read:
        {
            lfs_ssize_t result;
            lfs_size_t size;

            if (!ctx.flags.mounted || !ctx.flags.fileOpen)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            if (request->payload.file_read.size > US_LITTLEFS_MAX_DATA)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidParam_SizeExceedAllowed);
                return;
            }

            size = (lfs_size_t)request->payload.file_read.size;

            result = lfs_file_read(&ctx.lfs, &ctx.file,
                response.payload.file_read.buffer, size);

            response.payload.file_read.result = (int)result;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_file_close:
        {
            int result;

            if (!ctx.flags.mounted || !ctx.flags.fileOpen)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            result = lfs_file_close(&ctx.lfs, &ctx.file);
            response.payload.file_close.result = result;

            if (result == LFS_ERR_OK)
            {
                ctx.flags.fileOpen = 0;
            }

            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        case usOp_remove:
        {
            int result;

            if (!ctx.flags.mounted)
            {
                sendError(senderID, request->header.operation, usStatus_InvalidOperation);
                return;
            }

            request->payload.remove.path[US_LITTLEFS_MAX_PATH - 1] = '\0';

            result = lfs_remove(&ctx.lfs, request->payload.remove.path);
            response.payload.remove.result = result;
            response.header.status = usStatus_Success;
            (void)Sys_SendMessage(senderID, (uint8_t*)&response, sizeof(usResponsePackage), &sequenceNo);
            break;
        }

        /* Unrecognised operation */
        default:
            sendError(senderID, response.header.operation, usStatus_InvalidOperation);
            break;
    }
}

PRIVATE void sendError(uint8_t receiverID, uint16_t operation, uint8_t status)
{
    uint32_t sequenceNo;
    (void)sequenceNo;
    usResponsePackage response =
    {
        .header.operation = operation,
        .header.status = status,
        .header.length = 0
    };

    (void)Sys_SendMessage(receiverID, (uint8_t*)&response, sizeof(response), &sequenceNo);
}