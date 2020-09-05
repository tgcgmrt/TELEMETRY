#define TCPCONFIG 		0
#define USE_ETHERNET  	1


#define MY_IP_ADDR    	"192.168.8.163"
#define MY_NET_MASK   	"255.255.255.0"
#define MY_GATEWAY		"192.168.8.1"


#define SERVER_ADDR		"192.168.21.10"


#define SERVER_PORT	 	"3000"      // In House CMS testing port //

#define TCP_BUF_SIZE  	16384

#define USE_RABBITWEB 1
#define USE_HTTP_BASIC_AUTHENTICATION 1


#define SCAN_DELAY		900		// Doing background monitoring at every 4000 miliseconds //
#define PING_DELAY		30000


#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 100

#define FAT_BLOCK


#define ADC_SCLKBRATE 	115200ul
#define GAINSET 		   GAIN_1
#define CHANNEL_1 		0
#define CHANNEL_2 		1
#define CHANNEL_3 		2
#define CHANNEL_4 		3

#define MAXDATA       	64
#define DATALEN       	64
#define MSGLEN				128


#define	System_1		"sentinel"        // Sentinel
#define	System_2		"front_end"       // Front End
#define	System_3		"fiber_optics"    // Fiber Optics
#define	System_4		"back_end"        // Back End
#define	System_5		"fps"             // Feed Positioning System
#define	System_6		"self_test"			// Testing MCM

#define	Opr_1			"mon"					// Monitoring
#define	Opr_2			"set"	 				// Controlling
#define	Opr_3			"init"				// Initialization
#define	Opr_4			"reset"				// Reset


#ximport  "MCM_Mon.zhtml"   MCM_Mon_html
#ximport  "Mon_Raw.xml"     Mon_Raw_xml
#ximport  "Mon_Sum.xml"     Mon_Sum_xml

typedef struct
 {
  char ant[5];
  char ip_addr[20];

  char sys_name[20];
  char sys_para[10][20];
  char arg1[10][20];
  char arg2[10][20];
 } Mon_Page_Info ;

Mon_Page_Info webpg_mon;

#web webpg_mon


int SS, FDB, FDV;
float MCM_Clk_Frq;

char SS1_Mon[10], SS2_Mon[10], FDB_Mon[5], FDV_Mon[5], MCM_Clk[10];
#web SS1_Mon
#web SS2_Mon
#web FDB_Mon
#web FDV_Mon
#web MCM_Clk

char msw[5], lsw[5];
#web msw
#web lsw

int  mon_arr[70];
#web mon_arr

char mon_vlt[70][10];
#web mon_vlt



struct tm rtc;
unsigned long sec_tm;
char RTC_time[20];
#web RTC_time

char RTC_date[20];
#web RTC_date

int mcm_addr;



char SEN_Para[][10]={
						   "dmask"		  		// Digital Mask		0000 to FFFF
                    };

#define	SEN_Para_No	sizeof(SEN_Para)/sizeof(SEN_Para[0])



char FE_Para[][15]={
                    "band_sel",			// Band Selection			50 - 150 - 233 - 327 - 610 - L Band
                    "slr_attn",			// Solar Attenuation		0 - 14 - 30 - 44 - Terminate
                    "channel",			// Channel					Swap - Unswap
                    "sub_band_sel",		// L-SubBand Selection	1060 - 1170 - 1280 - 1390 - Bypass
                    "rf",              // RF							On - Off
	  					  "cal_ns"           // Calibaration Noise	Lo - Med - Hi - ExtraHi
                   };

#define	FE_Para_No	sizeof(FE_Para)/sizeof(FE_Para[0])



char OF_Para[][10]={
						  "rf_attn"		  		// RF Attenuation 		1 to 50 dB
                   };

#define	OF_Para_No	sizeof(OF_Para)/sizeof(OF_Para[0])



/* Dont change the order of BE_Para values*/
char BE_Para[][10]={
						  "lo",              // LO Freq
                    "reflo",           // Reference LO
						  "attn",            // Attenuation
                    "filter",          // Filter            0 to 7
                    "lpf",             // Low Pass Filter   0 to 3
                    "source",          // Source				SigGen - Synthesizer
                    "signal",          // Signal 				Antenna - Noise
                    "path",            // Path 					Direct - Mixer
                    "channel"				// Channel				1 - 2
                   };

#define	BE_Para_No	sizeof(BE_Para)/sizeof(BE_Para[0])



char MCM_Para[][10]={
						   "ss",					// Spectrum Spreader
                     "fdb",				// Clock doubler
                     "fdv"					// Clock divider
                    };

#define	MCM_Para_No	sizeof(MCM_Para)/sizeof(MCM_Para[0])


typedef struct     /* total 1638 bytes */
 {
  unsigned long Sequence_No;
  char TimeStamp[64];

  char System_Name[16];
  char Operation_Name[16];

  int  Total_Parameter;
  char Parameter_Name[32][16];
  char Argument_Ch1[32][16];
  char Argument_Ch2[32][16];
 } Command;


typedef struct     /* Total 2650 bytes */
 {
  unsigned long Response_Type;
  unsigned long Sequence_No;
  char TimeStamp[64];

  char System_Name[16];

  char Mon_Raw[64][8];
  char Mon_Sum[32][64];
  int  Total_Resp_Msg;
  char Response_Message[32][64];
 } Response;

typedef struct    /* Total 68 bytes */
 {
  int Alarm_Priority;
  int Alarm_Type;
  char Alarm_Message[64];
 } Alarm;

Command		pc_cmd;
Response		mcm_resp;
Alarm			mcm_alarm;

#define 	pc_cmd_size			sizeof(pc_cmd)
#define 	mcm_resp_size	   sizeof(mcm_resp)
#define 	mcm_alarm_size	   sizeof(mcm_alarm)

int conn_flag = 1;

char reset[2];
#web reset






