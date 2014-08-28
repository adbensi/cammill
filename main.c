/*

 Copyright by Oliver Dippel <oliver@multixmedia.org>

*/

#include <AntTweakBar.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <pwd.h>
#include <GL/glut.h>
#include <dxf.h>
//#include <font.h>

void texture_init (void);

GLuint texture_load (char *filename);


#define FUZZY 0.01

#ifndef CALLBACK
#define CALLBACK
#endif

int winw = 1200;
int winh = 1200;
float size_x = 0.0;
float size_y = 0.0;
double min_x = 99999.0;
double min_y = 99999.0;
double max_x = 0.0;
double max_y = 0.0;
double tooltbl_diameters[100];
char mill_layer[1024];
int layer_sel = 0;
int layer_selections[100];
int layer_force[100];
double layer_depth[100];
TwEnumVal shapeEV[100];
int object_selections[MAX_OBJECTS];
int object_offset[MAX_OBJECTS];
int object_force[MAX_OBJECTS];
int object_overcut[MAX_OBJECTS];
int object_pocket[MAX_OBJECTS];
int object_laser[MAX_OBJECTS];
double object_depth[MAX_OBJECTS];
FILE *fd_out = NULL;
int object_last = 0;
char dxf_file[2048];
int batchmode = 0;
int translate_x = 0;
int translate_y = 0;
int save_gcode = 0;
float g_Rotation[] = {0.0f, 0.0f, 0.0f, 1.0f};
float g_RotateStart[] = {0.0f, 0.0f, 0.0f, 1.0f};
int g_AutoRotate = 0;
int g_RotateTime = 0;
char tool_descr[100][1024];
int tools_max = 0;
int tool_last = 0;
int material_max = 8;
char *material_texture[100];
int material_vc[100];
float material_fz[100][3];
TwEnumVal toolEV[100];
TwEnumVal materialEV[100];

typedef struct{
	char name[256];
	char group[256];
	char arg[128];
	int type;
	int vint;
	float vfloat;
	double vdouble;
	char vstr[2048];
	double min;
	double step;
	double max;
	char unit[16];
	int show;
} PARA;

enum {
	T_INT,
	T_BOOL,
	T_FLOAT,
	T_DOUBLE,
	T_STRING
};

enum {
	P_V_ZOOM,
	P_V_HELPLINES,
	P_V_HELPDIA,
	P_V_NCDEBUG,
	P_TOOL_SELECT,
	P_TOOL_NUM,
	P_TOOL_DIAMETER,
	P_TOOL_SPEED_MAX,
	P_TOOL_SPEED,
	P_TOOL_W,
	P_TOOL_LASERDIA,
	P_TOOL_TABLE,
	P_M_FEEDRATE_MAX,
	P_M_FEEDRATE,
	P_M_PLUNGE_SPEED,
	P_M_DEPTH,
	P_M_Z_STEP,
	P_CUT_SAVE,
	P_M_OVERCUT,
	P_M_LASERMODE,
	P_MFILE,
	P_MAT_SELECT,
	P_LAST
};

PARA PARAMETER[] = {
	{"Zoom",	"View",		"-zo",	T_FLOAT,	0,	1.0,	0.0,	"",	0.1,	0.1,	20.0,		"x", 1},
	{"Helplines",	"View", 	"-hl",	T_BOOL	,	1,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1},
	{"ShowTool",	"View", 	"-st",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1},
	{"NC-Debug",	"View", 	"-nd",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1},
	{"Select",	"Tool",		"-tn",	T_INT,		1,	0.0,	0.0,	"",	1.0,	1.0,	100.0,		"#", 0},
	{"Number",	"Tool",		"-tn",	T_INT,		1,	0.0,	0.0,	"",	1.0,	1.0,	18.0,		"#", 1},
	{"Diameter",	"Tool",		"-td",	T_DOUBLE,	0,	1.0,	1.0,	"",	0.01,	0.01,	18.0,		"mm", 1},
	{"CalcSpeed",	"Tool",		"-cs",	T_INT,		10000,	0.0,	0.0,	"",	1.0,	10.0,	100000.0,	"rpm", 1},
	{"Speed",	"Tool",		"-ts",	T_INT,		10000,	0.0,	0.0,	"",	1.0,	10.0,	100000.0,	"rpm", 1},
	{"Wings",	"Tool",		"-tw",	T_INT,		2,	0.0,	0.0,	"",	1.0,	1.0,	10.0,		"#", 1},
	{"LaserDiameter","Tool",	"-lw",	T_DOUBLE,	0,	0.0,	0.4,	"",	0.01,	0.01,	10.0,		"mm", 1},
	{"Table",	"Tool",		"-tt",	T_STRING,	0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 1},
	{"MaxFeedRate",	"Milling",	"-fm",	T_INT	,	200,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1},
	{"FeedRate",	"Milling",	"-fr",	T_INT	,	200,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1},
	{"PlungeRate",	"Milling",	"-pr",	T_INT	,	100,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1},
	{"Depth",	"Milling",	"-md",	T_DOUBLE,	0,	-4.0,	-4.0,	"",	-40.0,	0.01,	-0.1,		"mm", 1},
	{"Z-Step",	"Milling",	"-msp",	T_DOUBLE,	0,	-4.0,	-4.0,	"",	-40.0,	0.01,	-0.1,		"mm", 1},
	{"Save-Move",	"Milling",	"-msm",	T_DOUBLE,	0,	4.0,	4.0,	"",	1.0,	1.0,	80.0,		"mm", 1},
	{"Overcut",	"Milling",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1},
	{"Lasermode",	"Milling",	"-lm",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1},
	{"Output-File",	"Milling",	"-o",	T_STRING,	0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 1},
	{"Select",	"Material",	"-ms",	T_INT,		1,	0.0,	0.0,	"",	1.0,	1.0,	100.0,		"#", 0},
};

void read_setup (void) {
	char cfgfile[2048];
	char line2[2048];
	FILE *cfg_fp;
	int n = 0;
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	sprintf(cfgfile, "%s/.c-dxf2gcode.cfg", homedir);
	cfg_fp = fopen(cfgfile, "r");
	if (cfg_fp == NULL) {
		fprintf(stderr, "Can not read Setup: %s\n", cfgfile);
	} else {
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		while ((read = getline(&line, &len, cfg_fp)) != -1) {
			trimline(line2, 1024, line);
			for (n = 0; n < P_LAST; n++) {
				char name_str[1024];
				sprintf(name_str, "%s|%s=", PARAMETER[n].group, PARAMETER[n].name);
				if (strncmp(line2, name_str, strlen(name_str)) == 0) {
					if (PARAMETER[n].type == T_FLOAT) {
						PARAMETER[n].vfloat = atof(line2 + strlen(name_str));
					} else if (PARAMETER[n].type == T_DOUBLE) {
						PARAMETER[n].vdouble = atof(line2 + strlen(name_str));
					} else if (PARAMETER[n].type == T_INT) {
						PARAMETER[n].vint = atoi(line2 + strlen(name_str));
					} else if (PARAMETER[n].type == T_BOOL) {
						PARAMETER[n].vint = atoi(line2 + strlen(name_str));
					} else if (PARAMETER[n].type == T_STRING) {
						strcpy(PARAMETER[n].vstr, line2 + strlen(name_str));
					}
				}
			}
		}
		fclose(cfg_fp);
	}
}

