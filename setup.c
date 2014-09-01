#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <dxf.h>
#include <setup.h>

PARA PARAMETER[] = {
	{"Zoom",	"View",		"-zo",	T_FLOAT,	0,	1.0,	0.0,	"",	0.1,	0.1,	20.0,		"x", 1, 0, 0},
	{"Helplines",	"View", 	"-hl",	T_BOOL	,	1,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"ShowTool",	"View", 	"-st",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"NC-Debug",	"View", 	"-nd",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Rotate-X",	"View", 	"-rx",	T_FLOAT	,	0,	0.0,	0.0,	"",	-360.0,	1.0,	360.0,		"", 1, 0, 0},
	{"Rotate-Y",	"View", 	"-rx",	T_FLOAT	,	0,	0.0,	0.0,	"",	-360.0,	1.0,	360.0,		"", 1, 0, 0},
	{"Rotate-Z",	"View", 	"-rx",	T_FLOAT	,	0,	0.0,	0.0,	"",	-360.0,	1.0,	360.0,		"", 1, 0, 0},
	{"Translate-X",	"View", 	"-rx",	T_INT	,	0,	0.0,	0.0,	"",	-1000.0,1.0,	1000.0,		"", 1, 0, 0},
	{"Translate-Y",	"View", 	"-rx",	T_INT	,	0,	0.0,	0.0,	"",	-1000.0,1.0,	1000.0,		"", 1, 0, 0},
	{"Grid-Size",	"View", 	"-gs",	T_FLOAT	,	0,	1.0,	0.0,	"",	0.001,	0.1,	100.0,		"mm", 1, 0, 0},
	{"Arrow-Scale",	"View", 	"-as",	T_FLOAT	,	0,	1.0,	0.0,	"",	0.001,	0.1,	10.0,		"", 1, 0, 0},
	{"DXF-File",	"View",		"-d",	T_FILE,		0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 0, 0, 0},
	{"Select",	"Tool",		"-tn",	T_SELECT,	1,	0.0,	0.0,	"",	1.0,	1.0,	100.0,		"#", 0, 0, 0},
	{"Number",	"Tool",		"-tn",	T_INT,		1,	0.0,	0.0,	"",	1.0,	1.0,	18.0,		"#", 1, 0, 0},
	{"Diameter",	"Tool",		"-td",	T_DOUBLE,	0,	1.0,	1.0,	"",	0.01,	0.01,	18.0,		"mm", 1, 0, 0},
	{"CalcSpeed",	"Tool",		"-cs",	T_INT,		10000,	0.0,	0.0,	"",	1.0,	10.0,	100000.0,	"rpm", 1, 0, 0},
	{"Speed",	"Tool",		"-ts",	T_INT,		10000,	0.0,	0.0,	"",	1.0,	10.0,	100000.0,	"rpm", 1, 0, 0},
	{"Wings",	"Tool",		"-tw",	T_INT,		2,	0.0,	0.0,	"",	1.0,	1.0,	10.0,		"#", 1, 0, 0},
	{"Table",	"Tool",		"-tt",	T_FILE	,	0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 0, 0, 0},
	{"MaxFeedRate",	"Milling",	"-fm",	T_INT	,	200,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1, 0, 0},
	{"FeedRate",	"Milling",	"-fr",	T_INT	,	200,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1, 0, 0},
	{"PlungeRate",	"Milling",	"-pr",	T_INT	,	100,	0.0,	0.0,	"",	1.0,	1.0,	10000.0,	"mm/min", 1, 0, 0},
	{"Depth",	"Milling",	"-md",	T_DOUBLE,	0,	-4.0,	-4.0,	"",	-40.0,	0.01,	-0.1,		"mm", 1, 0, 0},
	{"Z-Step",	"Milling",	"-msp",	T_DOUBLE,	0,	-4.0,	-4.0,	"",	-40.0,	0.01,	-0.1,		"mm", 1, 0, 0},
	{"Save-Move",	"Milling",	"-msm",	T_DOUBLE,	0,	4.0,	4.0,	"",	1.0,	1.0,	80.0,		"mm", 1, 0, 0},
	{"Overcut",	"Milling",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Lasermode",	"Milling",	"-lm",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Rotary-Mode",	"Milling",	"-rm",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Tangencial-Mode","Milling",	"-tm",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Laser-Diameter","Machine",	"-lw",	T_DOUBLE,	0,	0.0,	0.4,	"",	0.01,	0.01,	10.0,		"mm", 1, 0, 0},
	{"Rotary-Axis",	"Machine",	"-ra",	T_SELECT,	0,	0.0,	0.0,	"",	0.0,	1.0,	2.0,		"A/B/C", 0, 0, 0},
	{"Tangencial-Axis","Machine",	"-ta",	T_SELECT,	1,	0.0,	0.0,	"",	0.0,	1.0,	2.0,		"A/B/C", 0, 0, 0},
	{"Tangencial-MaxAngle","Machine", "-tm",T_DOUBLE,	0,	0.0,	10.0,	"",	0.0,	1.0,	360.0,		"°", 1, 0, 0},
	{"Output-File",	"Milling",	"-o",	T_FILE,		0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 0, 0, 0},
	{"Select",	"Material",	"-ms",	T_SELECT,	1,	0.0,	0.0,	"",	1.0,	1.0,	100.0,		"#", 0, 0, 0},
	{"Rotary/Diameter","Material",	"-md",	T_DOUBLE,	0,	0.0,	10.0,	"",	0.01,	0.01,	300.0,		"mm", 1, 0, 0},
	{"Post-Command","Milling",	"-pc",	T_STRING,	0,	0.0,	0.0,	"",	0.0,	0.0,	0.0,		"", 0, 0, 0},
};

PARA OBJECT_PARAMETER[] = {
	{"Use",		"Object",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Overwite",	"Object",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Overcut",	"Object",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Pocket",	"Object",	"-oc",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Lasermode",	"Object",	"-lm",	T_BOOL	,	0,	0.0,	0.0,	"",	0.0,	1.0,	1.0,		"", 1, 0, 0},
	{"Depth",	"Object",	"-md",	T_DOUBLE,	0,	-4.0,	-4.0,	"",	-40.0,	0.01,	-0.1,		"mm", 1, 0, 0},
};


void SetupShow (void) {
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
		} else if (PARAMETER[n].type == T_FILE) {
			fprintf(stdout, "%22s: %s\n", name_str, PARAMETER[n].vstr);
		}
	}
	fprintf(stdout, "\n");
}

