/*

 Copyright by Oliver Dippel <oliver@multixmedia.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at
 your option) any later version.

 On Debian GNU/Linux systems, the complete text of the GNU General
 Public License can be found in `/usr/share/common-licenses/GPL'.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dxf.h>
#include <font.h>


int block = 0;
double block_x = 0.0;
double block_y = 0.0;
char block_name[1024];


double get_len (double x1, double y1, double x2, double y2);

int mtext_n = 0;
char dxf_options[256][256];
_OBJECT *myOBJECTS = NULL;
_LINE *myLINES = NULL;
_MTEXT myMTEXT[100];

char dxf_typename[TYPE_LAST][16];

int line_n = 1;
int line_last = 0;

void add_line (int type, char *layer, double x1, double y1, double x2, double y2, double opt) {
//	printf("## ADD_LINE: %f,%f -> %f,%f (%s / %f)\n", x1, y1, x2, y2, layer, opt);
	if (x1 > 10000.0 || y1 > 10000.0 || x2 > 10000.0 || y2 > 10000.0) {
		printf("###### LINE TO BIG; %f %f -> %f %f ######\n", x1, y1, x2, y2);
		return;
	}
	if (line_n < MAX_LINES) {
		if (myLINES == NULL) {
			myLINES = malloc(sizeof(_LINE) * 2);
		} else {
			myLINES = realloc(myLINES, sizeof(_LINE) * (line_last + 2));
		}
		myLINES[line_n].used = 1;
		myLINES[line_n].type = type;
		strcpy(myLINES[line_n].layer, layer);
		myLINES[line_n].x1 = x1;
		myLINES[line_n].y1 = y1;
		myLINES[line_n].x2 = x2;
		myLINES[line_n].y2 = y2;
		myLINES[line_n].opt = opt;
		myLINES[line_n].in_object = -1;
		strcpy(myLINES[line_n].block, block_name);
		line_n++;
		line_last = line_n;
	} else {
		printf("### TO MANY LINES ##\n");
		exit(1);
	}
}

size_t trimline (char *out, size_t len, const char *str) {
	if(len == 0) {
		return 0;
	}
	const char *end;
	size_t out_size;
	while (*str == ' ' || *str == '\r' || *str == '\n') {
		str++;
	}
	if (*str == 0) {
		*out = 0;
		return 1;
	}
	end = str + strlen(str) - 1;
	while(end > str && (*end == ' ' || *end == '\r' || *end == '\n')) {
		end--;
	}
	end++;
	out_size = (end - str) < len-1 ? (end - str) : len-1;
	memcpy(out, str, out_size);
	out[out_size] = 0;
	return out_size;
}

void clear_dxfoptions (void) {
	int num = 0;
	for (num = 0; num < 256; num++) {
		dxf_options[num][0] = 0;
	}
}

void dxf_read (char *file) {
	FILE *fp;
	char *line = NULL;
	char line2[1024];
	size_t len = 0;
	ssize_t read;

	line_last = 0;
	if (myLINES != NULL) {
		free(myLINES);
		myLINES = NULL;
	}

	strcpy(dxf_typename[TYPE_NONE], "None");
	strcpy(dxf_typename[TYPE_LINE], "Line");
	strcpy(dxf_typename[TYPE_ARC], "Arc");
	strcpy(dxf_typename[TYPE_CIRCLE], "Circle");
	strcpy(dxf_typename[TYPE_MTEXT], "Text");
	strcpy(dxf_typename[TYPE_POINT], "Point");
	strcpy(dxf_typename[TYPE_POLYLINE], "Polyline");
	strcpy(dxf_typename[TYPE_VERTEX], "Vertex");

	fp = fopen(file, "r");
	if (fp == NULL) {
		return;
	}

	clear_dxfoptions();

	char last_0[256];
	strcpy(last_0, "");

	int lwpl_flag = 0;
	int pl_flag = 0;
	int pl_closed = 0;
	double pl_first_x = 0.0;
	double pl_first_y = 0.0;
	double pl_last_x = 0.0;
	double pl_last_y = 0.0;

	while ((read = getline(&line, &len, fp)) != -1) {
		trimline(line2, 1024, line);
		int dxfoption = atoi(line2);
		if ((read = getline(&line, &len, fp)) != -1) {
			trimline(line2, 1024, line);
			if (dxfoption == 0) {
				if (last_0[0] != 0) {
					if (strcmp(last_0, "BLOCK") == 0) {
						block = 1;
						block_x = atof(dxf_options[OPTION_POINT_X]);
						block_y = atof(dxf_options[OPTION_POINT_Y]);
						strcpy(block_name, dxf_options[OPTION_BLOCKNAME]);
					} else if (strcmp(last_0, "ENDBLK") == 0) {
						block = 0;
						block_name[0] = 0;
					} else if (strcmp(last_0, "INSERT") == 0) {
						block_x = atof(dxf_options[OPTION_POINT_X]);
						block_y = atof(dxf_options[OPTION_POINT_Y]);
						strcpy(block_name, dxf_options[2]);
						int num = 0;
						for (num = 0; num < line_last; num++) {
							if (strcmp(myLINES[num].block, block_name) == 0) {
								myLINES[num].x1 += block_x;
								myLINES[num].y1 += block_y;
								myLINES[num].x2 += block_x;
								myLINES[num].y2 += block_y;
							}
						}
					} else if (strcmp(last_0, "LINE") == 0) {
						double p_x1 = atof(dxf_options[OPTION_LINE_X1]);
						double p_y1 = atof(dxf_options[OPTION_LINE_Y1]);
						double p_x2 = atof(dxf_options[OPTION_LINE_X2]);
						double p_y2 = atof(dxf_options[OPTION_LINE_Y2]);
						add_line(TYPE_LINE, dxf_options[8], p_x1, p_y1, p_x2, p_y2, 0.0);
					} else if (strcmp(last_0, "VERTEX") == 0) {
						double p_x1 = atof(dxf_options[OPTION_LINE_X1]);
						double p_y1 = atof(dxf_options[OPTION_LINE_Y1]);
						double p_r1 = atof(dxf_options[42]);
						if (pl_flag == 0) {
							pl_first_x = p_x1;
							pl_first_y = p_y1;
						} else {
							if (p_r1 != 0.0) {
								double chord = sqrt(pow(abs(pl_last_x - p_x1), 2.0) + pow(abs(pl_last_y - p_y1), 2.0));
								double s = chord / 2.0 * p_r1;
								double radius = (pow(chord / 2.0, 2.0) + pow(s, 2.0)) / (2.0 * s);
								double len = get_len(pl_last_x, pl_last_y, p_x1, p_y1);
								if (radius * 2 < len) {
									radius *= 2.0;
								}
								add_line(TYPE_ARC, dxf_options[8], pl_last_x, pl_last_y, p_x1, p_y1, radius);
							} else {
								add_line(TYPE_LINE, dxf_options[8], pl_last_x, pl_last_y, p_x1, p_y1, 0.0);
							}
						}
						pl_last_x = p_x1;
						pl_last_y = p_y1;
						pl_flag = 1;
					} else if (strcmp(last_0, "SEQEND") == 0) {
						if (pl_closed == 1) {
							add_line(TYPE_LINE, dxf_options[8], pl_last_x, pl_last_y, pl_first_x, pl_first_y, 0.0);
						}
						pl_flag = 0;
						pl_closed = 0;
					} else if (strcmp(last_0, "POLYLINE") == 0) {
						pl_closed = atoi(dxf_options[70]);
						pl_flag = 0;
					} else if (strcmp(last_0, "LWPOLYLINE") == 0) {
						pl_closed = atoi(dxf_options[70]);
						double p_x1 = atof(dxf_options[OPTION_LINE_X1]);
						double p_y1 = atof(dxf_options[OPTION_LINE_Y1]);
						double p_r1 = atof(dxf_options[42]);
						pl_last_x = p_x1;
						pl_last_y = p_y1;
						dxf_options[42][0] = 0;
						if (pl_closed == 1) {
							add_line(TYPE_LINE, dxf_options[8], pl_last_x, pl_last_y, pl_first_x, pl_first_y, p_r1);
						}
						lwpl_flag = 0;
						pl_closed = 0;

					} else if (strcmp(last_0, "POINT") == 0) {
						double p_x1 = atof(dxf_options[OPTION_POINT_X]);
						double p_y1 = atof(dxf_options[OPTION_POINT_Y]);
						double p_x2 = atof(dxf_options[OPTION_POINT_X]);
						double p_y2 = atof(dxf_options[OPTION_POINT_Y]);
						add_line(TYPE_POINT, dxf_options[8], p_x1, p_y1, p_x2, p_y2, 0.0);
					} else if (strcmp(last_0, "SPLINE") == 0) {
					} else if (strcmp(last_0, "ARC") == 0 || strcmp(last_0, "CIRCLE") == 0) {
						double p_x1 = atof(dxf_options[OPTION_ARC_X]);
						double p_y1 = atof(dxf_options[OPTION_ARC_Y]);
						double p_y2 = atof(dxf_options[OPTION_ARC_RADIUS]);
						double p_a1 = atof(dxf_options[OPTION_ARC_BEGIN]);
						double p_a2 = atof(dxf_options[OPTION_ARC_END]);
						if (strcmp(last_0, "CIRCLE") == 0) {
							p_a1 = 0.0;
							p_a2 = 360.0;
						}
						if (p_a1 > p_a2) {
							p_a2 += 360.0;
						}
						double r = p_y2;
						double angle2 = toRad(p_a1);
						double x2 = r * cos(angle2);
						double y2 = r * sin(angle2);
						double last_x = (p_x1 + x2);
						double last_y = (p_y1 + y2);
						double an = 0;

						double p_rast = (p_a2 - p_a1) / 10.0;

						for (an = p_a1 + p_rast; an <= p_a2 - (p_rast / 2.0); an += p_rast) {
							double angle1 = toRad(an);
							double x1 = r * cos(angle1);
							double y1 = r * sin(angle1);
							if (strcmp(last_0, "CIRCLE") == 0) {
								add_line(TYPE_CIRCLE, dxf_options[8], last_x, last_y, p_x1 + x1, p_y1 + y1, r);
							} else {
								add_line(TYPE_ARC, dxf_options[8], last_x, last_y, p_x1 + x1, p_y1 + y1, r);
							}
							last_x = p_x1 + x1;
							last_y = p_y1 + y1;
						}
						double angle3 = toRad(p_a2);
						double x3 = r * cos(angle3);
						double y3 = r * sin(angle3);
						if (strcmp(last_0, "CIRCLE") == 0) {
							add_line(TYPE_CIRCLE, dxf_options[8], last_x, last_y, p_x1 + x3, p_y1 + y3, r);
						} else {
							add_line(TYPE_ARC, dxf_options[8], last_x, last_y, p_x1 + x3, p_y1 + y3, r);
						}
					} else if (strcmp(last_0, "MTEXT") == 0) {
						double p_x1 = atof(dxf_options[OPTION_MTEXT_X]);
						double p_y1 = atof(dxf_options[OPTION_MTEXT_Y]);
						double p_y2 = atof(dxf_options[OPTION_MTEXT_SIZE]);
						myMTEXT[mtext_n].used = 1;
						myMTEXT[mtext_n].x = p_x1;
						myMTEXT[mtext_n].y = p_y1;
						myMTEXT[mtext_n].s = p_y2;
						strcpy(myMTEXT[mtext_n].text, dxf_options[OPTION_MTEXT_TEXT]);
						strcpy(myMTEXT[mtext_n].layer, dxf_options[OPTION_LAYERNAME]);
						output_text_dxf(myMTEXT[mtext_n].text, myMTEXT[mtext_n].x, myMTEXT[mtext_n].y, 0.0, myMTEXT[mtext_n].s);
						mtext_n++;
					} else {
						pl_flag = 0;
						lwpl_flag = 0;
					}
					clear_dxfoptions();
				}
				strcpy(last_0, line2);
			}
//			printf("## %i: %s\n", dxfoption, line2);
			if (dxfoption < 256) {
				strcpy(dxf_options[dxfoption], line2);
				if (strcmp(last_0, "LWPOLYLINE") == 0) {
					if (dxfoption == 10) {
					} else if (dxfoption == 20) {
						double p_x1 = atof(dxf_options[OPTION_LINE_X1]);
						double p_y1 = atof(dxf_options[OPTION_LINE_Y1]);
						double p_r1 = atof(dxf_options[42]);
						if (lwpl_flag > 0) {
							if (dxf_options[42][0] == 0) {
								add_line(TYPE_LINE, dxf_options[8], pl_last_x, pl_last_y, p_x1, p_y1, 0.0);
							} else {
								double chord = sqrt(pow(abs(pl_last_x - p_x1), 2.0) + pow(abs(pl_last_y - p_y1), 2.0));
								double s = chord / 2.0 * p_r1;
								double radius = (pow(chord / 2.0, 2.0) + pow(s, 2.0)) / (2.0 * s);
								double len = get_len(pl_last_x, pl_last_y, p_x1, p_y1);
								if (radius * 2 < len) {
									radius *= 2.0;
								}
								add_line(TYPE_ARC, dxf_options[8], pl_last_x, pl_last_y, p_x1, p_y1, radius);
							}
						} else {
							pl_first_x = p_x1;
							pl_first_y = p_y1;
						}
						dxf_options[42][0] = 0;
						pl_last_x = p_x1;
						pl_last_y = p_y1;
						lwpl_flag++;
					}
				}
			}
		}
	}

//exit(0);

}

