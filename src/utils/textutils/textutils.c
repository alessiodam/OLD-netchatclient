#include "textutils.h"
#include <string.h>

// THIS WAS TAKEN FROM TIny_Hacker with permission!
unsigned int spaceSearch(const char *string, const unsigned int start) {
    for (int k = start; k >= 0; k--) {
        if (string[k] == ' ') {
            return k + 1;
        }
    }

    return start;
}

bool StartsWith(const char *a, const char *b)
{
    if(strncmp(a, b, strlen(b)) == 0) return true;
    return false;
}
