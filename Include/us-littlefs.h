#ifndef US_LITTLEFS_H
#define US_LITTLEFS_H

#include "uService.h"

/*
 * Default Status
 */
typedef enum
{
    usStatus_Success = 0,
    /* Operation not defined or the access not granted */
    usStatus_InvalidOperation,
    /* Timeout occurred during the opereration */
    usStatus_Timeout,
    /* Microservice does not have any available session */
    usStatus_NoSessionSlotAvailable,
    /* Request to an invalid session */
    usStatus_InvalidSession,
    /* Invalid Parameter - Insufficient Input or Output Size  */
    usStatus_InvalidParam_UnsufficientSize,
    /* Invalid Parameter - Input or Output exceeds the allowed capacity  */
    usStatus_InvalidParam_SizeExceedAllowed,
    /* The developer can defines custom statuses */
    usStatus_CustomStart = 32
} usStatus;

/*
 * ============================================================================
 *  us-littlefs Microservice - Public API
 * ============================================================================
 *
 *  This Microservice provides a filesystem service backed by the eMR NV
 *  Storage "Universal Resource". The "NV Storage" Resource MUST be assigned
 *  to this Microservice, otherwise the storage operations will fail.
 *
 *  Session model:
 *    - Only ONE session (caller) can use the filesystem at a time.
 *    - A caller must acquire the session with us_littlefs_open() and release
 *      it with us_littlefs_close(). While a session is held by one caller,
 *      requests from other callers are rejected.
 *
 *  Buffer/Path limitations (due to the IPC package limit of 244 bytes for
 *  the payload):
 *    - The maximum path length is US_LITTLEFS_MAX_PATH bytes
 *      (including the null terminator).
 *    - The maximum data chunk size per read/write call is
 *      US_LITTLEFS_MAX_DATA bytes. Larger transfers must be split by the
 *      caller into multiple calls.
 * ============================================================================
 */

/* Maximum path length (including the null terminator) accepted by the API */
#define US_LITTLEFS_MAX_PATH    64

/* Maximum data chunk (in bytes) transferred in a single read/write call */
#define US_LITTLEFS_MAX_DATA    200

/* File open flags (mirrors littlefs open flags) */
#define US_LITTLEFS_O_RDONLY    0x00000001
#define US_LITTLEFS_O_WRONLY    0x00000002
#define US_LITTLEFS_O_RDWR      0x00000003
#define US_LITTLEFS_O_CREAT     0x00000100
#define US_LITTLEFS_O_EXCL      0x00000200
#define US_LITTLEFS_O_TRUNC     0x00000400
#define US_LITTLEFS_O_APPEND    0x00000800

/*
 * Initialise the Microservice user library. Must be called once before any
 * other API call.
 */
SysStatus us_littlefs_Initialise(void);

/*
 * Format the underlying NV Storage with a littlefs filesystem.
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_format(void);

/*
 * Mount the filesystem and acquire the exclusive session for the calling
 * execution. If the filesystem is already mounted by another caller, this
 * returns an error.
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_mount(void);

/*
 * Unmount the filesystem and release the session.
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_unmount(void);

/*
 * Open a file. Only one file can be open at a time in this Microservice.
 *
 * @param path  Null terminated path (max US_LITTLEFS_MAX_PATH bytes).
 * @param flags Combination of US_LITTLEFS_O_* flags.
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_file_open(const char* path, int flags);

/*
 * Write a chunk of data to the currently open file.
 *
 * @param buffer Data to write.
 * @param size   Number of bytes to write (max US_LITTLEFS_MAX_DATA).
 *
 * @retval Number of bytes written on success, negative littlefs error code
 *         on failure.
 */
int us_littlefs_file_write(const void* buffer, uint32_t size);

/*
 * Read a chunk of data from the currently open file.
 *
 * @param buffer Buffer to read into.
 * @param size   Number of bytes to read (max US_LITTLEFS_MAX_DATA).
 *
 * @retval Number of bytes read on success, negative littlefs error code on
 *         failure.
 */
int us_littlefs_file_read(void* buffer, uint32_t size);

/*
 * Close the currently open file.
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_file_close(void);

/*
 * Remove a file or (empty) directory.
 *
 * @param path Null terminated path (max US_LITTLEFS_MAX_PATH bytes).
 *
 * @retval 0 on success, negative littlefs error code on failure.
 */
int us_littlefs_remove(const char* path);

#endif /* US_LITTLEFS_H */