// **************************************************************************** //
// *******                    Program : FES_MCM                        ******** //
// *******                    Date : 31/12/2015                        ******** //
// *******                     Version : V1.00                         ******** //
// *******               Client for Existing Online Server             ******** //
// *******        HTTP Server & client for Remote Firmware Update.     ******** //
// *******     Remote Firmware Update (v1.0 & v2.0) using USER BLOCK   ******** //
// *******                  Modified by Rahul Bhor.                    ******** //
// **************************************************************************** //

#class    auto
#memmap   xmem

#use      "rcm43xx.lib"	             // Use with RCM 4300
#use      "adc_ads7870.lib"
#include  "define.c"
#include  "firmware_update.c"
#include  "init.c"
#use	    "dcrtcp.lib"
#use  	 "http.lib"
//#use      "spi.lib"

SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".gif", "image/gif"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/",MCM_Mon_zhtml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Sum.xml",Mon_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Raw.xml",Mon_Raw_xml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_check.xml",Mon_check_xml),
	SSPEC_RESOURCE_XMEMFILE("/MCM_Set.zhtml",MCM_Set_zhtml),
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END

static tcp_Socket Socket;
static int FPS_conn, OFC_conn;
//.... Generating milisecond delay .... //

void Ms_delay(unsigned int delay)
{
	auto unsigned long delay_time;

	delay_time = MS_TIMER + delay;
   while((long)(MS_TIMER - delay_time) < 0 );
}

// .... Opening Server Socket at desired port address .... //

cofunc int mcm_clnt()
{
   int port, write_bytes, read_bytes, wait_var;
   longword host;

   ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP, IFS_END);

   while (ifpending(IF_ETH0) == IF_COMING_UP)
         tcp_tick(NULL);

   if(!(host = inet_addr(SERVER_ADDR)))
   {
      puts("\nCould not resolve host");
      exit(3);
   }
   printf("\n\nHost hex address %lx\n",host);

   port = atoi(SERVER_PORT);

   printf("\nAttempting to open '%s' on port %u\n", SERVER_ADDR, port );

   while( ! tcp_open(&Socket, 0, host, port, NULL))
    {
     tcp_tick(NULL);
     printf("\nUnable to open TCP session");
    }

   wait_var = 0;

   while((!sock_established(&Socket) && sock_bytesready(&Socket)== -1))
    {
      http_handler();
      tcp_tick(NULL);
      wait_var++;
      if(wait_var == 100)
      {
        tcp_open(&Socket, 0, host, port, NULL);
        wait_var = 0;
      }
      yield;
    }
   printf("\nSocket Established\n");

   if(sock_established(&Socket))
   {
    read_bytes = 0;
    do{
       http_handler();
       if(sock_bytesready(&Socket) > 0)
       {
         read_bytes = sock_read(&Socket, &Wrpr_Cmd, Wrpr_Cmd_Size);

         printf("\nMCM has read command of %d bytes",read_bytes);

         if(form_inter_resp() == 10)    // Form intermediate response and send it over socket //
         {
           write_bytes = sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
           printf("\nMCM has sent intermediate response of %d bytes",write_bytes);
           printf("\nIntermediate response :\n %d \n",MCM_Resp.event);
           http_handler();
           form_final_resp(SUCCESS,FINAL);   // Process the command, Form final response and send it over socket //
           write_bytes = sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
   		  printf("\nMCM has sent final response of %d bytes",write_bytes);
   		  printf("\nFinal response :\n %d\n",MCM_Resp.event);
         }
         else
         {
   		 write_bytes = sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
         }
       }
       read_bytes = 0;
       yield;
      }while(tcp_tick(&Socket));    // This loop continuously listens over socket for incoming command //
   }
   http_handler();
   sock_close(&Socket);
	printf("\nConnection closed for port-%d",port);

   return 0;
}

// ..... Main start .... //
main()
{
  auto unsigned long scn_dl;

  fwinit();
  brdInit();
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();
  serial_init();

  strcpy(ip, MY_IP_ADDR);
  strcpy(mask, MY_NET_MASK);
  strcpy(gateway, MY_GATEWAY);

  tcp_reserveport(80);

  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP, IFS_END);
  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);

  while(1)
  {
    http_handler();

     costate
     {
      wfd mcm_clnt();
     }

    costate    // This will do background monitoring in parallel at every SCAN_DELAY miliseconds //
     {
       scn_dl = MS_TIMER + SCAN_DELAY;
       Ms_delay(1);  // This delay is necessary for proper working of following loop. //
       while(((scn_dl - MS_TIMER) > 0) && ((scn_dl - MS_TIMER) < 1000))
       yield;
       bg_mon();
       FE_mon();
     }

    costate
     {
       http_handler();
       tcp_tick(NULL);
     }
  }
}