void TW_CALL SaveSetup (void *clientData) {
	char cfgfile[2048];
	FILE *cfg_fp;
	int n = 0;
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	sprintf(cfgfile, "%s/.c-dxf2gcode.cfg", homedir);
	cfg_fp = fopen(cfgfile, "w");
	if (cfg_fp == NULL) {
		fprintf(stderr, "Can not write Setup: %s\n", cfgfile);
		return;
	}
	for (n = 0; n < P_LAST; n++) {
		char name_str[1024];
		sprintf(name_str, "%s|%s", PARAMETER[n].group, PARAMETER[n].name);
		if (PARAMETER[n].type == T_FLOAT) {
			fprintf(cfg_fp, "%s=%f\n", name_str, PARAMETER[n].vfloat);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			fprintf(cfg_fp, "%s=%f\n", name_str, PARAMETER[n].vdouble);
		} else if (PARAMETER[n].type == T_INT) {
			fprintf(cfg_fp, "%s=%i\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_BOOL) {
			fprintf(cfg_fp, "%s=%i\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_STRING) {
			fprintf(cfg_fp, "%s=%s\n", name_str, PARAMETER[n].vstr);
		}
	}
	fclose(cfg_fp);
}

void ShowSetup (void) {
	int n = 0;
	fprintf(stdout, "\n");
	for (n = 0; n < P_LAST; n++) {
		char name_str[1024];
		sprintf(name_str, "%s-%s", PARAMETER[n].group, PARAMETER[n].name);
		if (PARAMETER[n].type == T_FLOAT) {
			fprintf(stdout, "%22s: %f\n", name_str, PARAMETER[n].vfloat);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			fprintf(stdout, "%22s: %f\n", name_str, PARAMETER[n].vdouble);
		} else if (PARAMETER[n].type == T_INT) {
			fprintf(stdout, "%22s: %i\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_BOOL) {
			fprintf(stdout, "%22s: %i\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_STRING) {
			fprintf(stdout, "%22s: %s\n", name_str, PARAMETER[n].vstr);
		}
	}
	fprintf(stdout, "\n");
}

void ShowSetup_gCode (FILE *out) {
	int n = 0;
	fprintf(out, "(GENERATED BY: c-dxf2gcode)\n");
	for (n = 0; n < P_LAST; n++) {
		char name_str[1024];
		sprintf(name_str, "%s-%s", PARAMETER[n].group, PARAMETER[n].name);
		if (PARAMETER[n].type == T_FLOAT) {
			fprintf(out, "(%22s: %f)\n", name_str, PARAMETER[n].vfloat);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			fprintf(out, "(%22s: %f)\n", name_str, PARAMETER[n].vdouble);
		} else if (PARAMETER[n].type == T_INT) {
			fprintf(out, "(%22s: %i)\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_BOOL) {
			fprintf(out, "(%22s: %i)\n", name_str, PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_STRING) {
			fprintf(out, "(%22s: %s)\n", name_str, PARAMETER[n].vstr);
		}
	}
	fprintf(out, "\n");
}

void ShowHelp (void) {
	int n = 0;
	fprintf(stdout, "\n");
	fprintf(stdout, "c-dxf2gcode [OPTIONS] FILE\n");
	for (n = 0; n < P_LAST; n++) {
		char name_str[1024];
		sprintf(name_str, "%s-%s", PARAMETER[n].group, PARAMETER[n].name);
		if (PARAMETER[n].type == T_FLOAT) {
			fprintf(stdout, "%5s FLOAT    %s\n", PARAMETER[n].arg, name_str);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			fprintf(stdout, "%5s DOUBLE   %s\n", PARAMETER[n].arg, name_str);
		} else if (PARAMETER[n].type == T_INT) {
			fprintf(stdout, "%5s INT      %s\n", PARAMETER[n].arg, name_str);
		} else if (PARAMETER[n].type == T_BOOL) {
			fprintf(stdout, "%5s 0/1      %s\n", PARAMETER[n].arg, name_str);
		} else if (PARAMETER[n].type == T_STRING) {
			fprintf(stdout, "%5s STRING   %s\n", PARAMETER[n].arg, name_str);
		}
	}
	fprintf(stdout, "\n");
}

void line_invert (int num) {
	double tempx = myLINES[num].x2;
	double tempy = myLINES[num].y2;
	myLINES[num].x2 = myLINES[num].x1;
	myLINES[num].y2 = myLINES[num].y1;
	myLINES[num].x1 = tempx;
	myLINES[num].y1 = tempy;
	myLINES[num].opt *= -1;
}

int point_in_object (int object_num, int object_ex, double testx, double testy) {
	int num = 0;
	int c = 1;
	int onum = object_num;
	if (object_num == -1) {
		for (onum = 0; onum < object_last; onum++) {
			if (onum == object_ex) {
				continue;
			}
			if (myOBJECTS[onum].closed == 0) {
				continue;
			}
			for (num = 0; num < line_last; num++) {
				if (myOBJECTS[onum].line[num] != 0) {
					int lnum = myOBJECTS[onum].line[num];
					if (((myLINES[lnum].y2 > testy) != (myLINES[lnum].y1 > testy)) && (testx < (myLINES[lnum].x1 - myLINES[lnum].x2) * (testy - myLINES[lnum].y2) / (myLINES[lnum].y1 - myLINES[lnum].y2) + myLINES[lnum].x2)) {
						c = !c;
					}
				}
			}
		}
	} else {
		if (myOBJECTS[onum].closed == 0) {
			return 0;
		}
		for (num = 0; num < line_last; num++) {
			if (myOBJECTS[onum].line[num] != 0) {
				int lnum = myOBJECTS[onum].line[num];
				if (((myLINES[lnum].y2 > testy) != (myLINES[lnum].y1 > testy)) && (testx < (myLINES[lnum].x1 - myLINES[lnum].x2) * (testy - myLINES[lnum].y2) / (myLINES[lnum].y1 - myLINES[lnum].y2) + myLINES[lnum].x2)) {
					c = !c;
				}
			}
		}
	}
	return c;
}

void CALLBACK beginCallback(GLenum which) {
   glBegin(which);
}

void CALLBACK errorCallback(GLenum errorCode) {
	const GLubyte *estring;
	estring = gluErrorString(errorCode);
//	fprintf(stderr, "Tessellation Error: %s\n", (char *) estring);
//	exit(0);
}

void CALLBACK endCallback(void) {
	glEnd();
}

void CALLBACK vertexCallback(GLvoid *vertex) {
	const GLdouble *pointer;
	pointer = (GLdouble *) vertex;
	glColor3dv(pointer+3);
	glVertex3dv(pointer);
}

void CALLBACK combineCallback(GLdouble coords[3], GLdouble *vertex_data[4], GLfloat weight[4], GLdouble **dataOut ) {
	GLdouble *vertex;
	int i;
	vertex = (GLdouble *)malloc(6 * sizeof(GLdouble));
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];
	for (i = 3; i < 6; i++) {
		vertex[i] = weight[0] * vertex_data[0][i] + weight[1] * vertex_data[1][i] + weight[2] * vertex_data[2][i] + weight[3] * vertex_data[3][i];
	}
	*dataOut = vertex;
}

void object2poly (int object_num, double depth, double depth2) {
	int num = 0;
	int nverts = 0;
	GLUtesselator *tobj;
	GLdouble rect2[MAX_LINES][3];

	glColor4f(0.0, 0.5, 0.2, 1.0);

//	glColor4f(1.0, 1.0, 1.0, 1.0);
//	texture_load(material_texture[PARAMETER[P_MAT_SELECT].vint]);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexGend(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
	glTexGend(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(0.002, 0.002, 0.002);
	glTranslatef(0.0, 0.0, 0.0);

	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (CALLBACK*) ()) &glVertex3dv);
	gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (CALLBACK*) ()) &beginCallback);
	gluTessCallback(tobj, GLU_TESS_END, (GLvoid (CALLBACK*) ()) &endCallback);
	gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (CALLBACK*) ()) &errorCallback);
	glShadeModel(GL_FLAT);
	gluTessBeginPolygon(tobj, NULL);
	gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
	gluTessNormal(tobj, 0, 0, 1);

	gluTessBeginContour(tobj);
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			int lnum = myOBJECTS[object_num].line[num];
			rect2[nverts][0] = (GLdouble)myLINES[lnum].x1;
			rect2[nverts][1] = (GLdouble)myLINES[lnum].y1;
			rect2[nverts][2] = (GLdouble)depth;
			gluTessVertex(tobj, rect2[nverts], rect2[nverts]);
			if (depth != depth2) {
				glBegin(GL_QUADS);
				glVertex3f((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, depth);
				glVertex3f((float)myLINES[lnum].x2, (float)myLINES[lnum].y2, depth);
				glVertex3f((float)myLINES[lnum].x2, (float)myLINES[lnum].y2, depth2);
				glVertex3f((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, depth2);
				glEnd();
			}
			nverts++;
		}
	}
	int num5 = 0;
	for (num5 = 0; num5 < object_last; num5++) {
		if (num5 != object_num && myOBJECTS[num5].closed == 1 && myOBJECTS[num5].inside == 1) {
			int lnum = myOBJECTS[num5].line[0];
			int pipret = 0;
			double testx = myLINES[lnum].x1;
			double testy = myLINES[lnum].y1;
			pipret = point_in_object(object_num, -1, testx, testy);
			if (pipret == 0) {
				gluNextContour(tobj, GLU_INTERIOR);
				for (num = 0; num < line_last; num++) {
					if (myOBJECTS[num5].line[num] != 0) {
						int lnum = myOBJECTS[num5].line[num];
						rect2[nverts][0] = (GLdouble)myLINES[lnum].x1;
						rect2[nverts][1] = (GLdouble)myLINES[lnum].y1;
						rect2[nverts][2] = (GLdouble)depth;
						gluTessVertex(tobj, rect2[nverts], rect2[nverts]);
						if (depth != depth2) {
							glBegin(GL_QUADS);
							glVertex3f((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, depth);
							glVertex3f((float)myLINES[lnum].x2, (float)myLINES[lnum].y2, depth);
							glVertex3f((float)myLINES[lnum].x2, (float)myLINES[lnum].y2, depth2);
							glVertex3f((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, depth2);
							glEnd();
						}
						nverts++;
					}
				}
			}
		}
	}
	gluTessEndPolygon(tobj);
	gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (CALLBACK*) ()) &vertexCallback);
	gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (CALLBACK*) ()) &beginCallback);
	gluTessCallback(tobj, GLU_TESS_END, (GLvoid (CALLBACK*) ()) &endCallback);
	gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (CALLBACK*) ()) &errorCallback);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (GLvoid (CALLBACK*) ()) &combineCallback);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_MODELVIEW);
}

int object_line_last (int object_num) {
	int num = 0;
	int ret = 0;
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			ret = num;
		}
	}
	return ret;
}

void draw_text (void *font, char *text, int x, int y) {
	glRasterPos2i(x, y);
	while (*text != '\0') {
		glutBitmapCharacter(font, *text);
		++text;
	}
}

double get_len (double x1, double y1, double x2, double y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	double len = sqrt(dx * dx + dy * dy);
	return len;
}

/* set new first line in object */
void resort_object (int object_num, int start) {
	int num4 = 0;
	int num5 = 0;
	int OTEMPLINE[MAX_LINES];
	for (num4 = 0; num4 < line_last; num4++) {
		OTEMPLINE[num4] = 0;
	}
	for (num4 = start; num4 < line_last; num4++) {
		if (myOBJECTS[object_num].line[num4] != 0) {
			OTEMPLINE[num5++] = myOBJECTS[object_num].line[num4];
		}
	}
	for (num4 = 0; num4 < start; num4++) {
		if (myOBJECTS[object_num].line[num4] != 0) {
			OTEMPLINE[num5++] = myOBJECTS[object_num].line[num4];
		}
	}
	for (num4 = 0; num4 < num5; num4++) {
		myOBJECTS[object_num].line[num4] = OTEMPLINE[num4];
	}
}

/* reverse lines in object */
void redir_object (int object_num) {
	int num = 0;
	int num5 = 0;
	int OTEMPLINE[MAX_LINES];
	for (num = 0; num < line_last; num++) {
		OTEMPLINE[num] = 0;
	}
	for (num = line_last - 1; num >= 0; num--) {
		if (myOBJECTS[object_num].line[num] != 0) {
			OTEMPLINE[num5++] = myOBJECTS[object_num].line[num];
			int lnum = myOBJECTS[object_num].line[num];
			line_invert(lnum);
		}
	}
	for (num = 0; num < num5; num++) {
		myOBJECTS[object_num].line[num] = OTEMPLINE[num];
	}
}

double line_len (int lnum) {
	double dx = myLINES[lnum].x2 - myLINES[lnum].x1;
	double dy = myLINES[lnum].y2 - myLINES[lnum].y1;
	double len = sqrt(dx * dx + dy * dy);
	return len;
}

double line_angle (int lnum) {
	double dx = myLINES[lnum].x2 - myLINES[lnum].x1;
	double dy = myLINES[lnum].y2 - myLINES[lnum].y1;
	double alpha = toDeg(atan(dy / dx));
	if (dx < 0 && dy >= 0) {
		alpha = alpha + 180;
	} else if (dx < 0 && dy < 0) {
		alpha = alpha - 180;
	}
	return alpha;
}

void add_angle_offset (double *check_x, double *check_y, double radius, double alpha) {
	double angle = toRad(alpha);
	*check_x += radius * cos(angle);
	*check_y += radius * sin(angle);
}

/* optimize dir of object / inside=cw, outside=ccw */
void object_optimize_dir (int object_num) {
	int pipret = 0;
	if (myOBJECTS[object_num].line[0] != 0) {
		if (myOBJECTS[object_num].closed == 1) {
			int lnum = myOBJECTS[object_num].line[0];
			double alpha = line_angle(lnum);
			double len = line_len(lnum);
			double check_x = myLINES[lnum].x1;
			double check_y = myLINES[lnum].y1;
			add_angle_offset(&check_x, &check_y, len / 2.0, alpha);
			add_angle_offset(&check_x, &check_y, 0.001, alpha + 90);
			pipret = point_in_object(object_num, -1, check_x, check_y);
			if ((pipret == 0 && myOBJECTS[object_num].inside == 0) || (pipret == 1 && myOBJECTS[object_num].inside == 1)) {
			} else {
				redir_object(object_num);
			}
		}
	}
}

void intersect (double l1x1, double l1y1, double l1x2, double l1y2, double l2x1, double l2y1, double l2x2, double l2y2, double *x, double *y) {
	double a1 = l1x2 - l1x1;
	double b1 = l2x1 - l2x2;
	double c1 = l2x1 - l1x1;
	double a2 = l1y2 - l1y1;
	double b2 = l2y1 - l2y2;
	double c2 = l2y1 - l1y1;
	double t = (b1 * c2 - b2 * c1) / (a2 * b1 - a1 * b2);
	*x = l1x1 + t * (l1x2 - l1x1);
	*y = l1y1 + t * (l1y2 - l1y1);
	return;
}

void DrawLine (float x1, float y1, float x2, float y2, float z, float w) {
	float angle = atan2(y2 - y1, x2 - x1);
	float t2sina1 = w / 2 * sin(angle);
	float t2cosa1 = w / 2 * cos(angle);
	glBegin(GL_QUADS);
	glVertex3f(x1 + t2sina1, y1 - t2cosa1, z);
	glVertex3f(x2 + t2sina1, y2 - t2cosa1, z);
	glVertex3f(x2 - t2sina1, y2 + t2cosa1, z);
	glVertex3f(x1 - t2sina1, y1 + t2cosa1, z);
	glEnd();
}

void DrawArrow (float x1, float y1, float x2, float y2, float z, float w) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float len = sqrt(dx * dx + dy * dy);
	float asize = 2.0;
	if (len < asize) {
		return;
	}
	float angle = atan2(dy, dx);
	float off_x = asize * cos(angle + toRad(45.0 + 90.0));
	float off_y = asize * sin(angle + toRad(45.0 + 90.0));
	float off2_x = asize * cos(angle + toRad(-45.0 - 90.0));
	float off2_y = asize * sin(angle + toRad(-45.0 - 90.0));
	float half_x = x1 + (x2 - x1) / 2.0;
	float half_y = y1 + (y2 - y1) / 2.0;
	glBegin(GL_LINES);
	glVertex3f(half_x, half_y, z);
	glVertex3f(half_x + off_x, half_y + off_y, z);
	glVertex3f(half_x, half_y, z);
	glVertex3f(half_x + off2_x, half_y + off2_y, z);
	glEnd();
}

void draw_line (float x1, float y1, float z1, float x2, float y2, float z2, float width) {
	if (PARAMETER[P_V_HELPDIA].vint == 1) {
		glColor4f(1.0, 1.0, 1.0, 1.0);
		DrawLine(x1, y1, x2, y2, z1, width);
		GLUquadricObj *quadric=gluNewQuadric();
		gluQuadricNormals(quadric, GLU_SMOOTH);
		gluQuadricOrientation(quadric,GLU_OUTSIDE);
		glPushMatrix();
		glTranslatef(x1, y1, z1);
		gluDisk(quadric, 0.0, width / 2.0, 18, 1);
		glPopMatrix();
		glPushMatrix();
		glTranslatef(x2, y2, z1);
		gluDisk(quadric, 0.0, width / 2.0, 18, 1);
		glPopMatrix();
		gluDeleteQuadric(quadric);
	}
	glColor4f(0.0, 0.0, 1.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(x1, y1, z1 + 0.02);
	glVertex3f(x2, y2, z2 + 0.02);
	glEnd();
	DrawArrow(x1, y1, x2, y2, z1 + 0.02, width);
}

void draw_line2 (float x1, float y1, float z1, float x2, float y2, float z2, float width) {
	if (PARAMETER[P_V_HELPLINES].vint == 1) {
		DrawLine(x1, y1, x2, y2, z1, width);
		GLUquadricObj *quadric=gluNewQuadric();
		gluQuadricNormals(quadric, GLU_SMOOTH);
		gluQuadricOrientation(quadric,GLU_OUTSIDE);
		glPushMatrix();
		glTranslatef(x1, y1, z1);
		gluDisk(quadric, 0.0, width / 2.0, 18, 1);
		glPopMatrix();
		glPushMatrix();
		glTranslatef(x2, y2, z1);
		gluDisk(quadric, 0.0, width / 2.0, 18, 1);
		glPopMatrix();
		gluDeleteQuadric(quadric);
	}
	glBegin(GL_LINES);
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y2, z2);
	glEnd();
}

