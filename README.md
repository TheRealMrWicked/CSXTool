# CSXTool
This program created to modify cotopha/EntisGLS binary (csx) files, without the need to decompile the file. It simply just edits the hexadecimal values and replaces the text.

Usage: csxtool (import [--alternativeSpace decimalFirstByteLittleEndian decimalSecondByteLittleEndian] [--ignoreExactLineBreaks] file.csx scenario1.txt [scenario2.txt scenario3.txt ...]|export file.csx)

import options

--alternativeSpace decimalFirstByteLittleEndian decimalSecondByteLittleEndian

This option changes the space character. The default is 0x20 0x00.

--ignoreExactLineBreaks

This option makes it so that when the line is exactly 56 half-width characters wide, a new line is not
inserted but instead the word wrapper built into the game takes care of word wrapping. This resolves
a HNS issue with double line breaks, but breaks YNS due to YNS' word wrapper using dash character to
indicate a word-wrapped word.

This program was created by Amanojaku, I requested his permission to upload it.

# Line Merger

This program is used to merge two txt files that are created by the csxtool, this is useful to combine various translations of the same csx file.

Usage: linemerger mainFile.txt TLFile.txt

The program will place lines from TLFile.txt below the lines in mainFile.txt, it should be noted that if the txt file is modified in any way then the program may not work.
In that case re-export a new txt file from the csx file using the csxtool.

The main (textMerger) function was made by [Shuvi](https://github.com/ShuviSchwarze) and the framework was done by [MrWicked](https://github.com/TheRealMrWicked).