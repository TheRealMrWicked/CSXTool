#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <stdint.h>
#include "imagesection.h"
#include "talk.h"

static const char chkFlagOnShortHop[] = { 0x09, 0x00, 0x00, 0x00, 'C', 0x00, 'h', 0x00, 'k', 0x00, 'F', 0x00, 'l', 0x00, 'a', 0x00, 'g', 0x00, 'O', 0x00, 'n', 0x00 };
static const char chkSelectShortHop[] = { 0x09, 0x00, 0x00, 0x00, 'C', 0x00, 'h', 0x00, 'k', 0x00, 'S', 0x00, 'e', 0x00, 'l', 0x00, 'e', 0x00, 'c', 0x00, 't', 0x00 };
static const char onFlagShortHop[] = { 0x06, 0x00, 0x00, 0x00, 'O', 0x00, 'n', 0x00, 'F', 0x00, 'l', 0x00, 'a', 0x00, 'g', 0x00 };
static const char isGameClearShortHop[] = { 0x0b, 0x00, 0x00, 0x00, 'I', 0x00, 's', 0x00, 'G', 0x00, 'a', 0x00, 'm', 0x00, 'e', 0x00, 'C', 0x00, 'l', 0x00, 'e', 0x00, 'a', 0x00, 'r', 0x00 };
static const char isRecollectModeShortHop[] = { 0x0f, 0x00, 0x00, 0x00, 'I', 0x00, 's', 0x00, 'R', 0x00, 'e', 0x00, 'c', 0x00, 'o', 0x00, 'l', 0x00, 'l', 0x00, 'e', 0x00, 'c', 0x00, 't', 0x00, 'M', 0x00, 'o', 0x00, 'd', 0x00, 'e', 0x00 };
static const uint16_t jumpPrefixArg1[] = { 0x0601, 0x0007 };
static const uint16_t jumpPrefixArg2 = 0x0605;
static const char doubleSkipPrefix = 0x06;
static const char lineWidthCode[] = { 0x0F, 0xFF, 0x03, 0xFF, 0x01, 0x00, 0x01, 0x04, 0x03, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x75, 0x00, 0x6D, 0x00, 0x02, 0x00, 0x04 };

void ImageSection::extractStrings() {
    messages.clear();
    choices.clear();

    int messageIndex = 1;
    int choiceIndex = 1;
    for (size_t i = 0; i < size; ++i) {
        try {
            if (isTalkPrefix(content, size, i)) {
                Talk talk = parseTalk(content, size, i, position + 16);
                talk.index = messageIndex;
                ++messageIndex;
                messages.push_back(talk);
            } else {
                Choice choice;
                if (tryParseChoice(&choice, content, size, i, position + 16)) {
                    choice.index = choiceIndex;
                    choices.push_back(choice);
                    ++choiceIndex;
                }
            }
        } catch (std::runtime_error e) {
            std::cout << "Warning: Could not extract a talk. " << e.what() << "\n";
        }
    }
}

class ImageStringGroup {
public:
    ImageStringGroup(const Talk &talk): message(&talk), choice(0), special(0) {}
    ImageStringGroup(const Choice &ch): message(0), choice(&ch), special(0) {}
    ImageStringGroup(const Special &sp): message(0), choice(0), special(&sp) {}

    bool operator <(const ImageStringGroup &rhs) const {
        return pos() < rhs.pos();
    }

    size_t pos() const {
        if (message != 0) {
            return message->talkPos;
        } else if (choice != 0) {
            return choice->choicePos;
        } else {
            return special->pos;
        }
    }

    const Talk *message;
    const Choice *choice;
    const Special *special;
};