void object_draw (FILE *fd_out, int object_num) {
	int num = 0;
	int num2 = 0;
	int last = 0;
	int lasermode = 0;
	// real milling depth
	double mill_depth_real = PARAMETER[P_M_DEPTH].vdouble;
	if (strncmp(myOBJECTS[object_num].layer, "depth-", 6) == 0) {
		mill_depth_real = atof(myOBJECTS[object_num].layer + 5);
	}
	if (strncmp(myOBJECTS[object_num].layer, "laser", 5) == 0) {
		lasermode = 1;
	} else {
		lasermode = object_laser[object_num];
	}
	for (num2 = 1; num2 < 100; num2++) {
		if (shapeEV[num2].Label != NULL && strcmp(shapeEV[num2].Label, myOBJECTS[object_num].layer) == 0) {
			if (layer_force[num2] == 0) {
				layer_depth[num2] = mill_depth_real;
			} else {
				mill_depth_real = layer_depth[num2];
			}
		}
	}
	/* find last line in object */
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			last = myOBJECTS[object_num].line[num];
		}
	}
/*
	if (object_pocket[object_num] == 1 && myOBJECTS[object_num].closed == 1 && myOBJECTS[object_num].inside == 1) {
		double lmx = 0.0;
		double lmy = 0.0;
		double fdmx = 0.0;
		double fdmy = 0.0;
		double ldx = 0.0;
		double ldy = 0.0;
		double pmx = 0.0;
		double pmy = 0.0;
		int pipret = 0;
		int last_ret = 0;
		float ystep = PARAMETER[P_TOOL_DIAMETER].vdouble / 2.0;
		float xstep = PARAMETER[P_TOOL_DIAMETER].vdouble / 2.0;
		for (pmy = -10.0; pmy < 260.0; pmy += ystep) {
			for (pmx = -10.0; pmx < 220.0; pmx += xstep) {
				pipret = point_in_object(object_num, -1, pmx, pmy);
				if (pipret == 0) {
					pipret = point_in_object(-1, object_num, pmx, pmy);
					float an = 0;
					for (an = 0.0; an < 360.0; an += 3.6) {
						float rangle = toRad(an);
						float rx = (PARAMETER[P_TOOL_DIAMETER].vdouble / 3.0 * 2.0) * cos(rangle);
						float ry = (PARAMETER[P_TOOL_DIAMETER].vdouble / 3.0 * 2.0) * sin(rangle);
						pipret += point_in_object(object_num, -1, pmx + rx, pmy + ry);
					}
					if (pipret == 0) {
						if (last_ret == 0) {
							fdmx = pmx;
							fdmy = pmy;
						}
						lmx = pmx;
						lmy = pmy;
						last_ret = 1;
					} else {
						if (last_ret == 1) {
							draw_line(ldx, ldy, 0.0, fdmx, fdmy, 0.0, 2.0);
							draw_line(fdmx, fdmy, 0.0, lmx, lmy, 0.0, 2.0);
							ldx = lmx;
							ldy = lmy;
						}
						last_ret = 0;
					}
				} else {
					if (last_ret == 1) {
						draw_line(ldx, ldy, 0.0, fdmx, fdmy, 0.0, 2.0);
						draw_line(fdmx, fdmy, 0.0, lmx, lmy, 0.0, 2.0);
						ldx = lmx;
						ldy = lmy;
					}
					last_ret = 0;
				}
			}
			pmy += ystep;
			for (pmx = 220.0; pmx > -10.0; pmx -= xstep) {
				pipret = point_in_object(object_num, -1, pmx, pmy);
				if (pipret == 0) {
					pipret = point_in_object(-1, object_num, pmx, pmy);
					float an = 0;
					for (an = 0.0; an < 360.0; an += 3.6) {
						float rangle = toRad(an);
						float rx = (PARAMETER[P_TOOL_DIAMETER].vdouble / 3.0 * 2.0) * cos(rangle);
						float ry = (PARAMETER[P_TOOL_DIAMETER].vdouble / 3.0 * 2.0) * sin(rangle);
						pipret += point_in_object(object_num, -1, pmx + rx, pmy + ry);
					}
					if (pipret == 0) {
						if (last_ret == 0) {
							fdmx = pmx;
							fdmy = pmy;
						}
						lmx = pmx;
						lmy = pmy;
						last_ret = 1;
					} else {
						if (last_ret == 1) {
							draw_line(ldx, ldy, 0.0, fdmx, fdmy, 0.0, 2.0);
							draw_line(fdmx, fdmy, 0.0, lmx, lmy, 0.0, 2.0);
							ldx = lmx;
							ldy = lmy;
						}
						last_ret = 0;
					}
				} else {
					if (last_ret == 1) {
						draw_line(ldx, ldy, 0.0, fdmx, fdmy, 0.0, 2.0);
						draw_line(fdmx, fdmy, 0.0, lmx, lmy, 0.0, 2.0);
						ldx = lmx;
						ldy = lmy;
					}
					last_ret = 0;
				}
			}
		}
	}
*/

	if (object_selections[object_num] == 0) {
		return;
	}

	if (PARAMETER[P_V_NCDEBUG].vint == 1) {
		if (save_gcode == 1) {
			fprintf(fd_out, "\n");
			fprintf(fd_out, "(--------------------------------------------------)\n");
			fprintf(fd_out, "(Object: #%i)\n", object_num);
			fprintf(fd_out, "(Layer: %s)\n", myOBJECTS[object_num].layer);
			if (lasermode == 1) {
				fprintf(fd_out, "(Laser-Mode: On)\n");
			} else { 
				fprintf(fd_out, "(Depth: %f)\n", mill_depth_real);
			}
			fprintf(fd_out, "(--------------------------------------------------)\n");
			fprintf(fd_out, "\n");
		}
	}
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			int lnum = myOBJECTS[object_num].line[num];
			glColor4f(1.0, 0.0, 0.0, 1.0);
			glBegin(GL_LINES);
			glVertex3f((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, 0.0);
			glVertex3f((float)myLINES[lnum].x2, (float)myLINES[lnum].y2, 0.0);
			glEnd();
			if (myOBJECTS[object_num].closed == 0) {
				draw_line2((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, 0.01, (float)myLINES[lnum].x2, (float)myLINES[lnum].y2, 0.01, (PARAMETER[P_TOOL_DIAMETER].vdouble));
			}
			if (PARAMETER[P_V_NCDEBUG].vint == 1) {
				if (save_gcode == 1) {
					if (num == 0) {
						if (lasermode == 1) {
							if (tool_last != 5) {
								fprintf(fd_out, "M06 T%i (Change-Tool / Laser-Mode)\n", 5);
							}
							tool_last = 5;
						}
						fprintf(fd_out, "G00 X%f Y%f F%i (%s / %f - no-offset)\n", myLINES[lnum].x1, myLINES[lnum].y1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum].type], myLINES[lnum].opt);
						if (lasermode == 1) {
							fprintf(fd_out, "G00 Z0.0\n");
							fprintf(fd_out, "M03 (Laser-On)\n");
						}
					}
					if (myLINES[lnum].type == TYPE_ARC || myLINES[lnum].type == TYPE_CIRCLE) {
						if (myLINES[lnum].opt < 0) {
							fprintf(fd_out, "G02 X%f Y%f R%f F%i (aa %s / %f)\n", myLINES[lnum].x2, myLINES[lnum].y2, myLINES[lnum].opt * -1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum].type], myLINES[lnum].opt);
						} else {
							fprintf(fd_out, "G03 X%f Y%f R%f F%i (bb %s / %f)\n", myLINES[lnum].x2, myLINES[lnum].y2, myLINES[lnum].opt, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum].type], myLINES[lnum].opt);//
						}
					} else {
						fprintf(fd_out, "G01 X%f Y%f F%i (%s / %f - no-offset)\n", myLINES[lnum].x2, myLINES[lnum].y2, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum].type], myLINES[lnum].opt);
					}
				}
			}
			if (num == 0) {
				glColor4f(1.0, 1.0, 1.0, 1.0);
				char tmp_str[1024];
				sprintf(tmp_str, "%i", object_num);
				draw_text(GLUT_BITMAP_HELVETICA_18, tmp_str, (float)myLINES[lnum].x1, (float)myLINES[lnum].y1);
			}
		}
	}
	if (PARAMETER[P_V_HELPLINES].vint == 1) {
		if (myOBJECTS[object_num].closed == 1 && myOBJECTS[object_num].inside == 0) {
			object2poly(object_num, 0.0, mill_depth_real);
		}
	}
}

void output_text (char *text) {
	draw_text(GLUT_BITMAP_HELVETICA_18, text, 0.0, 0.0);
}

