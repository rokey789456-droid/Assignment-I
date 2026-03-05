CXX      = g++
CXXFLAGS = -std=c++98 -Wall -pedantic
TARGET   = simulador

all: $(TARGET)

$(TARGET): simulador.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) simulador.cpp

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.out

.PHONY: all run clean
