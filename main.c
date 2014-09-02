/*

 Copyright by Oliver Dippel <oliver@multixmedia.org>

 MacOSX - Changes by McUles <http://fpv-community.de>
	Yosemite (OSX 10.10)

*/

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <pwd.h>
#include <dxf.h>
#include <font.h>
#include <setup.h>

void texture_init (void);
GLuint texture_load (char *filename);

#define FUZZY 0.01

#ifndef CALLBACK
#define CALLBACK
#endif

int winw = 1600;
int winh = 1200;
float size_x = 0.0;
float size_y = 0.0;
double min_x = 99999.0;
double min_y = 99999.0;
double max_x = 0.0;
double max_y = 0.0;
int mill_start = 0;
int mill_start_all = 0;
double mill_last_x = 0.0;
double mill_last_y = 0.0;
double mill_last_z = 0.0;
double tooltbl_diameters[100];
char mill_layer[1024];
int layer_sel = 0;
int layer_selections[100];
int layer_force[100];
double layer_depth[100];
int object_selections[MAX_OBJECTS];
int object_offset[MAX_OBJECTS];
int object_force[MAX_OBJECTS];
int object_overcut[MAX_OBJECTS];
int object_pocket[MAX_OBJECTS];
int object_laser[MAX_OBJECTS];
double object_depth[MAX_OBJECTS];
FILE *fd_out = NULL;
int object_last = 0;
int batchmode = 0;
int save_gcode = 0;
char tool_descr[100][1024];
int tools_max = 0;
int tool_last = 0;
int LayerMax = 1;
int material_max = 8;
char *material_texture[100];
int material_vc[100];
float material_fz[100][3];

char *rotary_axis[3] = {"A", "B", "C"};

int last_mouse_x = 0;
int last_mouse_y = 0;
int last_mouse_button = -1;
int last_mouse_state = 0;

void ParameterUpdate (void);
void ParameterChanged (GtkWidget *widget, gpointer data);

GtkWidget *SizeInfoLabel;
GtkWidget *StatusBar;
GtkTreeStore *treestore;
GtkListStore *ListStore[P_LAST];
GtkWidget *ParamValue[P_LAST];
GtkWidget *ParamButton[P_LAST];
GtkWidget *glCanvas;
int width = 800;
int height = 600;
int need_init = 1;

GtkWidget *window;
GtkWidget *dialog;


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

void point_rotate (float y, float depth, float *ny, float *nz) {
	float radius = (PARAMETER[P_MAT_DIAMETER].vdouble / 2.0) + depth;
	float an = y / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
	float rangle = toRad(an - 90.0);
	*ny = radius * cos(rangle);
	*nz = radius * -sin(rangle);
}

void translateAxisX (double x, char *ret_str) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1 && PARAMETER[P_H_ROTARYAXIS].vint == 1) {
		double an = x / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
		sprintf(ret_str, "%s%f", rotary_axis[PARAMETER[P_H_ROTARYAXIS].vint], an);
	} else {
		sprintf(ret_str, "X%f", x);
	}
}

void translateAxisY (double y, char *ret_str) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1 && PARAMETER[P_H_ROTARYAXIS].vint == 0) {
		double an = y / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
		sprintf(ret_str, "%s%f", rotary_axis[PARAMETER[P_H_ROTARYAXIS].vint], an);
	} else {
		sprintf(ret_str, "Y%f", y);
	}
}

void translateAxisZ (double z, char *ret_str) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		sprintf(ret_str, "Z%f", z + (PARAMETER[P_MAT_DIAMETER].vdouble / 2.0));
	} else {
		sprintf(ret_str, "Z%f", z);
	}
}

void object2poly (int object_num, double depth, double depth2, int invert) {
	int num = 0;
	int nverts = 0;
	GLUtesselator *tobj;
	GLdouble rect2[MAX_LINES][3];

	if (invert == 0) {
		glColor4f(0.0, 0.5, 0.2, 0.5);
	} else {
		glColor4f(0.0, 0.2, 0.5, 0.5);
	}

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
	if (invert == 0) {
		gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NEGATIVE);
	} else {
		gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
	}
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
//	glRasterPos2i(x, y);
//	while (*text != '\0') {
//		glutBitmapCharacter(font, *text);
//		++text;
//	}
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

double line_angle2 (int lnum) {
	double dx = myLINES[lnum].x2 - myLINES[lnum].x1;
	double dy = myLINES[lnum].y2 - myLINES[lnum].y1;
	double alpha = toDeg(atan2(dx, dy));
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
				redir_object(object_num);
			} else {
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

void draw_line_wrap_conn (float x1, float y1, float depth1, float depth2) {
	float ry = 0.0;
	float rz = 0.0;
	glBegin(GL_LINES);
	point_rotate(y1, depth1, &ry, &rz);
	glVertex3f(x1, ry, rz);
	point_rotate(y1, depth2, &ry, &rz);
	glVertex3f(x1, ry, rz);
	glEnd();
}

void draw_line_wrap (float x1, float y1, float x2, float y2, float depth) {
	float radius = (PARAMETER[P_MAT_DIAMETER].vdouble / 2.0) + depth;
	float dX = x2 - x1;
	float dY = y2 - y1;
	float dashes = dY;
	if (dashes < -1.0) {
		dashes *= -1;
	}
	float an = y1 / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
	float rangle = toRad(an - 90.0);
	float ry = radius * cos(rangle);
	float rz = radius * sin(rangle);
	glBegin(GL_LINE_STRIP);
	glVertex3f(x1, ry, -rz);
	if (dashes > 1.0) {
		float dashX = dX / dashes;
		float dashY = dY / dashes;
		float q = 0.0;
		while (q++ < dashes) {
			x1 += dashX;
			y1 += dashY;
			an = y1 / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
			rangle = toRad(an - 90.0);
			ry = radius * cos(rangle);
			rz = radius * sin(rangle);
			glVertex3f(x1, ry, -rz);
		}
	}
	an = y2 / (PARAMETER[P_MAT_DIAMETER].vdouble * PI) * 360;
	rangle = toRad(an - 90.0);
	ry = radius * cos(rangle);
	rz = radius * sin(rangle);
	glVertex3f(x2, ry, -rz);
	glEnd();
}

void draw_oline (float x1, float y1, float x2, float y2, float depth) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		draw_line_wrap(x1, y1, x2, y2, 0.0);
		draw_line_wrap(x1, y1, x2, y2, depth);
		draw_line_wrap_conn(x1, y1, 0.0, depth);
		draw_line_wrap_conn(x2, y2, 0.0, depth);
	} else {
		glBegin(GL_LINES);
		glVertex3f(x1, y1, 0.02);
		glVertex3f(x2, y2, 0.02);
	//	if (PARAMETER[P_V_HELPLINES].vint == 1) {
			glBegin(GL_LINES);
			glVertex3f(x1, y1, depth);
			glVertex3f(x2, y2, depth);
			glVertex3f(x1, y1, depth);
			glVertex3f(x1, y1, 0.02);
			glVertex3f(x2, y2, depth);
			glVertex3f(x2, y2, 0.02);
	//	}
		glEnd();
	}
}

void draw_line2 (float x1, float y1, float z1, float x2, float y2, float z2, float width) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		draw_line_wrap(x1, y1, x2, y2, 0.0);
		draw_line_wrap(x1, y1, x2, y2, z1);
		draw_line_wrap_conn(x1, y1, 0.0, z1);
		draw_line_wrap_conn(x2, y2, 0.0, z1);
	} else {
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
}

void draw_line (float x1, float y1, float z1, float x2, float y2, float z2, float width) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		glColor4f(1.0, 0.0, 1.0, 1.0);
		draw_line_wrap(x1, y1, x2, y2, 0.0);
		draw_line_wrap(x1, y1, x2, y2, z1);
		draw_line_wrap_conn(x1, y1, 0.0, z1);
		draw_line_wrap_conn(x2, y2, 0.0, z1);
	} else {
		if (PARAMETER[P_V_HELPDIA].vint == 1) {
			glColor4f(1.0, 1.0, 0.0, 1.0);
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
		glColor4f(1.0, 0.0, 1.0, 1.0);
		glBegin(GL_LINES);
		glVertex3f(x1, y1, z1 + 0.02);
		glVertex3f(x2, y2, z2 + 0.02);
		glEnd();
		DrawArrow(x1, y1, x2, y2, z1 + 0.02, width);
	}
}

void draw_line3 (float x1, float y1, float z1, float x2, float y2, float z2) {
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		glColor4f(1.0, 0.0, 1.0, 1.0);
		draw_line_wrap(x1, y1, x2, y2, z1);
	} else {
		glColor4f(1.0, 0.0, 1.0, 1.0);
		glBegin(GL_LINES);
		glVertex3f(x1, y1, z1 + 0.02);
		glVertex3f(x2, y2, z2 + 0.02);
		glEnd();
	}
}

void mill_z (int gcmd, double z) {
	char tz_str[128];
	if (save_gcode == 1) {
		translateAxisZ(z, tz_str);
		if (gcmd == 0) {
			fprintf(fd_out, "G0%i %s\n", gcmd, tz_str);
		} else {
			fprintf(fd_out, "G0%i %s F%i\n", gcmd, tz_str, PARAMETER[P_M_PLUNGE_SPEED].vint);
		}
	}
	if (mill_start_all != 0) {
		glColor4f(0.0, 1.0, 1.0, 1.0);
		if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
			draw_line_wrap_conn((float)mill_last_x, (float)mill_last_y, (float)mill_last_z, (float)z);
		} else {
			glBegin(GL_LINES);
			glVertex3f((float)mill_last_x, (float)mill_last_y, (float)mill_last_z);
			glVertex3f((float)mill_last_x, (float)mill_last_y, (float)z);
			glEnd();
		}
	}
	mill_last_z = z;
}