void object_draw_offset (FILE *fd_out, int object_num, double *next_x, double *next_y) {
	int error = 0;
	int lnum1 = 0;
	int lnum2 = 0;
	int num = 0;
	int num2 = 0;
	int last = 0;
	int last_lnum = 0;
	int reverse = 0;
	double first_x = 0.0;
	double first_y = 0.0;
	double last_x = 0.0;
	double last_y = 0.0;
	double depth = 0.0;
	double tool_offset = 0.0;
	int overcut = 0;
	int lasermode = 0;

	if (PARAMETER[P_M_OVERCUT].vint == 1) {
		overcut = 1;
	}
	if (PARAMETER[P_M_LASERMODE].vint == 1) {
		lasermode = 1;
	}

	// real milling depth
	double mill_depth_real = PARAMETER[P_M_DEPTH].vdouble;
	if (strncmp(myOBJECTS[object_num].layer, "depth-", 6) == 0) {
		mill_depth_real = atof(myOBJECTS[object_num].layer + 5);
	}
	if (strncmp(myOBJECTS[object_num].layer, "laser", 5) == 0) {
		lasermode = 1;
	}
	for (num2 = 1; num2 < 100; num2++) {
		if (shapeEV[num2].Label != NULL && strcmp(shapeEV[num2].Label, myOBJECTS[object_num].layer) == 0) {
			if (layer_force[num2] == 0) {
				layer_depth[num2] = mill_depth_real;
			} else {
				mill_depth_real = layer_depth[num2];
			}
		}
	}
	int offset = 0;
	if (myOBJECTS[object_num].closed == 1) {
		if (myOBJECTS[object_num].inside == 1) {
			offset = 1;
		} else {
			offset = 2;
		}
	} else {
		offset = 0;
	}

	if (object_force[object_num] == 1) {
		mill_depth_real = object_depth[object_num];
		if (object_offset[object_num] == 1) {
			redir_object(object_num);
		} else if (object_offset[object_num] == 2) {
			offset = 0;
		}
		overcut = object_overcut[object_num];
		lasermode = object_laser[object_num];
	} else {
		object_depth[object_num] = mill_depth_real;
		object_overcut[object_num] = overcut;
		object_laser[object_num] = lasermode;
	}
	if (lasermode == 1) {
		tool_offset = PARAMETER[P_TOOL_LASERDIA].vdouble / 2.0;
		mill_depth_real = 0.0;
	} else {
		tool_offset = PARAMETER[P_TOOL_DIAMETER].vdouble / 2.0;
	}


	if (object_selections[object_num] == 0) {
		return;
	}

	if (save_gcode == 1) {
		fprintf(fd_out, "\n");
		fprintf(fd_out, "(--------------------------------------------------)\n");
		fprintf(fd_out, "(Object: #%i)\n", object_num);
		fprintf(fd_out, "(Layer: %s)\n", myOBJECTS[object_num].layer);
		fprintf(fd_out, "(Overcut: %i)\n",  overcut);
		if (lasermode == 1) {
			fprintf(fd_out, "(Laser-Mode: On)\n");
		} else { 
			fprintf(fd_out, "(Depth: %f)\n", mill_depth_real);
		}
		if (offset == 0) {
			fprintf(fd_out, "(Offset: None)\n");
		} else if (offset == 1) {
			fprintf(fd_out, "(Offset: Inside)\n");
		} else {
			fprintf(fd_out, "(Offset: Outside)\n");
		}
		fprintf(fd_out, "(--------------------------------------------------)\n");
		fprintf(fd_out, "\n");
	}



	/* find last line in object */
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			last = myOBJECTS[object_num].line[num];
		}
	}

	if (save_gcode == 1) {
		fprintf(fd_out, " O%04i sub\n", object_num);
	}

	glNewList(1, GL_COMPILE);
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			if (num == 0) {
				lnum1 = last;
			} else {
				lnum1 = last_lnum;
			}
			lnum2 = myOBJECTS[object_num].line[num];
			if (myOBJECTS[object_num].closed == 1 && offset != 0) {
				// line1 Offsets & Angle
				double alpha1 = line_angle(lnum1);
//				double len1 = line_len(lnum1);
				double check1_x = myLINES[lnum1].x1;
				double check1_y = myLINES[lnum1].y1;
//				add_angle_offset(&check1_x, &check1_y, len1 / 2.0, alpha1);
				if (reverse == 1) {
					add_angle_offset(&check1_x, &check1_y, tool_offset, alpha1 + 90);
				} else {
					add_angle_offset(&check1_x, &check1_y, tool_offset, alpha1 - 90);
				}
				double check1b_x = myLINES[lnum1].x2;
				double check1b_y = myLINES[lnum1].y2;
				add_angle_offset(&check1b_x, &check1b_y, 0.0, alpha1);
				if (reverse == 1) {
					add_angle_offset(&check1b_x, &check1b_y, tool_offset, alpha1 + 90);
				} else {
					add_angle_offset(&check1b_x, &check1b_y, tool_offset, alpha1 - 90);
				}

				// line2 Offsets & Angle
				double alpha2 = line_angle(lnum2);
//				double len2 = line_len(lnum2);
				double check2_x = myLINES[lnum2].x1;
				double check2_y = myLINES[lnum2].y1;
				add_angle_offset(&check2_x, &check2_y, 0.0, alpha2);
				if (reverse == 1) {
					add_angle_offset(&check2_x, &check2_y, tool_offset, alpha2 + 90);
				} else {
					add_angle_offset(&check2_x, &check2_y, tool_offset, alpha2 - 90);
				}
				double check2b_x = myLINES[lnum2].x2;
				double check2b_y = myLINES[lnum2].y2;
//				add_angle_offset(&check2b_x, &check2b_y, len2 / 2.0, alpha2 - 180);
				if (reverse == 1) {
					add_angle_offset(&check2b_x, &check2b_y, tool_offset, alpha2 + 90);
				} else {
					add_angle_offset(&check2b_x, &check2b_y, tool_offset, alpha2 - 90);
				}
				// Angle-Diff
				alpha1 = alpha1 + 180.0;
				alpha2 = alpha2 + 180.0;
				double alpha_diff = alpha2 - alpha1;
				if (alpha_diff < 0.0) {
					alpha_diff += 360.0;
				}
				if (alpha_diff > 360.0) {
					alpha_diff -= 360.0;
				}

				if (alpha_diff == 0.0) {
				} else if (alpha_diff <= 180.0) {
					// Aussenkante
					if (num != 0) {
						draw_line((float)last_x, (float)last_y, 0.0, (float)check1b_x, (float)check1b_y, 0.0, (tool_offset * 2.0));
					}
					last_x = check1b_x;
					last_y = check1b_y;
					double an = 0;
					for (an = alpha1 + 90.0; an < alpha1 + 90.0 + alpha_diff; an += 1.0) {
						double rangle = toRad(an);
						double rx = tool_offset * cos(rangle);
						double ry = tool_offset * sin(rangle);
						draw_line((float)last_x, (float)last_y, 0.0, (float)myLINES[lnum1].x2 + rx, (float)myLINES[lnum1].y2 + ry, 0.0, (tool_offset * 2.0));
						last_x = myLINES[lnum1].x2 + rx;
						last_y = myLINES[lnum1].y2 + ry;
					}
					draw_line((float)last_x, (float)last_y, 0.0, (float)check2_x, (float)check2_y, 0.0, (tool_offset * 2.0));
					if (num == 0) {
						first_x = check1b_x;
						first_y = check1b_y;
						if (save_gcode == 1) {
							if (reverse == 1) {
								fprintf(fd_out, "  G02 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
							} else {
								fprintf(fd_out, "  G03 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
							}
						}
						last_x = check2_x;
						last_y = check2_y;
					} else {
						if (save_gcode == 1) {
							if (myLINES[lnum1].type == TYPE_ARC || myLINES[lnum1].type == TYPE_CIRCLE) {
								if (myLINES[lnum1].opt < 0) {
									fprintf(fd_out, "  G02 X%f Y%f R%f F%i (aa %s / %f)\n", check1b_x, check1b_y, (myLINES[lnum1].opt + tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								} else {
									fprintf(fd_out, "  G03 X%f Y%f R%f F%i (bb %s / %f)\n", check1b_x, check1b_y, (myLINES[lnum1].opt + tool_offset), PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);//
								}
								if (myLINES[lnum2].type == TYPE_ARC || myLINES[lnum2].type == TYPE_CIRCLE) {
								} else {
									if (reverse == 1) {
										fprintf(fd_out, "  G02 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
									} else {
										fprintf(fd_out, "  G03 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
									}
								}
							} else {
								fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", check1b_x, check1b_y, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								if (reverse == 1) {
									fprintf(fd_out, "  G02 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								} else {
									fprintf(fd_out, "  G03 X%f Y%f R%f F%i (%s / %f)\n", check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								}
							}
						}
						last_x = check2_x;
						last_y = check2_y;
					}
				} else {
					// Innenkante
					if (myLINES[lnum2].type == TYPE_ARC || myLINES[lnum2].type == TYPE_CIRCLE) {
						if (myLINES[lnum2].opt < 0 && myLINES[lnum2].opt * -1 < tool_offset) {
							error = 1;
							break;
						}
					}
					double px = 0.0;
					double py = 0.0;
					intersect(check1_x, check1_y, check1b_x, check1b_y, check2_x, check2_y, check2b_x, check2b_y, &px, &py);
					double enx = px;
					double eny = py;
					if (num == 0) {
						first_x = px;
						first_y = py;
						last_x = px;
						last_y = py;
						if (overcut == 1 && myLINES[lnum1].type == TYPE_LINE && myLINES[lnum2].type == TYPE_LINE) {
							double adx = myLINES[lnum2].x1 - px;
							double ady = myLINES[lnum2].y1 - py;
							double aalpha = toDeg(atan(ady / adx));
							if (adx < 0 && ady >= 0) {
								aalpha = aalpha + 180;
							} else if (adx < 0 && ady < 0) {
								aalpha = aalpha - 180;
							}
							double len = sqrt(adx * adx + ady * ady);
							add_angle_offset(&enx, &eny, len - tool_offset, aalpha);

							draw_line((float)last_x, (float)last_y, 0.0, (float)enx, (float)eny, 0.0, (tool_offset * 2.0));
							last_x = enx;
							last_y = eny;
							draw_line((float)last_x, (float)last_y, 0.0, (float)px, (float)py, 0.0, (tool_offset * 2.0));
							last_x = px;
							last_y = py;

							if (save_gcode == 1) {
								fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f - ecke ausfraesen)\n", enx, eny, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", px, py, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
							}

						}
					} else {
						draw_line((float)last_x, (float)last_y, 0.0, (float)px, (float)py, 0.0, (tool_offset * 2.0));
						last_x = px;
						last_y = py;
						if (overcut == 1 && myLINES[lnum1].type == TYPE_LINE && myLINES[lnum2].type == TYPE_LINE) {
							double adx = myLINES[lnum1].x2 - px;
							double ady = myLINES[lnum1].y2 - py;
							double aalpha = toDeg(atan(ady / adx));
							if (adx < 0 && ady >= 0) {
								aalpha = aalpha + 180;
							} else if (adx < 0 && ady < 0) {
								aalpha = aalpha - 180;
							}
							double len = sqrt(adx * adx + ady * ady);
							add_angle_offset(&enx, &eny, len - tool_offset, aalpha);

							draw_line((float)last_x, (float)last_y, 0.0, (float)enx, (float)eny, 0.0, (tool_offset * 2.0));
							last_x = enx;
							last_y = eny;
							draw_line((float)last_x, (float)last_y, 0.0, (float)px, (float)py, 0.0, (tool_offset * 2.0));
							last_x = px;
							last_y = py;
						}
						if (save_gcode == 1) {
							if (myLINES[lnum1].type == TYPE_ARC || myLINES[lnum1].type == TYPE_CIRCLE) {
								if (myLINES[lnum1].opt < 0) {
									fprintf(fd_out, "  G02 X%f Y%f R%f F%i (aa %s / %f)\n", px, py, (myLINES[lnum1].opt + tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								} else {
									fprintf(fd_out, "  G03 X%f Y%f R%f F%i (bb %s / %f)\n", px, py, (myLINES[lnum1].opt + tool_offset), PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);//
								}
							} else {
								fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", px, py, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								if (overcut == 1 && myLINES[lnum1].type == TYPE_LINE && myLINES[lnum2].type == TYPE_LINE) {
									fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f - ecke ausfraesen)\n", enx, eny, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
									fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", px, py, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum1].type], myLINES[lnum1].opt);
								}
							}
						}
					}
				}
			} else {
				if (num == 0) {
					draw_line((float)myLINES[lnum2].x1, (float)myLINES[lnum2].y1, 0.0, (float)myLINES[lnum2].x2, (float)myLINES[lnum2].y2, 0.0, (tool_offset * 2.0));
					if (save_gcode == 1) {
						fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", myLINES[lnum2].x1, myLINES[lnum2].y1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum2].type], myLINES[lnum2].opt);
					}
					first_x = myLINES[lnum2].x1;
					first_y = myLINES[lnum2].y1;
				} else {
					draw_line((float)last_x, (float)last_y, 0.0, (float)myLINES[lnum2].x2, (float)myLINES[lnum2].y2, 0.0, (tool_offset * 2.0));
				}
				if (save_gcode == 1) {
					if (myLINES[lnum2].type == TYPE_ARC || myLINES[lnum2].type == TYPE_CIRCLE) {
						if (myLINES[lnum2].opt < 0) {
							fprintf(fd_out, "  G02 X%f Y%f R%f F%i (aa %s / %f)\n", myLINES[lnum2].x2, myLINES[lnum2].y2, myLINES[lnum2].opt * -1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum2].type], myLINES[lnum2].opt);
						} else {
							fprintf(fd_out, "  G03 X%f Y%f R%f F%i (bb %s / %f)\n", myLINES[lnum2].x2, myLINES[lnum2].y2, myLINES[lnum2].opt, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum2].type], myLINES[lnum2].opt);//
						}
					} else {
						fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", myLINES[lnum2].x2, myLINES[lnum2].y2, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[lnum2].type], myLINES[lnum2].opt);
					}
				}
				last_x = myLINES[lnum2].x2;
				last_y = myLINES[lnum2].y2;
			}
			last_lnum = lnum2;
		}
	}
	if (myOBJECTS[object_num].closed == 1) {
		draw_line((float)last_x, (float)last_y, 0.0, (float)first_x, (float)first_y, 0.0, (tool_offset * 2.0));
		if (save_gcode == 1) {
			if (myLINES[last].type == TYPE_ARC || myLINES[last].type == TYPE_CIRCLE) {
				if (myLINES[last].opt < 0) {
					fprintf(fd_out, "  G02 X%f Y%f R%f F%i (aa %s / %f)\n", first_x, first_y, (myLINES[last].opt + tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[last].type], myLINES[last].opt);
				} else {
					fprintf(fd_out, "  G03 X%f Y%f R%f F%i (bb %s / %f)\n", first_x, first_y, (myLINES[last].opt + tool_offset), PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[last].type], myLINES[last].opt);//
				}
			} else {
				fprintf(fd_out, "  G01 X%f Y%f F%i (%s / %f)\n", first_x, first_y, PARAMETER[P_M_FEEDRATE].vint, dxf_typename[myLINES[last].type], myLINES[last].opt);
			}
		}
		last_x = first_x;
		last_y = first_y;
	}
	glEndList();


	if (save_gcode == 1) {
		fprintf(fd_out, " O%04i endsub\n", object_num);
		fprintf(fd_out, "\n");
	}

	if (error > 0) {
		return;
	}

	// move to
	if (save_gcode == 1) {
		if (lasermode == 1) {
			if (tool_last != 5) {
				fprintf(fd_out, "M06 T%i (Change-Tool / Laser-Mode)\n", 5);
				fprintf(fd_out, "G00 Z%f\n", PARAMETER[P_CUT_SAVE].vdouble);
			}
			tool_last = 5;
			fprintf(fd_out, "G00 X%f Y%f\n", first_x, first_y);
			fprintf(fd_out, "G00 Z%f\n", 0.0);
			fprintf(fd_out, "M03 (Laser-On)\n");
		} else {
			if (tool_last != PARAMETER[P_TOOL_NUM].vint) {
				fprintf(fd_out, "M06 T%i (Change-Tool)\n", PARAMETER[P_TOOL_NUM].vint);
			}
			tool_last = PARAMETER[P_TOOL_NUM].vint;
			fprintf(fd_out, "M03 S%i (Spindle-On / CW)\n", PARAMETER[P_TOOL_SPEED_MAX].vint);
			fprintf(fd_out, "G00 Z%f\n", PARAMETER[P_CUT_SAVE].vdouble);
			fprintf(fd_out, "G00 X%f Y%f\n", first_x, first_y);
		}
	}
	glColor3f(0.0, 1.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(*next_x, *next_y, PARAMETER[P_CUT_SAVE].vdouble);
	glVertex3f(first_x, first_y, PARAMETER[P_CUT_SAVE].vdouble);
	glEnd();

	// move in
	glBegin(GL_LINES);
	glVertex3f(first_x, first_y, PARAMETER[P_CUT_SAVE].vdouble);
	glVertex3f(first_x, first_y, mill_depth_real);
	glEnd();

	// offset for each depth-step
	for (depth = PARAMETER[P_M_Z_STEP].vdouble; depth > mill_depth_real; depth += PARAMETER[P_M_Z_STEP].vdouble) {
		if (save_gcode == 1) {
			fprintf(fd_out, "G01 Z%f F%i\n", depth, PARAMETER[P_M_PLUNGE_SPEED].vint);
			fprintf(fd_out, "O%04i call\n", object_num);
			fprintf(fd_out, "\n");
		}
		glTranslatef(0.0, 0.0, depth);
		glCallList(1);
		glTranslatef(0.0, 0.0, -depth);
		if (myOBJECTS[object_num].closed == 0) {
			// back to start on open lines
			if (save_gcode == 1) {
				fprintf(fd_out, "G00 Z%f\n", PARAMETER[P_CUT_SAVE].vdouble);
				fprintf(fd_out, "G00 X%f Y%f\n", first_x, first_y);
			}
			glBegin(GL_LINES);
			glVertex3f(last_x, last_y, PARAMETER[P_CUT_SAVE].vdouble);
			glVertex3f(first_x, first_y, PARAMETER[P_CUT_SAVE].vdouble);
			glEnd();
		}
	}
	depth = mill_depth_real;
	if (save_gcode == 1) {
		fprintf(fd_out, "G01 Z%f F%i\n", depth, PARAMETER[P_M_PLUNGE_SPEED].vint);
		fprintf(fd_out, "O%04i call\n", object_num);
		fprintf(fd_out, "\n");
	}
	glTranslatef(0.0, 0.0, depth);
	glCallList(1);
	glTranslatef(0.0, 0.0, -depth);

	// move out
	if (save_gcode == 1) {
		if (lasermode == 1) {
			fprintf(fd_out, "M05 (Laser-Off)\n");
			fprintf(fd_out, "\n");
		} else {
			fprintf(fd_out, "G00 Z%f\n", PARAMETER[P_CUT_SAVE].vdouble);
			fprintf(fd_out, "\n");
		}
	}
	glBegin(GL_LINES);
	glVertex3f(last_x, last_y, mill_depth_real);
	glVertex3f(last_x, last_y, PARAMETER[P_CUT_SAVE].vdouble);
	glEnd();

	*next_x = last_x;
	*next_y = last_y;

	glDeleteLists(1, 1);
}

