
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
	P_TOOL_TABLE,
	P_M_FEEDRATE_MAX,
	P_M_FEEDRATE,
	P_M_PLUNGE_SPEED,
	P_M_DEPTH,
	P_M_Z_STEP,
	P_CUT_SAVE,
	P_M_OVERCUT,
	P_M_LASERMODE,
	P_M_ROTARYMODE,
	P_M_KNIFEMODE,
	P_H_LASERDIA,
	P_H_ROTARYAXIS,
	P_H_KNIFEAXIS,
	P_H_KNIFEMAXANGLE,
	P_MFILE,
	P_MAT_SELECT,
	P_MAT_DIAMETER,
	P_POST_CMD,
	P_LAST
};

extern PARA PARAMETER[];

void SetupShow (void);
void SetupShowGcode (FILE *out);
void SetupShowHelp (void);
void SetupSave (void);
void SetupLoad (void);
int SetupArgCheck (char *arg, char *arg2);