void mill_xy (int gcmd, double x, double y, double r, int feed, char *comment) {
	char tx_str[128];
	char ty_str[128];
	if (gcmd != 0) {
		if (mill_start_all != 0) {
			if (gcmd == 2 || gcmd == 3) {
				double e = x - mill_last_x;
				double f = y - mill_last_y;
				double p = sqrt(e*e + f*f);
				double k = (p*p + r*r - r*r) / (2 * p);
				double c1x = mill_last_x + e * k/p + (f/p) * sqrt(r * r - k * k);
				double c1y = mill_last_y + f * k/p - (e/p) * sqrt(r * r - k * k);
				double c2x = mill_last_x + e * k/p - (f/p) * sqrt(r * r - k * k);
				double c2y = mill_last_y + f * k/p + (e/p) * sqrt(r * r - k * k);
				if (gcmd == 2) {
					double dx = mill_last_x - c1x;
					double dy = mill_last_y - c1y;
					double alpha = toDeg(atan(dy / dx));
					if (dx < 0 && dy >= 0) {
						alpha = alpha + 180;
					} else if (dx < 0 && dy < 0) {
						alpha = alpha - 180;
					}
					if (alpha < 0.0) {
						alpha += 360.0;
					}
					dx = x - c1x;
					dy = y - c1y;
					double alpha2 = toDeg(atan(dy / dx));
					if (dx < 0 && dy >= 0) {
						alpha2 = alpha2 + 180;
					} else if (dx < 0 && dy < 0) {
						alpha2 = alpha2 - 180;
					}
					if (alpha2 > 360.0) {
						alpha2 -= 360.0;
					}

					if (alpha - alpha2 > 360.0) {
						alpha -= 360.0;
					}
					double lx = x;
					double ly = y;
					float an = 0;
					for (an = alpha2; an < alpha; an += 36.0) {
						float rangle = toRad(an);
						float rx = r * cos(rangle);
						float ry = r * sin(rangle);
						draw_line((float)c1x + rx, (float)c1y + ry, (float)mill_last_z, (float)lx, (float)ly, (float)mill_last_z, PARAMETER[P_TOOL_DIAMETER].vdouble);
					}
					draw_line((float)mill_last_x, (float)mill_last_y, (float)mill_last_z, (float)lx, (float)ly, (float)mill_last_z, PARAMETER[P_TOOL_DIAMETER].vdouble);
				}
				if (gcmd == 3) {
					double dx = mill_last_x - c2x;
					double dy = mill_last_y - c2y;
					double alpha = toDeg(atan(dy / dx));
					if (dx < 0 && dy >= 0) {
						alpha = alpha + 180;
					} else if (dx < 0 && dy < 0) {
						alpha = alpha - 180;
					}
					if (alpha < 0.0) {
						alpha += 360.0;
					}
					dx = x - c2x;
					dy = y - c2y;
					double alpha2 = toDeg(atan(dy / dx));
					if (dx < 0 && dy >= 0) {
						alpha2 = alpha2 + 180;
					} else if (dx < 0 && dy < 0) {
						alpha2 = alpha2 - 180;
					}
					if (alpha2 > 360.0) {
						alpha2 -= 360.0;
					}
					if (alpha2 - alpha > 360.0) {
						alpha2 -= 360.0;
					}
					if (alpha2 < alpha) {
						alpha2 += 360.0;
					}
					double lx = mill_last_x;
					double ly = mill_last_y;
					float an = 0;
					for (an = alpha; an < alpha2; an += 36.0) {
						float rangle = toRad(an);
						float rx = r * cos(rangle);
						float ry = r * sin(rangle);
						draw_line((float)lx, (float)ly, (float)mill_last_z, (float)c2x + rx, (float)c2y + ry, (float)mill_last_z, PARAMETER[P_TOOL_DIAMETER].vdouble);
					}
					draw_line((float)lx, (float)ly, (float)mill_last_z, (float)x, (float)y, (float)mill_last_z, PARAMETER[P_TOOL_DIAMETER].vdouble);
				}
			} else {
				draw_line((float)mill_last_x, (float)mill_last_y, (float)mill_last_z, (float)x, (float)y, (float)mill_last_z, PARAMETER[P_TOOL_DIAMETER].vdouble);
			}
		}
	} else {
		if (mill_start_all != 0) {
			glColor4f(0.0, 1.0, 1.0, 1.0);
			draw_line3((float)mill_last_x, (float)mill_last_y, (float)mill_last_z, (float)x, (float)y, (float)mill_last_z);
		}
	}
	mill_start_all = 1;
	mill_last_x = x;
	mill_last_y = y;
	if (save_gcode == 1) {
		translateAxisX(x, tx_str);
		translateAxisY(y, ty_str);
		if (gcmd == 0) {
			fprintf(fd_out, "G00 %s %s\n", tx_str, ty_str);
		} else if (gcmd == 1) {
			fprintf(fd_out, "G0%i %s %s F%i\n", gcmd, tx_str, ty_str, feed);
		} else if (gcmd == 2 || gcmd == 3) {
			fprintf(fd_out, "G0%i %s %s R%f F%i\n", gcmd, tx_str, ty_str, r, feed);
		}
	}
}