int find_next_line (int object_num, int first, int num, int dir, int depth) {
	int fnum = 0;
	int num4 = 0;
	int num5 = 0;
	double px = 0;
	double py = 0;
	int ret = 0;
	if (dir == 0) {
		px = myLINES[num].x1;
		py = myLINES[num].y1;
	} else {
		px = myLINES[num].x2;
		py = myLINES[num].y2;
	}
//	for (num4 = 0; num4 < depth; num4++) {
//		printf(" ");
//	}
	for (num5 = 0; num5 < MAX_OBJECTS; num5++) {
		if (myOBJECTS[num5].line[0] == 0) {
			break;
		}
		for (num4 = 0; num4 < line_last; num4++) {
			if (myOBJECTS[num5].line[num4] == num) {
//				printf("##LINE %i in OBJECT %i / %i\n", num, num5, num4);
				return 2;
			}
		}
	}
	for (num4 = 0; num4 < line_last; num4++) {
		if (myOBJECTS[object_num].line[num4] == 0) {
//			printf("##ADD LINE %i to OBJECT %i / %i\n", num, object_num, num4);
			myOBJECTS[object_num].line[num4] = num;
			strcpy(myOBJECTS[object_num].layer, myLINES[num].layer);
			break;
		}
	}
	int num2 = 0;

	fnum = 0;
	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1 && num != num2 && strcmp(myLINES[num2].layer, myLINES[num].layer) == 0) {
			if (px >= myLINES[num2].x1 - FUZZY && px <= myLINES[num2].x1 + FUZZY && py >= myLINES[num2].y1 - FUZZY && py <= myLINES[num2].y1 + FUZZY) {
//				printf("###### %i NEXT LINE: %f,%f -> %f,%f START\n", depth, myLINES[num2].x1, myLINES[num2].y1, myLINES[num2].x2, myLINES[num2].y2);
				fnum++;
			} else if (px >= myLINES[num2].x2 - FUZZY && px <= myLINES[num2].x2 + FUZZY && py >= myLINES[num2].y2 - FUZZY && py <= myLINES[num2].y2 + FUZZY) {
//				printf("###### %i NEXT LINE: %f,%f -> %f,%f END\n", depth, myLINES[num2].x1, myLINES[num2].y1, myLINES[num2].x2, myLINES[num2].y2);
				fnum++;
			}
		}
	}

//printf("## FNUM: %i  %i \n", fnum, num);

	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1 && num != num2 && strcmp(myLINES[num2].layer, myLINES[num].layer) == 0) {
			if (px >= myLINES[num2].x1 - FUZZY && px <= myLINES[num2].x1 + FUZZY && py >= myLINES[num2].y1 - FUZZY && py <= myLINES[num2].y1 + FUZZY) {
//				printf("###### %i NEXT LINE: %f,%f -> %f,%f START\n", depth, myLINES[num2].x1, myLINES[num2].y1, myLINES[num2].x2, myLINES[num2].y2);
				if (num2 != first) {
					ret = find_next_line(object_num, first, num2, 1, depth + 1);
					if (ret == 1) {
						return 1;
					}
				} else {
//					printf("###### OBJECT CLOSED\n");
					return 1;
				}
			} else if (px >= myLINES[num2].x2 - FUZZY && px <= myLINES[num2].x2 + FUZZY && py >= myLINES[num2].y2 - FUZZY && py <= myLINES[num2].y2 + FUZZY) {
				line_invert(num2);
//				printf("###### %i NEXT LINE: %f,%f -> %f,%f END\n", depth, myLINES[num2].x1, myLINES[num2].y1, myLINES[num2].x2, myLINES[num2].y2);
				if (num2 != first) {
					ret = find_next_line(object_num, first, num2, 1, depth + 1);
					if (ret == 1) {
						return 1;
					}
				} else {
//					printf("###### OBJECT CLOSED\n");
					return 1;
				}
			}
		}
	}
	return ret;
}

int line_open_check (int num) {
	int ret = 0;
	int dir = 0;
	int num2 = 0;
	int onum = 0;
	double px = 0.0;
	double py = 0.0;
	for (onum = 0; onum < MAX_OBJECTS; onum++) {
		if (myOBJECTS[onum].line[0] == 0) {
			break;
		}
		for (num2 = 0; num2 < line_last; num2++) {
			if (myOBJECTS[onum].line[num2] == num) {
//				printf("##LINE %i in OBJECT %i / %i\n", num, onum, num2);
				return 0;
			}
		}
	}
	px = myLINES[num].x1;
	py = myLINES[num].y1;
	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1 && num != num2 && strcmp(myLINES[num2].layer, myLINES[num].layer) == 0) {
			if (px >= myLINES[num2].x1 - FUZZY && px <= myLINES[num2].x1 + FUZZY && py >= myLINES[num2].y1 - FUZZY && py <= myLINES[num2].y1 + FUZZY) {
				ret++;
				dir = 1;
				break;
			} else if (px >= myLINES[num2].x2 - FUZZY && px <= myLINES[num2].x2 + FUZZY && py >= myLINES[num2].y2 - FUZZY && py <= myLINES[num2].y2 + FUZZY) {
				ret++;
				dir = 1;
				break;
			}
		}
	}
	px = myLINES[num].x2;
	py = myLINES[num].y2;
	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1 && num != num2 && strcmp(myLINES[num2].layer, myLINES[num].layer) == 0) {
			if (px >= myLINES[num2].x1 - FUZZY && px <= myLINES[num2].x1 + FUZZY && py >= myLINES[num2].y1 - FUZZY && py <= myLINES[num2].y1 + FUZZY) {
				ret++;
				dir = 2;
				break;
			} else if (px >= myLINES[num2].x2 - FUZZY && px <= myLINES[num2].x2 + FUZZY && py >= myLINES[num2].y2 - FUZZY && py <= myLINES[num2].y2 + FUZZY) {
				ret++;
				dir = 2;
				break;
			}
		}
	}
	if (ret == 1) {
		return dir;
	} else if (ret == 0) {
		return 3;
	}
	return 0;
}

void SetQuaternionFromAxisAngle (const float *axis, float angle, float *quat) {
	float sina2, norm;
	sina2 = (float)sin(0.5f * angle);
	norm = (float)sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
	quat[0] = sina2 * axis[0] / norm;
	quat[1] = sina2 * axis[1] / norm;
	quat[2] = sina2 * axis[2] / norm;
	quat[3] = (float)cos(0.5f * angle);
}

void ConvertQuaternionToMatrix (const float *quat, float *mat) {
	float yy2 = 2.0f * quat[1] * quat[1];
	float xy2 = 2.0f * quat[0] * quat[1];
	float xz2 = 2.0f * quat[0] * quat[2];
	float yz2 = 2.0f * quat[1] * quat[2];
	float zz2 = 2.0f * quat[2] * quat[2];
	float wz2 = 2.0f * quat[3] * quat[2];
	float wy2 = 2.0f * quat[3] * quat[1];
	float wx2 = 2.0f * quat[3] * quat[0];
	float xx2 = 2.0f * quat[0] * quat[0];
	mat[0*4+0] = - yy2 - zz2 + 1.0f;
	mat[0*4+1] = xy2 + wz2;
	mat[0*4+2] = xz2 - wy2;
	mat[0*4+3] = 0;
	mat[1*4+0] = xy2 - wz2;
	mat[1*4+1] = - xx2 - zz2 + 1.0f;
	mat[1*4+2] = yz2 + wx2;
	mat[1*4+3] = 0;
	mat[2*4+0] = xz2 + wy2;
	mat[2*4+1] = yz2 - wx2;
	mat[2*4+2] = - xx2 - yy2 + 1.0f;
	mat[2*4+3] = 0;
	mat[3*4+0] = mat[3*4+1] = mat[3*4+2] = 0;
	mat[3*4+3] = 1;
}

void MultiplyQuaternions (const float *q1, const float *q2, float *qout) {
	float qr[4];
	qr[0] = q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
	qr[1] = q1[3]*q2[1] + q1[1]*q2[3] + q1[2]*q2[0] - q1[0]*q2[2];
	qr[2] = q1[3]*q2[2] + q1[2]*q2[3] + q1[0]*q2[1] - q1[1]*q2[0];
	qr[3]  = q1[3]*q2[3] - (q1[0]*q2[0] + q1[1]*q2[1] + q1[2]*q2[2]);
	qout[0] = qr[0]; qout[1] = qr[1]; qout[2] = qr[2]; qout[3] = qr[3];
}

int GetTimeMs (void) {
	return glutGet(GLUT_ELAPSED_TIME);
}

