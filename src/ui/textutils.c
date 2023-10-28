#include "textutils.h"

// THIS WAS TAKEN FROM TIny_Hacker with permission!
unsigned int spaceSearch(const char *string, const unsigned int start) {
    for (int k = start; k >= 0; k--) {
        if (string[k] == ' ') {
            return k + 1;
        }
    }

    return start;
}
