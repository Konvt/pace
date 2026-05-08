COMPILER = g++
OFLAG = Og
STANDARD = c++11
CFLAGS = -Wpedantic -Wall -Wextra
IFLAG = ../include

%: %.cpp
	$(COMPILER) -$(OFLAG) -std=$(STANDARD) $(CFLAGS) -I$(IFLAG) $< -o $@

all: complex iterate progressbar blockbar spinbar sweepbar flowbar multibar dynamicbar unicode
clean:
	find . -maxdepth 1 -type f -executable ! -name '*.*' ! -name 'Makefile' -exec rm {} +
