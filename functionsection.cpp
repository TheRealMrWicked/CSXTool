#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "functionsection.h"

void FunctionSection::updateOffsets(const PositionComputer &posCom) {
    char *curr = content;
    const char *sectionEnd = content + size;

    if (size < 4) {
        throw std::runtime_error("Corrupted function section (size too small).");
    }

    uint32_t offsetsCount1 = *((uint32_t *)curr);
    curr += 4;

    if (size < 16 + offsetsCount1 * 4) {
        throw std::runtime_error("Corrupted function section (size too small).");
    }

    for (uint32_t i = 0; i < offsetsCount1; ++i) {
        uint32_t offset = *((uint32_t *)curr);
        *((uint32_t *)curr) = posCom.position(offset);
        curr += 4;
    }

    curr += 4;
    uint32_t offsetsCount2 = *((uint32_t *)curr);
    curr += 4;

    for (uint32_t i = 0; i < offsetsCount2; ++i) {
        if (curr + 8 >= sectionEnd) {
            std::stringstream msg;
            msg << "Corrupted function section (unexpected end of record at byte 0x" << std::hex << position + 16 + (uint32_t)(curr - content) << ").";
            throw std::runtime_error(msg.str());
        }
        uint32_t offset = *((uint32_t *)curr);
        *((uint32_t *)curr) = posCom.position(offset);
        curr += 4;
        uint32_t nameLen = *((uint32_t *)curr);
        curr += 4 + nameLen * 2;
        if (curr > sectionEnd) {
            std::stringstream msg;
            msg << "Corrupted function section (unexpected end of record at byte 0x" << std::hex << position + 16 + (uint32_t)(curr - content) << ").";
            throw std::runtime_error(msg.str());
        }
    }
}
