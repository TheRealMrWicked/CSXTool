#ifndef SPECIAL_H
#define SPECIAL_H

#include <fstream>
#include "unicodestring.h"

class Special {
public:
    Special(): pos(0) {}

    size_t pos;
    UnicodeString substitution;

    size_t writeToData(char *data) const;
};

size_t parseSpecialByteSize(const char *source, const char *sourceEnd, uint32_t sourcePosition);

#endif // SPECIAL_H