void object_draw (FILE *fd_out, int object_num) {
	int num = 0;
	int num2 = 0;
	int last = 0;
	int lasermode = 0;
	char tmp_str[1024];
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
//		if (strcmp(shapeEV[num2].Label, myOBJECTS[object_num].layer) == 0) {
//			if (layer_force[num2] == 0) {
//				layer_depth[num2] = mill_depth_real;
//			} else {
//				mill_depth_real = layer_depth[num2];
//			}
//		}
	}
	if (object_force[object_num] == 1) {
		mill_depth_real = object_depth[object_num];
	} else {
		object_depth[object_num] = mill_depth_real;
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
			glColor4f(0.0, 1.0, 0.0, 1.0);
			draw_oline((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, (float)myLINES[lnum].x2, (float)myLINES[lnum].y2, mill_depth_real);
			if (myOBJECTS[object_num].closed == 0) {
				draw_line2((float)myLINES[lnum].x1, (float)myLINES[lnum].y1, 0.01, (float)myLINES[lnum].x2, (float)myLINES[lnum].y2, 0.01, (PARAMETER[P_TOOL_DIAMETER].vdouble));
			}
			if (PARAMETER[P_V_NCDEBUG].vint == 11) {
				if (num == 0) {
					if (lasermode == 1) {
						if (tool_last != 5) {
							if (save_gcode == 1) {
								fprintf(fd_out, "M06 T%i (Change-Tool / Laser-Mode)\n", 5);
							}
						}
						tool_last = 5;
					}
					mill_xy(0, myLINES[lnum].x1, myLINES[lnum].y1, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
					if (lasermode == 1) {
						mill_z(0, 0.0);
						fprintf(fd_out, "M03 (Laser-On)\n");
					}
				}
				if (myLINES[lnum].type == TYPE_ARC || myLINES[lnum].type == TYPE_CIRCLE) {
					if (myLINES[lnum].opt < 0) {
						mill_xy(2, myLINES[lnum].x2, myLINES[lnum].y2, myLINES[lnum].opt * -1, PARAMETER[P_M_FEEDRATE].vint, "");
					} else {
						mill_xy(3, myLINES[lnum].x2, myLINES[lnum].y2, myLINES[lnum].opt, PARAMETER[P_M_FEEDRATE].vint, "");
					}
				} else {
					mill_xy(1, myLINES[lnum].x2, myLINES[lnum].y2, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
				}
			}
			if (num == 0) {
				if (PARAMETER[P_M_ROTARYMODE].vint == 0) {
					glColor4f(1.0, 1.0, 1.0, 1.0);
					sprintf(tmp_str, "%i", object_num);
					draw_text(GLUT_BITMAP_HELVETICA_18, tmp_str, (float)myLINES[lnum].x1, (float)myLINES[lnum].y1);
				}
			}
		}
	}

/*
	GLUquadricObj *quadratic = gluNewQuadric();
	glPushMatrix();
	glTranslatef((float)size_x / (float)width * (float)last_mouse_x, (float)size_y - ((float)size_y / (float)height * (float)last_mouse_y), 4.0);
	gluCylinder(quadratic, 0.0, 5.0, 5.0, 32, 1);
	glPopMatrix();
*/

	if (PARAMETER[P_M_ROTARYMODE].vint == 0) {
		if (PARAMETER[P_V_HELPLINES].vint == 1) {
			if (myOBJECTS[object_num].closed == 1 && myOBJECTS[object_num].inside == 0) {
				object2poly(object_num, 0.0, mill_depth_real, 0);
			} else if (myOBJECTS[object_num].inside == 1 && mill_depth_real > PARAMETER[P_M_DEPTH].vdouble) {
				object2poly(object_num, mill_depth_real - 0.001, mill_depth_real - 0.001, 1);
			}
		}
	}
}

void mill_move_in (double x, double y, double depth, int lasermode) {
	// move to
	if (lasermode == 1) {
		if (tool_last != 5) {
			if (save_gcode == 1) {
				fprintf(fd_out, "M06 T%i (Change-Tool / Laser-Mode)\n", 5);
			}
			mill_z(0, PARAMETER[P_CUT_SAVE].vdouble);
		}
		tool_last = 5;
		mill_xy(0, x, y, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
		mill_z(0, 0.0);
		if (save_gcode == 1) {
			fprintf(fd_out, "M03 (Laser-On)\n");
		}
	} else {
		if (tool_last != PARAMETER[P_TOOL_NUM].vint) {
			if (save_gcode == 1) {
				fprintf(fd_out, "M06 T%i (Change-Tool)\n", PARAMETER[P_TOOL_NUM].vint);
			}
		}
		tool_last = PARAMETER[P_TOOL_NUM].vint;
		if (save_gcode == 1) {
			fprintf(fd_out, "M03 S%i (Spindle-On / CW)\n", PARAMETER[P_TOOL_SPEED_MAX].vint);
			fprintf(fd_out, "\n");
		}

		mill_z(0, PARAMETER[P_CUT_SAVE].vdouble);
		mill_xy(0, x, y, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
	}
}


void mill_move_out (int lasermode) {
	// move out
	if (lasermode == 1) {
		if (save_gcode == 1) {
			fprintf(fd_out, "M05 (Laser-Off)\n");
			fprintf(fd_out, "\n");
		}
	} else {
		mill_z(0, PARAMETER[P_CUT_SAVE].vdouble);
		if (save_gcode == 1) {
			fprintf(fd_out, "\n");
		}
	}
}


void object_draw_offset_depth (FILE *fd_out, int object_num, double depth, double *next_x, double *next_y, double tool_offset, int overcut, int lasermode, int offset) {
	int error = 0;
	int lnum1 = 0;
	int lnum2 = 0;
	int num = 0;
	int last = 0;
	int last_lnum = 0;
	double first_x = 0.0;
	double first_y = 0.0;
	double last_x = 0.0;
	double last_y = 0.0;

	/* find last line in object */
	for (num = 0; num < line_last; num++) {
		if (myOBJECTS[object_num].line[num] != 0) {
			last = myOBJECTS[object_num].line[num];
		}
	}

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
				double check1_x = myLINES[lnum1].x1;
				double check1_y = myLINES[lnum1].y1;
				add_angle_offset(&check1_x, &check1_y, tool_offset, alpha1 + 90);
				double check1b_x = myLINES[lnum1].x2;
				double check1b_y = myLINES[lnum1].y2;
				add_angle_offset(&check1b_x, &check1b_y, 0.0, alpha1);
				add_angle_offset(&check1b_x, &check1b_y, tool_offset, alpha1 + 90);

				// line2 Offsets & Angle
				double alpha2 = line_angle(lnum2);
				double check2_x = myLINES[lnum2].x1;
				double check2_y = myLINES[lnum2].y1;
				add_angle_offset(&check2_x, &check2_y, 0.0, alpha2);
				add_angle_offset(&check2_x, &check2_y, tool_offset, alpha2 + 90);
				double check2b_x = myLINES[lnum2].x2;
				double check2b_y = myLINES[lnum2].y2;
				add_angle_offset(&check2b_x, &check2b_y, tool_offset, alpha2 + 90);

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
				alpha1 = line_angle2(lnum1);
				alpha2 = line_angle2(lnum2);
				alpha_diff = alpha2 - alpha1;
				if (alpha_diff > 180.0) {
					alpha_diff -= 360.0;
				}
				if (alpha_diff < -180.0) {
					alpha_diff += 360.0;
				}
				if (alpha_diff == 0.0) {
				} else if (alpha_diff > 0.0) {
					// Aussenkante
					if (num == 0) {
						first_x = check1b_x;
						first_y = check1b_y;
						if (mill_start == 0) {
							mill_move_in(first_x, first_y, depth, lasermode);
							mill_start = 1;
						}
						if (save_gcode == 1) {
							fprintf(fd_out, "\n");
						}
						mill_z(1, depth);
						mill_xy(2, check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, "");
					} else {
						if (myLINES[lnum1].type == TYPE_ARC || myLINES[lnum1].type == TYPE_CIRCLE) {
							if (myLINES[lnum1].opt < 0) {
								mill_xy(2, check1b_x, check1b_y, (myLINES[lnum1].opt - tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, "");
							} else {
								mill_xy(3, check1b_x, check1b_y, (myLINES[lnum1].opt - tool_offset), PARAMETER[P_M_FEEDRATE].vint, "");
							}
							if (myLINES[lnum2].type == TYPE_ARC || myLINES[lnum2].type == TYPE_CIRCLE) {
							} else {
								mill_xy(2, check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, "");
							}
						} else {
							mill_xy(1, check1b_x, check1b_y, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
							mill_xy(2, check2_x, check2_y, tool_offset, PARAMETER[P_M_FEEDRATE].vint, "");
						}
					}
					last_x = check2_x;
					last_y = check2_y;
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
						if (mill_start == 0) {
							mill_move_in(first_x, first_y, depth, lasermode);
							mill_start = 1;
							last_x = first_x;
							last_y = first_y;
						}
						if (save_gcode == 1) {
							fprintf(fd_out, "\n");
						}
						mill_z(1, depth);
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
							mill_xy(1, enx, eny, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
							mill_xy(1, px, py, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
							last_x = px;
							last_y = py;
						}
					} else {
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
						}
						if (myLINES[lnum1].type == TYPE_ARC || myLINES[lnum1].type == TYPE_CIRCLE) {
							if (myLINES[lnum1].opt < 0) {
								mill_xy(2, px, py, (myLINES[lnum1].opt - tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, "");
							} else {
								mill_xy(3, px, py, (myLINES[lnum1].opt - tool_offset), PARAMETER[P_M_FEEDRATE].vint, "");
							}
						} else {
							mill_xy(1, px, py, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
							if (overcut == 1 && myLINES[lnum1].type == TYPE_LINE && myLINES[lnum2].type == TYPE_LINE) {
								mill_xy(1, enx, eny, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
								mill_xy(1, px, py, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
							}
						}
						last_x = px;
						last_y = py;
					}
				}
			} else {
				if (num == 0) {
					first_x = myLINES[lnum2].x1;
					first_y = myLINES[lnum2].y1;
					mill_move_in(first_x, first_y, depth, lasermode);
					mill_start = 1;
					if (save_gcode == 1) {
						fprintf(fd_out, "\n");
					}
					mill_z(1, depth);
				}
				double alpha1 = line_angle2(lnum1);
				double alpha2 = line_angle2(lnum2);
				double alpha_diff = alpha2 - alpha1;
				if (myLINES[lnum2].type == TYPE_ARC || myLINES[lnum2].type == TYPE_CIRCLE) {
					if (myLINES[lnum2].opt < 0) {
						mill_xy(2, myLINES[lnum2].x2, myLINES[lnum2].y2, myLINES[lnum2].opt * -1, PARAMETER[P_M_FEEDRATE].vint, "");
					} else {
						mill_xy(3, myLINES[lnum2].x2, myLINES[lnum2].y2, myLINES[lnum2].opt, PARAMETER[P_M_FEEDRATE].vint, "");
					}
				} else {
					if (PARAMETER[P_M_KNIFEMODE].vint == 1) {
						if (alpha_diff > 180.0) {
							alpha_diff -= 360.0;
						}
						if (alpha_diff < -180.0) {
							alpha_diff += 360.0;
						}
						if (alpha_diff > PARAMETER[P_H_KNIFEMAXANGLE].vdouble || alpha_diff < -PARAMETER[P_H_KNIFEMAXANGLE].vdouble) {
							mill_z(0, PARAMETER[P_CUT_SAVE].vdouble);
							if (save_gcode == 1) {
								fprintf(fd_out, "  (TAN: %f)\n", alpha2);
							}
							mill_z(1, depth);
						} else {
							if (save_gcode == 1) {
								fprintf(fd_out, "  (TAN: %f)\n", alpha2);
							}
						}
					}
					mill_xy(1, myLINES[lnum2].x2, myLINES[lnum2].y2, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
				}
				last_x = myLINES[lnum2].x2;
				last_y = myLINES[lnum2].y2;
			}
			last_lnum = lnum2;
		}
	}
	if (myOBJECTS[object_num].closed == 1) {
		if (myLINES[last].type == TYPE_ARC || myLINES[last].type == TYPE_CIRCLE) {
			if (myLINES[last].opt < 0) {
				mill_xy(2, first_x, first_y, (myLINES[last].opt - tool_offset) * -1, PARAMETER[P_M_FEEDRATE].vint, "");
			} else {
				mill_xy(3, first_x, first_y, (myLINES[last].opt - tool_offset), PARAMETER[P_M_FEEDRATE].vint, "");
			}
		} else {
			mill_xy(1, first_x, first_y, 0.0, PARAMETER[P_M_FEEDRATE].vint, "");
		}
		last_x = first_x;
		last_y = first_y;
	}

	*next_x = last_x;
	*next_y = last_y;

	if (save_gcode == 1) {
		fprintf(fd_out, "\n");
	}

	if (error > 0) {
		return;
	}
}


void object_draw_offset (FILE *fd_out, int object_num, double *next_x, double *next_y) {
	int num2 = 0;
	double depth = 0.0;
	double tool_offset = 0.0;
	int overcut = 0;
	int lasermode = 0;
	int offset = 0;

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
//		if (shapeEV[num2].Label != NULL && strcmp(shapeEV[num2].Label, myOBJECTS[object_num].layer) == 0) {
//			if (layer_force[num2] == 0) {
//				layer_depth[num2] = mill_depth_real;
//			} else {
//				mill_depth_real = layer_depth[num2];
//			}
//		}
	}
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
		tool_offset = PARAMETER[P_H_LASERDIA].vdouble / 2.0;
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

	mill_start = 0;

	// offset for each depth-step
	double new_depth = 0.0;
	for (depth = PARAMETER[P_M_Z_STEP].vdouble; depth > mill_depth_real + PARAMETER[P_M_Z_STEP].vdouble; depth += PARAMETER[P_M_Z_STEP].vdouble) {
		if (depth < mill_depth_real) {
			new_depth = mill_depth_real;
		} else {
			new_depth = depth;
		}
		object_draw_offset_depth(fd_out, object_num, new_depth, next_x, next_y, tool_offset, overcut, lasermode, offset);
	}

	mill_move_out(lasermode);
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
			if (myLINES[num2].type == TYPE_MTEXT) {
				myOBJECTS[object_num].closed = 0;
				object_num++;
			} else if (ret == 1) {
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

	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
		GLUquadricObj *quadratic = gluNewQuadric();
		float radius = (PARAMETER[P_MAT_DIAMETER].vdouble / 2.0) + PARAMETER[P_M_DEPTH].vdouble;
		float radius2 = (PARAMETER[P_MAT_DIAMETER].vdouble / 2.0);

		glPushMatrix();
		glTranslatef(0.0, -radius2 - 10.0, 0.0);
		float lenX = size_x;
		float offXYZ = 10.0;
		float arrow_d = 1.0;
		float arrow_l = 6.0;
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
		glTranslatef(-lenX / 2.0, -arrow_d * 2.0 - 11.0, 0.0);
		sprintf(tmp_str, "%0.2fmm", lenX);
		output_text_gl_center(tmp_str, 0.0, 0.0, 0.0, 0.2);
		glPopMatrix();
		glRotatef(-90.0, 0.0, 1.0, 0.0);
		gluCylinder(quadratic, 0.0, (arrow_d * 3), arrow_l ,32, 1);
		glTranslatef(0.0, 0.0, arrow_l);
		gluCylinder(quadratic, arrow_d, arrow_d, lenX - arrow_l * 2.0 ,32, 1);
		glTranslatef(0.0, 0.0, lenX - arrow_l * 2.0);
		gluCylinder(quadratic, (arrow_d * 3), 0.0, arrow_l ,32, 1);
		glPopMatrix();
		glPopMatrix();

		glColor4f(0.2, 0.2, 0.2, 0.5);
		glPushMatrix();
		glRotatef(90.0, 0.0, 1.0, 0.0);
		gluCylinder(quadratic, radius, radius, size_x ,64, 1);
		glTranslatef(0.0, 0.0, -size_x);
		gluCylinder(quadratic, radius2, radius2, size_x ,64, 1);
		glTranslatef(0.0, 0.0, size_x * 2);
		gluCylinder(quadratic, radius2, radius2, size_x ,64, 1);
		glPopMatrix();

		return;
	}

	/* Scale-Arrow's */
	float gridXYZ = PARAMETER[P_V_HELP_GRID].vfloat * 10.0;
	float gridXYZmin = PARAMETER[P_V_HELP_GRID].vfloat;
	float lenY = size_y;
	float lenX = size_x;
	float lenZ = PARAMETER[P_M_DEPTH].vdouble * -1;
	float offXYZ = 10.0;
	float arrow_d = 1.0 * PARAMETER[P_V_HELP_ARROW].vfloat;
	float arrow_l = 6.0 * PARAMETER[P_V_HELP_ARROW].vfloat;
	GLUquadricObj *quadratic = gluNewQuadric();

	int pos_n = 0;
	glColor4f(1.0, 1.0, 1.0, 0.3);
	for (pos_n = 0; pos_n <= lenY; pos_n += gridXYZ) {
		glBegin(GL_LINES);
		glVertex3f(0.0, pos_n, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glVertex3f(lenX, pos_n, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glEnd();
	}
	for (pos_n = 0; pos_n <= lenX; pos_n += gridXYZ) {
		glBegin(GL_LINES);
		glVertex3f(pos_n, 0.0, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glVertex3f(pos_n, lenY, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glEnd();
	}
	glColor4f(1.0, 1.0, 1.0, 0.2);
	for (pos_n = 0; pos_n <= lenY; pos_n += gridXYZmin) {
		glBegin(GL_LINES);
		glVertex3f(0.0, pos_n, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glVertex3f(lenX, pos_n, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glEnd();
	}
	for (pos_n = 0; pos_n <= lenX; pos_n += gridXYZmin) {
		glBegin(GL_LINES);
		glVertex3f(pos_n, 0.0, PARAMETER[P_M_DEPTH].vdouble - 0.1);
		glVertex3f(pos_n, lenY, PARAMETER[P_M_DEPTH].vdouble - 0.1);
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
	sprintf(tmp_str, "%0.2fmm", lenY);
	glPushMatrix();
	glTranslatef(0.0, 4.0, 0.0);
	output_text_gl_center(tmp_str, 0.0, 0.0, 0.0, 0.2);
	glPopMatrix();
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
	sprintf(tmp_str, "%0.2fmm", lenX);
	glPushMatrix();
	glTranslatef(0.0, -4.0, 0.0);
	output_text_gl_center(tmp_str, 0.0, 0.0, 0.0, 0.2);
	glPopMatrix();
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
	sprintf(tmp_str, "%0.2fmm", lenZ);
	glPushMatrix();
	glTranslatef(0.0, -4.0, 0.0);
	output_text_gl_center(tmp_str, 0.0, 0.0, 0.0, 0.2);
	glPopMatrix();
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
	char tmp_str[1024];

/*
	sprintf(tmp_str, "Width=%0.1fmm / Height=%0.1fmm (%f, %f)", size_x, size_y, \
		 (float)size_x / (float)width * (float)last_mouse_x, \
		 (float)size_y - ((float)size_y / (float)height * (float)last_mouse_y) \
	);
*/
	sprintf(tmp_str, "Width=%0.1fmm / Height=%0.1fmm", size_x, size_y);
	gtk_label_set_text(GTK_LABEL(SizeInfoLabel), tmp_str);

	/* get diameter from tooltable by number */
	if (PARAMETER[P_TOOL_SELECT].vint > 0) {
		PARAMETER[P_TOOL_NUM].vint = PARAMETER[P_TOOL_SELECT].vint;
		PARAMETER[P_TOOL_DIAMETER].vdouble = tooltbl_diameters[PARAMETER[P_TOOL_NUM].vint];
	} else {
	}
	if (PARAMETER[P_M_LASERMODE].vint == 1) {
	} else {
	}
	if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
	} else {
	}
	if (PARAMETER[P_M_KNIFEMODE].vint == 1) {
	} else {
	}
	PARAMETER[P_TOOL_SPEED_MAX].vint = (int)(((float)material_vc[PARAMETER[P_MAT_SELECT].vint] * 1000.0) / (PI * (PARAMETER[P_TOOL_DIAMETER].vdouble)));
	if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 4.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][0] * (float)PARAMETER[P_TOOL_W].vint);
	} else if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 8.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][1] * (float)PARAMETER[P_TOOL_W].vint);
	} else if ((PARAMETER[P_TOOL_DIAMETER].vdouble) < 12.0) {
		PARAMETER[P_M_FEEDRATE_MAX].vint = (int)((float)PARAMETER[P_TOOL_SPEED].vint * material_fz[PARAMETER[P_MAT_SELECT].vint][2] * (float)PARAMETER[P_TOOL_W].vint);
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

		glScalef(PARAMETER[P_V_ZOOM].vfloat, PARAMETER[P_V_ZOOM].vfloat, PARAMETER[P_V_ZOOM].vfloat);
		glScalef(scale, scale, scale);
		glTranslatef(PARAMETER[P_V_TRANSX].vint, PARAMETER[P_V_TRANSY].vint, 0.0);

		glRotatef(PARAMETER[P_V_ROTZ].vfloat, 0.0, 0.0, 1.0);
		glRotatef(PARAMETER[P_V_ROTY].vfloat, 0.0, 1.0, 0.0);
		glRotatef(PARAMETER[P_V_ROTX].vfloat, 1.0, 0.0, 0.0);

		glTranslatef(-size_x / 2.0, 0.0, 0.0);
		if (PARAMETER[P_M_ROTARYMODE].vint == 0) {
			glTranslatef(0.0, -size_y / 2.0, 0.0);
		}
		if (PARAMETER[P_V_HELPLINES].vint == 1) {
			if (PARAMETER[P_M_ROTARYMODE].vint == 0) {
				draw_helplines();
			}
		}

	} else {
		PARAMETER[P_V_HELPLINES].vint = 0;
		save_gcode = 1;
	}

	mill_start_all = 0;

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
		SetupShowGcode(fd_out);
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
			if (myLINES[myOBJECTS[shortest_object].line[0]].type == TYPE_MTEXT) {
				if (PARAMETER[P_M_TEXT].vint == 1) {
					object_draw_offset(fd_out, shortest_object, &next_x, &next_y);
					object_draw(fd_out, shortest_object);
				}
			} else {
				object_draw_offset(fd_out, shortest_object, &next_x, &next_y);
				object_draw(fd_out, shortest_object);
			}
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
			if (myLINES[myOBJECTS[shortest_object].line[0]].type == TYPE_MTEXT) {
			} else {
				object_draw_offset(fd_out, shortest_object, &next_x, &next_y);
				object_draw(fd_out, shortest_object);
			}
			last_x = next_x;
			last_y = next_y;


		} else {
			break;
		}
	}

	if (PARAMETER[P_V_HELPLINES].vint == 1) {
		if (PARAMETER[P_M_ROTARYMODE].vint == 1) {
			draw_helplines();
		}
	}

	/* draw text / opengl only */
	int nnum = 0;
	if (batchmode == 0) {
		for (nnum = 0; nnum < 100; nnum++) {
			if (myMTEXT[nnum].used == 1) {
				if (PARAMETER[P_M_TEXT].vint == 0) {
					glColor4f(1.0, 1.0, 1.0, 1.0);
					output_text_gl(myMTEXT[nnum].text, myMTEXT[nnum].x, myMTEXT[nnum].y - myMTEXT[nnum].s, 0.10, myMTEXT[nnum].s);
				}
			}
		}
	}

	/* exit gcode */
	if (save_gcode == 1) {
		fprintf(fd_out, "M05 (Spindle-/Laser-Off)\n");
		fprintf(fd_out, "M02 (Programm-End)\n");
		fclose(fd_out);
		if (PARAMETER[P_POST_CMD].vstr[0] != 0) {
			char cmd_str[2048];
			sprintf(cmd_str, PARAMETER[P_POST_CMD].vstr, PARAMETER[P_MFILE].vstr);
			system(cmd_str);
		}
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "saving g-code...done"), "saving g-code...done");
		save_gcode = 0;
	}

	if (batchmode == 1) {
		onExit();
		exit(0);
	} else {
		glPopMatrix();
	}
	return;
}


void MaterialLoadList (void) {
/*
    Stahl: 40 – 120 m/min
    Aluminium: 100 – 500 m/min
    Kupfer, Messing und Bronze: 100 – 200 m/min
    Kunststoffe: 50 – 150 m/min
    Holz: Bis zu 3000 m/min
*/

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "Aluminium(Langspanend)", -1);
	material_vc[0] = 200;
	material_fz[0][0] = 0.04;
	material_fz[0][1] = 0.05;
	material_fz[0][2] = 0.10;
	material_texture[0] = "metall.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "Aluminium(Kurzspanend)", -1);
	material_vc[1] = 150;
	material_fz[1][0] = 0.04;
	material_fz[1][1] = 0.05;
	material_fz[1][2] = 0.10;
	material_texture[1] = "metall.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "NE-Metalle(Messing,Bronze,Kupfer,Zink,Rotguß)", -1);
	material_vc[2] = 150;
	material_fz[2][0] = 0.04;
	material_fz[2][1] = 0.05;
	material_fz[2][2] = 0.10;
	material_texture[2] = "metall.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "VA-Stahl", -1);
	material_vc[3] = 100;
	material_fz[3][0] = 0.05;
	material_fz[3][1] = 0.06;
	material_fz[3][2] = 0.07;
	material_texture[3] = "metall.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "Thermoplaste", -1);
	material_vc[4] = 100;
	material_fz[4][0] = 0.0;
	material_fz[4][1] = 0.0;
	material_fz[4][2] = 0.0;
	material_texture[4] = "plast.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "Duroplaste(mit Füllstoffen)", -1);
	material_vc[5] = 125;
	material_fz[5][0] = 0.04;
	material_fz[5][1] = 0.08;
	material_fz[5][2] = 0.10;
	material_texture[5] = "plast.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "GFK", -1);
	material_vc[6] = 125;
	material_fz[6][0] = 0.04;
	material_fz[6][1] = 0.08;
	material_fz[6][2] = 0.10;
	material_texture[6] = "gfk.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "CFK", -1);
	material_vc[7] = 125;
	material_fz[7][0] = 0.04;
	material_fz[7][1] = 0.08;
	material_fz[7][2] = 0.10;
	material_texture[7] = "cfk.bmp";

	gtk_list_store_insert_with_values(ListStore[P_MAT_SELECT], NULL, -1, 0, NULL, 1, "Holz", -1);
	material_vc[8] = 2000;
	material_fz[8][0] = 0.04;
	material_fz[8][1] = 0.08;
	material_fz[8][2] = 0.10;
	material_texture[8] = "holz.bmp";



	material_max = 9;
}