// ********* This function will validate command against various checks. ********* //
// **** Syntax error, Invalid, Incomplete command, Other error will be checked **** //

int cmd_validation()
 {
   int cmpl=0, i, j;
   char tmpstr[256];

    _f_strcpy(tmpstr,Wrpr_Cmd.cmd_elem.cmd_name);
    _f_strcat(tmpstr, Wrpr_Cmd.cmd_elem.subsysid);
    printf("\n #### Command received: %s \n", tmpstr);

    _f_strcpy(tmpstr, Wrpr_Cmd.cmd_elem.timestamp); printf("time: %s \n", tmpstr);
    _f_strcpy(tmpstr, Wrpr_Cmd.cmd_elem.cmd_id);  printf("cmdId: %s \n", tmpstr);

    printf("NumPkt %d \n", Wrpr_Cmd.cmd_data.numpkt);

    for (i=0;i< Wrpr_Cmd.cmd_data.numpkt;i++)
    {
     _f_strcpy(tmpstr,Wrpr_Cmd.cmd_data.prmname[i]); printf("Name %s ", tmpstr);
     _f_strcpy(tmpstr,Wrpr_Cmd.cmd_data.prmvalue[i]); printf("value %s \n", tmpstr);
    }

   printf("\nEntering cmd_validation");

      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_2))
       {
         printf("\nSubsystem validated");

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_201))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_202))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_203))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_204))
          {
           return 10;
          }
         /*else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_205))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_206))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_207))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_208))
          {
           return 10;
          } */
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_209))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_210))
          {
           return 10;
          }
          /*else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_211))
          {
           return 10;
          }   */
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_212))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_213))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_214))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_215))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_216))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_217))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_218))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_219))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_220))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_221))
          {
           return 10;
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_222))
          {
           return 10;
          }
         else
          {
           return 12;     // Incase of Syntax Error in command //
          }
       }
      else
       {
        return 14;
       }

 }

// ****** Forming intermediate response for all system commands ****** //

int form_inter_resp()
{
  int loop;

  printf("\nEntering form_inter_resp");

  MCM_Resp.code = cmd_validation();
  MCM_Resp.event = INTERMEDIATE;
  MCM_Resp.MCM_alarm.level=1;
  switch(MCM_Resp.code)
  {
    case 10 :  _f_strcpy(MCM_Resp.message,"Command received successfully");
               printf("\nCommand received successfully");
               break;
    case 12 :  _f_strcpy(MCM_Resp.message,"Command Syntax Error");
               printf("\nCommand Syntax Error");
               break;
    case 13 :  _f_strcpy(MCM_Resp.message,"Command Incomplete");
               printf("\nCommand Incomplete");
               break;
    case 14 :  _f_strcpy(MCM_Resp.message,"Command Invalid");
               printf("\nCommand Invalid");
               break;
    case 100:  _f_strcpy(MCM_Resp.message,"Error Unknown");
               printf("\nError Unknown");
               break;
    default :  break;
  };

 // ******** Write in to Tx_BasicFlds *********** //
  if ( MCM_Resp.code != 10 )
  MCM_Resp.event = FINAL;

  MCM_Resp.resp_elem.seq = Wrpr_Cmd.cmd_elem.seq;
  MCM_Resp.resp_elem.version = Wrpr_Cmd.cmd_elem.version;
  _f_strcpy(MCM_Resp.resp_elem.subsysid,Wrpr_Cmd.cmd_elem.subsysid);
  _f_strcpy(MCM_Resp.resp_elem.cmd_id,Wrpr_Cmd.cmd_elem.cmd_id);
  _f_strcpy(MCM_Resp.resp_elem.cmd_name,Wrpr_Cmd.cmd_elem.cmd_name);
  _f_strcpy(MCM_Resp.resp_elem.timestamp,Wrpr_Cmd.cmd_elem.timestamp);
  MCM_Resp.resp_elem.priority = Wrpr_Cmd.cmd_elem.priority;
  MCM_Resp.resp_elem.timeout = Wrpr_Cmd.cmd_elem.timeout;

  // ******** Write in to Tx_DataPkt *********** //
  MCM_Resp.resp_data.numpkt = 0;
  for(loop=0; loop<MAXDATA; loop++)
   {
     _f_strcpy(MCM_Resp.resp_data.prmname[loop],"\0");
     _f_strcpy(MCM_Resp.resp_data.prmvalue[loop],"\0");
   }
  // ******** Write in to Tx_alarm *********** //
  MCM_Resp.MCM_alarm.id = 0;
  MCM_Resp.MCM_alarm.level = 0;
  _f_strcpy(MCM_Resp.MCM_alarm.description,"\0");

  printf("\nMCM_Resp.code: %d\n", MCM_Resp.code);
  return MCM_Resp.code;
}

