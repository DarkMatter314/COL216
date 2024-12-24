all: compile 

compile: 5stage.cpp 5stage_bypass.cpp 79stage.cpp 79stage_bypass.cpp MIPS_Pipelining.hpp MIPS_Forwarding.hpp MIPS_79Pipelining.hpp MIPS_79Forwarding.hpp
	g++ 5stage.cpp MIPS_Pipelining.hpp -o 5stage.o
	g++ 5stage_bypass.cpp MIPS_Forwarding.hpp -o 5stage_bypass.o
	g++ 79stage.cpp MIPS_79Pipelining.hpp -o 79stage.o
	g++ 79stage_bypass.cpp MIPS_79Forwarding.hpp -o 79stage_bypass.o

run_5stage: 5stage.o
	./5stage.o input.asm

run_5stage_bypass: 5stage_bypass.o
	./5stage_bypass.o input.asm

run_79stage: 79stage.o
	./79stage.o input.asm

run_79stage_bypass: 79stage_bypass.o
	./79stage_bypass.o input.asm

clean:
	rm -f *.o