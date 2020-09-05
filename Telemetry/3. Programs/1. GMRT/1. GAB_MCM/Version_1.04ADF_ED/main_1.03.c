// **************************************************************************** //
// *******                    Program : GAB_MCM                        ******** //
// *******                    Date : 13/08/2018                        ******** //
// *******                     Version : V1.04                        ******** //
// *******               Client for Existing Online Server             ******** //
// *******        HTTP Server & client for Remote Firmware Update.     ******** //
// *******     Remote Firmware Update (v1.0 & v2.0) using USER BLOCK   ******** //
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
#use      "spi.lib"

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
	SSPEC_RESOURCE_XMEMFILE("/MCM_Set.zhtml",MCM_Set_zhtml),
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END

static tcp_Socket Socket;

//.... Generating milisecond delay .... //

void Ms_delay(unsigned int delay)
{
	auto unsigned long delay_time;

	delay_time = MS_TIMER + delay;
   while((long)(MS_TIMER - delay_time) < 0 );
}

int atox(char *mask)
{
  int hex_value;
  char msk[4];

  strcpy(msk, mask);

  printf("%s ",msk);

  if((msk[0] < 58) && (msk[0] > 47))
   msk[0] = msk[0] - '0';
  else if((msk[0] > 64) && (msk[0] < 71))
   msk[0] = msk[0] - 'A' + 10;
  else if((msk[0] > 96) && (msk[0] < 103))
   msk[0] = msk[0] - 'a' + 10;
  else
   msk[0] = 0;

  if((msk[1] < 58) && (msk[1] > 47))
   msk[1] = msk[1] - '0';
  else if((msk[1] > 64) && (msk[1] < 71 ))
   msk[1] = msk[1] - 'A' + 10;
  else if((msk[1] > 96) && (msk[1] < 103))
   msk[1] = msk[1] - 'a' + 10;
  else
   msk[1] = 0;

  hex_value = (((msk[0]) * 16) + (msk[1]));

  return hex_value;
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
   sock_abort(&Socket);
	printf("\nConnection closed for port-%d",port);

   return 0;
}


// ..... Main start .... //
main()
{
  auto unsigned long scn_dl;
  int k;

  fwinit();
  brdInit();
  SPIinit();
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();

  tcp_reserveport(80);

  BitWrPortI(PEDR, &PEDRShadow, 1, 0);    // set LE
  for(k=0;k<10;k++);
  BitWrPortI(PEDR, &PEDRShadow, 0, 0);    // clear LE

  printf("\n\nsizeof(parseCMSCmd) : %d",sizeof(parseCMSCmd));
  printf("\nsizeof(devResponse) : %d",sizeof(devResponse));

   BitWrPortI(PADR, &PADRShadow, 1, 0); // Useful for setting LO in sigcon //
   BitWrPortI(PADR, &PADRShadow, 0, 2);
   BitWrPortI(PADR, &PADRShadow, 0, 4);
   if_vccp = 12; if_vccn = 12; if_vdd = 5;
   lo_vccp = 12; lo_vccn = 12; lo_vdd = 5;

  while(1)
  {
    http_handler();
    costate co_m_clnt always_on   // This will open client socket at port 3000 //
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
   char cmdstr1[64];

   printf("\nEntering cmd_validation");

      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_1))
       {
         printf("\nSubsystem validated");
         _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
         fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_101))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_103))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_104))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_105))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_106))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_107))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_108))
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

  return MCM_Resp.code;
}

// ******** Forming final response ******** //

void form_final_resp(int code, int event)
{
  //printf("\nEntering form_final_resp");

  MCM_Resp.code = code;
  MCM_Resp.event = command_process_gab();

}

