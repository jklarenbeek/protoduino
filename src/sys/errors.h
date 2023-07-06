#ifndef __ERRORS_H__
#define __ERRORS_H__

#define ERR_SUCCESS              (0)
#define ERR_GENERAL              (4)
#define ERR_FRAME_ERROR          (ERR_GENERAL + 1)
#define ERR_DATA_OVERFLOW        (ERR_GENERAL + 2)
#define ERR_PARITY_ERROR         (ERR_GENERAL + 3)
#define ERR_INVALID_FUNCTION     (ERR_GENERAL + 4)
#define ERR_FILE_NOT_FOUND       (ERR_GENERAL + 5)
#define ERR_INVALID_HANDLE       (ERR_GENERAL + 6)
#define ERR_NOT_READY            (ERR_GENERAL + 21)
#define ERR_BAD_LENGTH           (ERR_GENERAL + 24)
#define ERR_NOT_SUPPORTED        (ERR_GENERAL + 50)
#define ERR_BUFFER_OVERFLOW      (ERR_GENERAL + 111)
#define ERR_NOT_IMPLEMENTED      (ERR_GENERAL + 119)
#define ERR_CALL_NOT_IMPLEMENTED (ERR_GENERAL + 120)
#define ERR_BAD_ARGUMENTS        (ERR_GENERAL + 160)

#endif