void TW_CALL SetAutoRotateCB (const void *value, void *clientData) {
	g_AutoRotate = *(const int *)value;
	if( g_AutoRotate!=0 ) {
		g_RotateTime = GetTimeMs();
		g_RotateStart[0] = g_Rotation[0];
		g_RotateStart[1] = g_Rotation[1];
		g_RotateStart[2] = g_Rotation[2];
		g_RotateStart[3] = g_Rotation[3];
		TwDefine("Parameter/ObjRotation readonly");
	} else {
		TwDefine("Parameter/ObjRotation readwrite");
	}
}

void TW_CALL GetAutoRotateCB (void *value, void *clientData) {
	*(int *)value = g_AutoRotate;
}

void onExit (void) {
}

void init_objects (void) {
	int num2 = 0;
	int num4b = 0;
	int num5b = 0;
	int object_num = 0;

	/* init objects */
	for (object_num = 0; object_num < MAX_OBJECTS; object_num++) {
		object_selections[object_num] = 1;
		object_force[object_num] = 0;
		object_offset[object_num] = 0;
		object_overcut[object_num] = 0;
		object_pocket[object_num] = 0;
		object_laser[object_num] = 0;
		myOBJECTS[object_num].visited = 0;
		for (num2 = 0; num2 < line_last; num2++) {
			myOBJECTS[object_num].line[num2] = 0;
		}
	}

	/* first find objects on open lines */
	object_num = 0;
	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1) {
			int ends = line_open_check(num2);
			if (ends == 1) {
				line_invert(num2);
			}
			if (ends > 0) {
				find_next_line(object_num, num2, num2, 1, 0);
				myOBJECTS[object_num].closed = 0;
				object_num++;
			}
		}
	}

	/* find objects and check if open or close */
	for (num2 = 1; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1) {
			int ret = find_next_line(object_num, num2, num2, 1, 0);
			if (ret == 1) {
				myOBJECTS[object_num].closed = 1;
				object_num++;
			} else if (ret == 0) {
				myOBJECTS[object_num].closed = 0;
				object_num++;
			}
		}
	}
	object_last = object_num;

	/* check if object inside or outside */
	for (num5b = 0; num5b < object_last; num5b++) {
		int flag = 0;
		for (num4b = 0; num4b < line_last; num4b++) {
			if (myOBJECTS[num5b].line[num4b] != 0) {
				int lnum = myOBJECTS[num5b].line[num4b];
				int pipret = 0;
				double testx = myLINES[lnum].x1;
				double testy = myLINES[lnum].y1;
				/* Workaround, set minimal offset (+0.0000313) so i hope no line is on the same level */
				pipret = point_in_object(-1, num5b, testx + 0.0000313, testy + 0.0000313);
				if (pipret == 0) {
					flag = 1;
				}
				pipret = 0;
				testx = myLINES[lnum].x2;
				testy = myLINES[lnum].y2;
				pipret = point_in_object(-1, num5b, testx + 0.0000313, testy + 0.0000313);
				if (pipret == 0) {
					flag = 1;
				}
			}
		}
		if (flag > 0) {
			myOBJECTS[num5b].inside = 1;
		} else if (myOBJECTS[num5b].line[0] != 0) {
			myOBJECTS[num5b].inside = 0;
		}
	}
}

void draw_helplines (void) {
	char tmp_str[128];
	/* Scale-Arrow's */
	float gridXYZ = 10.0;
	float gridXYZmin = 1.0;
	float lenY = size_y;
	float lenX = size_x;
	float lenZ = PARAMETER[P_M_DEPTH].vdouble * -1;
	float offXYZ = 10.0;
	float arrow_d = 1.0;
	float arrow_l = 6.0;
	GLUquadricObj *quadratic = gluNewQuadric();

	int pos_n = 0;
	glColor4f(1.0, 1.0, 1.0, 0.3);
	for (pos_n = 0; pos_n <= lenY; pos_n += gridXYZ) {
		glBegin(GL_LINES);
		glVertex3f(0.0, pos_n, PARAMETER[P_M_DEPTH].vdouble);
		glVertex3f(lenX, pos_n, PARAMETER[P_M_DEPTH].vdouble);
		glEnd();
	}
	for (pos_n = 0; pos_n <= lenX; pos_n += gridXYZ) {
		glBegin(GL_LINES);
		glVertex3f(pos_n, 0.0, PARAMETER[P_M_DEPTH].vdouble);
		glVertex3f(pos_n, lenY, PARAMETER[P_M_DEPTH].vdouble);
		glEnd();
	}

	glColor4f(1.0, 1.0, 1.0, 0.2);
	for (pos_n = 0; pos_n <= lenY; pos_n += gridXYZmin) {
		glBegin(GL_LINES);
		glVertex3f(0.0, pos_n, PARAMETER[P_M_DEPTH].vdouble);
		glVertex3f(lenX, pos_n, PARAMETER[P_M_DEPTH].vdouble);
		glEnd();
	}
	for (pos_n = 0; pos_n <= lenX; pos_n += gridXYZmin) {
		glBegin(GL_LINES);
		glVertex3f(pos_n, 0.0, PARAMETER[P_M_DEPTH].vdouble);
		glVertex3f(pos_n, lenY, PARAMETER[P_M_DEPTH].vdouble);
		glEnd();
	}

	glColor4f(1.0, 0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(-offXYZ, 0.0, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, lenY, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, lenY, 0.0);
	glVertex3f(-offXYZ, lenY, 0.0);
	glEnd();
	glPushMatrix();
	glTranslatef(0.0 - offXYZ, -0.0, 0.0);
	glPushMatrix();
	glTranslatef(arrow_d * 2.0 + 1.0, lenY / 2.0, 0.0);
	glRotatef(90.0, 0.0, 0.0, 1.0);
	glScalef(0.01, 0.01, 0.01);
	sprintf(tmp_str, "%0.2fmm", lenY);
	output_text(tmp_str);
	glPopMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(quadratic, 0.0, (arrow_d * 3), arrow_l ,32, 1);
	glTranslatef(0.0, 0.0, arrow_l);
	gluCylinder(quadratic, arrow_d, arrow_d, lenY - arrow_l * 2.0 ,32, 1);
	glTranslatef(0.0, 0.0, lenY - arrow_l * 2.0);
	gluCylinder(quadratic, (arrow_d * 3), 0.0, arrow_l ,32, 1);
	glPopMatrix();

	glColor4f(0.0, 1.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, -offXYZ, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(lenX, 0.0, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(lenX, -offXYZ, 0.0);
	glVertex3f(lenX, 0.0, 0.0);
	glEnd();
	glPushMatrix();
	glTranslatef(lenX, -offXYZ, 0.0);
	glPushMatrix();
	glTranslatef(-lenX / 2.0, -arrow_d * 2.0 - 1.0, 0.0);
	glScalef(0.01, 0.01, 0.01);
	sprintf(tmp_str, "%0.2fmm", lenX);
	output_text(tmp_str);
	glPopMatrix();
	glRotatef(-90.0, 0.0, 1.0, 0.0);
	gluCylinder(quadratic, 0.0, (arrow_d * 3), arrow_l ,32, 1);
	glTranslatef(0.0, 0.0, arrow_l);
	gluCylinder(quadratic, arrow_d, arrow_d, lenX - arrow_l * 2.0 ,32, 1);
	glTranslatef(0.0, 0.0, lenX - arrow_l * 2.0);
	gluCylinder(quadratic, (arrow_d * 3), 0.0, arrow_l ,32, 1);
	glPopMatrix();

	glColor4f(0.0, 0.0, 1.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(-offXYZ, -offXYZ, 0.0);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, -lenZ);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, -lenZ);
	glVertex3f(-offXYZ, -offXYZ, -lenZ);
	glEnd();
	glPushMatrix();
	glTranslatef(-offXYZ, -offXYZ, -lenZ);
	glPushMatrix();
	glTranslatef(arrow_d * 2.0 - 1.0, -arrow_d * 2.0 - 1.0, lenZ / 2.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	glScalef(0.01, 0.01, 0.01);
	sprintf(tmp_str, "%0.2fmm", lenZ);
	output_text(tmp_str);
	glPopMatrix();
	glRotatef(-90.0, 0.0, 0.0, 1.0);
	gluCylinder(quadratic, 0.0, (arrow_d * 3), arrow_l ,32, 1);
	glTranslatef(0.0, 0.0, arrow_l);
	gluCylinder(quadratic, arrow_d, arrow_d, lenZ - arrow_l * 2.0 ,32, 1);
	glTranslatef(0.0, 0.0, lenZ - arrow_l * 2.0);
	gluCylinder(quadratic, (arrow_d * 3), 0.0, arrow_l ,32, 1);
	glPopMatrix();
}

void mainloop (void) {
	int object_num = 0;
	size_x = (max_x - min_x);
	size_y = (max_y - min_y);
	float scale = (4.0 / size_x);
	if (scale > (4.0 / size_y)) {
		scale = (4.0 / size_y);
	}
	/* get diameter from tooltable by number */
	if (PARAMETER[P_TOOL_SELECT].vint != 0) {
		PARAMETER[P_TOOL_NUM].vint = PARAMETER[P_TOOL_SELECT].vint;
		PARAMETER[P_TOOL_DIAMETER].vdouble = tooltbl_diameters[PARAMETER[P_TOOL_NUM].vint];
		TwDefine("Parameter/'Tool|Number' readonly=true");
		TwDefine("Parameter/'Tool|Diameter' readonly=true");
	} else {
		TwDefine("Parameter/'Tool|Number' readonly=false");
		TwDefine("Parameter/'Tool|Diameter' readonly=false");
	}
	PARAMETER[P_TOOL_SPEED_MAX].vint = (int)(((float)material_vc[PARAMETER[P_MAT_SELECT].vint] * 1000.0) / (PI * (PARAMETER[P_TOOL_DIAMETER].vdouble)));
	if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 4.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][0] * (float)PARAMETER[P_TOOL_W].vint);
	} else if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 8.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][1] * (float)PARAMETER[P_TOOL_W].vint);
	} else if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 12.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][2] * (float)PARAMETER[P_TOOL_W].vint);
	}
	if (shapeEV[layer_sel].Label != NULL) {
		strcpy(mill_layer, shapeEV[layer_sel].Label);
	} else {
		mill_layer[0] = 0;
	}

	if (batchmode == 0) {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_NORMALIZE);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushMatrix();
		glTranslatef(0.5f, -0.3f, 0.0f);
		float mat[4*4]; // rotation matrix
		if (g_AutoRotate) {
			float axis[3] = { 0, 1, 0 };
			float angle = (float)(GetTimeMs()-g_RotateTime)/1000.0f;
			float quat[4];
			SetQuaternionFromAxisAngle(axis, angle, quat);
			MultiplyQuaternions(g_RotateStart, quat, g_Rotation);
		}
		ConvertQuaternionToMatrix(g_Rotation, mat);
		glMultMatrixf(mat);
		glScalef(PARAMETER[P_V_ZOOM].vfloat, PARAMETER[P_V_ZOOM].vfloat, PARAMETER[P_V_ZOOM].vfloat);
		glScalef(scale, scale, scale);
		glTranslatef(-size_x / 2.0, -size_y / 2.0, 0.0);
		glTranslatef(translate_x, translate_y, 0.0);
	} else {
		PARAMETER[P_V_HELPLINES].vint = 0;
		save_gcode = 1;
	}

	/* init gcode */
	if (save_gcode == 1) {
		if (strcmp(PARAMETER[P_MFILE].vstr, "-") == 0) {
			fd_out = stdout;
		} else {
			fd_out = fopen(PARAMETER[P_MFILE].vstr, "w");
		}
		if (fd_out == NULL) {
			fprintf(stderr, "Can not open file: %s\n", PARAMETER[P_MFILE].vstr);
			exit(0);
		}
		tool_last = -1;
		ShowSetup_gCode(fd_out);
		fprintf(fd_out, "\n");
		fprintf(fd_out, "G21 (Metric)\n");
		fprintf(fd_out, "G40 (No Offsets)\n");
		fprintf(fd_out, "G90 (Absolute-Mode)\n");
		fprintf(fd_out, "F%i\n", PARAMETER[P_M_FEEDRATE].vint);
		fprintf(fd_out, "\n");
	}

	/* 'shortest' path / first inside than outside objects */ 
	double last_x = 0.0;
	double last_y = 0.0;
	double next_x = 0.0;
	double next_y = 0.0;
	for (object_num = 0; object_num < MAX_OBJECTS; object_num++) {
		myOBJECTS[object_num].visited = 0;
	}

	/* inside and open objects */
	for (object_num = 0; object_num < object_last; object_num++) {
		double shortest_len = 9999999.0;
		int shortest_object = -1;
		int shortest_line = -1;
		int flag = 0;
		int object_num2 = 0;
		for (object_num2 = 0; object_num2 < object_last; object_num2++) {
			int nnum = 0;
			for (nnum = 0; nnum < line_last; nnum++) {
				if (myOBJECTS[object_num2].line[nnum] != 0 && myOBJECTS[object_num2].inside == 1 && myOBJECTS[object_num2].visited == 0) {
					int lnum2 = myOBJECTS[object_num2].line[nnum];
					double len = get_len(last_x, last_y, myLINES[lnum2].x1, myLINES[lnum2].y1);
					if (len < shortest_len) {
						shortest_len = len;
						shortest_object = object_num2;
						shortest_line = nnum;
						flag = 1;
					}
				}
			}
			nnum = 0;
			if (myOBJECTS[object_num2].line[nnum] != 0 && myOBJECTS[object_num2].closed == 0 && myOBJECTS[object_num2].visited == 0) {
				int lnum2 = myOBJECTS[object_num2].line[nnum];
				double len = get_len(last_x, last_y, myLINES[lnum2].x1, myLINES[lnum2].y1);
				if (len < shortest_len) {
					shortest_len = len;
					shortest_object = object_num2;
					shortest_line = nnum;
					flag = 1;
				}
			}
			nnum = object_line_last(object_num2);
			if (myOBJECTS[object_num2].line[nnum] != 0 && myOBJECTS[object_num2].closed == 0 && myOBJECTS[object_num2].visited == 0) {
				int lnum2 = myOBJECTS[object_num2].line[nnum];
				double len = get_len(last_x, last_y, myLINES[lnum2].x2, myLINES[lnum2].y2);
				if (len < shortest_len) {
					shortest_len = len;
					shortest_object = object_num2;
					shortest_line = nnum;
					flag = 2;
				}
			}
		}
		if (flag > 0) {
			myOBJECTS[shortest_object].visited = 1;
			if (flag > 1) {
				redir_object(shortest_object);
			}
			if (myOBJECTS[shortest_object].closed == 1) {
				resort_object(shortest_object, shortest_line);
				object_optimize_dir(shortest_object);
			}
			object_draw_offset(fd_out, shortest_object, &next_x, &next_y);
			object_draw(fd_out, shortest_object);

			last_x = next_x;
			last_y = next_y;

		} else {
			break;
		}
	}

	/* outside objects */
	for (object_num = 0; object_num < object_last; object_num++) {
		double shortest_len = 9999999.0;
		int shortest_object = -1;
		int shortest_line = -1;
		int flag = 0;
		int object_num2 = 0;
		for (object_num2 = 0; object_num2 < object_last; object_num2++) {
			int nnum = 0;
			for (nnum = 0; nnum < line_last; nnum++) {
				if (myOBJECTS[object_num2].line[nnum] != 0 && (myOBJECTS[object_num2].inside == 0 && myOBJECTS[object_num2].closed == 1) && myOBJECTS[object_num2].visited == 0) {
					int lnum2 = myOBJECTS[object_num2].line[nnum];
					double len = get_len(last_x, last_y, myLINES[lnum2].x1, myLINES[lnum2].y1);
					if (len < shortest_len) {
						shortest_len = len;
						shortest_object = object_num2;
						shortest_line = nnum;
						flag = 1;
					}
				}
			}
		}
		if (flag == 1) {
//			printf("##WEIGHT## %i == %f %i\n", shortest_object, shortest_len, flag);
			myOBJECTS[shortest_object].visited = 1;
			resort_object(shortest_object, shortest_line);
			object_optimize_dir(shortest_object);
			object_draw_offset(fd_out, shortest_object, &next_x, &next_y);
			object_draw(fd_out, shortest_object);
			last_x = next_x;
			last_y = next_y;


		} else {
			break;
		}
	}

	/* draw text / opengl only */
	int nnum = 0;
	if (batchmode == 0) {
		for (nnum = 0; nnum < 100; nnum++) {
			if (myMTEXT[nnum].used == 1) {
				draw_text(GLUT_BITMAP_HELVETICA_18, myMTEXT[nnum].text, myMTEXT[nnum].x, myMTEXT[nnum].y - myMTEXT[nnum].s);
			}
		}
	}

	/* exit gcode */
	if (save_gcode == 1) {
		fprintf(fd_out, "M05 (Spindle-/Laser-Off)\n");
		fprintf(fd_out, "M02 (Programm-End)\n");
		fclose(fd_out);
		save_gcode = 0;
	}

	if (batchmode == 1) {
		onExit();
		exit(0);
	} else {
		if (PARAMETER[P_V_HELPLINES].vint == 1) {
			draw_helplines();
		}
		glPopMatrix();
		TwDraw();
		glutSwapBuffers();
		glutPostRedisplay();
	}
	return;
}