int command_process_gab()
{
  double lo_frq1, lo_frq2, lor_frq1, lor_frq2;
  int IF_Attn1, IF_Attn2;
  int rch = 0;

  printf("\nEntering Command Process");

  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_101))        // do monitoring. //
  {
    MCM_Resp.resp_data.numpkt = 68;

      sec_tm = read_rtc();
      mktm(&rtc, sec_tm);
      sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

    _f_strcpy(MCM_Resp.resp_data.prmname[0],"time");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[0],time_ar);      // This value should be filled in bg_mon function. //
    _f_strcpy(MCM_Resp.resp_data.prmname[1],"dmask");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[1],mswT);
    _f_strcat(MCM_Resp.resp_data.prmvalue[1],lswT);
    _f_strcpy(MCM_Resp.resp_data.prmname[2],"lo1");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[2],LO1_Mon);
    _f_strcpy(MCM_Resp.resp_data.prmname[3],"lo2");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[3],LO2_Mon);

    for (rch=0; rch < 64; rch++)
    {
      sprintf(MCM_Resp.resp_data.prmname[rch+4],"Ch%.2d",rch);
      sprintf(MCM_Resp.resp_data.prmvalue[rch+4],"%d",rw[rch]);
    }

   // Sig_decode();

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
  {
    lo_frq1 = _f_atof(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lo_frq2 = _f_atof(Wrpr_Cmd.cmd_data.prmvalue[1]);
	printf("\nCommand for Set LO : %lf KHz %lf KHz Received..................",lo_frq1, lo_frq2);
   // printf("\nCommand for Set LO : %f KHz %f KHz Received..................",lo_frq1, lo_frq2);
    //Set_LO_Ptrn(lo_frq1,0);
    //Set_LO_Ptrn(lo_frq2,1);
    LO1= lo_frq1;
    LO2= lo_frq2;
    Set_LO();
    //Set_LO(lo_frq2);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_103))
  {
    _f_strcpy(dmask, Wrpr_Cmd.cmd_data.prmvalue[0]);
    printf("\nReceived Dmask : %s",dmask);
    strcpy(lsw,dmask+4);
    dmask[4] = 0;
    strcpy(msw,dmask);
    Set_DMask();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_104))
  {
    IF_Attn1 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
    IF_Attn2 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set IF Attn : %ddB %ddB ..................", IF_Attn1, IF_Attn2);
    Set_Attn(IF_Attn2, IF_Attn1);
  }

   else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_105))
  {
    lor_frq1 = _f_atof(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lor_frq2 = _f_atof(Wrpr_Cmd.cmd_data.prmvalue[1]);
    CHsp1 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[2]);
    CHsp2 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[3]);
    printf("\nCommand for Set LOR : %lf KHz %lf KHz Received..................",lor_frq1, lor_frq2);

    LOR1= lor_frq1;
    LOR2= lor_frq2;
    Set_LO();

    //Set_LOR_Ptrn(lor_frq1,0);
    //Set_LOR_Ptrn(lor_frq2,1);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_106))
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_107))
  {
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_108))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
    printf("\n RTC set to: ,%s", Wrpr_Cmd.cmd_elem.timestamp);
  }

  else
  {
    printf("Nothing Matched, Received : %s",Wrpr_Cmd.cmd_elem.cmd_name);
  }

  return 12;
}
// New ADF Code
unsigned int GCD(unsigned int a, unsigned int b)
{
if (a == 0)
return b;
if (b == 0)
return a;
if (a > b)
return GCD(a%b, b);
else
return GCD(a, b%a);
}

