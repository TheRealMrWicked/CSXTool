#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include "txtparser.h"

static const char *choiceTlPrefix = "[EN|CHOICE";
static const char *specialTlPrefix = "[EN|SPECIAL";
static const char *tlPrefix = "[EN";
static const char openingBracket[] = { '[', 0x00 };
static const char endingBracket[] = { ']', 0x00 };
static const char space[] = { ' ', 0x00 };
static const char utf8Bom[] = { 0xef, 0xbb, 0xbf };
static const char scenarioNewLine[] = { 0x0F, 0xFF };

TxtParser::Stats &TxtParser::Stats::operator +=(const TxtParser::Stats &rhs) {
    translatedMessages += rhs.translatedMessages;
    totalMessages += rhs.totalMessages;
    translatedChoices += rhs.translatedChoices;
    totalChoices += rhs.totalChoices;
    return *this;
}

double TxtParser::Stats::messagesPercent() const {
    if (totalMessages == 0) {
        return 100;
    } else {
        return 100 * (double)translatedMessages / (double)totalMessages;
    }
}

double TxtParser::Stats::choicesPercent() const {
    if (totalChoices == 0) {
        return 100;
    } else {
        return 100 * (double)translatedChoices / (double)totalChoices;
    }
}

void TxtParser::writeTxt(std::string utfFilename) const {
    std::ofstream fout(utfFilename.c_str(), std::ios_base::out | std::ios_base::binary);

    for (size_t i = 0; i < choices.size(); ++i) {
        const Choice &choice = choices[i];

        fout << "[JP|CHOICE" << std::setw(2) << std::setfill('0') << choice.index << "] ";
        printUtf8(fout, choice.choice1);
        fout << "\r\n";

        fout << "[JP|CHOICE" << std::setw(2) << std::setfill('0') << choice.index << "] ";
        printUtf8(fout, choice.choice2);
        fout << "\r\n";

        fout << choiceTlPrefix << std::setw(2) << std::setfill('0') << choice.index << "] \r\n";
        fout << choiceTlPrefix << std::setw(2) << std::setfill('0') << choice.index << "] \r\n";

        fout << "\r\n\r\n";
    }

    for (size_t i = 0; i < messages.size(); ++i) {
        const Talk &talk = messages[i];
        std::deque<UnicodeString> lines = talk.message.split(scenarioNewLine, sizeof(scenarioNewLine) / 2);

        for (size_t j = 0; j < lines.size(); ++j) {
            fout << "[JP" << std::setw(5) << std::setfill('0') << talk.index << "] ";

            if (j == 0 && talk.hasSpeaker()) {
                fout << "[";
                printUtf8(fout, talk.speaker);
                fout << "] ";
            }

            printUtf8(fout, lines[j]);
            fout << "\r\n";
        }

        for (size_t j = 0; j < lines.size(); ++j) {
            fout << tlPrefix << std::setw(5) << std::setfill('0') << talk.index << "] ";

            if (j == 0 && talk.hasSpeaker()) {
                fout << "[";
                printUtf8(fout, talk.speaker);
                fout << "] ";
            }

            // exporting message id and position in original csx for debug
            //fout << talk.index << " " << std::hex << (talk.talkPos + sizeof(Header) + 16) << std::dec;

            fout << "\r\n";
        }

        fout << "\r\n\r\n";
    }

    fout.close();
}

static int parseNumber(const char *numStr, size_t length) {
    int res = 0;
    for (size_t i = 0; i < length; ++i) {
        res *= 10;
        if (numStr[i] < '0' || numStr[i] > '9') {
            throw std::runtime_error("Invalid number.");
        }
        res += numStr[i] - '0';
    }
    return res;
}

struct Line {
    const char *content;
    size_t size;
    size_t lineNumber;
};

