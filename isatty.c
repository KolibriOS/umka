#include "isatty.h"

#ifdef _WIN32

int isatty(int fd) {
    // TODO: Make it work for win32
    return 0;
}

int fileno(FILE *fp) {
    // TODO: Make it work for win32
	return 0;
}

#endif // _WIN32
