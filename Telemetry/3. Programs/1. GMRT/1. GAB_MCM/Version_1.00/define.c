#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

#define MY_IP_ADDR      "192.168.8.163"        // For lab testing
#define MY_GATEWAY	   "192.168.8.1"
#define MY_NET_MASK   	"255.255.255.0"
#define SERVER_ADDR	   "192.168.8.143"
#define SERVER_PORT	   "3000"

/*#define MY_IP_ADDR    	"192.168.30.27"     // For GAB System testing
#define MY_NET_MASK   	"255.255.255.0"
#define MY_GATEWAY		"192.168.30.1"
#define SERVER_ADDR		"192.168.4.65"
#define SERVER_PORT	   "3000"     */        // For 192.168.4.65

//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************

#define BU_TEMP_USE_SBF
#define  STDIO_DEBUG_SERIAL   SADR
#define	STDIO_DEBUG_BAUD		115200
#define	STDIO_DEBUG_ADDCR
#define USE_HTTP_UPLOAD		// Required for this, to include upload code.
#define DISABLE_DNS			// No name lookups required
#define SSPEC_FLASHRULES
#define HTTP_MAXSERVERS 1
#define HTTP_SOCK_BUF_SIZE (12 * MAX_MTU)
#ifdef BU_TEMP_USE_FAT
	// This value is required to enable FAT blocking mode for ZServer.lib
	#define FAT_BLOCK

	#define FAT_USE_FORWARDSLASH		// Necessary for zserver.

	#use "fat16.lib"							// Must use FAT before http/zserver
#endif

//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************

#define TCP_BUF_SIZE    16392
//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************
#define MAX_SOCKETS     2
#define MAX_TCP_SOCKET_BUFFERS 2
//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************
#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds //

#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 200

#define SYSTEM_0			"sentinal"
#define SYSTEM_1			"gab"
#define SYSTEM_2			"backend"

#define Cmd_001			"domon"
#define Cmd_002			"setcmd"
#define Cmd_003			"init"
#define Cmd_004			"reset"

#define Cmd_101			"domon"
#define Cmd_102			"set gablo"
#define Cmd_103			"set gabconf"
#define Cmd_104			"set attn"
#define Cmd_105			"set reflo"
#define Cmd_106			"init"
#define Cmd_107			"reset"

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
//#define USE_HTTP_BASIC_AUTHENTICATION 1
#define USE_HTTP_DIGEST_AUTHENTICATION 1

#ximport  "MCM_Mon.zhtml"   MCM_Mon_zhtml
#ximport  "Mon_Raw.xml"     Mon_Raw_xml
#ximport  "Mon_Sum.xml"     Mon_Sum_xml
#ximport  "MCM_Set.zhtml"   MCM_Set_zhtml
#ximport  "upload.zhtml"    upload_zhtml


typedef enum { WRAPPER=10, INTERMEDIATE=11, FINAL=12, ASYNCH=13, ALARM=100 } EventEnum ;
typedef enum { SUCCESS=10, FAILED=11, SYNTX=12, INCOMPLETE=13, INVALID=14, UNKNOWNERR=100 } CodeEnum ;


void Set_LO();
void Set_SS();
void Set_FDB();
void Set_FDV();
void Rd_MCM_Clk();
void Set_NW();

int cmd_validation();
int form_inter_resp();
int command_process_sen();
int command_process_sig();
void form_final_resp(int, int);
void bg_mon();
void Sig_decode();
void Set_DMask();
void Set_RTC(char far *);
void Set_LO();
void Set_LO_Ptrn(unsigned long, int);
void Set_LOR_Ptrn(unsigned long, int);
void Set_Attn(int, int);

//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************
void Set_LOR();
void Set_LORef();
void install_firmware();

struct {
	char	running[40];		// information on the currently running firmware
	char	temp[40];			// information on the uploaded file
	int	show_temp;			// 1 == show info on temp firmware (set to 0 for
									// boards that only allow direct uploading)
	int	show_install;		// flag whether to show install button or not
									// 1 == show, 0 == don't show, -1 == show error
   int	show_status;
	char	error[80];
	int	install;				// flag to initiate install and to indicate whether
									// there was an error (0 == normal, 1 == initiate
									// install, -1 == error attempting install)
   long signed int fwlen;
} firmware;

#web firmware
#web firmware.install
#web_update firmware.install install_firmware

float db1, db2;
#web db1
#web db2

int filter1, filter2;
#web filter1
#web filter2

char lpf1[15], lpf2[15];
#web lpf1
#web lpf2

char in1[10], in2[10], path1[10], path2[10];
#web in1
#web in2
#web path1
#web path2

char source1[20], source2[20];
#web source1
#web source2

//******************************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*****************

char ip[20], mask[20], gateway[20];
#web ip
#web mask
#web gateway

char LO1[5], LO2[5];
#web LO1
#web LO2
//#web Temp_LO1
//#web Temp_LO2

char LO1_Mon[20], LO2_Mon[20];
#web LO1_Mon
#web LO2_Mon

char Attn1_Mon[10], Attn2_Mon[10];
#web Attn1_Mon
#web Attn2_Mon

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

char msw[5], lsw[5], mswT[5], lswT[5];
#web msw
#web lsw
#web mswT
#web lswT

char LOR1_Mon[20], LOR2_Mon[20];
#web LOR1_Mon
#web LOR2_Mon

char ref_lo1[4], ref_lo2[4];
#web ref_lo1
#web ref_lo2
//#web Temp_LOR1
//#web Temp_LOR2

struct tm  rtc;
unsigned long t0, sec_tm;
char time1[10], date[12];
#web time1
#web date

bbram char fwitime[10], fwidate[12], fwutime[10], fwudate[12];
#web fwitime
#web fwidate
#web fwutime
#web fwudate

int rw[70];
#web rw

char vl[70][10];
#web vl

#web_update LO1, LO2  				Set_LO
#web_update SS			 				Set_SS
#web_update FDB		 				Set_FDB
#web_update FDB		 				Rd_MCM_Clk
#web_update FDV		 				Set_FDV
#web_update FDV       				Rd_MCM_Clk
#web_update msw, lsw, db1, db2, filter1, filter2, lpf1, lpf2, source1, source2, in1, in2, path1, path2  	Set_DMask
#web_update ref_lo1, ref_lo2        	Set_LORef
#web_update ip, mask, gateway		Set_NW

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

float if_vccp, if_vccn, if_vdd, lo_vccp, lo_vccn, lo_vdd;
char dmask[10];