void ImageSection::substituteStrings(std::deque<Talk> newMessages, std::deque<Choice> newChoices, std::deque<Special> specials, bool ignoreExactLineBreaks) {
    for (size_t i = 0; i < specials.size(); ++i) {
        specials[i].pos -= contentStartByte;
    }

    posComp.reset();

    // creating mapping index -> message/choice
    std::map<int, const Talk *> newMessagesMap;
    for (size_t i = 0; i < newMessages.size(); ++i) {
        if (newMessagesMap.find(newMessages[i].index) != newMessagesMap.end()) {
            std::stringstream ss;
            ss << "Duplicate message index " << newMessages[i].index;
            throw std::runtime_error(ss.str());
        }
        newMessagesMap[newMessages[i].index] = &newMessages[i];
    }

    std::map<int, const Choice *> newChoicesMap;
    for (size_t i = 0; i < newChoices.size(); ++i) {
        if (newChoicesMap.find(newChoices[i].index) != newChoicesMap.end()) {
            std::stringstream ss;
            ss << "Duplicate choice index: " << newChoices[i].index;
            throw std::runtime_error(ss.str());
        }
        newChoicesMap[newChoices[i].index] = &newChoices[i];
    }

    // initializing queue of messages and choices in order of appearance in image content
    std::deque<ImageStringGroup> origStringsQueue;
    for (size_t i = 0; i < messages.size(); ++i) {
        origStringsQueue.push_back(ImageStringGroup(messages[i]));
    }
    for (size_t i = 0; i < choices.size(); ++i) {
        origStringsQueue.push_back(ImageStringGroup(choices[i]));
    }
    for (size_t i = 0; i < specials.size(); ++i) {
        if (specials[i].pos >= size) {
            throw std::runtime_error("Invalid special position.");
        }
        origStringsQueue.push_back(ImageStringGroup(specials[i]));
    }
    std::sort(origStringsQueue.begin(), origStringsQueue.end());

    // estimating new size of content
    size_t totalNewStringsSize = 0;
    for (size_t i = 0; i < newMessages.size(); ++i) {
        totalNewStringsSize += newMessages[i].speaker.byteSize() + newMessages[i].message.byteSize();
    }
    for (size_t i = 0; i < newChoices.size(); ++i) {
        totalNewStringsSize += newChoices[i].byteSize();
    }
    for (size_t i = 0; i < specials.size(); ++i) {
        totalNewStringsSize += specials[i].substitution.byteSize();
    }

    // rewriting content with new messages while keeping track of changes in positions
    char *newContent = new char[size * 2 + totalNewStringsSize];
    size_t pos = 0;
    size_t origPos = 0;

    for (size_t i = 0; i < origStringsQueue.size(); ++i) {
        const ImageStringGroup &origImageString = origStringsQueue[i];

        if (origImageString.pos() < origPos) {
            throw std::runtime_error("Not disjunctive data.");
        }

        if (origImageString.message != 0) {
            if (newMessagesMap.find(origImageString.message->index) == newMessagesMap.end()) {
                continue;
            }

            const Talk &origMessage = *origImageString.message;
            const Talk &message = *newMessagesMap[origMessage.index];

            if (origMessage.hasSpeaker()) {
                memcpy(newContent + pos, content + origPos, origMessage.speakerPos - origPos);
                pos += origMessage.speakerPos - origPos;
                message.speaker.writeToData(newContent + pos);
                pos += message.speaker.byteSize();
                origPos = origMessage.speakerPos + origMessage.speaker.byteSize();
                posComp.setPos(origPos, pos);
            }

            memcpy(newContent + pos, content + origPos, origMessage.talkPos - origPos);
            pos += origMessage.talkPos - origPos;
            origPos = origMessage.talkPos;
            pos += message.writeToData(newContent + pos, ignoreExactLineBreaks);
            origPos += origMessage.talkContentBytes;
            posComp.setPos(origPos, pos);
        } else if (origImageString.choice != 0) {
            if (newChoicesMap.find(origImageString.choice->index) == newChoicesMap.end()) {
                continue;
            }

            const Choice &origChoice = *origImageString.choice;
            const Choice &choice = *newChoicesMap[origChoice.index];

            memcpy(newContent + pos, content + origPos, origChoice.choicePos - origPos);
            pos += origChoice.choicePos - origPos;
            origPos = origChoice.choicePos;
            pos += choice.writeToData(newContent + pos);
            origPos += origChoice.byteSize();
            posComp.setPos(origPos, pos);
        } else {
            assert(origImageString.special != 0);

            const Special &special = *origImageString.special;
            size_t origByteSize = parseSpecialByteSize(content + special.pos, content + size, special.pos + position + 16);

            std::cerr << "Substituting special ";
            for (size_t i = 0; i < 16 && i < origByteSize; ++i) {
                std::cerr << std::hex << std::setw(2) << std::setfill('0') << (unsigned int) (unsigned char) (content[special.pos + i]) << " ";
            }
            if (origByteSize > 16) {
                std::cerr << "...";
            }
            std::cerr << "\n";

            memcpy(newContent + pos, content + origPos, special.pos - origPos);
            pos += special.pos - origPos;
            origPos = special.pos;
            pos += special.writeToData(newContent + pos);
            origPos += origByteSize;
            posComp.setPos(origPos, pos);
        }
    }

    memcpy(newContent + pos, content + origPos, size - origPos);
    pos += size - origPos;
    assert(posComp.position(size) == pos);

    delete[] content;
    content = newContent;
    size = pos;
}

