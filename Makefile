


INCLUDES = -I./
LIBS = -lGL -lglut -lGLU -lX11 -lm -lpthread -lstdc++ -lXext -ldl -lXi -lxcb -lXau -lXdmcp -lgcc -lc `pkg-config gtk+-2.0 --libs` `pkg-config gtk+-2.0 --cflags` `pkg-config gtkglext-x11-1.0 --libs` `pkg-config gtkglext-x11-1.0 --cflags`

#CFLAGS+="-DGTK_DISABLE_SINGLE_INCLUDES"
CFLAGS+="-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
CFLAGS+="-DGSEAL_ENABLE"



all: c-dxf2gcode

c-dxf2gcode: main.c setup.c dxf.c dxf.h texture.c
#	gcc -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}
	clang -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}

gprof:
	gcc -pg -Wall -O3 -o c-dxf2gcode main.c setup.c dxf.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}
	echo "./c-dxf2gcode"
	echo "gprof c-dxf2gcode gmon.out"

clean:
	rm -rf c-dxf2gcode




