
#libwebkitgtk-dev
#msgfmt de.po -o intl/de_DE.UTF-8/LC_MESSAGES/cammill.mo

HERSHEY_FONTS_DIR = ./
INCLUDES += -I./
LIBS += -lGL -lglut -lGLU -lX11 -lm -lpthread -lstdc++ -lXext -ldl -lXi -lxcb -lXau -lXdmcp -lgcc -lc `pkg-config gtk+-2.0 --libs` `pkg-config gtk+-2.0 --cflags` `pkg-config gtkglext-x11-1.0 --libs` `pkg-config gtkglext-x11-1.0 --cflags`
LIBS += `pkg-config gtksourceview-2.0 --libs` `pkg-config gtksourceview-2.0 --cflags`
LIBS += `pkg-config lua5.1 --libs` `pkg-config lua5.1 --cflags`
#LIBS += `pkg-config gthread-2.0 --libs`
#LIBS += `pkg-config gtk-vnc-1.0 --libs` `pkg-config gtk-vnc-1.0 --cflags`
#LIBS += `pkg-config webkit-1.0 --libs` `pkg-config webkit-1.0 --cflags`

#CFLAGS+="-DGTK_DISABLE_SINGLE_INCLUDES"
CFLAGS+="-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
CFLAGS+="-DGSEAL_ENABLE"
CFLAGS+="-DHERSHEY_FONTS_DIR=\"./\""
CFLAGS+="-DUSE_POSTCAM"
#CFLAGS+="-DUSE_VNC"
#CFLAGS+="-DUSE_WEBKIT"

all: cammill

cammill: main.c pocket.c calc.c hersheyfont.c postprocessor.c setup.c dxf.c dxf.h font.c font.h texture.c
	msgfmt de.po -o intl/de_DE.UTF-8/LC_MESSAGES/cammill.mo
#	gcc -fopenmp -ggdb -Wall -O3 -o cammill main.c pocket.c calc.c hersheyfont.c postprocessor.c setup.c dxf.c font.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}
	clang -ggdb -Wall -Wno-unknown-pragmas -O3 -o cammill main.c pocket.c calc.c hersheyfont.c postprocessor.c setup.c dxf.c font.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}

gprof:
	gcc -pg -ggdb -Wall -O3 -o cammill main.c pocket.c calc.c hersheyfont.c postprocessor.c setup.c dxf.c font.c texture.c ${LIBS} ${INCLUDES} ${CFLAGS}
	@echo "./cammill"
	@echo "gprof cammill gmon.out"

clean:
	rm -rf cammill

