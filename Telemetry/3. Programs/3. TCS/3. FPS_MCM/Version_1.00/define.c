
#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

#define MY_IP_ADDR     "192.168.8.163"
#define MY_GATEWAY     "192.168.8.1"
#define MY_NET_MASK    "255.255.255.0"
#define SERVER_ADDR    "192.168.8.143"  // Server for lab testing
#define SERVER_PORT    "3004"           // For 192.168.8.143
// 3004 - fps
//#define SERVER_ADDR    "192.168.4.65"   // Server for GAB System
//#define SERVER_PORT    "3000"           // For 192.168.4.65

#define TCP_BUF_SIZE    16392
#define MAX_SOCKETS     2
#define MAX_TCP_SOCKET_BUFFERS 2
#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds

void Ms_delay(unsigned int delay);
void Set_DMask();

char msw[5], lsw[5];
#web msw
#web lsw

protected int fps_def_cnt[4] = { 1300, 6500, 11000, 16500 } ;

float temp;

//**************************** Defination part for FPS System *************************//

#define SYSTEM_4			"fps"
#define Cmd_401			"domon"
#define Cmd_402			"set_tng_pnt"
#define Cmd_403			"set_rmp_dncnt"
#define Cmd_404			"set_lrpm_lmt"
#define Cmd_405			"set_brkcnt_diff"
#define Cmd_406			"set_rmp_upcnt"
#define Cmd_407			"set_stop_cnt"
#define Cmd_408			"set_max_ang"
#define Cmd_409			"set_min_ang"
#define Cmd_410			"set_max_pwm"
#define Cmd_411			"rd_tng_pnt"
#define Cmd_412			"rd_rmp_dncnt"
#define Cmd_413			"rd_lrpm_lmt"
#define Cmd_414			"rd_brkcnt_diff"
#define Cmd_415			"rd_rpm_upcnt"
#define Cmd_416			"rd_stop_cnt"
#define Cmd_417			"rd_max_ang"
#define Cmd_418			"rd_min_ang"
#define Cmd_419			"rd_max_pwm"
#define Cmd_420			"rd_version"
#define Cmd_421			"rd_ua0_ang"
#define Cmd_422			"calibrate"
#define Cmd_423			"runfree"
#define Cmd_424			"ua0calib"
#define Cmd_425			"runcpreset"
#define Cmd_426			"rundpreset"
#define Cmd_427			"finectune"
#define Cmd_428			"finedtune"
#define Cmd_429			"fpsboot"
#define Cmd_430			"fpsstop"
#define Cmd_431			"ldfpspos"
#define Cmd_432			"mvfps150"
#define Cmd_433			"mvfps375"
#define Cmd_434			"mvfps700"
#define Cmd_435			"mvfps1420"
#define Cmd_436			"initfps"      // Batch command can be implemented in later stage.
#define Cmd_437			"reset"
#define Cmd_438			"settime"

#define CINBUFSIZE 31
#define COUTBUFSIZE 31
#define RS232_NOCHARASSYINBRK
#define MSSG_TMOUT 5UL
#define IDLE_TMOUT 50UL

#ximport  "data.zhtml"    data_zhtml

typedef enum { CURR_ENC, TARGET_ENC, CURR_ANG, TARGET_ANG, CURR_RPM, MODE, FPS_VER, RAMP_DNTIME,
RAMP_UPTIME, LOW_RPM_LMT, LOW_RPM_TIME, BRK_CNT_DIFF, STEP_SLW_TIMECNT,
FINE_TUNE_PWM, MAX_PWM, MAX_ANG, MIN_ANG, UA0_ANG, OVER_CURR, COMMERR,
CMDFAIL, TIMEOUT_CNT, CHKSUM_CNT, RST_CNT, FPS_CTRL_INFO, FPS_MOTOR_TEMP,
MOTOR_CURR, DOMON_STAT, TNG_PT, STOP_CNT, TVAR } ENUM_FPSPARA;

char par_value[TVAR][64];
char in_d[32];
#web in_d
int resp_msg_cntr ;
int resp_code ;
char out_data[32], mcm_addr;

void serial_init();
char chksum(char *ary);
void enable_485Tx( void );
void disable_485Tx( void );
void transmit();
void ser2eth_comm(void);
int command_process_fps();