void ToolLoadTable (void) {
	/* import Tool-Diameter from Tooltable */
	int n = 0;
	char tmp_str[1024];
	tools_max = 0;
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
		gtk_list_store_clear(ListStore[P_TOOL_SELECT]);
		sprintf(tmp_str, "FREE");
		gtk_list_store_insert_with_values(ListStore[P_TOOL_SELECT], NULL, -1, 0, NULL, 1, tmp_str, -1);
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
					gtk_list_store_insert_with_values(ListStore[P_TOOL_SELECT], NULL, -1, 0, NULL, 1, tmp_str, -1);
					n++;
					tools_max++;
				}
			}
		}
		fclose(tt_fp);
	}
}

void LayerLoadList (void) {
}

void DrawCheckSize (void) {
	int num2 = 0;
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
}

void DrawSetZero (void) {
	int num = 0;
	/* set bottom-left to 0,0 */
	for (num = 0; num < line_last; num++) {
		if (myLINES[num].used == 1) {
			myLINES[num].x1 -= min_x;
			myLINES[num].y1 -= min_y;
			myLINES[num].x2 -= min_x;
			myLINES[num].y2 -= min_y;
		}
	}
	for (num = 0; num < 100; num++) {
		if (myMTEXT[num].used == 1) {
			myMTEXT[num].x -= min_x;
			myMTEXT[num].y -= min_y;
		}
	}
}