void Set_LO_pattern(int Ch)
{
double RFout, REFin, ChannelSpacing, PFDFreq, N, R=10, D=0, T=0, fracN=0.0;
unsigned int outputdivider_index = 0, j, k, INT, MOD, FRAC, gcd;
unsigned int bscm=0, abp=0, chargecancel=0, prescalar=0;
int i;
unsigned long Reg[6], Reg1[6];

bscm=1;

if(Ch==0)
{
RFout = LO1;
tLO1 = (double)LO1;
//tLO1 = LO1;
//tLO1 = (float)LO1;
REFin = tLOR1 = LOR1;
ChannelSpacing = tCHsp1 = CHsp1;
}
else
{
RFout = LO2;
tLO2 = (double)LO2;
//tLO2 = LO2;
//tLO1 = (float)LO1;
REFin = tLOR2 = LOR2;
ChannelSpacing = tCHsp2 = CHsp2;
}

//if(REFin<15)
//R=10;

  if (RFout < 2200)
     outputdivider_index = 1;
  if (RFout < 1100)
     outputdivider_index = 2;
  if (RFout < 550)
     outputdivider_index = 3;
  if (RFout < 275)
     outputdivider_index = 4;
  if (RFout < 137.5)
     outputdivider_index = 5;
  if (RFout < 68.75)
     outputdivider_index = 6;


  if (RFout > 3600)
  	  prescalar=1;

  PFDFreq = REFin*((1+D)/(R*(1+T)));
  //N = ((RFout * pow(2, outputdivider_index)) / PFDFreq);
  N = (RFout);
  INT = (unsigned int)N;
  fracN = N-INT;

  //MOD  = (unsigned int)floor((REFin/(ChannelSpacing*2))*1000);
  MOD  = (unsigned int)floor((PFDFreq / ChannelSpacing)*1000);
  FRAC = (unsigned int)ceil((N - INT) * MOD);
  gcd  = GCD((unsigned int)MOD, (unsigned int)FRAC);
  MOD  = (MOD / gcd);
  FRAC = (FRAC / gcd);

 printf("INT = %d, FRAC = %d, MOD = %d, PFDFreq = %5.3f, N = %5.3f, fracN=%5.3f\n", INT, FRAC, MOD, PFDFreq, N, fracN);
  //printf("INT = %d, FRAC = %d, MOD = %d, PFDFreq = %6.2f, N = %6.2f, fracN=%6.2f\n", INT, FRAC, MOD, PFDFreq, N, fracN);

  if(fracN==0.0)
  {
  abp=1;
  chargecancel=1;
  }

  Reg[0] = ((0xFFFFFFFF & INT)<<15) | ((0xFFF & FRAC)<<3);
  Reg[1] = (0x002D0001 | ((0xFFF & MOD)<<3)) | ((0xFFFFFFFF & prescalar)<<27);
  if(REFin<15)
  Reg[2] = 0x7C028E42;
  else
  Reg[2] = 0x7C1A4E42;
  Reg[3] = (((0x000107F3 | ((0xFFFFFFFF & bscm)<<23)) | ((0xFFFFFFFF & abp)<<22)) | ((0xFFFFFFFF & chargecancel)<<21));
  Reg[4] = (0x00040124 | ((0xFFFFFFFF & (unsigned int)outputdivider_index)<<20));
  Reg[5] = 0x00580005;

  for(i=5;i>=0;i--)
  {
  Reg1[i]=Reg[i];
  Reg1[i]=Reg1[i]&0xFFFF0000;
  Reg1[i]=Reg1[i]>>16;
  printf("Reg[%d] = %04X ", i, Reg1[i]);
  printf("%04X \n", Reg[i]);
  }

  for(i=5;i>=0;i--)
  {
	for(j=0;j<32;j++)
	{
	if((Reg[i]&0x80000000)==0x80000000)
	BitWrPortI(PCDR, &PCDRShadow, 1, 0);     // set DATA
	else
	BitWrPortI(PCDR, &PCDRShadow, 0, 0);     // clear DATA
	BitWrPortI(PEDR, &PEDRShadow, 1, 3);     // set CLOCK
	Reg[i]=Reg[i]<<1;
   for(k=0;k<300;k++);
	BitWrPortI(PEDR, &PEDRShadow, 0, 3);     // clear CLOCK
   for(k=0;k<302;k++);
	}

  if(Ch==0)
  BitWrPortI(PEDR, &PEDRShadow, 1, 0);        //set LE
  else
  BitWrPortI(PEDR, &PEDRShadow, 1, 1);        //set LE
  for(k=0;k<300;k++);
  if(Ch==0)
  BitWrPortI(PEDR, &PEDRShadow, 0, 0);        //clear LE
  else
  BitWrPortI(PEDR, &PEDRShadow, 0, 1);        //clear LE
  printf("\n LO1,LO2 OK \n");
  }

}

