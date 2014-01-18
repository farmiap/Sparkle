all: Sparkle


Sparkle: main.o RegimeContainer.o Regime.o Pathes.o ImageAverager.o
	g++ -landor -lncurses main.o RegimeContainer.o Regime.o Pathes.o ImageAverager.o -o Sparkle
	
main.o: main.cpp RegimeContainer.cpp RegimeContainer.h ImageAverager.h
	g++ -c main.cpp
	
RegimeContainer.o: RegimeContainer.cpp RegimeContainer.h Regime.cpp Regime.h
	g++ -c RegimeContainer.cpp
	
Regime.o: Regime.cpp Regime.h Pathes.cpp Pathes.h
	g++ -c Regime.cpp
	
Pathes.o: Pathes.cpp Pathes.h
	g++ -c Pathes.cpp
	
ImageAverager.o: ImageAverager.cpp ImageAverager.h
	g++ -c ImageAverager.cpp

clean:
	rm -rf *.o Sparkle
	