
#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

//commented for remote fw on multiple MCM
#define MY_IP_ADDR     "192.168.8.132"
#define MY_GATEWAY     "192.168.8.1"
#define MY_NET_MASK    "255.255.255.0"

//#define SERVER_ADDR    "192.168.8.143"  // Server for lab testing
//#define SERVER_PORT    "3000"           // For 192.168.8.143

#define SERVER_ADDR    "192.168.4.65"   // Server for GAB System
#define SERVER_PORT    "3000"           // For 192.168.4.65

#define TCP_BUF_SIZE    16392
#define MAX_SOCKETS     2
#define MAX_TCP_SOCKET_BUFFERS 2
#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds

#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 200

void Ms_delay(unsigned int delay);
void Set_DMask();

#define SYSTEM_1			"gab"
#define Cmd_101			"domon"
#define Cmd_102			"set gablo"
#define Cmd_103			"set gabconf"
#define Cmd_104			"set attn"
#define Cmd_105			"set reflo"
#define Cmd_106			"init"
#define Cmd_107			"reset"
#define Cmd_108			"settime"

#ximport  "MCM_Mon.zhtml"   MCM_Mon_zhtml
#ximport  "Mon_Raw.xml"     Mon_Raw_xml
#ximport  "Mon_Sum.xml"     Mon_Sum_xml
#ximport  "MCM_Set.zhtml"   MCM_Set_zhtml

void Set_LO();
void Set_LO_Ptrn(unsigned long, int);
void Set_LOR_Ptrn(unsigned long, int);
void Set_Attn(int, int);
void Set_LORef();
int command_process_gab();

char msw[5], lsw[5];
#web msw
#web lsw

char Attn1_Mon[10], Attn2_Mon[10];
#web Attn1_Mon
#web Attn2_Mon

int filter1, filter2;
#web filter1
#web filter2

char lpf1[15], lpf2[15];
#web lpf1
#web lpf2

char in1[10], in2[10], path1[10], path2[10], source1[20], source2[20];
#web in1
#web in2
#web path1
#web path2
#web source1
#web source2

int ref_lo1, ref_lo2;
#web ref_lo1
#web ref_lo2

unsigned long LO1, LO2;
#web LO1
#web LO2

char LOR1_Mon[10], LOR2_Mon[10], LO1_Mon[10], LO2_Mon[10];
#web LOR1_Mon
#web LOR2_Mon
#web LO1_Mon
#web LO2_Mon

#web_update LO1, LO2  				Set_LO
#web_update ref_lo1, ref_lo2     Set_LORef
#web_update msw, lsw             Set_DMask

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