void ArgsRead (int argc, char **argv) {
	int num = 0;
	if (argc < 2) {
//		SetupShowHelp();
//		exit(1);
	}
	mill_layer[0] = 0;
	PARAMETER[P_V_DXF].vstr[0] = 0;
	strcpy(PARAMETER[P_MFILE].vstr, "-");
	for (num = 1; num < argc; num++) {
		if (SetupArgCheck(argv[num], argv[num + 1]) == 1) {
			num++;
		} else if (strcmp(argv[num], "-h") == 0) {
			SetupShowHelp();
			exit(0);
		} else if (num != argc - 1) {
			fprintf(stderr, "### unknown argument: %s ###\n", argv[num]);
			SetupShowHelp();
			exit(1);
		} else {
			strcpy(PARAMETER[P_V_DXF].vstr, argv[argc - 1]);
		}
	}
}

void view_init_gl(void) {
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(glCanvas);
	GdkGLContext *glcontext = gtk_widget_get_gl_context(glCanvas);
	if (gdk_gl_drawable_gl_begin(gldrawable, glcontext)) {
		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective((M_PI / 4) / M_PI * 180, (float)width / (float)height, 0.1, 1000.0);
		gluLookAt(0, 0, 6,  0, 0, 0,  0, 1, 0);
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_NORMALIZE);
		gdk_gl_drawable_gl_end(gldrawable);
	}
}

void view_draw (void) {
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(glCanvas);
	GdkGLContext *glcontext = gtk_widget_get_gl_context(glCanvas);
	if (gdk_gl_drawable_gl_begin(gldrawable, glcontext)) {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ParameterUpdate();
		mainloop();
		if (gdk_gl_drawable_is_double_buffered (gldrawable)) {
			gdk_gl_drawable_swap_buffers(gldrawable);
		} else {
			glFlush();
		}
		gdk_gl_drawable_gl_end(gldrawable);
	}
}

void handler_destroy (GtkWidget *widget, gpointer data) {
	gtk_main_quit();
}

void handler_load_dxf (GtkWidget *widget, gpointer data) {
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Load DXF-Drawing",
		GTK_WINDOW(window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	GtkFileFilter *ffilter;
	ffilter = gtk_file_filter_new();
	gtk_file_filter_set_name(ffilter, "DXF-Drawings");
	gtk_file_filter_add_pattern(ffilter, "*.dxf");
	gtk_file_filter_add_pattern(ffilter, "*.DXF");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ffilter);
/*
	ffilter = gtk_file_filter_new();
	gtk_file_filter_set_name(ffilter, "DWG-Drawings");
	gtk_file_filter_add_pattern(ffilter, "*.dwg");
	gtk_file_filter_add_pattern(ffilter, "*.DXF");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ffilter);
	
	ffilter = gtk_file_filter_new();
	gtk_file_filter_set_name(ffilter, "SVG-Drawings");
	gtk_file_filter_add_pattern(ffilter, "*.scg");
	gtk_file_filter_add_pattern(ffilter, "*.SVG");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ffilter);
*/
	if (PARAMETER[P_TOOL_TABLE].vstr[0] == 0) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), "/tmp");
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "test.dxf");
	} else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), PARAMETER[P_V_DXF].vstr);
	}
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		strcpy(PARAMETER[P_V_DXF].vstr, filename);
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "reading dxf..."), "reading dxf...");
		dxf_read(PARAMETER[P_V_DXF].vstr);
		init_objects();
		DrawCheckSize();
		DrawSetZero();
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "reading dxf...done"), "reading dxf...done");
		g_free(filename);
	}
	gtk_widget_destroy (dialog);
}

char *suffix_remove (char *mystr) {
	char *retstr;
	char *lastdot;
	if (mystr == NULL) {
		return NULL;
	}
	if ((retstr = malloc (strlen (mystr) + 1)) == NULL) {
        	return NULL;
	}
	strcpy(retstr, mystr);
	lastdot = strrchr(retstr, '.');
	if (lastdot != NULL) {
		*lastdot = '\0';
	}
	return retstr;
}

void handler_save_gcode (GtkWidget *widget, gpointer data) {
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Save G-Code File",
		GTK_WINDOW(window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	GtkFileFilter *ffilter;
	ffilter = gtk_file_filter_new();
	gtk_file_filter_set_name(ffilter, "G-Code");
	gtk_file_filter_add_pattern(ffilter, "*.ngc");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ffilter);

	if (PARAMETER[P_MFILE].vstr[0] == 0) {
		char dir[2048];
		strcpy(dir, PARAMETER[P_V_DXF].vstr);
		dirname(dir);
		char file[2048];
		strcpy(file, basename(PARAMETER[P_V_DXF].vstr));
		char *file_nosuffix = suffix_remove(file);
		file_nosuffix = realloc(file_nosuffix, strlen(file_nosuffix) + 5);
		strcat(file_nosuffix, ".ngc");

		if (strstr(PARAMETER[P_V_DXF].vstr, "/") > 0) {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);
		} else {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "./");
		}

		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), file_nosuffix);
		free(file_nosuffix);
	} else {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (dialog), PARAMETER[P_MFILE].vstr);
	}
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		strcpy(PARAMETER[P_MFILE].vstr, filename);
		g_free(filename);
		save_gcode = 1;
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "saving g-code..."), "saving g-code...");
	}
	gtk_widget_destroy (dialog);
}

void handler_load_tooltable (GtkWidget *widget, gpointer data) {
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Load Tooltable",
		GTK_WINDOW(window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	GtkFileFilter *ffilter;
	ffilter = gtk_file_filter_new();
	gtk_file_filter_set_name(ffilter, "Tooltable");
	gtk_file_filter_add_pattern(ffilter, "*.tbl");
	gtk_file_filter_add_pattern(ffilter, "*.TBL");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ffilter);

	if (PARAMETER[P_TOOL_TABLE].vstr[0] == 0) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), "/tmp");
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "tool.tbl");
	} else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), PARAMETER[P_TOOL_TABLE].vstr);
	}
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		strcpy(PARAMETER[P_TOOL_TABLE].vstr, filename);
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "loading tooltable..."), "loading tooltable...");
		ToolLoadTable();
		gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "loading tooltable...done"), "loading tooltable...done");
		g_free(filename);
	}
	gtk_widget_destroy (dialog);
}

