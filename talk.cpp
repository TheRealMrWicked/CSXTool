#include <cstring>
#include <stdexcept>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "talk.h"

static const char talkMsg[] = { 0x08, 0x05, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 'T', 0x00, 'a', 0x00, 'l', 0x00, 'k', 0x00, 0x01, 0x02, 0x00, 0x06 };
static const char messMsg[] = { 0x08, 0x05, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 'M', 0x00, 'e', 0x00, 's', 0x00, 's', 0x00 };
static const char finalMessPrefix[] = { 0x01, 0x02, 0x03, 0x04 };
static const char blackoutFinalMessPrefix[] = { 0x01, 0x02, 0x00, 0x04 };
static const char specialFinalMessPrefix[] = { 0x01, 0x02, 0x00, 0x06, 0x08, 0x00, 0x00, 0x00, 'C', 0x00, 'F', 0x00 };
static const char contMessPrefix[] = { 0x01, 0x02, 0x00, 0x06 };
static const char afterName1[] = { 0x02, 0x00, 0x06 };
static const char afterName1WithNull[] = { 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00 };
static const char narratorName[] = { 0xC3, 0x5F, 0x6E, 0x30, 0xF0, 0x58 };
static const char space[] = { ' ', 0x00 };
static const char scenarioNewLine[] = { 0x0F, 0xFF };
static const size_t lineWidth = 2 * 29;

size_t Talk::writeToData(char *data, bool ignoreExactLineBreaks) const {
    size_t len = 0;
    memcpy(data, talkMsg, sizeof(talkMsg));
    len += sizeof(talkMsg);

    // word wrapping
    // keeping whole words unless there is a word longer than line width - in that case it will be alone
    UnicodeString spaceUs(space, sizeof(space) / 2);
    UnicodeString scenarioNewLineUs(scenarioNewLine, sizeof(scenarioNewLine) / 2);
    UnicodeString wordWrappedMessage;

    std::deque<UnicodeString> lines = message.split(scenarioNewLine, sizeof(scenarioNewLine) / 2);
    for (size_t l = 0; l < lines.size(); ++l) {
        if (l != 0) {
            wordWrappedMessage += scenarioNewLineUs;
        }

        std::deque<UnicodeString> words = lines[l].split(space, sizeof(space) / 2);
        size_t currentLineWidth = 0;
        for (size_t i = 0; i < words.size(); ++i) {
            if (i != 0) {
                if (ignoreExactLineBreaks && currentLineWidth + scenarioNewLineUs.width() == lineWidth) {
                    // a hack that relies on the software to word-wrap the line by simply cutting the line automatically after lineWidth characters
                    // this is needed because scenarioNewLineUs as the last character causes the software to make two line breaks in HNS
                    currentLineWidth = 0;
                } else if (currentLineWidth + spaceUs.width() + scenarioNewLineUs.width() + words[i].width() > lineWidth) {
                    wordWrappedMessage += scenarioNewLineUs;
                    currentLineWidth = 0;
                } else {
                    wordWrappedMessage += spaceUs;
                    currentLineWidth += spaceUs.width();
                }
            }

            wordWrappedMessage += words[i];
            currentLineWidth += words[i].width();
        }
    }

    wordWrappedMessage.writeToData(data + len);
    len += wordWrappedMessage.byteSize();

    return len;
}

bool isTalkPrefix(const char *contentBegin, size_t contentLength, size_t offset) {
    return offset + sizeof(talkMsg) + 6 < contentLength && memcmp(contentBegin + offset, talkMsg, sizeof(talkMsg)) == 0;
}

