#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

#define MY_IP_ADDR    	"192.168.8.163"
//#define MY_IP_ADDR    	"192.168.8.163"
#define MY_GATEWAY      "192.168.8.1"
#define MY_NET_MASK   	"255.255.255.0"
//#define SERVER_ADDR	 	"192.168.8.143"
//#define SERVER_ADDR	 	"192.168.4.65"       // Acer Laptop
#define SERVER_ADDR		"192.168.60.4"    // Raj PC

#define SERVER_PORT	   "8000"      // For Sentinal
//#define SERVER_PORT	 	"7570"      // For Sigcon

#define TCP_BUF_SIZE  	4096

#define SCAN_DELAY		4000		// Doing background monitoring at every 4000 miliseconds //

#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 100

#define SYSTEM_0			"sentinal"
#define SYSTEM_1			"sigcon"
#define SYSTEM_2			"backend"

#define Cmd_001			"domon"
#define Cmd_002			"setcmd"
#define Cmd_003			"init"
#define Cmd_004			"reset"

#define Cmd_101			"domon"
#define Cmd_102			"set lo"
#define Cmd_103			"set attn"
#define Cmd_104			"set reflo"
#define Cmd_105			"init"
#define Cmd_106			"reset"

#define ADC_SCLKBRATE 	115200ul
#define GAINSET 		   GAIN_1
#define CHANNEL_1 		0
#define CHANNEL_2 		1
#define CHANNEL_3 		2
#define CHANNEL_4 		3

#define MAXDATA       	128
#define DATALEN       	64
#define MSGLEN				128


typedef enum { WRAPPER=10, INTERMEDIATE=11, FINAL=12, ASYNCH=13, ALARM=100 } EventEnum ;
typedef enum { SUCCESS=10, FAILED=11, SYNTX=12, INCOMPLETE=13, INVALID=14, UNKNOWNERR=100 } CodeEnum ;

char temp[10], temp_raw[10], time_ar[10];
int mon_arr[70];
struct tm rtc;
unsigned long sec_tm;
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
    long  seq;                 // In new Wrapper ver-V1 seq has been defined as int //  /* Sequence number of Command */
    float version;
    short priority ;
    short timeout ;
    char  subsysid[64];
    char  cmd_name[64];        /* Command name at user level */
    char  timestamp[64];
    char  cmd_id[8];           /* Command ID Device Specific */

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

int mcm_addr;
char SenMon_Para[][10]={"time","status","temp","chan","rawcnt"};

#define SMP_rawsize	sizeof(SenMon_Para)/sizeof(SenMon_Para[0])

int ch1_attn, ch2_attn;
float if_amp_vccp, if_amp_vccn, if_attn_vccp, if_attn_vccn, if_vdd, vco2_vccp, vco3_vccp, vco2_vdd, vco3_vdd, amp_vdd;
float ibob_vcc, ibob_gnd;