

all: c-dxf2gcode

c-dxf2gcode: main.c dxf.c dxf.h
	(cd ./AntTweakBar/src ; make)
#	gcc -o c-dxf2gcode main.c dxf.c -lGL -lglut -lGLU -lX11 -I./ -Wall -O3 -I./AntTweakBar/include/ -L./AntTweakBar/lib/ ./AntTweakBar/lib/libAntTweakBar.a
	clang -o c-dxf2gcode main.c dxf.c -lGL -lglut -lGLU -lX11 -I./ -Wall -O3 -I./AntTweakBar/include/ -L./AntTweakBar/lib/ ./AntTweakBar/lib/libAntTweakBar.a

gprof:
	gcc -pg -o c-dxf2gcode main.c dxf.c -lGL -lglut -lGLU -lX11 -I./ -Wall -O3 -I./AntTweakBar/include/ -L./AntTweakBar/lib/ ./AntTweakBar/lib/libAntTweakBar.a
	echo "./c-dxf2gcode"
	echo "gprof c-dxf2gcode gmon.out"

clean:
	rm -rf ./AntTweakBar/src/debug32
	rm -rf ./AntTweakBar/src/debug64
	rm -rf ./AntTweakBar/src/release32
	rm -rf ./AntTweakBar/src/release64
	rm -rf ./AntTweakBar/src/ipch
	rm -rf ./AntTweakBar/src/*.ncb
	rm -rf ./AntTweakBar/src/*.aps
	rm -rf ./AntTweakBar/src/*.o
	rm -rf ./AntTweakBar/src/*.bak
	rm -rf ./AntTweakBar/src/*.user
	rm -rf ./AntTweakBar/src/*.sdf
	rm -rf ./AntTweakBar/lib/*
	rm -rf c-dxf2gcode