// splits utf-8 text into lines and trims lines
static std::deque<Line> splitIntoLines(const char *str, size_t strLen) {
    std::deque<Line> result;
    size_t begin = 0;
    size_t lineNumber = 1;

    for (size_t i = 0; i < strLen; ++i) {
        if (str[i] == '\n' || i == strLen - 1) {
            size_t end = i;
            while (begin < end && (str[end - 1] == '\r' || str[end - 1] == ' ')) {
                --end;
            }

            while (begin < end && str[begin] == ' ') {
                ++begin;
            }

            if (begin < end) {
                Line line;
                line.content = str + begin;
                line.size = end - begin;
                line.lineNumber = lineNumber;
                result.push_back(line);
            }

            begin = i + 1;
            ++lineNumber;
        }
    }

    return result;
}

static size_t numOfSpaces(const char *str, size_t length) {
    size_t spaces = 0;
    while (spaces < length && memcmp(str + spaces, space, sizeof(space)) == 0) {
        ++spaces;
    }
    return spaces;
}

static size_t findSubstring(const char *str, size_t strLen, const char *substr, size_t substrLen) {
    for (size_t i = 0; i < strLen - substrLen + 1; ++i) {
        if (memcmp(str + i * 2, substr, substrLen * 2) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

TxtParser::Stats TxtParser::parseTxt(std::string utfFilename, const char *customSpace) {
    size_t initMessagesSize = messages.size() + specials.size();
    size_t initChoicesSize = choices.size();
    Stats stats;

    // reading the whole file into a buffer
    std::ifstream fin(utfFilename.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!fin) {
        throw std::runtime_error("Could not open the file for reading (file " + utfFilename + ").");
    }

    int buffLen = 4 * 1024 * 1024;
    int len = 0;
    char *contents = new char[buffLen];

    while (fin) {
        fin.read(contents + len, buffLen - len);
        len += fin.gcount();
        char *temp = new char[buffLen + 1024 * 1024];
        memcpy(temp, contents, buffLen);
        delete[] contents;
        contents = temp;
        buffLen += 1024 * 1024;
    }

    if (len < 128) {
        throw std::runtime_error("File too short (file " + utfFilename + ").");
    }

    // support for UTF-8 with and without BOM
    int utfBomSkip = 0;
    if (memcmp(contents, utf8Bom, sizeof(utf8Bom)) != 0) {
        utfBomSkip = 3;
    }

    const size_t tlPrefixLen = strlen(tlPrefix);
    const size_t choiceTlPrefixLen = strlen(choiceTlPrefix);
    const size_t specialTlPrefixLen = strlen(specialTlPrefix);
    bool expectsSecondChoice = false;

    auto lines = splitIntoLines(contents + utfBomSkip, len - utfBomSkip);
    for (size_t i = 0; i < lines.size(); ++i) {
        Line line = lines[i];
        if (choiceTlPrefixLen + 3 <= line.size && memcmp(line.content, choiceTlPrefix, choiceTlPrefixLen) == 0) {
            if (expectsSecondChoice) {
                ++stats.totalChoices;
            }

            int index = parseNumber(line.content + choiceTlPrefixLen, 2);
            if (line.content[choiceTlPrefixLen + 2] != ']') {
                throw std::runtime_error("Invalid choice (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
            }

            if (choiceTlPrefixLen + 3 < line.size) {
                UnicodeString lineContent = utf8ToUnicodeString(line.content + choiceTlPrefixLen + 3, line.size - choiceTlPrefixLen - 3);
                size_t offset = 2 * numOfSpaces(lineContent.str, lineContent.length);

                UnicodeString choiceString = UnicodeString(lineContent.str + offset, lineContent.length - offset / 2, customSpace);

                if (choiceString.length > 0) {
                    if (expectsSecondChoice) {
                        if (!choices.empty() && choices.back().index != index) {
                            throw std::runtime_error("Invalid choice index (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
                        }
                        choices.back().choice2 = choiceString;
                    } else {
                        Choice choice;
                        choice.index = index;
                        choice.choice1 = choiceString;
                        choices.push_back(choice);
                    }
                }
            }

            expectsSecondChoice = !expectsSecondChoice;
        } else if (specialTlPrefixLen + 10 <= line.size && memcmp(line.content, specialTlPrefix, specialTlPrefixLen) == 0) {
            Special special;
            special.pos = parseNumber(line.content + specialTlPrefixLen, 9);
            if (line.content[specialTlPrefixLen + 9] != ']') {
                throw std::runtime_error("Invalid special message (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
            }

            ++stats.totalMessages;

            if (specialTlPrefixLen + 10 < line.size) {
                UnicodeString lineContent = utf8ToUnicodeString(line.content + specialTlPrefixLen + 10, line.size - specialTlPrefixLen - 10);
                size_t offset = 2 * numOfSpaces(lineContent.str, lineContent.length);

                special.substitution = UnicodeString(lineContent.str + offset, lineContent.length - offset / 2, customSpace);
                if (special.substitution.length > 0) {
                    specials.push_back(special);
                }
            }
        } else if (tlPrefixLen + 6 <= line.size && memcmp(line.content, tlPrefix, tlPrefixLen) == 0) {
            Talk talk;
            talk.index = parseNumber(line.content + tlPrefixLen, 5);
            if (line.content[tlPrefixLen + 5] != ']') {
                throw std::runtime_error("Invalid message (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
            }

            if (messages.size() == 0 || messages.back().index != talk.index) {
                ++stats.totalMessages;
            }

            if (tlPrefixLen + 6 < line.size) {
                UnicodeString lineContent = utf8ToUnicodeString(line.content + tlPrefixLen + 6, line.size - tlPrefixLen - 6);
                size_t offset = 2 * numOfSpaces(lineContent.str, lineContent.length);

                if (offset + sizeof(openingBracket) < lineContent.length * 2 && memcmp(lineContent.str + offset, openingBracket, sizeof(openingBracket)) == 0) {
                    offset += sizeof(openingBracket);
                    const char *usStr = lineContent.str + offset;
                    size_t usStrLen = findSubstring(usStr, lineContent.length - offset / 2, endingBracket, sizeof(endingBracket) / 2);
                    if (usStrLen == (size_t)-1) {
                        throw std::runtime_error("Invalid speaker name (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
                    }
                    talk.speaker = UnicodeString(usStr, usStrLen);
                    offset += 2 * talk.speaker.length + sizeof(endingBracket);
                    offset += 2 * numOfSpaces(lineContent.str + offset, lineContent.length - offset / 2);
                }

                talk.message = UnicodeString(lineContent.str + offset, lineContent.length - offset / 2, customSpace);
                if (talk.message.length > 0) {
                    if (messages.size() == 0 || messages.back().index != talk.index) {
                        messages.push_back(talk);
                    } else {
                        if (talk.hasSpeaker()) {
                            throw std::runtime_error("Speaker name not in present in the first line (line " + std::to_string(line.lineNumber) + " of file " + utfFilename + ").");
                        }
                        messages.back().message += UnicodeString(scenarioNewLine, sizeof(scenarioNewLine) / 2);
                        messages.back().message += talk.message;
                    }
                }
            }
        }
    }

    if (expectsSecondChoice) {
        throw std::runtime_error("Both choices from one question or neither of them must be translated (file " + utfFilename + ").");
    }

    delete[] contents;

    stats.translatedMessages = messages.size() + specials.size() - initMessagesSize;
    stats.translatedChoices = choices.size() - initChoicesSize;

    return stats;
}

std::ostream &operator <<(std::ostream &os, const TxtParser::Stats &stats) {
    os << stats.translatedMessages << "/" << stats.totalMessages << " (" << std::setprecision(2) << std::setiosflags(std::ios_base::fixed) << stats.messagesPercent() << "%) lines, ";
    os << stats.translatedChoices << "/" << stats.totalChoices << " (" << stats.choicesPercent() << "%) choices";
    return os;
}