// ******** Forming final response ******** //

void form_final_resp(int code, int event)
{
  printf("\nEntering form_final_resp");
  resp_msg_cntr = 0 ;
  MCM_Resp.code = code;
  MCM_Resp.event = command_process_FE();
  MCM_Resp.resp_data.numpkt = resp_msg_cntr;
}

void FE_atten()
{
Set_FE_atten(Ch2Att, Ch1Att);
}

void Set_FE_atten(int ch2, int ch1)
{
 printf("Entering Set_FE_attenuation: %d dB  %d dB\n ", ch1, ch2);

 sprintf(Attn1_Mon, "%d dB", ch1);
 sprintf(Attn2_Mon, "%d dB", ch2);

 BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 BitWrPortI(PBDR, &PBDRShadow, 0, 3);
 BitWrPortI(PBDR, &PBDRShadow, 0, 4);
 BitWrPortI(PBDR, &PBDRShadow, 0, 5);

 WrPortI(PADR, &PADRShadow, 0x00);
 if((ch2 & 0x01) == 0x01)
 ch1 = ch1 | 0x40;
 if((ch2 & 0x02) == 0x02)
 ch1 = ch1 | 0x80;

 WrPortI(PADR, &PADRShadow, ch1);
 BitWrPortI(PBDR, &PBDRShadow, 1, 2);     // latch 1st latch ic

 WrPortI(PADR, &PADRShadow, 0x00);
 ch2 = ch2 >> 2;
 WrPortI(PADR, &PADRShadow, ch2);
 BitWrPortI(PBDR, &PBDRShadow, 1, 3);     // latch 2nd latch ic

 WrPortI(PADR, &PADRShadow, 0x00);
 BitWrPortI(PBDR, &PBDRShadow, 1, 4);
 BitWrPortI(PBDR, &PBDRShadow, 1, 5);
 BitWrPortI(PBDR, &PBDRShadow, 0, 6);
}

void form_alarm(int Alrm_id, int Alrm_lvl)
{
  MCM_Resp.code = 10;
  MCM_Resp.event = 100;

  MCM_Resp.MCM_alarm.id = Alrm_id;
  MCM_Resp.MCM_alarm.level = Alrm_lvl;
  _f_strcpy(Wrpr_Cmd.cmd_elem.cmd_name,"domon");
  command_process_FE();
  sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
}

void FE_mon()
{
}

// ******** Processing the FPS command ******** //
int command_process_FE()
{
  int arg_fpscnt=0;
  resp_msg_cntr=0;

  printf("\nEntering Command_Process_FE");
  memset(out_data,0x00,sizeof(out_data));

  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_201))           // domon
  {
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"time");

  sec_tm = read_rtc();
  mktm(&rtc, sec_tm);
  sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],time_ar);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_NGCAL");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 5);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_ON_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_ON_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_CWCAL");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RFTERM_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RFTERM_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_FILTER_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_FILTER_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_WALSH_SW");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_WALSH_GROUP");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RFFREQ_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RFFREQ_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "NGSET");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_FILTER_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_FILTER_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "BAND_SELECT_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "BAND_SELECT_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "SOL_ATTEN_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "SOL_ATTEN_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_SWAP");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RFCM_SW");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_POW");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "serialized_output");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "BAND_SELECT_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "BAND_SELECT_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "SOL_ATTEN_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "SOL_ATTEN_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_SWAP_M1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_SWAP_M2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "+VCC_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "+VCC_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "-VCC_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "-VCC_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "-VRef_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "-VRef_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_POW_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_POW_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_TEMP");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 28.5);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_ON_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_ON_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_LNA_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_LNA_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_POW_M");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", 1);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_PREAMP_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_PREAMP_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_POSTAMP_M_CH1");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "CB_POSTAMP_M_CH2");
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", 1.0);

  //transmit();
  //ser2eth_comm();
  return 99;
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_202))      // Init
  {
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_203))      // Reset
  {
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_204))
  {

  }

 /*  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_205))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_206))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_207))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_208))
  {

  }   */

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_209))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_210))
  {

  }

 /* else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_211))
  {

  }  */

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_212))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_213))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_214))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_215))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_216))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_217))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_218))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_219))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_220))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_221))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_222))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
    return 12;
  }

  else
  {
    printf("Nothing Matched, Received : %s",Wrpr_Cmd.cmd_elem.cmd_name);
    return 12;
  }

  //transmit();
  //ser2eth_comm();
  return 12;
}

