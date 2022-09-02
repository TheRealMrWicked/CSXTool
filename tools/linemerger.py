import sys
import os
import re


def mainFileFormatter():  # Formats the file to work with the textMerger function.
    textformat = open(sys.argv[1], "r", encoding="UTF-8")  # Opens the new file into the file stream.
    data = textformat.read()  # Declaring the data variable used to store the changes to the text.

    data = re.sub("\[EN.*] ", "", string=data)  # Removes the blank EN text boxes.
    data = re.sub("\n\n\n\n\n\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n", "\n\n\n", string=data)  # Removes whitespaces.

    textformat = open("temp.txt", "w", encoding="UTF-8")  # Reopens the file for writing the new data to the file.
    textformat.write(data)  # Writes the variable into the file.
    textformat.close()  # Closes the file stream.


def TLFileFormatter():  # Formats the file to work with the textMerger function.
    textformat = open(sys.argv[2], "r", encoding="UTF-8")  # Opens the new file into the file stream.
    data = textformat.read()  # Declaring the data variable used to store the changes to the text.

    data = re.sub("\[EN.*] ", "", string=data)  # Removes the blank EN text boxes.
    data = re.sub("\n\n\n\n\n\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n\n", "\n", string=data)  # Removes whitespaces.
    data = re.sub("\n\n", "\n", string=data)  # Removes whitespaces.
    data = data.replace("[JP", "[EN")  # Changes the tag from JP to EN.

    textformat = open("temp2.txt", "w", encoding="UTF-8")  # Reopens the file for writing the new data to the file.
    textformat.write(data)  # Writes the variable into the file.
    textformat.close()  # Closes the file stream.


def removeLastLine():  # Removes the last line to prevent an index error in the textMerger function.
    fd = open("temp2.txt", "r", encoding="UTF-8")
    d = fd.read()
    fd.close()
    m = d.split("\n")
    s = "\n".join(m[:-1])
    fd = open("temp2.txt", "w+", encoding="UTF-8")
    for i in range(len(s)):
        fd.write(s[i])
    fd.close()


def textMerger():  # Merges the two files together.
    insertTag = "[JP"
    if os.path.exists("temp.txt") and os.path.exists("temp2.txt"):
        file_1 = open("temp.txt", "r", encoding="UTF-8")
        file_2 = open("temp2.txt", "r", encoding="UTF-8")
        output = open("merged.txt", "x", encoding="UTF-8")

        line_block = file_1.read().split("\n\n")
        translations = file_2.read().split("\n")
        tl_len = len(translations)
        flush = " "*len(str(tl_len))

        for block in line_block:
            tl_lines = []
            tl_rem = 0

            block = block.split("\n")
            block_len = len(block)

            match_start = False
            match_end = False
            tl_print = True
            line_print = True

            for i in range(block_len):
                line = block[i]
                if line[:3] == insertTag:
                    match_start = True
                elif match_start:
                    match_end = True

                if line[:3] == insertTag and i == block_len - 1:
                    output.write(f"{line}\n")
                    line_print = False
                    match_end = True

                if match_end and tl_print:
                    found = False
                    tl_print = False
                    match_end = False

                    try:
                        tag = line.split()[0][3:]
                    except IndexError:
                        pass

                    for tl in translations:
                        if tl.split()[0][3:] == tag:
                            output.write(f"{tl}\n")
                            tl_rem += 1
                            found = True
                        elif found:
                            break

                if line_print:
                    output.write(f"{line}\n")
                    line_print = True

            output.write("\n")

            for i in range(tl_rem):
                del translations[0]
            print(f"{len(translations)}/{tl_len} remaining{flush}",end="\r",flush=True)


if len(sys.argv) != 3:
    print("Usage: linemerger mainFile.txt TLFile.txt")
    quit()

try:
    os.remove("merged.txt")
except OSError:
    pass

print("Processing...")

mainFileFormatter()
TLFileFormatter()
removeLastLine()
textMerger()

print("\nFinished! The file merged.txt has been created.")

try:
    os.remove("temp.txt") or os.remove("temp2.txt")
except OSError:
    pass
