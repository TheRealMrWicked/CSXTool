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

As usual help would be appreciated by making a **pull request**.

This program was created by Amanojaku, I requested his permission to upload it.