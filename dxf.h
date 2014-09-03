
#define ARC_RAST 18.0

#define OPTION_LINE_X1		10
#define OPTION_LINE_Y1		20
#define OPTION_LINE_X2		11
#define OPTION_LINE_Y2		21

#define OPTION_POINT_X		10
#define OPTION_POINT_Y		20

#define OPTION_ARC_X		10
#define OPTION_ARC_Y		20
#define OPTION_ARC_RADIUS	40
#define OPTION_ARC_BEGIN	50
#define OPTION_ARC_END		51

#define OPTION_MTEXT_X		10
#define OPTION_MTEXT_Y		20
#define OPTION_MTEXT_SIZE	40
#define OPTION_MTEXT_TEXT	 1
#define OPTION_BLOCKNAME	 3
#define OPTION_LAYERNAME	 8

#define PI 3.14159265358979323846264338327950288419716939937511
#define toDeg(x) (x*180.0)/PI
#define toRad(x) (PI/180.0) * x

#define MAX_LINES		10000
#define MAX_OBJECTS		10000

enum {
	TYPE_NONE,
	TYPE_LINE,
	TYPE_ARC,
	TYPE_CIRCLE,
	TYPE_MTEXT,
	TYPE_POINT,
	TYPE_POLYLINE,
	TYPE_VERTEX,
	TYPE_LAST
};

typedef struct{
	int line[MAX_LINES];
	int closed;
	int inside;
	int visited;
	int selection;
	int offset;
	int force;
	int overcut;
	int pocket;
	int laser;
	int depth;
	double object_depth;
	char layer[256];
} _OBJECT;

typedef struct{
	int used;
	int type;
	char layer[256];
	char block[256];
	double x1;
	double y1;
	double x2;
	double y2;
	double opt;
	int in_object;
	int blockdata;
} _LINE;

typedef struct{
	int used;
	char layer[256];
	char block[256];
	float x;
	float y;
	float s;
	char text[1024];
} _MTEXT;

extern _OBJECT *myOBJECTS;
extern _LINE *myLINES;
extern _MTEXT myMTEXT[100];
extern int mtext_n;
extern char dxf_options[256][256];
extern char dxf_typename[TYPE_LAST][16];
extern int line_last;

void dxf_read (char *file);
size_t trimline (char *out, size_t len, const char *str);
void add_line (int type, char *layer, double x1, double y1, double x2, double y2, double opt);

