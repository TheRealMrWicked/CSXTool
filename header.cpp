#include "header.h"

std::ifstream &operator >>(std::ifstream &fin, Header &header) {
    fin.read((char *)header.fileType, 8);
    fin.read((char *)&header.unknown1, 4);
    fin.read((char *)&header.zero, 4);
    fin.read((char *)header.imageType, 40);
    fin.read((char *)&header.size, 4);
    fin.read((char *)&header.unknown2, 4);
    return fin;
}

std::ofstream &operator <<(std::ofstream &fout, const Header &header) {
    fout.write((const char *)header.fileType, 8);
    fout.write((const char *)&header.unknown1, 4);
    fout.write((const char *)&header.zero, 4);
    fout.write((const char *)header.imageType, 40);
    fout.write((const char *)&header.size, 4);
    fout.write((const char *)&header.unknown2, 4);
    return fout;
}