void SetupShowGcode (FILE *out) {
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
		} else if (PARAMETER[n].type == T_FILE) {
			fprintf(out, "(%22s: %s)\n", name_str, PARAMETER[n].vstr);
		}
	}
	fprintf(out, "\n");
}

void SetupShowHelp (void) {
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
		} else if (PARAMETER[n].type == T_FILE) {
			fprintf(stdout, "%5s FILE     %s\n", PARAMETER[n].arg, name_str);
		}
	}
	fprintf(stdout, "\n");
}

void SetupSave (void) {
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
		} else if (PARAMETER[n].type == T_FILE) {
			fprintf(cfg_fp, "%s=%s\n", name_str, PARAMETER[n].vstr);
		}
	}
	fclose(cfg_fp);
}

void SetupLoad (void) {
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
					} else if (PARAMETER[n].type == T_FILE) {
						strcpy(PARAMETER[n].vstr, line2 + strlen(name_str));
					}
				}
			}
		}
		fclose(cfg_fp);
	}
}

int SetupArgCheck (char *arg, char *arg2) {
	int n = 0;
	for (n = 0; n < P_LAST; n++) {
		if (strcmp(arg, PARAMETER[n].arg) == 0) {
			if (PARAMETER[n].type == T_FLOAT) {
				PARAMETER[n].vfloat = atof(arg2);
			} else if (PARAMETER[n].type == T_DOUBLE) {
				PARAMETER[n].vdouble = atof(arg2);
			} else if (PARAMETER[n].type == T_INT) {
				PARAMETER[n].vint = atoi(arg2);
			} else if (PARAMETER[n].type == T_BOOL) {
				PARAMETER[n].vint = atoi(arg2);
			} else if (PARAMETER[n].type == T_STRING) {
				strcpy(PARAMETER[n].vstr, arg2);
			} else if (PARAMETER[n].type == T_FILE) {
				strcpy(PARAMETER[n].vstr, arg2);
			}
			return 1;
		}
	}
	return 0;
}