char chksum(char *ary)
{
char temp=0, i=0;

  for(i=0; i< (ary[0] -1); i++)
   {
    temp += ary[i];
   }
  temp = ~temp;
  temp += 1;

  return temp;
}

nodebug
void enable_485Tx( void )
{
#asm
	push	ip							;save off IP state
	ipset	1							;set interrupt priority to level 1
	//ld		a,(PDDRShadow)		;get copy of shadow reg V2 - PD4 as RTS
   ld		a,(PBDRShadow)			;get copy of shadow reg V3 - PB7 as RTS
	and		0x7F					;set bit PB7 to LOW to make RTS\ high v3 PB7// v2 PD4 // old PC1 high
	ld		(PBDRShadow),a			;update shadow reg
	ioi	ld	(PBDR),a				;set PE0 high
  	;IOWRITE_A(PBDR)            // commented on 21 July 2014
	pop	ip							;restore IP to the previous state
  ;	ret
#endasm
}

nodebug
void disable_485Tx( void )
{
#asm
	push	ip							;save off IP state
	ipset	1							;set interrupt priority to level 1
	ld		a,(PBDRShadow)			;get copy of shadow reg
	or		0x80						;V3 set bit PB7 HIGH to make RTS\ low; v20xEF //clear bit PC1  old ver
	ld		(PBDRShadow),a			;update shadow reg
	ioi	ld	(PBDR),a				;set PB7low
  	;IOWRITE_A(PBDR)           // commented on 21 July 2014
	pop	ip							;restore IP to the previous state
 	;ret
#endasm
}

void transmit()
{
int j;
mcm_addr = 14;

 for(j=0; j<out_data[0]; j++)
 printf("%2x \n",out_data[j]);

 serCparity(PARAM_MPARITY); //0x04...mark parity
 enable_485Tx();     // make rts\ high at 7404 ic output

 if( !(serCputc(mcm_addr)) ) // this is ONLY for FPS sys.
  printf("cmd_pkt...Serial C port trans buf is full or locked \n");

 serCparity(PARAM_SPARITY); //0x05...space parity

 serCwrite(out_data,out_data[0]);//original, for all mcm except fps sys.
 while(!((BitRdPortI(SCSR,2) == 0)&&(BitRdPortI(SCSR,3)== 0)) );

  disable_485Tx();       //make rts\ low
  memset(out_data,0x00,sizeof(out_data));
  serCwrFlush();
}

void ser2eth_comm(void)
{
 int i, l;
 int maxpktlen;    //holds maximum mcm cmd len

 bytecntr_in = 0;
 resp_msg_cntr = 0;
 maxpktlen = 0;
 l=0;

 //memset(in_d,0x00,sizeof(in_d));

// ------------ Read Packet --------------//

maxpktlen = sizeof(in_d)-1; //max cmd 50 bytes

disable_485Tx();
while((bytecntr_in = serCread(in_d,maxpktlen,MSSG_TMOUT)) <= 5 && l++ <300);
//bytecntr_in = serCread(&in_d,maxpktlen,MSSG_TMOUT);

printf("\nbytecntr_in: %d\n", bytecntr_in);
printf("in_d: \n");
for(i=0;i<bytecntr_in;i++)
{
printf("[%2d] : %x \n", i, in_d[i]);
}

BitWrPortI(PBDR, &PBDRShadow, 0, 7); // PB7=0 (RE\=1, DE=0) will enable Tx & disable Rx.

i=0;
while(in_d[i]!=0x0e)
{
i=i+1;
if(i>5)
break;
}

printf("end of serial transmission... ser2eth \n\n");
resp_code = 10;

}

// ****** MCM reset function will reset MCM card
void MCM_Reset()
{
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    forceWatchdogTimeout();
    //exit(0);
}