void handler_save_setup (GtkWidget *widget, gpointer data) {
	gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "saving setup...done"), "saving setup...done");
	SetupSave();
	gtk_statusbar_push(GTK_STATUSBAR(StatusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar), "saving setup...done"), "saving setup...done");
}

void handler_about (GtkWidget *widget, gpointer data) {
	GtkWidget *dialog = gtk_dialog_new();
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
	gtk_window_set_title(GTK_WINDOW(dialog), "About");
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_QUIT, 1);
	GtkWidget *label = gtk_label_new("OpenCAM 2D\nCopyright by Oliver Dippel <oliver@multixmedia.org>\nMac-Port by McUles <fpv-community.de>");
	gtk_widget_modify_font(label, pango_font_description_from_string("Tahoma 18"));
	GtkWidget *image = gtk_image_new_from_file("logo.png");
	GtkWidget *box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
}

void handler_draw (GtkWidget *w, GdkEventExpose* e, void *v) {
}

void handler_scrollwheel (GtkWidget *w, GdkEventButton* e, void *v) {
	if (e->state == 0) {
		PARAMETER[P_V_ZOOM].vfloat += 0.05;
	} else if (e->state == 1) {
		PARAMETER[P_V_ZOOM].vfloat -= 0.05;
	}
}

void handler_button_press (GtkWidget *w, GdkEventButton* e, void *v) {
//	printf("button_press x=%g y=%g b=%d state=%d\n", e->x, e->y, e->button, e->state);
	int mouseX = e->x;
	int mouseY = e->y;
	int state = e->state;
	int button = e->button;;

	if (button == 4 && state == 0) {
		PARAMETER[P_V_ZOOM].vfloat += 0.05;
	} else if (button == 5 && state == 0) {
		PARAMETER[P_V_ZOOM].vfloat -= 0.05;
	} else if (button == 1) {
		if (state == 0) {
			last_mouse_x = mouseX - PARAMETER[P_V_TRANSX].vint * 2;
			last_mouse_y = mouseY - PARAMETER[P_V_TRANSY].vint * -2;
			last_mouse_button = button;
			last_mouse_state = state;
		} else {
			last_mouse_button = button;
			last_mouse_state = state;
		}
	} else if (button == 2) {
		if (state == 0) {
			last_mouse_x = mouseX - (int)(PARAMETER[P_V_ROTY].vfloat * 5.0);
			last_mouse_y = mouseY - (int)(PARAMETER[P_V_ROTX].vfloat * 5.0);
			last_mouse_button = button;
			last_mouse_state = state;
		} else {
			last_mouse_button = button;
			last_mouse_state = state;
		}
	} else if (button == 3) {
		if (state == 0) {
			last_mouse_x = mouseX - (int)(PARAMETER[P_V_ROTZ].vfloat * 5.0);;
			last_mouse_y = mouseY - (int)(PARAMETER[P_V_ZOOM].vfloat * 100.0);
			last_mouse_button = button;
			last_mouse_state = state;
		} else {
			last_mouse_button = button;
			last_mouse_state = state;
		}
	}
}

void handler_button_release (GtkWidget *w, GdkEventButton* e, void *v) {
//	printf("button_release x=%g y=%g b=%d state=%d\n", e->x, e->y, e->button, e->state);
	last_mouse_button = -1;
}

void handler_motion (GtkWidget *w, GdkEventMotion* e, void *v) {
//	printf("button_motion x=%g y=%g state=%d\n", e->x, e->y, e->state);
	int mouseX = e->x;
	int mouseY = e->y;
	if (last_mouse_button == 1 && last_mouse_state == 0) {
		PARAMETER[P_V_TRANSX].vint = (mouseX - last_mouse_x) / 2;
		PARAMETER[P_V_TRANSY].vint = (mouseY - last_mouse_y) / -2;
	} else if (last_mouse_button == 2 && last_mouse_state == 0) {
		PARAMETER[P_V_ROTY].vfloat = (float)(mouseX - last_mouse_x) / 5.0;
		PARAMETER[P_V_ROTX].vfloat = (float)(mouseY - last_mouse_y) / 5.0;
	} else if (last_mouse_button == 3 && last_mouse_state == 0) {
		PARAMETER[P_V_ROTZ].vfloat = (float)(mouseX - last_mouse_x) / 5.0;
		PARAMETER[P_V_ZOOM].vfloat = (float)(mouseY - last_mouse_y) / 100;
	} else {
		last_mouse_x = mouseX;
		last_mouse_y = mouseY;
	}
}

void handler_keypress (GtkWidget *w, GdkEventKey* e, void *v) {
//	printf("key_press state=%d key=%s\n", e->state, e->string);
}

void handler_configure (GtkWidget *w, GdkEventConfigure* e, void *v) {
//	printf("configure width=%d height=%d ratio=%g\n", e->width, e->height, e->width/(float)e->height);
	width = e->width;
	height = e->height;
	need_init = 1;
}

int handler_periodic_action (gpointer d) {
	while ( gtk_events_pending() ) {
		gtk_main_iteration();
	}
	if (need_init == 1) {
		need_init = 0;
		view_init_gl();
	}
	view_draw();
	return(1);
}

GtkWidget *create_gl () {
	static GdkGLConfig *glconfig = NULL;
	static GdkGLContext *glcontext = NULL;
	GtkWidget *area;
	if (glconfig == NULL) {
		glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
		if (glconfig == NULL) {
			glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH);
			if (glconfig == NULL) {
				exit(1);
			}
		}
	}
	area = gtk_drawing_area_new();
	gtk_widget_set_gl_capability(area, glconfig, glcontext, TRUE, GDK_GL_RGBA_TYPE); 
	gtk_widget_set_events(GTK_WIDGET(area)
		,GDK_POINTER_MOTION_MASK 
		|GDK_BUTTON_PRESS_MASK 
		|GDK_BUTTON_RELEASE_MASK
		|GDK_ENTER_NOTIFY_MASK
		|GDK_KEY_PRESS_MASK
		|GDK_KEY_RELEASE_MASK
		|GDK_EXPOSURE_MASK
	);
	gtk_signal_connect(GTK_OBJECT(area), "enter-notify-event", GTK_SIGNAL_FUNC(gtk_widget_grab_focus), NULL);
	return(area);
}

void ParameterChanged (GtkWidget *widget, gpointer data) {
	int n = (int)data;
//	printf("UPDATED(%i): %s\n", n, PARAMETER[n].name);
	if (PARAMETER[n].type == T_FLOAT) {
		PARAMETER[n].vfloat = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));
	} else if (PARAMETER[n].type == T_DOUBLE) {
		PARAMETER[n].vdouble = (double)gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));
	} else if (PARAMETER[n].type == T_INT) {
		PARAMETER[n].vint = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	} else if (PARAMETER[n].type == T_SELECT) {
		PARAMETER[n].vint = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	} else if (PARAMETER[n].type == T_BOOL) {
		PARAMETER[n].vint = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	} else if (PARAMETER[n].type == T_STRING) {
		strcpy(PARAMETER[n].vstr, (char *)gtk_entry_get_text(GTK_ENTRY(widget)));
	} else if (PARAMETER[n].type == T_FILE) {
		strcpy(PARAMETER[n].vstr, (char *)gtk_entry_get_text(GTK_ENTRY(widget)));
	}
}

void ParameterUpdate (void) {
	GtkTreeIter iter;
	char path[1024];
	char value2[1024];
	int n = 0;
	for (n = 0; n < P_LAST; n++) {
		if (PARAMETER[n].type == T_FLOAT) {
			if (PARAMETER[n].vfloat != gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ParamValue[n]))) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(ParamValue[n]), PARAMETER[n].vfloat);
			}
		} else if (PARAMETER[n].type == T_DOUBLE) {
			if (PARAMETER[n].vdouble != gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ParamValue[n]))) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(ParamValue[n]), PARAMETER[n].vdouble);
			}
		} else if (PARAMETER[n].type == T_INT) {
			if (PARAMETER[n].vint != gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ParamValue[n]))) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(ParamValue[n]), PARAMETER[n].vint);
			}
		} else if (PARAMETER[n].type == T_SELECT) {
			if (PARAMETER[n].vint != gtk_combo_box_get_active(GTK_COMBO_BOX(ParamValue[n]))) {
				gtk_combo_box_set_active(GTK_COMBO_BOX(ParamValue[n]), PARAMETER[n].vint);
			}
		} else if (PARAMETER[n].type == T_BOOL) {
			if (PARAMETER[n].vint != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ParamValue[n]))) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ParamValue[n]), PARAMETER[n].vint);
			}
		} else if (PARAMETER[n].type == T_STRING) {
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), PARAMETER[n].vstr);
		} else if (PARAMETER[n].type == T_FILE) {
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), PARAMETER[n].vstr);
		} else {
			continue;
		}
	}
	for (n = 0; n < P_LAST; n++) {
		sprintf(path, "0:%i:%i", PARAMETER[n].l1, PARAMETER[n].l2);
		if (PARAMETER[n].type == T_FLOAT) {
			sprintf(value2, "%f", PARAMETER[n].vfloat);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			sprintf(value2, "%f", PARAMETER[n].vdouble);
		} else if (PARAMETER[n].type == T_INT) {
			sprintf(value2, "%i", PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_SELECT) {
			sprintf(value2, "%i", PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_BOOL) {
			sprintf(value2, "%i", PARAMETER[n].vint);
		} else if (PARAMETER[n].type == T_STRING) {
			sprintf(value2, "%s", PARAMETER[n].vstr);
		} else if (PARAMETER[n].type == T_FILE) {
			sprintf(value2, "%s", PARAMETER[n].vstr);
		} else {
			continue;
		}
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore), &iter, path);
		gtk_tree_store_set(GTK_TREE_STORE(treestore), &iter, 1, value2, -1);
	}
}

