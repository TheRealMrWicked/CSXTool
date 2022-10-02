fileName = input("What is the filename?\n")
originalIndex = input("What is the starting index?\n")
finalIndex = input("What is the final index in the list?\n")
newIndex = int(originalIndex) - 1

textformat = open(fileName, "r", encoding="UTF-8")  # Opens the new file into the file stream.
data = textformat.read()  # Declaring the data variable used to store the changes to the text.

while int(originalIndex) <= int(finalIndex):
    data = data.replace("[EN" + str(originalIndex).zfill(5) + "]", "[EN" + str(newIndex).zfill(5) + "]")
    data = data.replace("[JP" + str(originalIndex).zfill(5) + "]", "[JP" + str(newIndex).zfill(5) + "]")
    originalIndex = int(originalIndex) + 1
    newIndex = int(newIndex) + 1

textformat = open("final.txt", "w", encoding="UTF-8")  # Reopens the file for writing the new data to the file.
textformat.write(data)  # Writes the variable into the file.
textformat.close()  # Closes the file stream.