#include "section.h"

Section::Section():
    content(0) {
}

Section::~Section() {
    delete[] content;
}

std::ifstream &operator >>(std::ifstream &fin, Section &section) {
    fin.read((char *)section.id, 8);
    fin.read((char *)&section.size, 4);
    fin.read((char *)&section.unknown, 4);
    section.content = new char[section.size];
    fin.read(section.content, section.size);
    return fin;
}

std::ofstream &operator <<(std::ofstream &fout, const Section &section) {
    fout.write((const char *)section.id, 8);
    fout.write((const char *)&section.size, 4);
    fout.write((const char *)&section.unknown, 4);
    fout.write(section.content, section.size);
    return fout;
}