Talk parseTalk(const char *contentBegin, size_t contentLength, size_t offset, uint32_t sectionPosition) {
    assert(offset < contentLength);
    const char *contentEnd = contentBegin + contentLength;

    Talk talk;
    // parsing speaker name
    talk.speakerPos = 0;

    const char *name = 0;
    if (offset >= sizeof(afterName1WithNull) && memcmp(contentBegin + offset - sizeof(afterName1WithNull), afterName1WithNull, sizeof(afterName1WithNull)) == 0) {
        // Haruka and Kokoro no Koe
        name = searchBackwardsForShortUnicodeString(contentBegin, offset - sizeof(afterName1WithNull));
    } else {
        // The rest
        const char *unknownId = searchBackwardsForShortUnicodeString(contentBegin, offset);
        int unknownIdOffset = (unknownId - sizeof(afterName1)) - contentBegin;
        if (unknownId != 0 && unknownIdOffset >= 0 && memcmp(unknownId - sizeof(afterName1), afterName1, sizeof(afterName1)) == 0) {
            name = searchBackwardsForShortUnicodeString(contentBegin, unknownIdOffset);
        }
    }

    if (name == 0) {
        std::stringstream msg;
        msg << "No speaker in talk at byte 0x" << std::hex << offset + sectionPosition << ".";
        throw std::runtime_error(msg.str());
    } else {
        assert(name >= contentBegin);
        talk.speaker = parseUnicodeString(name, contentEnd, (uint32_t)(name - contentBegin) + sectionPosition);
        assert(name + talk.speaker.byteSize() <= contentEnd);
        UnicodeString narrator(narratorName, sizeof(narratorName) / 2);

        if (narrator == talk.speaker) {
            talk.speaker = UnicodeString();
        } else {
            talk.speakerPos = name - contentBegin;
        }
    }

    // parsing message content
    talk.talkPos = offset;
    const char *curr = contentBegin + offset;

    if (!isTalkPrefix(contentBegin, contentLength, offset)) {
        std::stringstream msg;
        msg << "Invalid talk prefix at byte 0x" << std::hex << offset + sectionPosition << ".";
        throw std::runtime_error(msg.str());
    }
    curr += sizeof(talkMsg);

    // parsing the first part of the talk (the "message")
    talk.message = parseUnicodeString(curr, contentEnd, (uint32_t)(curr - contentBegin) + sectionPosition);
    assert(curr + talk.message.byteSize() <= contentEnd);
    curr += talk.message.byteSize();

    if (talk.message.length > lineWidth) {
        std::cerr << "Warning: Mess longer than two lines at byte 0x" << std::hex << (uint32_t)(curr - talk.message.byteSize() - contentBegin) + sectionPosition << ".\n";
    }

    while (curr + sizeof(messMsg) < contentEnd && memcmp(curr, messMsg, sizeof(messMsg)) == 0) {
        curr += sizeof(messMsg);

        // end of message
        if ((curr + sizeof(finalMessPrefix) < contentEnd && memcmp(curr, finalMessPrefix, sizeof(finalMessPrefix)) == 0) ||
                (curr + sizeof(specialFinalMessPrefix) < contentEnd && memcmp(curr, specialFinalMessPrefix, sizeof(specialFinalMessPrefix)) == 0) ||
                (curr + sizeof(blackoutFinalMessPrefix) < contentEnd && memcmp(curr, blackoutFinalMessPrefix, sizeof(blackoutFinalMessPrefix)) == 0)) {
            talk.talkContentBytes = (curr - sizeof(messMsg)) - (contentBegin + offset);
            if (talk.message.length == 0) {
                std::cerr << "Warning: empty Mess in talk at byte 0x" << std::hex << (uint32_t)(curr - sizeof(messMsg) - talk.message.byteSize() - contentBegin) + sectionPosition << ".\n";
            }
            return talk;
        // continued message
        } else if (curr + sizeof(contMessPrefix) < contentEnd && memcmp(curr, contMessPrefix, sizeof(contMessPrefix)) == 0) {
            curr += sizeof(contMessPrefix);
            UnicodeString us = parseUnicodeString(curr, contentEnd, (uint32_t)(curr - contentBegin) + sectionPosition);
            curr += us.byteSize();
            talk.message += us;

            if (talk.message.length > lineWidth) {
                std::cerr << "Warning: Mess longer than two lines at byte 0x" << std::hex << (uint32_t)(curr - talk.message.byteSize() - contentBegin) + sectionPosition << ".\n";
            }
        } else {
            break;
        }
    }

    std::stringstream msg;
    msg << "No final mess in talk at byte 0x" << std::hex << offset + sectionPosition << ".";
    throw std::runtime_error(msg.str());
}
