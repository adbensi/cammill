
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
	int inpreset;
	char help[512];
	int l1;
	int l2;
} PARA;

enum {
	T_INT,
	T_BOOL,
	T_FLOAT,
	T_DOUBLE,
	T_STRING,
	T_SELECT,
	T_FILE
};

enum {
	P_V_ZOOM,
	P_V_HELPLINES,
	P_V_OFFSETS,
	P_V_HELPDIA,
	P_V_TEXTURES,
	P_V_ROTX,
	P_V_ROTY,
	P_V_ROTZ,
	P_V_TRANSX,
	P_V_TRANSY,
	P_V_HELP_GRID,
	P_V_HELP_ARROW,
	P_V_DXF,
	P_TOOL_SELECT,
	P_TOOL_NUM,
	P_TOOL_DIAMETER,
	P_TOOL_SPEED_MAX,
	P_TOOL_SPEED,
	P_TOOL_W,
	P_TOOL_TABLE,
	P_M_FEEDRATE_MAX,
	P_M_FEEDRATE,
	P_M_PLUNGE_SPEED,
	P_M_DEPTH,
	P_M_Z_STEP,
	P_CUT_SAVE,
	P_M_OVERCUT,
	P_M_LASERMODE,
	P_M_CLIMB,
	P_M_TEXT,
	P_M_NCDEBUG,
	P_T_USE,
	P_T_DEPTH,
	P_T_LEN,
	P_T_TYPE,
	P_T_XGRID,
	P_H_FEEDRATE_FAST,
	P_H_POST,
	P_MFILE,
	P_MAT_SELECT,
	P_POST_CMD,
	P_O_SELECT,
	P_O_USE,
	P_O_FORCE,
	P_O_CLIMB,
	P_O_OFFSET,
	P_O_OVERCUT,
	P_O_POCKET,
	P_O_LASER,
	P_O_DEPTH,
	P_M_ROTARYMODE,
	P_H_ROTARYAXIS,
	P_MAT_DIAMETER,
	P_M_KNIFEMODE,
	P_H_KNIFEAXIS,
	P_H_KNIFEMAXANGLE,
	P_LAST,
};

extern PARA PARAMETER[];

void SetupShow (void);
void SetupShowGcode (FILE *out);
void SetupShowHelp (void);
void SetupSave (void);
void SetupLoad (void);
int SetupArgCheck (char *arg, char *arg2);
void SetupSavePreset (char *cfgfile);
void SetupLoadPreset (char *cfgfile);