GtkTreeModel *create_and_fill_model (void) {
	GtkTreeIter toplevel;
	GtkTreeIter value;
	treestore = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	gtk_tree_store_append(treestore, &toplevel, NULL);
	gtk_tree_store_set(treestore, &toplevel, 0, "Parameter", -1);

	GtkTreeIter childView;
	gtk_tree_store_append(treestore, &childView, &toplevel);
	gtk_tree_store_set(treestore, &childView, 0, "View", -1);

	GtkTreeIter childTool;
	gtk_tree_store_append(treestore, &childTool, &toplevel);
	gtk_tree_store_set(treestore, &childTool, 0, "Tool", -1);

	GtkTreeIter childMilling;
	gtk_tree_store_append(treestore, &childMilling, &toplevel);
	gtk_tree_store_set(treestore, &childMilling, 0, "Milling", -1);

	GtkTreeIter childMachine;
	gtk_tree_store_append(treestore, &childMachine, &toplevel);
	gtk_tree_store_set(treestore, &childMachine, 0, "Machine", -1);

	GtkTreeIter childMaterial;
	gtk_tree_store_append(treestore, &childMaterial, &toplevel);
	gtk_tree_store_set(treestore, &childMaterial, 0, "Material", -1);

	int n = 0;
	for (n = 0; n < P_LAST; n++) {
		char name_str[1024];
		char tmp_str[1024];
		sprintf(name_str, "%s", PARAMETER[n].name);

		if (strcmp(PARAMETER[n].group, "View") == 0) {
			gtk_tree_store_append(treestore, &value, &childView);
		} else if (strcmp(PARAMETER[n].group, "Tool") == 0) {
			gtk_tree_store_append(treestore, &value, &childTool);
		} else if (strcmp(PARAMETER[n].group, "Milling") == 0) {
			gtk_tree_store_append(treestore, &value, &childMilling);
		} else if (strcmp(PARAMETER[n].group, "Machine") == 0) {
			gtk_tree_store_append(treestore, &value, &childMachine);
		} else if (strcmp(PARAMETER[n].group, "Material") == 0) {
			gtk_tree_store_append(treestore, &value, &childMaterial);
		}
		if (PARAMETER[n].type == T_FLOAT) {
			sprintf(tmp_str, "%f", PARAMETER[n].vfloat);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			sprintf(tmp_str, "%f", PARAMETER[n].vdouble);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_INT) {
			sprintf(tmp_str, "%i", PARAMETER[n].vint);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_SELECT) {
			sprintf(tmp_str, "%i", PARAMETER[n].vint);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_BOOL) {
			sprintf(tmp_str, "%i", PARAMETER[n].vint);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_STRING) {
			sprintf(tmp_str, "%s", PARAMETER[n].vstr);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else if (PARAMETER[n].type == T_FILE) {
			sprintf(tmp_str, "%s", PARAMETER[n].vstr);
			gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
		} else {
			continue;
		}
	}


	gtk_tree_store_append(treestore, &toplevel, NULL);
	gtk_tree_store_set(treestore, &toplevel, 0, "Objects", -1);

	int num2 = 0;
	for (num2 = 0; num2 < MAX_OBJECTS; num2++) {
		if (myOBJECTS[num2].line[0] != 0) {
			char tmp_str[1024];
			sprintf(tmp_str, "Object #%i", num2);

			GtkTreeIter childObject;
			gtk_tree_store_append(treestore, &childObject, &toplevel);
			gtk_tree_store_set(treestore, &childObject, 0, tmp_str, -1);

			for (n = 0; n < O_P_LAST; n++) {
				char name_str[1024];
				sprintf(name_str, "%s", OBJECT_PARAMETER[n].name);
				gtk_tree_store_append(treestore, &value, &childObject);
				if (OBJECT_PARAMETER[n].type == T_FLOAT) {
					sprintf(tmp_str, "%f", OBJECT_PARAMETER[n].vfloat);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_DOUBLE) {
					sprintf(tmp_str, "%f", OBJECT_PARAMETER[n].vdouble);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_INT) {
					sprintf(tmp_str, "%i", OBJECT_PARAMETER[n].vint);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_SELECT) {
					sprintf(tmp_str, "%i", OBJECT_PARAMETER[n].vint);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_BOOL) {
					sprintf(tmp_str, "%i", OBJECT_PARAMETER[n].vint);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_STRING) {
					sprintf(tmp_str, "%s", OBJECT_PARAMETER[n].vstr);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else if (OBJECT_PARAMETER[n].type == T_FILE) {
					sprintf(tmp_str, "%s", OBJECT_PARAMETER[n].vstr);
					gtk_tree_store_set(treestore, &value, 0, name_str, 1, tmp_str, -1);
				} else {
					continue;
				}
			}
		}
	}

	return GTK_TREE_MODEL(treestore);
}

GtkWidget *create_view_and_model (void) {
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;
	GtkTreeModel *model;

	view = gtk_tree_view_new();
	renderer = gtk_cell_renderer_text_new();

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer,  "text", 0);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Value");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer,  "text", 1);

	model = create_and_fill_model();

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model); 

	GtkWidget *scwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scwin), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	return scwin;
}








GdkPixbuf *create_pixbuf(const gchar * filename) {
	GdkPixbuf *pixbuf;
	GError *error = NULL;
	pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	if(!pixbuf) {
		fprintf(stderr, "%s\n", error->message);
		g_error_free(error);
	}
	return pixbuf;
}



int main (int argc, char *argv[]) {
	SetupLoad();
	ArgsRead(argc, argv);
	SetupShow();

	/* import DXF */
	if (PARAMETER[P_V_DXF].vstr[0] != 0) {
		dxf_read(PARAMETER[P_V_DXF].vstr);
	}

	init_objects();
	DrawCheckSize();
	DrawSetZero();
//	LayerLoadList();


	GtkWidget *hbox;
	GtkWidget *vbox;
	gtk_init(&argc, &argv);

	gtk_gl_init(&argc, &argv);


	glCanvas = create_gl();
	gtk_widget_set_usize(GTK_WIDGET(glCanvas), 800, 600);
	gtk_signal_connect(GTK_OBJECT(glCanvas), "expose_event", GTK_SIGNAL_FUNC(handler_draw), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "button_press_event", GTK_SIGNAL_FUNC(handler_button_press), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "button_release_event", GTK_SIGNAL_FUNC(handler_button_release), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "configure_event", GTK_SIGNAL_FUNC(handler_configure), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "motion_notify_event", GTK_SIGNAL_FUNC(handler_motion), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "key_press_event", GTK_SIGNAL_FUNC(handler_keypress), NULL);  
	gtk_signal_connect(GTK_OBJECT(glCanvas), "scroll-event", GTK_SIGNAL_FUNC(handler_scrollwheel), NULL);  

	// top-menu
	GtkWidget *MenuBar = gtk_menu_bar_new();
	GtkWidget *MenuItem;
	GtkWidget *FileMenu = gtk_menu_item_new_with_label("File");
	GtkWidget *FileMenuList = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(FileMenu), FileMenuList);
	gtk_menu_bar_append(GTK_MENU_BAR(MenuBar), FileMenu);

		MenuItem = gtk_menu_item_new_with_label("Load DXF");
		gtk_menu_append(GTK_MENU(FileMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_load_dxf), NULL);

		MenuItem = gtk_menu_item_new_with_label("Save G-Code");
		gtk_menu_append(GTK_MENU(FileMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_save_gcode), NULL);

		MenuItem = gtk_menu_item_new_with_label("Load Tooltable");
		gtk_menu_append(GTK_MENU(FileMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_load_tooltable), NULL);

		MenuItem = gtk_menu_item_new_with_label("Save Setup");
		gtk_menu_append(GTK_MENU(FileMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_save_setup), NULL);

		MenuItem = gtk_menu_item_new_with_label("Quit");
		gtk_menu_append(GTK_MENU(FileMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_destroy), NULL);


	GtkWidget *HelpMenu = gtk_menu_item_new_with_label("Help");
	GtkWidget *HelpMenuList = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(HelpMenu), HelpMenuList);
	gtk_menu_bar_append(GTK_MENU_BAR(MenuBar), HelpMenu);

		MenuItem = gtk_menu_item_new_with_label("About");
		gtk_menu_append(GTK_MENU(HelpMenuList), MenuItem);
		gtk_signal_connect(GTK_OBJECT(MenuItem), "activate", GTK_SIGNAL_FUNC(handler_about), NULL);


	GtkWidget *ToolBar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(ToolBar), GTK_TOOLBAR_ICONS);

	GtkToolItem *ToolItemLoadDXF = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_tool_item_set_tooltip_text(ToolItemLoadDXF, "Load DXF");
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemLoadDXF, -1);
	g_signal_connect(G_OBJECT(ToolItemLoadDXF), "clicked", GTK_SIGNAL_FUNC(handler_load_dxf), NULL);

	GtkToolItem *ToolItemSaveGcode = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_tool_item_set_tooltip_text(ToolItemSaveGcode, "Save G-Code");
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemSaveGcode, -1);
	g_signal_connect(G_OBJECT(ToolItemSaveGcode), "clicked", GTK_SIGNAL_FUNC(handler_save_gcode), NULL);

	GtkToolItem *ToolItemSaveSetup = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_tool_item_set_tooltip_text(ToolItemSaveSetup, "Save Setup");
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemSaveSetup, -1);
	g_signal_connect(G_OBJECT(ToolItemSaveSetup), "clicked", GTK_SIGNAL_FUNC(handler_save_setup), NULL);

	GtkToolItem *ToolItemSep = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemSep, -1); 

	GtkToolItem *ToolItemSep2 = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemSep2, -1); 

	GtkToolItem *ToolItemExit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_tool_item_set_tooltip_text(ToolItemExit, "Quit");
	gtk_toolbar_insert(GTK_TOOLBAR(ToolBar), ToolItemExit, -1);
	g_signal_connect(G_OBJECT(ToolItemExit), "clicked", GTK_SIGNAL_FUNC(handler_destroy), NULL);


	GtkWidget *NbBox = gtk_table_new(2, 2, FALSE);
	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_table_attach_defaults(GTK_TABLE(NbBox), notebook, 0, 1, 0, 1);

	int ViewNum = 0;
	GtkWidget *ViewLabel = gtk_label_new("View");
	GtkWidget *ViewBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ViewBox, ViewLabel);

	int ToolNum = 0;
	GtkWidget *ToolLabel = gtk_label_new("Tool");
	GtkWidget *ToolBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ToolBox, ToolLabel);

	int MillingNum = 0;
	GtkWidget *MillingLabel = gtk_label_new("Milling");
	GtkWidget *MillingBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), MillingBox, MillingLabel);

	int MachineNum = 0;
	GtkWidget *MachineLabel = gtk_label_new("Machine");
	GtkWidget *MachineBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), MachineBox, MachineLabel);

	int MaterialNum = 0;
	GtkWidget *MaterialLabel = gtk_label_new("Material");
	GtkWidget *MaterialBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), MaterialBox, MaterialLabel);


	int n = 0;
	for (n = 0; n < P_LAST; n++) {
		GtkWidget *Box = gtk_hbox_new(0, 0);
		if (PARAMETER[n].type == T_FLOAT) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(PARAMETER[n].vdouble, PARAMETER[n].min, PARAMETER[n].max, PARAMETER[n].step, PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(PARAMETER[n].vdouble, PARAMETER[n].min, PARAMETER[n].max, PARAMETER[n].step, PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_INT) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(PARAMETER[n].vdouble, PARAMETER[n].min, PARAMETER[n].max, PARAMETER[n].step, PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_SELECT) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ListStore[n] = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			ParamValue[n] = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ListStore[n]));
			g_object_unref(ListStore[n]);
			GtkCellRenderer *column;
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ParamValue[n]), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ParamValue[n]), column, "cell-background", 0, "text", 1, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(ParamValue[n]), 1);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_BOOL) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_STRING) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), PARAMETER[n].vstr);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (PARAMETER[n].type == T_FILE) {
			GtkWidget *Label = gtk_label_new(PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), PARAMETER[n].vstr);
			ParamButton[n] = gtk_button_new();
			GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image(GTK_BUTTON(ParamButton[n]), image);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamButton[n], 0, 0, 0);
		} else {
			continue;
		}
		if (strcmp(PARAMETER[n].group, "View") == 0) {
			gtk_box_pack_start(GTK_BOX(ViewBox), Box, 0, 0, 0);
			PARAMETER[n].l1 = 0;
			PARAMETER[n].l2 = ViewNum;
			ViewNum++;
		} else if (strcmp(PARAMETER[n].group, "Tool") == 0) {
			gtk_box_pack_start(GTK_BOX(ToolBox), Box, 0, 0, 0);
			PARAMETER[n].l1 = 1;
			PARAMETER[n].l2 = ToolNum;
			ToolNum++;
		} else if (strcmp(PARAMETER[n].group, "Milling") == 0) {
			gtk_box_pack_start(GTK_BOX(MillingBox), Box, 0, 0, 0);
			PARAMETER[n].l1 = 2;
			PARAMETER[n].l2 = MillingNum;
			MillingNum++;
		} else if (strcmp(PARAMETER[n].group, "Machine") == 0) {
			gtk_box_pack_start(GTK_BOX(MachineBox), Box, 0, 0, 0);
			PARAMETER[n].l1 = 3;
			PARAMETER[n].l2 = MachineNum;
			MachineNum++;
		} else if (strcmp(PARAMETER[n].group, "Material") == 0) {
			gtk_box_pack_start(GTK_BOX(MaterialBox), Box, 0, 0, 0);
			PARAMETER[n].l1 = 4;
			PARAMETER[n].l2 = MaterialNum;
			MaterialNum++;
		}
	}


	GtkWidget *TreeLabel = gtk_label_new("Objects");
	GtkWidget *TreeBox = gtk_vbox_new(0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), TreeBox, TreeLabel);
	GtkWidget *TreeView = create_view_and_model();
	gtk_box_pack_start(GTK_BOX(TreeBox), TreeView, 1, 1, 0);

