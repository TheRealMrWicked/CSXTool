csxtool:
	g++ -Wno-narrowing -o csxtool -liconv -DNDEBUG *.cpp

clean:
	rm -f csxtool