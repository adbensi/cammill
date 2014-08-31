


INCLUDES = -I./
LIBS = -lGL -lglut -lGLU -lX11 -lm -lpthread -lstdc++ -lXext -ldl -lXi -lxcb -lXau -lXdmcp -lgcc -lc `pkg-config gtk+-2.0 --libs` `pkg-config gtk+-2.0 --cflags` `pkg-config gtkgl-2.0 --libs` `pkg-config gtkgl-2.0 --cflags`


all: c-dxf2gcode

c-dxf2gcode: main.c setup.c dxf.c dxf.h texture.c
#	gcc -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES}
	clang -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES}

gprof:
	gcc -pg -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES}
	echo "./c-dxf2gcode"
	echo "gprof c-dxf2gcode gmon.out"

clean:
	rm -rf c-dxf2gcode