void Set_LO()
{
printf("\nLO1  = %7.3f \t LO2  = %7.3f \n", LO1, LO2);
printf("LOR1 = %5.3f \t LOR2 = %5.3f \n", LOR1, LOR2);
printf("LOS1 = %5.3f \t LOS2 = %5.3f \n", CHsp1, CHsp2);

sprintf(LO1_Mon,"%7.3f MHz", LO1);
sprintf(LO2_Mon,"%7.3f MHz", LO2);
sprintf(LOR1_Mon,"%5.3f MHz", LOR1);
sprintf(LOR2_Mon,"%5.3f MHz", LOR2);
sprintf(CHsp1_Mon,"%5.3f KHz", CHsp1);
sprintf(CHsp2_Mon,"%5.3f KHz", CHsp2);

if(LO1!=0 && LOR1!=0 && CHsp1!=0)
Set_LO_pattern(0);
if(LO2!=0 && LOR2!=0 && CHsp2!=0)
Set_LO_pattern(1);
//LO1 = LO2 =0; LOR1 = LOR2 = CHsp1 = CHsp2 = 0;
}
void Set_LORef3()
{
 int i;

 unsigned long Reg[6];
 printf("\n LO3 = %7.3f \n",LO3);
 printf("\n LOR3 = %5.3f \n",LOR3);
 printf("\n LOS3 = %5.3f \n",CHsp);

 if(LO3!=0 && LOR3!=0 && CHsp!=0)
 //Set_LO_pattern1(2);
  /*
  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  WrPortI(PADR, &PADRShadow, (MSB_2 & 0xDF));
  BitWrPortI(PBDR, &PBDRShadow, 1, 5);
  BitWrPortI(PBDR, &PBDRShadow, 0, 6);   */

  Set_LO_pattern1(2);
 //SPIWrite(Reg[i],10);
  /*
  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  WrPortI(PADR, &PADRShadow, (MSB_2 | 0x20));
  BitWrPortI(PBDR, &PBDRShadow, 1, 5);
  BitWrPortI(PBDR, &PBDRShadow, 0, 6);
  printf("\n LO3 OK \n");  */
}
void Set_LO_pattern1(int Ch)
{
double RFout, REFin, ChannelSpacing, PFDFreq, N, R=10, D=0, T=0, fracN=0.0;
unsigned int outputdivider_index = 0, j, k, INT, MOD, FRAC, gcd;
unsigned int bscm=0, abp=0, chargecancel=0, prescalar=0;
int i;
unsigned long Reg[6], Reg1[6];

bscm=1;

if(Ch==2)
{
RFout = LO3;
tLO1 = (double)LO3;
//tLO1 = LO1;
//tLO1 = (float)LO1;
REFin = tLOR3 = LOR3;
ChannelSpacing = tCHsp = CHsp;
}
else
{
RFout = LO2;
tLO2 = (double)LO2;
//tLO2 = LO2;
//tLO1 = (float)LO1;
REFin = tLOR2 = LOR2;
ChannelSpacing = tCHsp2 = CHsp2;
}

//if(REFin<15)
//R=10;

  if (RFout < 2200)
     outputdivider_index = 1;
  if (RFout < 1100)
     outputdivider_index = 2;
  if (RFout < 550)
     outputdivider_index = 3;
  if (RFout < 275)
     outputdivider_index = 4;
  if (RFout < 137.5)
     outputdivider_index = 5;
  if (RFout < 68.75)
     outputdivider_index = 6;


  if (RFout > 3600)
  	  prescalar=1;

  PFDFreq = REFin*((1+D)/(R*(1+T)));
  //N = ((RFout * pow(2, outputdivider_index)) / PFDFreq);
  N = (RFout);
  INT = (unsigned int)N;
  fracN = N-INT;

  //MOD  = (unsigned int)floor((REFin/(ChannelSpacing*2))*1000);
  MOD  = (unsigned int)floor((PFDFreq / ChannelSpacing)*1000);
  FRAC = (unsigned int)ceil((N - INT) * MOD);
  gcd  = GCD((unsigned int)MOD, (unsigned int)FRAC);
  MOD  = (MOD / gcd);
  FRAC = (FRAC / gcd);

 printf("INT = %d, FRAC = %d, MOD = %d, PFDFreq = %5.3f, N = %5.3f, fracN=%5.3f\n", INT, FRAC, MOD, PFDFreq, N, fracN);
  //printf("INT = %d, FRAC = %d, MOD = %d, PFDFreq = %6.2f, N = %6.2f, fracN=%6.2f\n", INT, FRAC, MOD, PFDFreq, N, fracN);

  if(fracN==0.0)
  {
  abp=1;
  chargecancel=1;
  }

  Reg[0] = ((0xFFFFFFFF & INT)<<15) | ((0xFFF & FRAC)<<3);
  Reg[1] = (0x002D0001 | ((0xFFF & MOD)<<3)) | ((0xFFFFFFFF & prescalar)<<27);
  if(REFin<15)
  Reg[2] = 0x7C028E42;
  else
  Reg[2] = 0x7C1A4E42;
  Reg[3] = (((0x000107F3 | ((0xFFFFFFFF & bscm)<<23)) | ((0xFFFFFFFF & abp)<<22)) | ((0xFFFFFFFF & chargecancel)<<21));
  Reg[4] = (0x00040124 | ((0xFFFFFFFF & (unsigned int)outputdivider_index)<<20));
  Reg[5] = 0x00580005;

  for(i=5;i>=0;i--)
  {
  Reg1[i]=Reg[i];
  Reg1[i]=Reg1[i]&0xFFFF0000;
  Reg1[i]=Reg1[i]>>16;
  printf("Reg[%d] = %04X ", i, Reg1[i]);
  printf("%04X \n", Reg[i]);
  }

  for(i=5;i>=0;i--)
  {
	for(j=0;j<32;j++)
	{
	if((Reg[i]&0x80000000)==0x80000000)
	BitWrPortI(PCDR, &PCDRShadow, 1, 0);     // set DATA
	else
	BitWrPortI(PCDR, &PCDRShadow, 0, 0);     // clear DATA
	BitWrPortI(PEDR, &PEDRShadow, 1, 3);     // set CLOCK
	Reg[i]=Reg[i]<<1;
   for(k=0;k<300;k++);
	BitWrPortI(PEDR, &PEDRShadow, 0, 3);     // clear CLOCK
   for(k=0;k<302;k++);
	}

  if(Ch==2)
  BitWrPortI(PEDR, &PEDRShadow, 0, 0);        //set LE
  else
  BitWrPortI(PEDR, &PEDRShadow, 0, 0);        //set LE

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  WrPortI(PADR, &PADRShadow, (MSB_2 & 0xDF));
  BitWrPortI(PBDR, &PBDRShadow, 1, 5);
  BitWrPortI(PBDR, &PBDRShadow, 0, 6);

  for(k=0;k<1000;k++);

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  WrPortI(PADR, &PADRShadow, (MSB_2 | 0x20));
  BitWrPortI(PBDR, &PBDRShadow, 1, 5);
  BitWrPortI(PBDR, &PBDRShadow, 0, 6);
  printf("\n LO3 OK \n");

  }

}

