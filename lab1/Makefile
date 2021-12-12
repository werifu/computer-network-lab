src_files := main.cc client.cc client.hh packet.cc packet.hh logger.cc logger.hh
compile:
	g++ ${src_files} -std=c++17 -lws2_32 -o .\debug\a.exe
run: compile
	.\debug\a.exe