// ****** Doing background monitoring tasks and event generation ****** //
void bg_mon()
{
  int channel, mux_sel, set_mux_sel, i;
  float volt;

  //printf("\nMonitoring Array :");
  for(channel=0; channel<4; channel++)
  {
   for(mux_sel=0; mux_sel<16; mux_sel++)
    {
      set_mux_sel = mux_sel*16;

      WrPortI(PEDR, &PEDRShadow, ((RdPortI(PEDR)&0x0f) | set_mux_sel));

      rw[channel*16 + mux_sel] = (anaIn(channel, SINGLE, GAINSET));

      volt = (980 - rw[channel*16 + mux_sel])/196.0;

      volt = volt * 1.035;

      sprintf(vl[channel*16 + mux_sel],"%5.3f",volt);

    }
  }

  t0 = read_rtc();
  mktm(&rtc,t0);
  sprintf(time1,"%02d:%02d:%02d",rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
  sprintf(date,"%02d-%02d-%d",rtc.tm_mday,rtc.tm_mon,(1900+rtc.tm_year));

}

void Set_SS()
{
  if(!(SS))
   SS = Temp_SS;

  if(SS == 1)
   {
     WrPortI(GCM1R,NULL,0x00);
     strcpy(SS1_Mon,"Off");
     strcpy(SS2_Mon,"Off");
     Ms_delay(10);
   }
  else if(SS == 2)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x40);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Normal");
     strcpy(SS2_Mon,"Normal");
   }
  else if(SS == 3)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x00);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Normal");
     strcpy(SS2_Mon,"Strong");
   }
  else if(SS == 4)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x80);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Strong");
     strcpy(SS2_Mon,"Normal");
   }
 Temp_SS = SS;
}

void Set_FDB()
{
  if(!(FDB))
   FDB = Temp_FDB;

  if(FDB == 1)
   {
     WrPortI(GCDR,NULL,0x00);
     strcpy(FDB_Mon,"Off");
   }
  else if(FDB == 2)
   {
     WrPortI(GCDR,NULL,0x05);
     strcpy(FDB_Mon,"On");
   }
 Temp_FDB = FDB;
}

void Set_FDV()
{
 if(!(FDV))
   FDV = Temp_FDV;

 if(FDV == 1)
   {
     WrPortI(GCSR,NULL,0x09);
     strcpy(FDV_Mon,"1");
   }
  else if(FDV == 2)
   {
     WrPortI(GCSR,NULL,0x0D);
     strcpy(FDV_Mon,"2");
   }
  else if(FDV == 3)
   {
     WrPortI(GCSR,NULL,0x19);
     strcpy(FDV_Mon,"4");
   }
  else if(FDV == 4)
   {
     WrPortI(GCSR,NULL,0x1D);
     strcpy(FDV_Mon,"6");
   }
  else if(FDV == 5)
   {
     WrPortI(GCSR,NULL,0x01);
     strcpy(FDV_Mon,"8");
   }
 Temp_FDV = FDV;
}

void Rd_MCM_Clk()
{
  float Clock;

  Clock = (30.0*FDB)/FDV;
  sprintf(MCM_Clk,"%5.2f %s",Clock,"MHz");
}

// **** This function will set RTC to value provided within Init command **** //

void Set_RTC(char far * rtc_str)
{
  _f_strcpy(RTC_time,rtc_str);

  printf("\nRTC is set to : %s",RTC_time);

  // Setting RTC is hard-coded, follows timestamp formate : yyyy-mm-dd hh:mm:ss//

  rtc_str = RTC_time;

  RTC_time[4]=0;
  RTC_time[7]=0;
  RTC_time[10]=0;
  RTC_time[13]=0;
  RTC_time[16]=0;

  rtc_str  = rtc_str + 17;
  rtc.tm_sec = _f_atoi(rtc_str);            // storing second //

  rtc_str  = rtc_str - 3;
  rtc.tm_min = _f_atoi(rtc_str);            // storing minute //

  rtc_str  = rtc_str - 3;
  rtc.tm_hour = _f_atoi(rtc_str);           // storing hour //

  rtc_str  = rtc_str - 3;
  rtc.tm_mday =  _f_atoi(rtc_str);          // storing day //

  rtc_str  = rtc_str - 3;
  rtc.tm_mon =  _f_atoi(rtc_str);           // storing month //

  rtc_str  = rtc_str - 5;
  rtc.tm_year =  _f_atoi(rtc_str) - 1900;   // storing year //

  tm_wr(&rtc);											 // Writing to RTC //
  SEC_TIMER = mktime(&rtc);                   // mktime() is required to read current value correctly using tm_read()//
}

void Set_NW()
{

  if(!strcmp(psw,ADMIN_USER))
  {
  strcpy(MY_IP_ADDR, ip);
  strcpy(MY_NET_MASK, mask);
  strcpy(MY_GATEWAY, gateway);

  printf("\nSetting new network configuration\n");

  writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);

  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP,IFS_END);

  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);
  }

}