// **** After initialising port bits in PortInit(), 32 bit digital mask will be set here **** //
void Set_DMask()
{
  int i, MSB_2, LSB_2, MSB_1, LSB_1;
  int MSB_22, LSB_22, MSB_11, LSB_11;
  float db1=00.0, db2=00.0;

  filter1=1;
  filter2=1;
  strcpy(mswT, msw);
  strcpy(lswT, lsw);

  for(i=0;i<sizeof(mswT);i++)
   mswT[i] = toupper(mswT[i]);
  for(i=0;i<sizeof(lswT);i++)
   lswT[i] = toupper(lswT[i]);

  LSB_2 = atox(msw + 2);
  msw[2]=0;
  MSB_2 = atox(msw);    // don't need any shift operation, because atox is done only for two poiter locations //

  LSB_1 = atox(lsw + 2);
  lsw[2]=0;
  MSB_1 = atox(lsw);

   // For web-page monitoring by RB 7/4/2015
  LSB_11=LSB_1;
  LSB_22=LSB_2;
  MSB_11=MSB_1;
  MSB_22=MSB_2;
  if((LSB_11 & 0x01) == 0x00)
  db1=db1+0.5;
  if((LSB_11 & 0x02) == 0x00)
  db1=db1+1;
  if((LSB_11 & 0x04) == 0x00)
  db1=db1+2;
  if((LSB_11 & 0x08) == 0x00)
  db1=db1+4;
  if((LSB_11 & 0x10) == 0x00)
  db1=db1+8;
  if((LSB_11 & 0x20) == 0x00)
  db1=db1+16;

  sprintf(Attn1_Mon,"%5.1f dB", db1);

  if((MSB_11 & 0x80) == 0x00)
  db2=db2+0.5;
  if((LSB_22 & 0x01) == 0x00)
  db2=db2+1;
  if((LSB_22 & 0x02) == 0x00)
  db2=db2+2;
  if((LSB_22 & 0x04) == 0x00)
  db2=db2+4;
  if((LSB_22 & 0x08) == 0x00)
  db2=db2+8;
  if((LSB_22 & 0x10) == 0x00)
  db2=db2+16;

  sprintf(Attn2_Mon,"%5.1f dB", db2);

  if((MSB_11 & 0x02) == 0x02)
  filter1=filter1+1;
  if((MSB_11 & 0x04) == 0x04)
  filter1=filter1+2;
  if((MSB_11 & 0x08) == 0x08)
  filter1=filter1+4;

  if((MSB_22 & 0x01) == 0x01)
  filter2=filter2+1;
  if((MSB_22 & 0x02) == 0x02)
  filter2=filter2+2;
  if((MSB_22 & 0x04) == 0x04)
  filter2=filter2+4;

  if((MSB_11 & 0x30) == 0x00)
  strcpy(lpf1,"100 MHZ");
  if((MSB_11 & 0x30) == 0x10)
  strcpy(lpf1,"200 MHZ");
  if((MSB_11 & 0x30) == 0x20)
  strcpy(lpf1,"400 MHZ");
  if((MSB_11 & 0x30) == 0x30)
  strcpy(lpf1,"Direct Path");

  if((MSB_22 & 0x18) == 0x00)
  strcpy(lpf2,"100 MHZ");
  if((MSB_22 & 0x18) == 0x08)
  strcpy(lpf2,"200 MHZ");
  if((MSB_22 & 0x18) == 0x10)
  strcpy(lpf2,"400 MHZ");
  if((MSB_22 & 0x18) == 0x18)
  strcpy(lpf2,"Direct Path");

  if((MSB_11 & 0x01) == 0x01)
  strcpy(path1,"Direct");
  else
  strcpy(path1,"Mixer");

  if((LSB_11 & 0xC0) == 0x00)
  strcpy(in1,"RF");
  if((LSB_11 & 0xC0) == 0x40)
  strcpy(in1,"VGA");
  if((LSB_11 & 0xC0) == 0x80)
  strcpy(in1,"Buffer");

  if((LSB_22 & 0x80) == 0x80)
  strcpy(path2,"Direct");
  else
  strcpy(path2,"Mixer");

  if((LSB_22 & 0x60) == 0x00)
  strcpy(in2,"RF");
  if((LSB_22 & 0x60) == 0x20)
  strcpy(in2,"VGA");
  if((LSB_22 & 0x60) == 0x40)
  strcpy(in2,"Buffer");

  if((MSB_22 & 0x80) == 0x80)
  strcpy(source2,"Signal Generator");
  else
  strcpy(source2,"Synthesizer");
  if((MSB_22 & 0x40) == 0x40)
  strcpy(source1,"Signal Generator");
  else
  strcpy(source1,"Synthesizer");
  // Init Before setting the bits //
  // RB modification for webpage over

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 2);
  BitWrPortI(PBDR, &PBDRShadow, 0, 3);
  BitWrPortI(PBDR, &PBDRShadow, 0, 4);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);

  printf("\nMSB_2 LSB_2 MSB_1 LSB_1 : %x %x %x %x\n", MSB_2, LSB_2, MSB_1, LSB_1);
  // Set Most Significant Word and Least Significant Word on 32 Bit digital o/ps //

  WrPortI(PADR, &PADRShadow, LSB_1);
  BitWrPortI(PBDR, &PBDRShadow, 1, 2); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, LSB_2);
  BitWrPortI(PBDR, &PBDRShadow, 1, 4); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, MSB_1);
  BitWrPortI(PBDR, &PBDRShadow, 1, 3); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, MSB_2);
  BitWrPortI(PBDR, &PBDRShadow, 1, 5); // Latching value at LSB latch //

  BitWrPortI(PBDR, &PBDRShadow, 0, 6); // Enable output of both latches at same time //

}