/*
	for (n = 0; n < O_P_LAST; n++) {
		GtkWidget *Box = gtk_hbox_new(0, 0);
		if (OBJECT_PARAMETER[n].type == T_FLOAT) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(OBJECT_PARAMETER[n].vdouble, OBJECT_PARAMETER[n].min, OBJECT_PARAMETER[n].max, OBJECT_PARAMETER[n].step, OBJECT_PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, OBJECT_PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_DOUBLE) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(OBJECT_PARAMETER[n].vdouble, OBJECT_PARAMETER[n].min, OBJECT_PARAMETER[n].max, OBJECT_PARAMETER[n].step, OBJECT_PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, OBJECT_PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_INT) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			GtkAdjustment *Adj = (GtkAdjustment *)gtk_adjustment_new(OBJECT_PARAMETER[n].vdouble, OBJECT_PARAMETER[n].min, OBJECT_PARAMETER[n].max, OBJECT_PARAMETER[n].step, OBJECT_PARAMETER[n].step * 10.0, 0.0);
			ParamValue[n] = gtk_spin_button_new(Adj, OBJECT_PARAMETER[n].step, 3);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_SELECT) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ListStore[n] = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			ParamValue[n] = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ListStore[n]));
			g_object_unref(ListStore[n]);
			GtkCellRenderer *column;
			column = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ParamValue[n]), column, TRUE);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ParamValue[n]), column, "cell-background", 0, "text", 1, NULL);
			gtk_combo_box_set_active(GTK_COMBO_BOX(ParamValue[n]), 1);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_BOOL) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_check_button_new_with_label("On/Off");
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_STRING) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), OBJECT_PARAMETER[n].vstr);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else if (OBJECT_PARAMETER[n].type == T_FILE) {
			GtkWidget *Label = gtk_label_new(OBJECT_PARAMETER[n].name);
			GtkWidget *Align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
			gtk_container_add(GTK_CONTAINER(Align), Label);
			ParamValue[n] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(ParamValue[n]), OBJECT_PARAMETER[n].vstr);
			gtk_box_pack_start(GTK_BOX(Box), Align, 1, 1, 0);
			gtk_box_pack_start(GTK_BOX(Box), ParamValue[n], 0, 0, 0);
		} else {
			continue;
		}
		gtk_box_pack_start(GTK_BOX(TreeBox), Box, 0, 0, 0);
	}
*/
	gtk_list_store_insert_with_values(ListStore[P_H_ROTARYAXIS], NULL, -1, 0, NULL, 1, "A", -1);
	gtk_list_store_insert_with_values(ListStore[P_H_ROTARYAXIS], NULL, -1, 0, NULL, 1, "B", -1);
	gtk_list_store_insert_with_values(ListStore[P_H_ROTARYAXIS], NULL, -1, 0, NULL, 1, "C", -1);

	gtk_list_store_insert_with_values(ListStore[P_H_KNIFEAXIS], NULL, -1, 0, NULL, 1, "A", -1);
	gtk_list_store_insert_with_values(ListStore[P_H_KNIFEAXIS], NULL, -1, 0, NULL, 1, "B", -1);
	gtk_list_store_insert_with_values(ListStore[P_H_KNIFEAXIS], NULL, -1, 0, NULL, 1, "C", -1);

	g_signal_connect(G_OBJECT(ParamButton[P_MFILE]), "clicked", GTK_SIGNAL_FUNC(handler_save_gcode), NULL);
	g_signal_connect(G_OBJECT(ParamButton[P_TOOL_TABLE]), "clicked", GTK_SIGNAL_FUNC(handler_load_tooltable), NULL);
	g_signal_connect(G_OBJECT(ParamButton[P_V_DXF]), "clicked", GTK_SIGNAL_FUNC(handler_load_dxf), NULL);



	MaterialLoadList();
	ToolLoadTable();


	ParameterUpdate();
	for (n = 0; n < P_LAST; n++) {
		if (PARAMETER[n].type == T_FLOAT) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_DOUBLE) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_INT) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_SELECT) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_BOOL) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "toggled", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_STRING) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		} else if (PARAMETER[n].type == T_FILE) {
			gtk_signal_connect(GTK_OBJECT(ParamValue[n]), "changed", GTK_SIGNAL_FUNC(ParameterChanged), (gpointer)n);
		}
	}


	StatusBar = gtk_statusbar_new();

	hbox = gtk_hbox_new(0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), NbBox, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), glCanvas, 1, 1, 0);

	SizeInfoLabel = gtk_label_new("Width=0mm / Height=0mm");
	GtkWidget *SizeInfo = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(SizeInfo), SizeInfoLabel);

	GtkWidget *LogoIMG = gtk_image_new_from_file("logo-top.png");
	GtkWidget *Logo = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(Logo), LogoIMG);
	gtk_signal_connect(GTK_OBJECT(Logo), "button_press_event", GTK_SIGNAL_FUNC(handler_about), NULL);


	GtkWidget *topBox = gtk_hbox_new(0, 0);
	gtk_box_pack_start(GTK_BOX(topBox), ToolBar, 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(topBox), SizeInfo, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(topBox), Logo, 0, 0, 0);


	vbox = gtk_vbox_new(0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), MenuBar, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), topBox, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(vbox), StatusBar, 0, 0, 0);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "OpenCAM 2D");


	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("logo-top.png"));

	gtk_signal_connect(GTK_OBJECT(window), "destroy_event", GTK_SIGNAL_FUNC (handler_destroy), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC (handler_destroy), NULL);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	gtk_widget_show_all(window);

	gtk_timeout_add(1000/25, handler_periodic_action, NULL);
	gtk_main ();



	return 0;
}


