
#define TCPCONFIG 	 	0
#define USE_ETHERNET  	1

//#define MY_IP_ADDR     "192.168.4.100"
//#define MY_GATEWAY     "192.168.4.1"
//#define MY_NET_MASK    "255.255.255.0"
//#define SERVER_ADDR    "192.168.8.253"  // Server for lab testing
//#define SERVER_PORT    "3000"           // For 192.168.8.143

//#define TCP_BUF_SIZE    16392
//#define MAX_SOCKETS     2
//#define MAX_TCP_SOCKET_BUFFERS 2
//#define SCAN_DELAY		1000		// Doing background monitoring at every 4000 miliseconds

#define SPI_SER_D
#define SPI_RX_PORT SPI_RX_PD
#define SPI_CLK_DIVISOR 200

void Ms_delay(unsigned int delay);
void Set_rfi_atten();
void Set_nf_atten();
void Set_pwm();
void Set_atten_mode();
void Set_time_mode();
void Set_on_time();
void Set_total_time();

void Set_32bit();

#ximport  "MCM_Mon.zhtml"   MCM_Mon_zhtml
#ximport  "Mon_Sum.xml"     Mon_Sum_xml
#ximport  "gmrt.jpg"        gmrt_jpg
#ximport  "homep.zhtml"     homep_zhtml
#ximport  "calculations.zhtml"     calculations_zhtml

int atten_mode, time_mode, rfi_atten, t_on, nf_atten, t_total;

#web atten_mode select("Atten Mode" = 0, "Direct Connection to ADC","Radiation Through Antenna","Random Pulse noise attenuation")

#web time_mode select("Time Mode" = 0, "Fix duty cycle","Random time instances")

#web rfi_atten select("Select Atten" = 0, "0", "0.5", "1", "1.5", "2", "2.5", "3", "3.5", "4", "4.5", "5", "5.5", "6", \
                      "6.5", "7", "7.5", "8", "8.5", "9", "9.5", "10", "10.5", "11", "11.5", "12", "12.5", "13", "13.5", "14", "14.5", "15", \
                      "15.5", "16", "16.5", "17", "17.5", "18", "18.5", "19", "19.5", "20", "20.5", "21", "21.5", "22", "22.5", "23", "23.5", \
                      "24", "24.5", "25", "25.5", "26", "26.5", "27", "27.5", "28", "28.5", "29", "29.5", "30", "30.5", "31", "31.5")

#web t_on select("ON Time" = 0, "1 us", "2 us", "3 us", "4 us", "5 us", "6 us", "7 us", "8 us", "9 us", "10 us", "11 us", \
                 "12 us", "13 us", "14 us", "15 us", "16 us", "17 us", "18 us", "19 us", "20 us", "21 us", "22 us", "23 us", \
                 "24 us", "25 us", "26 us", "27 us", "28 us", "29 us", "30 us", "31 us", "32 us", "33 us", "34 us", "35 us", \
                 "36 us", "37 us", "38 us", "39 us", "40 us", "41 us", "42 us", "43 us", "44 us", "45 us", "46 us", "47 us", \
                 "48 us", "49 us", "50 us", "51 us", "52 us", "53 us", "54 us", "55 us", "56 us", "57 us", "58 us", "59 us", \
                 "60 us", "61 us", "62 us", "63 us", "64 us")

#web nf_atten select("Select Atten" = 0, "0", "0.5", "1", "1.5", "2", "2.5", "3", "3.5", "4", "4.5", "5", "5.5", "6", \
                     "6.5", "7", "7.5", "8", "8.5", "9", "9.5", "10", "10.5", "11", "11.5", "12", "12.5", "13", "13.5", "14", "14.5", "15", \
                     "15.5", "16", "16.5", "17", "17.5", "18", "18.5", "19", "19.5", "20", "20.5", "21", "21.5", "22", "22.5", "23", "23.5", \
                     "24", "24.5", "25", "25.5", "26", "26.5", "27", "27.5", "28", "28.5", "29", "29.5", "30", "30.5", "31", "31.5")

#web t_total select("ON Time" = 0, "8 (1) ms", "16 (2) ms", "24 (3) ms", "32 (4) ms", "40 (5) ms", "48 (6) ms", "56 (7) ms", "64 (8) ms", "72 (9) ms", "80 (10) ms", \
                    "88 (11) ms", "96 (12) ms", "104 (13) ms", "112 (14) ms", "120 (15) ms", "128 (16) ms", "136 (17) ms", "144 (18) ms", "152 (19) ms", "160 (20) ms", \
                    "168 (21) ms", "176 (22) ms", "184 (23) ms", "192 (24) ms", "200 (25) ms", "208 (26) ms", "216 (27) ms", "224 (28) ms", "232 (29) ms", "240 (30) ms", \
                    "248 (31) ms", "256 (32) ms", "264 (33) ms", "272 (34) ms", "280 (35) ms", "288 (36) ms", "296 (37) ms", "304 (38) ms", "312 (39) ms", "320 (40) ms", \
                    "328 (41) ms", "336 (42) ms", "344 (43) ms", "352 (44) ms", "360 (45) ms", "368 (46) ms", "376 (47) ms", "384 (48) ms", "392 (49) ms", "400 (50) ms", \
                    "408 (51) ms", "416 (52) ms", "424 (53) ms", "432 (54) ms", "440 (55) ms", "448 (56) ms", "456 (57) ms", "464 (58) ms", "472 (59) ms", "480 (60) ms", \
                    "488 (61) ms", "496 (62) ms", "504 (63) ms", "512 (64) ms", "520 (65) ms", "528 (66) ms", "536 (67) ms", "544 (68) ms", "552 (69) ms", "560 (70) ms", \
                    "568 (71) ms", "576 (72) ms", "584 (73) ms", "592 (74) ms", "600 (75) ms")

#web_update atten_mode        Set_atten_mode
#web_update time_mode   		Set_time_mode
#web_update rfi_atten   		Set_rfi_atten
#web_update nf_atten    		Set_nf_atten
#web_update t_total           Set_total_time
#web_update t_on              Set_on_time

int word32_1, word32_2, word32_3, word32_4;

#define ADC_SCLKBRATE 	115200ul
#define GAINSET 		   GAIN_1
#define CHANNEL_1 		0
#define CHANNEL_2 		1
#define CHANNEL_3 		2
#define CHANNEL_4 		3

#define USE_RABBITWEB 1
#define USE_HTTP_DIGEST_AUTHENTICATION 1

#ifndef MY_IP_ADDR
char MY_IP_ADDR[20], MY_GATEWAY[20], MY_NET_MASK[20];
#endif

char ip[20], mask[20], gateway[20], psw[10];
#web ip
#web mask
#web gateway
#web psw

char rfi_atten_Mon[8], nf_atten_Mon[8], t_on_mon[8], t_total_mon[8], rfi_mode_mon[35], time_mode_mon[35];
#web rfi_atten_Mon
#web nf_atten_Mon
#web t_on_mon
#web t_total_mon
#web rfi_mode_mon
#web time_mode_mon

struct tm  rtc;
unsigned long t0, sec_tm;

char time1[10], date[12], RTC_time[20];
#web time1
#web date



