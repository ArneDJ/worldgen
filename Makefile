CC=g++ -std=c++14
CFLAGS=-lm `sdl2-config --cflags --libs` -lGL -lGLEW
FASTNOISE=$(wildcard src/external/fastnoise/*.cpp)
IMGUI=$(wildcard src/external/imgui/*.cpp)
OUTPUT=world.out

SRC = $(wildcard src/*.cpp)
GRAPHICS=$(wildcard src/graphics/*.cpp)
WORLDMAP=$(wildcard src/worldmap/*.cpp)

main : $(src)
	$(CC) -o $(OUTPUT) $(SRC) $(GRAPHICS) $(WORLDMAP) $(FASTNOISE) $(IMGUI) $(CFLAGS) 
