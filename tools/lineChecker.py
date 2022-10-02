import re
import os

while True:
    file = input("What is the file name?\n")  # Asks the user which file they want to check.
    if os.path.exists(file):
        break
    else:
        print("That file does not exist.\n")  # Tells the user that the file doesnt exist.
        continue

textformat = open("missed.txt", "w", encoding="UTF-8")  # Reopens the file for writing the new data to the file.
pattern = re.compile("\[EN.*\] \n")  # Outputs the blank lines.
for line in open(file, "r", encoding="UTF-8"):
    for match in re.finditer(pattern, line):
        textformat.write(line)  # Writes the variable into the file.

textformat.close()  # Closes the file stream.

print("The missed lines are now in the missed.txt")
input("Press enter to end the program.")