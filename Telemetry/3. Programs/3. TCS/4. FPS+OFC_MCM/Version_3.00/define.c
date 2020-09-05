
#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

//#define MY_IP_ADDR     "192.168.8.163"
//#define MY_GATEWAY     "192.168.8.1"
//#define MY_NET_MASK    "255.255.255.0"
#define SERVER_ADDR    "192.168.21.16"  // Server for lab testing
#define SERVER_PORT    "3000"           // For 192.168.8.143
//#define SERVER_ADDR    "192.168.4.65"   // Server for GAB System
//#define SERVER_PORT    "3000"           // For 192.168.4.65

#define TCP_BUF_SIZE    16392
#define MAX_SOCKETS     2
#define MAX_TCP_SOCKET_BUFFERS 2
#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds

#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 200

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

int  cmd_validation();
int  form_inter_resp();
void form_final_resp(int, int);
void Set_SS();
void Set_FDB();
void Set_FDV();
void Rd_MCM_Clk();
void Set_NW();
void bg_mon();
void Set_RTC(char far *);
void MCM_Reset();
void Ms_delay(unsigned int delay);
void Set_DMask();

void sen_mon();
void ofc_mon();
void ofc_atten();
//void ofc_alarm_handler();
void Set_ofc_atten(int ch2, int ch1);
int  command_process_ofcsnt();
void serial_init();
char chksum(char *ary);
void enable_485Tx( void );
void disable_485Tx( void );
void transmit();
void ser2eth_comm(void);
int  command_process_fps();

#define SYSTEM_3			"ofcsnt"
#define SYSTEM_4			"fps"

char Cmd[][16] ={"domon","set attn","init","reset","settime","set_tng_pnt", "set_rmp_dncnt", \
"set_lrpm_lmt","set_brkcnt_diff","set_rmp_upcnt","set_stop_cnt","set_max_ang","set_min_ang", \
"set_max_pwm","rd_tng_pnt","rd_rmp_dncnt","rd_lrpm_lmt","rd_brkcnt_diff","rd_rpm_upcnt", \
"rd_stop_cnt","rd_max_ang","rd_min_ang","rd_max_pwm","rd_version","rd_ua0_ang","calibrate", \
"runfree","ua0calib","runcpreset","rundpreset","finectune","finedtune","fpsboot","fpsstop", \
"ldfpspos","mvfps150","mvfps375","mvfps700","mvfps1420","initfps","reset","settime"};

/*#define Cmd_301			"domon"
#define Cmd_302			"set attn"
#define Cmd_303			"init"
#define Cmd_304			"reset"
#define Cmd_305			"settime"

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
#define Cmd_438			"settime"  */

#ximport  "MCM_Mon.zhtml"   MCM_Mon_zhtml
#ximport  "Mon_Raw.xml"     Mon_Raw_xml
#ximport  "Mon_Sum.xml"     Mon_Sum_xml
#ximport  "MCM_Set.zhtml"   MCM_Set_zhtml
#ximport  "Mon_check.xml"   Mon_check_xml

#define CINBUFSIZE 31
#define COUTBUFSIZE 31
#define RS232_NOCHARASSYINBRK
#define MSSG_TMOUT 5UL
#define IDLE_TMOUT 50UL

#ifndef MY_IP_ADDR
char MY_IP_ADDR[16], MY_GATEWAY[16], MY_NET_MASK[16];
#endif

char ip[16], mask[16], gateway[16], psw[10];
#web ip
#web mask
#web gateway
#web psw

struct tm  rtc;
unsigned long t0, sec_tm;

char vl[64][5], mswT[5], lswT[5], dmask[10], msw[5], lsw[5], time1[10], date[12], temp_ar[10], time_ar[10];
#web vl
#web mswT
#web lswT
#web msw
#web lsw
#web time1
#web date

int SS, FDB, FDV, Temp_SS, Temp_FDB, Temp_FDV, rw[64], reset1, Ch1Att, Ch2Att, FDV_Mon;
#web SS	select("Choose SS" = 0, "Off", "Normal", "Strong-1", "Strong-2")
#web FDB	select("Choose FDB" = 0, "Off", "On")
#web FDV	select("Choose FDV" = 0, "1", "2", "4", "8")
#web rw
#web reset1
#web Ch1Att
#web Ch2Att
#web FDV_Mon

#web_update SS			 				Set_SS
#web_update FDB		 				Set_FDB
#web_update FDB		 				Rd_MCM_Clk
#web_update FDV		 				Set_FDV
#web_update FDV       				Rd_MCM_Clk
#web_update ip, mask, gateway		Set_NW
#web_update reset1               MCM_Reset
#web_update Ch1Att, Ch2Att       ofc_atten

char SS1_Mon[7], SS2_Mon[7], FDB_Mon[4], MCM_Clk[10], temps[6], state[7];
#web SS1_Mon
#web SS2_Mon
#web FDB_Mon
#web MCM_Clk
#web temps
#web state

int fps_def_cnt[4] = { 1300, 6500, 11000, 16500 } ;
float temp, i1, i2, i3, i4, p1, p2, p3, p4;

typedef enum { CURR_ENC, TARGET_ENC, CURR_ANG, TARGET_ANG, CURR_RPM, MODE, FPS_VER, RAMP_DNTIME,
RAMP_UPTIME, LOW_RPM_LMT, LOW_RPM_TIME, BRK_CNT_DIFF, STEP_SLW_TIMECNT,
FINE_TUNE_PWM, MAX_PWM, MAX_ANG, MIN_ANG, UA0_ANG, OVER_CURR, COMMERR,
CMDFAIL, TIMEOUT_CNT, CHKSUM_CNT, RST_CNT, FPS_CTRL_INFO, FPS_MOTOR_TEMP,
MOTOR_CURR, DOMON_STAT, TNG_PT, STOP_CNT, TVAR } ENUM_FPSPARA;

char par_value[TVAR][5], in_d[32], out_data[12], mcm_addr;
int bytecntr_in=0, resp_msg_cntr, resp_code ;

char smoke_detector[7], intruder_detector[7], Attn1_Mon[8], Attn2_Mon[8], temp1[6];
#web smoke_detector
#web intruder_detector
#web Attn1_Mon
#web Attn2_Mon
#web temp1

char lt1i1[6], lt2i2[6], lt3i3[6], lt4i4[6], lt1p1[6], lt2p2[6], lt3p3[6], lt4p4[6];
#web lt1i1
#web lt2i2
#web lt3i3
#web lt4i4
#web lt1p1
#web lt2p2
#web lt3p3
#web lt4p4

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

typedef enum { WRAPPER=10, INTERMEDIATE=11, FINAL=12, ASYNCH=13, ALARM=100 } EventEnum ;
typedef enum { SUCCESS=10, FAILED=11, SYNTX=12, INCOMPLETE=13, INVALID=14, UNKNOWNERR=100 } CodeEnum ;

//char SenMon_Para[][10]={"time","status","temp","chan","rawcnt"};

//#define SMP_rawsize	sizeof(SenMon_Para)/sizeof(SenMon_Para[0])