void ImageSection::updateShortHops() {
    updatedJumpPos.clear();

    for (size_t i = 0; i < size; ++i) {
        Jump jump = detectJump(i);
        switch (jump.type) {
        case Jump::Jump1Arg:
            updateJump(jump.arg1Pos);
            break;

        case Jump::Jump2Arg:
            updateJump(jump.arg1Pos);
            updateJump(jump.arg2Pos);
            break;

        default:
            break;
        }
    }
}

void ImageSection::fixLineWidth() {
    for (size_t i = 0; i < size - sizeof(lineWidthCode) + 1; ++i) {
        bool foundLineWidthCode = true;

        for (size_t j = 0; j < sizeof(lineWidthCode); ++j) {
            if (content[i + j] != lineWidthCode[j]) {
                foundLineWidthCode = false;
                break;
            }
        }

        if (foundLineWidthCode) {
            content[i + sizeof(lineWidthCode)] = 0x38;
            std::cout << "Found the line width code at byte " << i << " of the content of the image section. Replacing the value with 0x38.\n";
            return;
        }
    }

    std::cout << "Warning: Could not find the line width code. It was not replaced with 0x38.\n";
}

void ImageSection::check() const {
    // parsing talks/choices without saving them
    for (size_t i = 0; i < size; ++i) {
        try {
            if (isTalkPrefix(content, size, i)) {
                parseTalk(content, size, i, position + 16);
            } else {
                Choice choice;
                tryParseChoice(&choice, content, size, i, position + 16);
            }
        } catch (std::runtime_error e) {
            std::cout << "Warning: Could not read a talk. " << e.what() << "\n";
        }
    }

    // parsing jumps
    for (size_t i = 0; i < size; ++i) {
        Jump jump = detectJump(i);
        std::deque<size_t> jumpPositions;
        switch (jump.type) {
        case Jump::Jump1Arg:
            jumpPositions.push_back(jump.arg1Pos);
            break;

        case Jump::Jump2Arg:
            jumpPositions.push_back(jump.arg1Pos);
            jumpPositions.push_back(jump.arg2Pos);
            break;

        default:
            break;
        }

        for (size_t i = 0; i < jumpPositions.size(); ++i) {
            uint32_t distance = *((uint32_t *)(content + jumpPositions[i]));
            if (jumpPositions[i] + distance + 5 > size) {
                std::stringstream msg;
                msg << "Short jump distance (0x" << std::hex << distance << ") out of bounds at byte 0x" << std::hex << position + 16 + jumpPositions[i] << ".";
                throw std::runtime_error(msg.str());
            }
        }
    }
}

