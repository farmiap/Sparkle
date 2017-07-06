all: Sparkle

Sparkle: main.o RegimeContainer.o Regime.o Pathes.o ImageAverager.o StandaRotationStage.o StandaActuator.o StepRotation.o MirrorMotion.o CommandLogger.o
	g++ -L/home/safonov/cfitsio/lib -g -fstack-protector-all -O1 -o Sparkle main.o RegimeContainer.o Regime.o Pathes.o ImageAverager.o StandaRotationStage.o StandaActuator.o StepRotation.o MirrorMotion.o CommandLogger.o -lcfitsio -landor -lncurses -lximc -lc -lnova -Wl,--verbose
	chgrp observers Sparkle
	
main.o: main.cpp RegimeContainer.cpp RegimeContainer.h ImageAverager.h
	g++ -c -g main.cpp
	
RegimeContainer.o: RegimeContainer.cpp RegimeContainer.h Regime.cpp Regime.h
	g++ -c -g RegimeContainer.cpp
	
Regime.o: Regime.cpp Regime.h Pathes.cpp Pathes.h StandaRotationStage.cpp StandaRotationStage.h StandaActuator.cpp StandaActuator.h MirrorMotion.cpp MirrorMotion.h StepRotation.cpp StepRotation.h
	g++ -c -g Regime.cpp
	
Pathes.o: Pathes.cpp Pathes.h
	g++ -c -g Pathes.cpp
	
ImageAverager.o: ImageAverager.cpp ImageAverager.h
	g++ -c -g ImageAverager.cpp

StandaRotationStage.o: StandaRotationStage.cpp StandaRotationStage.h
	g++ -c -g StandaRotationStage.cpp

StandaActuator.o: StandaActuator.cpp StandaActuator.h
	g++ -c -g StandaActuator.cpp

StepRotation.o: StepRotation.cpp StepRotation.h
	g++ -c -g StepRotation.cpp

MirrorMotion.o: MirrorMotion.cpp MirrorMotion.h
	g++ -c -g MirrorMotion.cpp
		
CommandLogger.o: CommandLogger.cpp CommandLogger.h
	g++ -c -g CommandLogger.cpp

clean:
	rm -rf *.o Sparkle
	