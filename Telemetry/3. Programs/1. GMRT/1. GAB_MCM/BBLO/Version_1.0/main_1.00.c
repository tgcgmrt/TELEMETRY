// **************************************************************************** //
// *******                    Program : BBLO_MCM                        ******** //
// *******                    Date : 02/03/2016                        ******** //
// *******                     Version : V1.00                         ******** //
// *******               Client for Existing Online Server             ******** //
// *******        HTTP Server & client for Remote Firmware Update.     ******** //
// *******     Remote Firmware Update (v1.0 & v2.0) using USER BLOCK   ******** //
// *******           Added Authentication for Control webpage          ******** //
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
	SSPEC_RESOURCE_XMEMFILE("/set/MCM_Set.zhtml",MCM_Set_zhtml),
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
	printf("\nConnection closed for port-%d",port);

   return 0;
}


// ..... Main start .... //
main()
{
  auto unsigned long scn_dl;

  fwinit();
  brdInit();
  SPIinit();
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();

  tcp_reserveport(80);

  printf("\n\nsizeof(parseCMSCmd) : %d",sizeof(parseCMSCmd));
  printf("\nsizeof(devResponse) : %d",sizeof(devResponse));

  while(1)
  {
    http_handler();
    costate    // This will open client socket at port 3000 //
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
  unsigned long lo_frq1, lo_frq2, lor_frq1, lor_frq2;
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

  /*else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
  {
    lo_frq1 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lo_frq2 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set LO : %ld KHz %ld KHz Received..................",lo_frq1, lo_frq2);
    Set_LO_Ptrn(lo_frq1,0);
    Set_LO_Ptrn(lo_frq2,1);
  }   */

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_103))
  {
    _f_strcpy(dmask, Wrpr_Cmd.cmd_data.prmvalue[0]);
    printf("\nReceived Dmask : %s",dmask);
    strcpy(lsw,dmask+4);
    dmask[4] = 0;
    strcpy(msw,dmask);
    Set_DMask();
  }

  /*else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_104))
  {
    IF_Attn1 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
    IF_Attn2 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set IF Attn : %ddB %ddB ..................", IF_Attn1, IF_Attn2);
    Set_Attn(IF_Attn2, IF_Attn1);
  }

   else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_105))
  {
    lor_frq1 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lor_frq2 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set LOR : %ld KHz %ld KHz Received..................",lor_frq1, lor_frq2);
    Set_LOR_Ptrn(lor_frq1,0);
    Set_LOR_Ptrn(lor_frq2,1);
  }  */

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

// **** After initialising port bits in PortInit(), 32 bit digital mask will be set here **** //
void Set_DMask()
{
  int i, MSB_2, LSB_2, MSB_1, LSB_1;
  int MSB_22, LSB_22, MSB_11, LSB_11;

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
  float volt, temp;

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

  if(mswT[1]=='0')
  {
  strcpy(synth1,"(Synth2)");
  strcpy(synth2,"(Synth2)");
  }

  if(mswT[1]=='F')
  {
  strcpy(synth1,"(Synth1)");
  strcpy(synth2,"(Synth1)");
  }

  if(mswT[1]=='C')
  {
  strcpy(synth1,"(Synth1)");
  strcpy(synth2,"(Synth2)");
  }

  if(mswT[1]=='3')
  {
  strcpy(synth1,"(Synth2)");
  strcpy(synth2,"(Synth1)");
  }

  temp = ((((980 - rw[3])/196.0) * 1.035)/5)*100;
  sprintf(bb_temp1, "%5.2f", temp);

  temp = ((((980 - rw[7])/196.0) * 1.035)/5)*100;
  sprintf(bb_temp2, "%5.2f", temp);

  if(rw[0]<=800)
  strcpy(lock_indicator1,"Lock");
  else
  strcpy(lock_indicator1,"Unlock");

  if(rw[1]<=1100)
  strcpy(rf_power1,"No Power");
  else
  strcpy(rf_power1,"Ok");

  if(rw[2]>=450)
  strcpy(dc_power1,"No Power");
  else
  strcpy(dc_power1,"Ok");

  if(rw[4]<=800)
  strcpy(lock_indicator2,"Lock");
  else
  strcpy(lock_indicator2,"Unlock");

  if(rw[5]<=1100)
  strcpy(rf_power2,"No Power");
  else
  strcpy(rf_power2,"Ok");

  if(rw[6]>=450)
  strcpy(dc_power2,"No Power");
  else
  strcpy(dc_power2,"Ok");

  sprintf(BBLO1_Mon, "%c%c.%s", mswT[2]+5, mswT[3], lswT);
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