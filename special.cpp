#include "special.h"

size_t Special::writeToData(char *data) const {
    substitution.writeToData(data);
    return substitution.byteSize();
}

size_t parseSpecialByteSize(const char *source, const char *sourceEnd, uint32_t sourcePosition) {
    return parseUnicodeString(source, sourceEnd, sourcePosition).byteSize();
}