void Reshape (int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40, (double)width/height, 1, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0,0,5, 0,0,0, 0,1,0);
	glTranslatef(0, 0.6f, -1);
	TwWindowSize(width, height);
	char tmp_str[1024];
	sprintf(tmp_str, "Parameter size='250 %i' position='0 0' color='96 216 224'", height - 30);
	TwDefine(tmp_str);
}

int last_mouse_x = 0;
int last_mouse_y = 0;
int last_mouse_button = -1;
int last_mouse_state = 0;
void OnMouseFunc (int button, int state, int mouseX, int mouseY) {
	if (!TwEventMouseButtonGLUT(button, state, mouseX, mouseY)) {
		if (button == 3 && state == 0) {
			PARAMETER[P_V_ZOOM].vfloat += 0.05;
		} else if (button == 4 && state == 0) {
			PARAMETER[P_V_ZOOM].vfloat -= 0.05;
		} else if (button == 0) {
			if (state == 0) {
				last_mouse_x = mouseX - translate_x * 4;
				last_mouse_y = mouseY - translate_y * -4;
				last_mouse_button = button;
				last_mouse_state = state;
			} else {
				last_mouse_button = button;
				last_mouse_state = state;
			}
		} else if (button == 1) {
			if (state == 0) {
				last_mouse_x = mouseX - (int)(g_Rotation[1] * 180.0);
				last_mouse_y = mouseY - (int)(g_Rotation[0] * 180.0);
				last_mouse_button = button;
				last_mouse_state = state;
			} else {
				last_mouse_button = button;
				last_mouse_state = state;
			}
		} else if (button == 2) {
			if (state == 0) {
				last_mouse_x = mouseX - (int)(g_Rotation[2] * 180.0);;
				last_mouse_y = mouseY;
				last_mouse_button = button;
				last_mouse_state = state;
			} else {
				last_mouse_button = button;
				last_mouse_state = state;
			}
		}
	}
}

void OnMouseMotion (int mouseX, int mouseY) {
	if (!TwEventMouseMotionGLUT(mouseX, mouseY)) {
		if (last_mouse_button == 0 && last_mouse_state == 0) {
			translate_x = (mouseX - last_mouse_x) / 4;
			translate_y = (mouseY - last_mouse_y) / -4;
		} else if (last_mouse_button == 1 && last_mouse_state == 0) {
			g_Rotation[1] = (float)(mouseX - last_mouse_x) / 180.0;
			g_Rotation[0] = (float)(mouseY - last_mouse_y) / 180.0;
		} else if (last_mouse_button == 2 && last_mouse_state == 0) {
			g_Rotation[2] = (float)(mouseX - last_mouse_x) / 180.0;
		}
	}
}

void TW_CALL SaveGcode (void *clientData) { 
	save_gcode = 1;
}

void TW_CALL ReloadSetup (void *clientData) {
	read_setup();
}

int main (int argc, char *argv[]) {
	int num = 0;
	int num2 = 0;
	for (num = 0; num < line_last; num++) {
		myLINES[num].used = 0.0;
		myLINES[num].x1 = 0.0;
		myLINES[num].y1 = 0.0;
		myLINES[num].x2 = 0.0;
		myLINES[num].y2 = 0.0;
	}
	mill_layer[0] = 0;
	strcpy(PARAMETER[P_MFILE].vstr, "-");
	read_setup();
	if (argc < 2) {
		ShowHelp();
		exit(1);
	}
	for (num = 1; num < argc; num++) {
		int flag = 0;
		int n = 0;
		for (n = 0; n < P_LAST; n++) {
			if (strcmp(argv[num], PARAMETER[n].arg) == 0) {
				num++;
				flag = 1;
				if (PARAMETER[n].type == T_FLOAT) {
					PARAMETER[n].vfloat = atof(argv[num]);
				} else if (PARAMETER[n].type == T_DOUBLE) {
					PARAMETER[n].vdouble = atof(argv[num]);
				} else if (PARAMETER[n].type == T_INT) {
					PARAMETER[n].vint = atoi(argv[num]);
				} else if (PARAMETER[n].type == T_BOOL) {
					PARAMETER[n].vint = atoi(argv[num]);
				} else if (PARAMETER[n].type == T_STRING) {
					strcpy(PARAMETER[n].vstr, argv[num]);
				}
			}
		}
		if (flag == 1) {
		} else if (strcmp(argv[num], "-h") == 0) {
			ShowHelp();
			exit(0);
		} else if (num != argc - 1) {
			fprintf(stderr, "### unknown argument: %s ###\n", argv[num]);
			ShowHelp();
			exit(1);
		}
	}
	strcpy(dxf_file, argv[argc - 1]);

	/* import DXF */
	dxf_read(dxf_file);
	init_objects();

	/* check size */
	min_x = 99999.0;
	min_y = 99999.0;
	max_x = 0.0;
	max_y = 0.0;
	for (num2 = 0; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1) {
			if (max_x < myLINES[num2].x1) {
				max_x = myLINES[num2].x1;
			}
			if (max_x < myLINES[num2].x2) {
				max_x = myLINES[num2].x2;
			}
			if (max_y < myLINES[num2].y1) {
				max_y = myLINES[num2].y1;
			}
			if (max_y < myLINES[num2].y2) {
				max_y = myLINES[num2].y2;
			}
			if (min_x > myLINES[num2].x1) {
				min_x = myLINES[num2].x1;
			}
			if (min_x > myLINES[num2].x2) {
				min_x = myLINES[num2].x2;
			}
			if (min_y > myLINES[num2].y1) {
				min_y = myLINES[num2].y1;
			}
			if (min_y > myLINES[num2].y2) {
				min_y = myLINES[num2].y2;
			}
		}
	}
	ShowSetup();

