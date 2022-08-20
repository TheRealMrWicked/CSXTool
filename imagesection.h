#ifndef IMAGESECTION_H
#define IMAGESECTION_H

#include <deque>
#include <set>
#include "section.h"
#include "unicodestring.h"
#include "positioncomputer.h"
#include "talk.h"
#include "choice.h"
#include "special.h"

class ImageSection : public Section {
public:
    size_t contentStartByte;
    std::deque<Talk> messages;
    std::deque<Choice> choices;

    void extractStrings();
    void extractChoices();
    void substituteStrings(std::deque<Talk> newMessages, std::deque<Choice> newChoices, std::deque<Special> specials, bool ignoreExactLineBreaks);
    void updateShortHops();
    const PositionComputer &positionsComputer() { return posComp; }
    void fixLineWidth();

    void check() const;

private:
    struct Jump {
        enum JumpType {
            NoJump,
            Jump1Arg,
            Jump2Arg
        };

        JumpType type;
        size_t arg1Pos;
        size_t arg2Pos;
    };

    Jump detectJump(size_t pos) const;
    void updateJump(size_t pos);

    PositionComputer posComp;
    std::set<size_t> updatedJumpPos;
};

#endif // IMAGESECTION_H