ImageSection::Jump ImageSection::detectJump(size_t pos) const {
    Jump jump;
    jump.type = Jump::NoJump;

    size_t currPos = pos;
    bool try1Arg = false;
    bool try2Arg = false;

    if (currPos + 4 >= size || content[currPos + 1] != 0 || content[currPos + 2] != 0 || content[currPos + 3] != 0) {
        // small optimization
        return jump;
    } else if (currPos + sizeof(chkFlagOnShortHop) + 6 <= size && memcmp(content + currPos, chkFlagOnShortHop, sizeof(chkFlagOnShortHop)) == 0) {
        currPos += sizeof(chkFlagOnShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(chkSelectShortHop) + 6 <= size && memcmp(content + currPos, chkSelectShortHop, sizeof(chkSelectShortHop)) == 0) {
        currPos += sizeof(chkSelectShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(onFlagShortHop) + 6 <= size && memcmp(content + currPos, onFlagShortHop, sizeof(onFlagShortHop)) == 0) {
        currPos += sizeof(onFlagShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(isGameClearShortHop) + 6 <= size && memcmp(content + currPos, isGameClearShortHop, sizeof(isGameClearShortHop)) == 0) {
        currPos += sizeof(isGameClearShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(isRecollectModeShortHop) + 12 <= size && memcmp(content + currPos, isRecollectModeShortHop, sizeof(isRecollectModeShortHop)) == 0) {
        currPos += sizeof(isRecollectModeShortHop);
        try1Arg = true;
        try2Arg = true;
    }

    if (try1Arg) {
        uint16_t prefix = *((uint16_t *)(content + currPos));
        currPos += 2;
        for (size_t i = 0; i < sizeof(jumpPrefixArg1) / sizeof(jumpPrefixArg1[0]); ++i) {
            if (prefix == jumpPrefixArg1[i]) {
                jump.type = Jump::Jump1Arg;
                jump.arg1Pos = currPos;
                currPos += 4;
                break;
            }
        }

        if (try2Arg && jump.type == Jump::Jump1Arg) {
            uint16_t prefix = *((uint16_t *)(content + currPos));
            currPos += 2;
            if (prefix == jumpPrefixArg2) {
                jump.type = Jump::Jump2Arg;
                jump.arg2Pos = currPos;
            }
        }
    }

    return jump;
}

void ImageSection::updateJump(size_t pos) {
    // checking if the jump wasn't updated already
    if (updatedJumpPos.find(pos) != updatedJumpPos.end()) {
        return;
    }

    updatedJumpPos.insert(pos);

    uint32_t distance = *((uint32_t *)(content + pos));
    uint32_t delta = posComp.delta(pos + 4, distance);
    if (pos + distance + 5 > size) {
        std::stringstream msg;
        msg << "Original short jump distance (0x" << std::hex << distance << ") out of bounds.";
        throw std::runtime_error(msg.str());
    }
    //std::cerr << "updating jump on " << std::hex << (pos + 80) << std::dec << " " << distance << " -> " << delta << "\n";
    *((uint32_t *)(content + pos)) = delta;
    if (pos + delta + 5 > size) {
        throw std::runtime_error("Short jump distance out of bounds.");
    }

    if (content[pos + delta + 4] == doubleSkipPrefix) {
        updateJump(pos + delta + 5);
    } else {
        if ((content[pos + delta + 4] != 2 || content[pos + delta + 5] != 0 || content[pos + delta + 6] != 6) &&
                (content[pos + delta + 4] != 2 || content[pos + delta + 5] != 0 || content[pos + delta + 6] != 4) &&
                (content[pos + delta + 4] != 8 || content[pos + delta + 5] != 5 || content[pos + delta + 6] != 0) &&
                (content[pos + delta + 4] != 9 || content[pos + delta + 5] != 0 || content[pos + delta + 6] != 2) &&
                (content[pos + delta + 4] != 4 || content[pos + delta + 5] != 0 || content[pos + delta + 6] != 0)) {
            std::cerr << "Warning: unusual data after short jump.\n";
        }
    }
}