/*
    Stahl: 40 – 120 m/min
    Aluminium: 100 – 500 m/min
    Kupfer, Messing und Bronze: 100 – 200 m/min
    Kunststoffe: 50 – 150 m/min
    Holz: Bis zu 3000 m/min
*/

	materialEV[0].Value = 0;
	materialEV[0].Label = "Aluminium(Langspanend)";
	material_vc[0] = 200;
	material_fz[0][0] = 0.04;
	material_fz[0][1] = 0.05;
	material_fz[0][2] = 0.10;
	material_texture[0] = "metall.bmp";

	materialEV[1].Value = 1;
	materialEV[1].Label = "Aluminium(Kurzspanend)";
	material_vc[1] = 150;
	material_fz[1][0] = 0.04;
	material_fz[1][1] = 0.05;
	material_fz[1][2] = 0.10;
	material_texture[1] = "metall.bmp";

	materialEV[2].Value = 2;
	materialEV[2].Label = "NE-Metalle(Messing,Bronze,Kupfer,Zink,Rotguß)";
	material_vc[2] = 150;
	material_fz[2][0] = 0.04;
	material_fz[2][1] = 0.05;
	material_fz[2][2] = 0.10;
	material_texture[2] = "metall.bmp";

	materialEV[3].Value = 3;
	materialEV[3].Label = "VA-Stahl";
	material_vc[3] = 100;
	material_fz[3][0] = 0.05;
	material_fz[3][1] = 0.06;
	material_fz[3][2] = 0.07;
	material_texture[3] = "metall.bmp";

	materialEV[4].Value = 4;
	materialEV[4].Label = "Thermoplaste";
	material_vc[4] = 100;
	material_fz[4][0] = 0.0;
	material_fz[4][1] = 0.0;
	material_fz[4][2] = 0.0;
	material_texture[4] = "plast.bmp";

	materialEV[5].Value = 5;
	materialEV[5].Label = "Duroplaste(mit Füllstoffen)";
	material_vc[5] = 125;
	material_fz[5][0] = 0.04;
	material_fz[5][1] = 0.08;
	material_fz[5][2] = 0.10;
	material_texture[5] = "plast.bmp";

	materialEV[6].Value = 6;
	materialEV[6].Label = "GFK";
	material_vc[6] = 125;
	material_fz[6][0] = 0.04;
	material_fz[6][1] = 0.08;
	material_fz[6][2] = 0.10;
	material_texture[6] = "gfk.bmp";

	materialEV[7].Value = 7;
	materialEV[7].Label = "CFK";
	material_vc[7] = 125;
	material_fz[7][0] = 0.04;
	material_fz[7][1] = 0.08;
	material_fz[7][2] = 0.10;
	material_texture[7] = "cfk.bmp";

	materialEV[8].Value = 8;
	materialEV[8].Label = "Holz";
	material_vc[8] = 2000;
	material_fz[8][0] = 0.04;
	material_fz[8][1] = 0.08;
	material_fz[8][2] = 0.10;
	material_texture[8] = "holz.bmp";

	material_max = 9;

	/* import Tool-Diameter from Tooltable */
	int n = 0;
	char tmp_str[1024];
	tools_max = 0;
	for (n = 0; n < 100; n++) {
		tooltbl_diameters[n] = 0.0;
		toolEV[n].Label = NULL;
	}
	if (PARAMETER[P_TOOL_TABLE].vstr[0] != 0) {
		FILE *tt_fp;
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		tt_fp = fopen(PARAMETER[P_TOOL_TABLE].vstr, "r");
		if (tt_fp == NULL) {
			fprintf(stderr, "Can not open Tooltable-File: %s\n", PARAMETER[P_TOOL_TABLE].vstr);
			exit(EXIT_FAILURE);
		}
		tooltbl_diameters[0] = 1;
		n = 0;
		sprintf(tmp_str, "FREE");
		toolEV[n].Value = n;
		toolEV[n].Label = (const char *)malloc(strlen(tmp_str) + 1);
		strcpy((char *)toolEV[n].Label, tmp_str);
		n++;
		while ((read = getline(&line, &len, tt_fp)) != -1) {
			if (strncmp(line, "T", 1) == 0) {
				char line2[2048];
				trimline(line2, 1024, line);
				int tooln = atoi(line2 + 1);
				double toold = atof(strstr(line2, " D") + 2);
				if (tooln > 0 && tooln < 100 && toold > 0.001) {
					tooltbl_diameters[tooln] = toold;
					tool_descr[tooln][0] = 0;
					if (strstr(line2, ";") > 0) {
						strcpy(tool_descr[tooln], strstr(line2, ";") + 1);
					}
					sprintf(tmp_str, "#%i D%0.2fmm (%s)", tooln, tooltbl_diameters[tooln], tool_descr[tooln]);
					toolEV[n].Value = n;
					toolEV[n].Label = (const char *)malloc(strlen(tmp_str) + 1);
					strcpy((char *)toolEV[n].Label, tmp_str);
					n++;
					tools_max++;
				}
			}
		}
		fclose(tt_fp);
	}

	/* set bottom-left to 0,0 */
	for (num2 = 0; num2 < line_last; num2++) {
		if (myLINES[num2].used == 1) {
			myLINES[num2].x1 -= min_x;
			myLINES[num2].y1 -= min_y;
			myLINES[num2].x2 -= min_x;
			myLINES[num2].y2 -= min_y;
		}
	}
	for (num2 = 0; num2 < 100; num2++) {
		if (myMTEXT[num2].used == 1) {
			myMTEXT[num2].x -= min_x;
			myMTEXT[num2].y -= min_y;
		}
	}

	/* read Layer-Names */
	for (num2 = 0; num2 < 100; num2++) {
		shapeEV[num2].Label = NULL;
	}
	shapeEV[0].Value = 0;
	shapeEV[0].Label = (const char *)malloc(6);
	strcpy((char *)shapeEV[0].Label, "ALL");
	int num1 = 0;
	int labeln = 1;
	for (num1 = 0; num1 < MAX_LINES; num1++) {
		if (myLINES[num1].layer[0] != 0) {
			int flag = 0;
			for (num2 = 0; num2 < 100; num2++) {
				if (shapeEV[num2].Label != NULL && strcmp(myLINES[num1].layer, shapeEV[num2].Label) == 0) {
					flag = 1;
				}
			}
			if (flag == 0 && labeln < 100) {
				shapeEV[labeln].Value = labeln;
				shapeEV[labeln].Label = (const char *)malloc(strlen(myLINES[num1].layer) + 1);
				strcpy((char *)shapeEV[labeln].Label, myLINES[num1].layer);
				if (strcmp(mill_layer, myLINES[num1].layer) == 0) {
					layer_sel = labeln;
				}
				layer_selections[labeln] = 1;
				layer_force[labeln] = 0;
				layer_depth[labeln] = 0.0;
				labeln++;
			}
		}
	}

	if (batchmode == 0) {
		/* init opengl */
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
		glutInitWindowSize(winw,winh);
		glutCreateWindow("c-dxf2gcode");
		glutCreateMenu(NULL);
		glutReshapeFunc(Reshape);
		glutDisplayFunc(mainloop);
		atexit(onExit);

		texture_init();

		TwBar *bar;
		float axis[] = { -0.7f, 0.0f, 0.0f }; // initial model rotation
		float angle = 0.8f;
		TwInit(TW_OPENGL, NULL);

//		glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
//		glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);

		glutMouseFunc(OnMouseFunc);
		glutMotionFunc(OnMouseMotion);

		glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
		glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
		glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
		TwGLUTModifiersFunc(glutGetModifiers);

		bar = TwNewBar("Parameter");
		TwDefine("Parameter size='230 600' color='96 216 224' "); // change default tweak bar size and color

		TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &g_Rotation, "group=View label='Object rotation' opened=true help='Change the object orientation.' ");
		TwAddVarCB(bar, "AutoRotate", TW_TYPE_BOOL32, SetAutoRotateCB, GetAutoRotateCB, NULL, "group=View label='Auto-rotate' key=space help='Toggle auto-rotate mode.' ");

		TwAddSeparator(bar, "Save", "");
		TwAddButton(bar, "Save G-Code", SaveGcode, NULL, " label='Save G-Code' ");
		TwAddButton(bar, "Save Setup", SaveSetup, NULL, " label='Save Setup' ");
		TwAddButton(bar, "Reload Setup", ReloadSetup, NULL, " label='Reload Setup' ");

		TwType tools = TwDefineEnum("toolType", toolEV, tools_max);
		TwAddVarRW(bar, "Tool-Select", tools, &PARAMETER[P_TOOL_SELECT].vint, "group=Tool label=Select");

		TwType material = TwDefineEnum("materialType", materialEV, material_max);
		TwAddVarRW(bar, "Material-Select", material, &PARAMETER[P_MAT_SELECT].vint, "group=Material label=Select");

		for (n = 0; n < P_LAST; n++) {
			char name_str[1024];
			char opt_str[1024];
			sprintf(name_str, "%s|%s", PARAMETER[n].group, PARAMETER[n].name);
			if (PARAMETER[n].unit[0] != 0) {
				sprintf(opt_str, "label='%s (%s)' group='%s' min=%f step=%f max=%f", PARAMETER[n].name, PARAMETER[n].unit, PARAMETER[n].group, PARAMETER[n].min, PARAMETER[n].step, PARAMETER[n].max);
			} else {
				sprintf(opt_str, "label='%s' group='%s' min=%f step=%f max=%f", PARAMETER[n].name, PARAMETER[n].group, PARAMETER[n].min, PARAMETER[n].step, PARAMETER[n].max);
			}
			if (PARAMETER[n].show == 0) {
			} else if (PARAMETER[n].type == T_FLOAT) {
				TwAddVarRW(bar, name_str, TW_TYPE_FLOAT, &PARAMETER[n].vfloat, opt_str);
			} else if (PARAMETER[n].type == T_DOUBLE) {
				TwAddVarRW(bar, name_str, TW_TYPE_DOUBLE, &PARAMETER[n].vdouble, opt_str);
			} else if (PARAMETER[n].type == T_INT) {
				TwAddVarRW(bar, name_str, TW_TYPE_INT32, &PARAMETER[n].vint, opt_str);
			} else if (PARAMETER[n].type == T_BOOL) {
				sprintf(opt_str, "label='%s' group='%s'", PARAMETER[n].name, PARAMETER[n].group);
				TwAddVarRW(bar, name_str, TW_TYPE_BOOL32, &PARAMETER[n].vint, opt_str);
			} else if (PARAMETER[n].type == T_STRING) {
			}
		}

		TwAddSeparator(bar, "Layers", "");
		TwType layers = TwDefineEnum("ShapeType", shapeEV, labeln);
		TwAddVarRW(bar, "Select", layers, &layer_sel, "group=Layer keyIncr='<' keyDecr='>' help='use only Layer X' ");
		for (num2 = 1; num2 < 100; num2++) {
			if (shapeEV[num2].Label != NULL) {
				char tmp_str[1024];
				char tmp_str2[1024];
				sprintf(tmp_str, "Layer: %s Use", shapeEV[num2].Label);
				sprintf(tmp_str2, "label='Use' group='Layer: %s'", shapeEV[num2].Label);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &layer_selections[num2], tmp_str2);

				sprintf(tmp_str, "Layer: %s Overwrite", shapeEV[num2].Label);
				sprintf(tmp_str2, "label='Overwrite' group='Layer: %s'", shapeEV[num2].Label);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &layer_force[num2], tmp_str2);

				sprintf(tmp_str, "Layer: %s Depth", shapeEV[num2].Label);
				sprintf(tmp_str2, "label='Depth' group='Layer: %s'  min=-100.0 max=10.0 step=0.1", shapeEV[num2].Label);
				TwAddVarRW(bar, tmp_str, TW_TYPE_DOUBLE, &layer_depth[num2], tmp_str2);

				sprintf(tmp_str, "Parameter/'Layer: %s' opened=false", shapeEV[num2].Label);
				TwDefine(tmp_str);
			}
		}

		TwAddSeparator(bar, "Objects", "");
		for (num2 = 0; num2 < MAX_OBJECTS; num2++) {
			if (myOBJECTS[num2].line[0] != 0) {
				char tmp_str[1024];
				char tmp_str2[1024];

				sprintf(tmp_str, "Object: #%i Closed", num2);
				sprintf(tmp_str2, "label='Closed' group='Object: #%i' readonly='true'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &myOBJECTS[num2].closed, tmp_str2);

				sprintf(tmp_str, "Object: #%i Inside", num2);
				sprintf(tmp_str2, "label='Inside' group='Object: #%i' readonly='true'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &myOBJECTS[num2].inside, tmp_str2);

				sprintf(tmp_str, "Object: #%i Use", num2);
				sprintf(tmp_str2, "label='Use' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &object_selections[num2], tmp_str2);

				sprintf(tmp_str, "Object: #%i Overwrite", num2);
				sprintf(tmp_str2, "label='Overwrite' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &object_force[num2], tmp_str2);

				sprintf(tmp_str, "Object: #%i Overcut", num2);
				sprintf(tmp_str2, "label='Overcut' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &object_overcut[num2], tmp_str2);

				sprintf(tmp_str, "Object: #%i Pocket", num2);
				sprintf(tmp_str2, "label='Pocket' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &object_pocket[num2], tmp_str2);

				sprintf(tmp_str, "Object: #%i Laser", num2);
				sprintf(tmp_str2, "label='Laser' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_BOOL32, &object_laser[num2], tmp_str2);

				sprintf(tmp_str, "Object: #%i Depth", num2);
				sprintf(tmp_str2, "label='Depth' group='Object: #%i'  min=-100.0 max=10.0 step=0.1", num2);
				TwAddVarRW(bar, tmp_str, TW_TYPE_DOUBLE, &object_depth[num2], tmp_str2);

			        TwEnumVal offsetEV[3] = { {0, "Normal"}, {1, "Reverse"}, {2, "None"} };
			        TwType offsetType = TwDefineEnum("offsetType", offsetEV, 3);
				sprintf(tmp_str, "Object: #%i Offset", num2);
				sprintf(tmp_str2, "label='Offset' group='Object: #%i'", num2);
				TwAddVarRW(bar, tmp_str, offsetType, &object_offset[num2], tmp_str2);

				sprintf(tmp_str, "Parameter/'Object: #%i' opened=false", num2);
				TwDefine(tmp_str);
			}
		}

		g_RotateTime = GetTimeMs();
		SetQuaternionFromAxisAngle(axis, angle, g_Rotation);
		SetQuaternionFromAxisAngle(axis, angle, g_RotateStart);

		glutMainLoop();
	} else {
		mainloop();
	}

	return 0;
}


