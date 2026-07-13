/* Enable all the log levels */
#define LOG_INFO_ENABLED        1
#define LOG_WARNING_ENABLED     1
#define LOG_ERROR_ENABLED       1
#include "SysCall.h"

#include "us-littlefs.h"

int main(void)
{
    SysStatus retVal;
    bool testPass = true;

    LOG_PRINTF(" > Container : Microservice Test User App");

    SYS_INITIALISE_IPC_MESSAGEBOX(retVal, 4);

    retVal = us_littlefs_Initialise();
    if (retVal != SysStatus_Success)
    {
        if (retVal == SysStatus_NotFound)
        {
            LOG_PRINTF(" > us_littlefs does not exists.");
        }
        else
        {
            LOG_PRINTF(" > us_littlefs_Initialise failed with error %d. Exiting the User Application...", retVal);
        }
        Sys_Exit();
    }

    /* Test Cases */
    {
        int result;
        const char* testPath = "test.txt";
        const char writeData[] = "Hello littlefs Microservice!";
        uint8_t readData[64];
        uint32_t writeLen = (uint32_t)(sizeof(writeData) - 1);
        uint32_t i;

        /* Test 1: format */
        LOG_PRINTF(" > Test 1: format");
        result = us_littlefs_format();
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 2: mount */
        LOG_PRINTF(" > Test 2: mount");
        result = us_littlefs_mount();
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 3: open for write */
        LOG_PRINTF(" > Test 3: file_open (write/create/trunc)");
        result = us_littlefs_file_open(testPath,
            US_LITTLEFS_O_WRONLY | US_LITTLEFS_O_CREAT | US_LITTLEFS_O_TRUNC);
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 4: write */
        LOG_PRINTF(" > Test 4: file_write");
        result = us_littlefs_file_write(writeData, writeLen);
        if (result == (int)writeLen)
        {
            LOG_PRINTF("   PASS (expected %d, actual %d)", (int)writeLen, result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected %d, actual %d)", (int)writeLen, result);
            testPass = false;
        }

        /* Test 5: close */
        LOG_PRINTF(" > Test 5: file_close");
        result = us_littlefs_file_close();
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 6: open for read */
        LOG_PRINTF(" > Test 6: file_open (read)");
        result = us_littlefs_file_open(testPath, US_LITTLEFS_O_RDONLY);
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 7: read and verify */
        LOG_PRINTF(" > Test 7: file_read and verify content");
        for (i = 0; i < sizeof(readData); i++)
        {
            readData[i] = 0;
        }
        result = us_littlefs_file_read(readData, writeLen);
        if (result == (int)writeLen)
        {
            bool match = true;
            for (i = 0; i < writeLen; i++)
            {
                if (readData[i] != (uint8_t)writeData[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                LOG_PRINTF("   PASS (read %d bytes, content matches)", result);
            }
            else
            {
                LOG_PRINTF("   FAIL (content mismatch)");
                testPass = false;
            }
        }
        else
        {
            LOG_PRINTF("   FAIL (expected %d, actual %d)", (int)writeLen, result);
            testPass = false;
        }

        /* Test 8: close */
        LOG_PRINTF(" > Test 8: file_close");
        result = us_littlefs_file_close();
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 9: remove */
        LOG_PRINTF(" > Test 9: remove");
        result = us_littlefs_remove(testPath);
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }

        /* Test 10: unmount */
        LOG_PRINTF(" > Test 10: unmount");
        result = us_littlefs_unmount();
        if (result == 0)
        {
            LOG_PRINTF("   PASS (expected 0, actual %d)", result);
        }
        else
        {
            LOG_PRINTF("   FAIL (expected 0, actual %d)", result);
            testPass = false;
        }
    }

    LOG_PRINTF(" > usTest : %s", testPass ? "SUCCESS" : "FAIL");
    /* Exit the Container */
    Sys_Exit();

    return 0;
}