#include <stddef.h>
#include <stdint.h>

/*
 * Mock implementations of standard C library functions for use in the micro-AES library.
 * These functions are used to provide basic memory manipulation capabilities without relying on the standard library.
 */

void us_memcpy(void* destination, const void* source, size_t length)
{
    uint8_t* dst = (uint8_t*)destination;
    const uint8_t* src = (const uint8_t*)source;

    while (length--)
    {
        *dst++ = *src++;
    }
}

void us_memset(void* destination, uint8_t value, size_t length)
{
    uint8_t* dst = (uint8_t*)destination;

    while (length--)
    {
        *dst++ = value;
    }
}

int us_memcmp(const void* ptr1, const void* ptr2, size_t length)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    while (length--)
    {
        if (*p1 != *p2)
        {
            return (int)(*p1 - *p2);
        }
        ++p1;
        ++p2;
    }
    return 0;
}
