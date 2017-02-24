CXX ?= c++
CXXFLAGS ?= -Os -Wall -Wextra -pedantic

all: amdmsrt

amdmsrt: Info.o AmdMsrTweaker.o WinRing0.o Worker.o
	$(CXX) $(CXXFLAGS) -o $@ $>$^
# $>$^ is to be compatible against BSD and GNU make(1)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

Info.o: Info.cpp Info.h
AmdMsrTweaker.o: AmdMsrTweaker.cpp Worker.h Info.h WinRing0.h
WinRing0.o: WinRing0.cpp WinRing0.h StringUtils.h
Worker.o: Worker.cpp Worker.h StringUtils.h WinRing0.h

clean:
	rm -f *.o amdmsrt

.PHONY: all clean
