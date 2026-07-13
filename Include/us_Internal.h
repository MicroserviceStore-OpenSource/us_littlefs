#ifndef __US_INTERNAL_H
#define __US_INTERNAL_H

#include "uService.h"

#include "us-littlefs.h"

#include "littlefs/lfs.h"

/*
 * Operations supported by this Microservice.
 */
typedef enum
{
    usOp_format = 0,
    usOp_mount,
    usOp_unmount,
    usOp_file_open,
    usOp_file_write,
    usOp_file_read,
    usOp_file_close,
    usOp_remove
} usOperations;

/*
 * Request Package
 */
typedef struct
{
    uServicePackageHeader header;

    union
    {
        /* usOp_format    : no input */
        /* usOp_mount     : no input */
        /* usOp_unmount   : no input */

        struct
        {
            char path[US_LITTLEFS_MAX_PATH];
            int flags;
        } file_open;

        struct
        {
            uint8_t buffer[US_LITTLEFS_MAX_DATA];
            uint32_t size;
        } file_write;

        struct
        {
            uint32_t size;
        } file_read;

        /* usOp_file_close : no input */

        struct
        {
            char path[US_LITTLEFS_MAX_PATH];
        } remove;
    } payload;
} usRequestPackage;

/*
 * Response Package
 */
typedef struct
{
    uServicePackageHeader header;

    union
    {
        struct
        {
            int result;
        } format;

        struct
        {
            int result;
        } mount;

        struct
        {
            int result;
        } unmount;

        struct
        {
            int result;
        } file_open;

        struct
        {
            int result;
        } file_write;

        struct
        {
            int result;
            uint8_t buffer[US_LITTLEFS_MAX_DATA];
        } file_read;

        struct
        {
            int result;
        } file_close;

        struct
        {
            int result;
        } remove;
    } payload;
} usResponsePackage;

#endif /* __US_INTERNAL_H */