//**************************** Defination part for Rabbit Card *************************//
#define ADC_SCLKBRATE 	115200ul
#define GAINSET 		   GAIN_1
#define CHANNEL_1 		0
#define CHANNEL_2 		1
#define CHANNEL_3 		2
#define CHANNEL_4 		3

#define MAXDATA       	128
#define DATALEN       	64
#define MSGLEN				128

#define USE_RABBITWEB 1
#define USE_HTTP_DIGEST_AUTHENTICATION 1

typedef enum { WRAPPER=10, INTERMEDIATE=11, FINAL=12, ASYNCH=13, ALARM=100 } EventEnum ;
typedef enum { SUCCESS=10, FAILED=11, SYNTX=12, INCOMPLETE=13, INVALID=14, UNKNOWNERR=100 } CodeEnum ;

void Set_SS();
void Set_FDB();
void Set_FDV();
void Rd_MCM_Clk();
void Set_NW();

int cmd_validation();
int form_inter_resp();
void form_final_resp(int, int);
void bg_mon();
void Set_RTC(char far *);
void MCM_Reset();

int reset1;
#web reset1

#ifndef MY_IP_ADDR
char MY_IP_ADDR[20], MY_GATEWAY[20], MY_NET_MASK[20];
#endif

char ip[20], mask[20], gateway[20], psw[10];
#web ip
#web mask
#web gateway
#web psw

int SS, FDB, FDV, Temp_SS, Temp_FDB, Temp_FDV;
#web SS	select("Choose SS" = 0, "Off", "Normal", "Strong-1", "Strong-2")
#web FDB	select("Choose FDB" = 0, "Off", "On")
#web FDV	select("Choose FDV" = 0, "1", "2", "4", "8")

char SS1_Mon[10], SS2_Mon[10], FDB_Mon[5], FDV_Mon[5], MCM_Clk[10];
#web SS1_Mon
#web SS2_Mon
#web FDB_Mon
#web FDV_Mon
#web MCM_Clk

struct tm  rtc;
unsigned long t0, sec_tm;

char time1[10], date[12];
#web time1
#web date

int rw[70], l=0;
#web rw

char vl[70][10], mswT[5], lswT[5];
#web vl
#web mswT
#web lswT

#web_update SS			 				Set_SS
#web_update FDB		 				Set_FDB
#web_update FDB		 				Rd_MCM_Clk
#web_update FDV		 				Set_FDV
#web_update FDV       				Rd_MCM_Clk
#web_update ip, mask, gateway		Set_NW
#web_update reset1               MCM_Reset

char temp_ar[10], time_ar[10] ;

char RTC_time[20];

typedef struct
 {
    int  id;
    int  level;
    char description[MSGLEN];
 } RespAlarm;

typedef struct
 {
    int numpkt;
    char prmname[MAXDATA][DATALEN];
    char prmvalue[MAXDATA][DATALEN];
 } DataPkt;

// Basic Fields Information resued by XMLIB to form XML ACK/NACK/RESPONSE //
typedef struct
 {
    long  seq;                  // Sequence number of Command
    float version;
    short priority ;
    short timeout ;
    char  subsysid[64];
    char  cmd_name[64];        // Command name at user level
    char  timestamp[64];
    char  cmd_id[8];           // Command ID Device Specific
 } BasicFlds;

typedef struct
 {
    int code;
    int event ;
    char message[MSGLEN];

    RespAlarm MCM_alarm;
    DataPkt resp_data;
    BasicFlds resp_elem;
 } devResponse;

typedef struct
 {
    BasicFlds cmd_elem;
    DataPkt cmd_data;
 } parseCMSCmd;

#define Wrpr_Cmd_Size  sizeof(parseCMSCmd)
#define MCM_Resp_Size  sizeof(devResponse)

far parseCMSCmd Wrpr_Cmd;
far devResponse MCM_Resp;

char SenMon_Para[][10]={"time","status","temp","chan","rawcnt"};

#define SMP_rawsize	sizeof(SenMon_Para)/sizeof(SenMon_Para[0])

float if_vccp, if_vccn, if_vdd, lo_vccp, lo_vccn, lo_vdd;
char dmask[10];

//********************************************************************************************//