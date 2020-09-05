
#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

#define MY_IP_ADDR     "192.168.8.160"
#define MY_GATEWAY     "192.168.8.1"
#define MY_NET_MASK    "255.255.255.0"
#define SERVER_ADDR    "192.168.8.145"  // Server for lab testing
#define SERVER_PORT    "3005"           // For 192.168.8.143

//#define SERVER_ADDR    "192.168.4.65"   // Server for FE System
//#define SERVER_PORT    "3000"           // For 192.168.4.65

#define TCP_BUF_SIZE    16392
#define MAX_SOCKETS     2
#define MAX_TCP_SOCKET_BUFFERS 2
#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds

void Ms_delay(unsigned int delay);
void Set_DMask();
void sen_mon();
void FE_mon();
void FE_atten();
void FE_alarm_handler();
void Set_FE_atten(int ch2, int ch1);
int command_process_FE();
void serial_init();
char chksum(char *ary);
void enable_485Tx( void );
void disable_485Tx( void );
void transmit();
void ser2eth_comm(void);
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

float temp;

//**************************** Defination part for FPS System *************************//

#define SYSTEM_2			"fecb"
#define Cmd_201			"domon"
#define Cmd_202			"init"
#define Cmd_203			"reset"
#define Cmd_204			"sel_febox"
//#define Cmd_206			"sel_fefilter_ch1"
//#define Cmd_207			"sel_fefilter_ch2"
//#define Cmd_208			"rfband"
#define Cmd_209			"rfswap"
#define Cmd_210			"rfattn"
//#define Cmd_211			"rfon"
#define Cmd_212			"cbterm"
#define Cmd_213			"rfterm"
#define Cmd_214			"rfngset"
#define Cmd_215			"rfngcal"
#define Cmd_216			"setwalsh"
#define Cmd_217			"sel_walshgrp"
#define Cmd_218			"setrfsys"
#define Cmd_219			"rfcm_sw"
#define Cmd_220			"rawmon"
#define Cmd_221			"restore"
#define Cmd_222			"settime"

#ximport  "MCM_Mon.zhtml"  MCM_Mon_zhtml
#ximport  "Mon_Raw.xml"     Mon_Raw_xml
#ximport  "Mon_Sum.xml"    Mon_Sum_xml
#ximport  "MCM_Set.zhtml"   MCM_Set_zhtml
#ximport  "Mon_check.xml"   Mon_check_xml

#define CINBUFSIZE 31
#define COUTBUFSIZE 31
#define RS232_NOCHARASSYINBRK
#define MSSG_TMOUT 5UL
#define IDLE_TMOUT 50UL

char in_d[32];
int bytecntr_in=0;
int resp_msg_cntr;
int resp_code;
char out_data[32], mcm_addr;

char temps[10], state[10];
#web temps
#web state

char msw[5], lsw[5];
#web msw
#web lsw

char Attn1_Mon[10], Attn2_Mon[10], temp1[10];
#web Attn1_Mon
#web Attn2_Mon
#web temp1

int Ch1Att, Ch2Att;
#web Ch1Att
#web Ch2Att
#web_update Ch1Att, Ch2Att FE_atten

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

char dmask[10];

//********************************************************************************************//