void Set_Attn(int ch2, int ch1)
{
 char Attn_bits;

 Attn_bits = 0;

 sprintf(Attn1_Mon, "%d dB", ch1);
 sprintf(Attn2_Mon, "%d dB", ch2);

 Attn_bits = ch1 & 0x0f;

 ch2 = ch2 << 4;

 Attn_bits = (Attn_bits | (ch2 & 0xf0));

 printf("\nAttn_bits : 0x%x",Attn_bits);
 WrPortI(PADR, &PADRShadow, Attn_bits);

 BitWrPortI(PBDR, &PBDRShadow, 1, 2); // Latching value at LSB latch //
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 BitWrPortI(PBDR, &PBDRShadow, 1, 2);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);

 BitWrPortI(PDDR, &PDDRShadow, 0, 5); // Enabling output of both Latch //
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

/* // For Analog backend lab-test only.
  if(rw[28]>450)
  {
  l=1;
  }
  if(rw[28]<450 && l==1)
  {
  l=0;
  ref_lo1= 105;
  ref_lo2= 105;
  Ms_delay(100);
  Set_LORef();
  ref_lo1= 105;
  ref_lo2= 105;
  Ms_delay(100);
  Set_LORef();
  }
*/
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
  sock_close(&Socket);
  printf("\nSetting new network configuration\n");

  writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);

  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP,IFS_END);

  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);
  }
  else
  {
  strcpy(ip, MY_IP_ADDR);
  strcpy(mask, MY_NET_MASK);
  strcpy(gateway, MY_GATEWAY);
  }

}