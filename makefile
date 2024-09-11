all: reset clean asembler linker emulator

emulator: src/Emulator.cpp src/TES.cpp
	g++ -o $(@) $(^)

linker: src/Linker.cpp src/TES.cpp
	g++ -o $(@) $(^)

asembler: src/parser.cpp src/lexer.cpp src/Asembler.cpp src/TES.cpp
	g++ -o $(@) $(^)

src/lexer.cpp: misc/lexer.l
	flex --outfile=$(@) $(^)

src/parser.cpp: misc/parser.y
	bison -Wnone -v --defines=inc/parser.hpp --output=$(@) $(^) 

clean: reset
	rm -f src/parser.cpp src/lexer.cpp inc/parser.hpp parser.output ./asembler src/parser.output ./linker ./emulator 

reset:
	rm -f *.txt *.hex *.o 

