#ifndef POWER_LOSS_TEST_H
#define POWER_LOSS_TEST_H

#include <asm-generic/ioctl.h>

typedef struct
{
    char a[32];
}CAHR_32_ARRAY;

#define PRINT_REBOOT_TIMES            _IOW('V', 1, int)
#define PRINT_DATA_COMPARE_ERR        _IOW('V', 2, CAHR_32_ARRAY)
#define PRINT_FILE_OPERATION_ERR      _IOW('V', 3, int)
#define PRINT_GENERAL_INFO            _IOW('V', 4, int)

#endif /* end of POWER_LOSS_TEST_H */
