#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <locale>
#include <cstring>
#include "csxparser.h"

void usage() {
    std::cerr << "Usage: csxtool (import [--alternativeSpace decimalFirstByteLittleEndian decimalSecondByteLittleEndian] [--ignoreExactLineBreaks] file.csx scenario1.txt [scenario2.txt scenario3.txt ...]|export file.csx)\n";
}

int main(int argc, const char **argv) {
    if (argc < 3) {
        usage();
        return 1;
    }

    std::cout << "csxtool 1.5\n";

    char customSpace[] = { 0x20, 0x00 };

    bool alternativeSpace = false;
    if (argc >= 6) {
        alternativeSpace = std::string(argv[2]) == std::string("--alternativeSpace");
        customSpace[0] = std::atoi(argv[3]);
        customSpace[1] = std::atoi(argv[4]);

        if (!alternativeSpace) {
            alternativeSpace = std::string(argv[3]) == std::string("--alternativeSpace");
            customSpace[0] = std::atoi(argv[4]);
            customSpace[1] = std::atoi(argv[5]);
        }
    }

    bool ignoreExactLineBreaks = false;
    if (argc >= 4) {
        ignoreExactLineBreaks = std::string(argv[2]) == std::string("--ignoreExactLineBreaks") || std::string(argv[3]) == std::string("--ignoreExactLineBreaks");
    }

    std::string command(argv[1]);
    std::string csxFilename(argv[2 + alternativeSpace * 3 + ignoreExactLineBreaks]);
    std::string filenamePrefix = csxFilename.substr(0, csxFilename.find_last_of('.'));

    if (command == "export") {
        if (argc != 3) {
            usage();
            return 1;
        }
        std::string utfFilename = filenamePrefix + ".txt";

        std::cout << "Exporting " << csxFilename << " to " << utfFilename << "..." << std::endl;

        CSXParser csx;
        try {
            csx.exportCsx(csxFilename, utfFilename);
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        std::cout << "Export successful!" << std::endl;
        return 0;
    } else if (command == "import") {
        if (argc < 4  + alternativeSpace * 3 + ignoreExactLineBreaks) {
            usage();
            return 1;
        }

        std::string origCsxFilename = filenamePrefix + "_original.csx";
        std::deque<std::string> utfFilenames;
        for (int i = 3 + alternativeSpace * 3 + ignoreExactLineBreaks; i < argc; ++i) {
            utfFilenames.push_back(argv[i]);
        }

        std::cout << "Importing ";
        for (size_t i = 0; i < utfFilenames.size(); ++i) {
            if (i != 0) {
                std::cout << ", ";
            }
            std::cout << utfFilenames[i];
        }
        std::cout << " and " << origCsxFilename << " to " << csxFilename << "..." << std::endl;

        if (alternativeSpace) {
            std::cout << "Alternative space mode: replacing spaces with " << (unsigned int)(unsigned char)customSpace[0] << " " << (unsigned int)(unsigned char)customSpace[1] << " UTF-16LE character." << std::endl;
        }

        if (ignoreExactLineBreaks) {
            std::cout << "Ignoring exact line breaks mode: not adding line breaks when the line is exactly 56 half-width characters long.";
        }

        CSXParser csx;
        try {
            csx.importCsx(origCsxFilename, csxFilename, utfFilenames, alternativeSpace ? customSpace : NULL, ignoreExactLineBreaks);
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        std::cout << "Import successful!" << std::endl;
        return 0;
    } else {
        usage();
        return 1;
    }

    return 0;
}
