// **************************************************************************** //
// ************************ Program : GMRT_MCM ********************************* //
// ************************ Date : 10/08/2015 ********************************* //
// ************************  Version : V1.00  ********************************* //
// ****           Contains GAB, OFC, Sentinel, FPS Code                   ***** //
// **** Client for Existing Online Server as well as being used with TCS  ***** //
// ****          HTTP Server & client for Remote Firmware Update.         ***** //
// ****                  Modified by Rahul Bhor.                          ***** //
// **** Remote Firmware Update (v1.0 & v2.0) and some monitoring parameters *** //
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
#ifndef mcm_addr4
   SSPEC_RESOURCE_XMEMFILE("/",MCM_Mon_zhtml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Sum.xml",Mon_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Raw.xml",Mon_Raw_xml),
   #ifdef mcm_addr3
   SSPEC_RESOURCE_XMEMFILE("/Mon_check.xml",Mon_check_xml),
   #endif
	SSPEC_RESOURCE_XMEMFILE("/MCM_Set.zhtml",MCM_Set_zhtml),
#endif
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
  #ifdef mcm_addr1
  SPIinit();
  #elif defined mcm_addr4
  serial_init();
  #endif
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();
  //rd_MCM_addr();

  tcp_reserveport(80);

  printf("\n\nsizeof(parseCMSCmd) : %d",sizeof(parseCMSCmd));
  printf("\nsizeof(devResponse) : %d",sizeof(devResponse));

  #ifdef mcm_addr1
   BitWrPortI(PADR, &PADRShadow, 1, 0); // Useful for setting LO in sigcon //
   BitWrPortI(PADR, &PADRShadow, 0, 2);
   BitWrPortI(PADR, &PADRShadow, 0, 4);
   if_vccp = 12; if_vccn = 12; if_vdd = 5;
   lo_vccp = 12; lo_vccn = 12; lo_vdd = 5;
  #endif

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
       #ifdef mcm_addr3
       //ofc_alarm_handler();
       ofc_mon();
       sen_mon();
       #endif
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

      if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_0)))
       {
         printf("\nSubsystem validated");
         _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
         fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_001))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_002))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_003))
          {
           return 10;

           for(j=0; j<Wrpr_Cmd.cmd_data.numpkt; j++) // Looking for Jth parameter //
            {
             for(i=0; i<SMP_rawsize; i++) // comparing Jth against Ith parameter //
              {
               if(!_f_strcmp(Wrpr_Cmd.cmd_data.prmname[j],SenMon_Para[i]))
                {
                 cmpl++;
                 break;
                }
              }
            }// Checking of all parameters over here //

            if(cmpl == Wrpr_Cmd.cmd_data.numpkt)
             return 10;
            else
             return 13;   // Incase of Incomplete command //
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_004))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_005))
          {
           return 10;
          }
         else
          {
           return 12;     // Incase of Syntax Error in command //
          }
       }
      //else
      // {
      //  printf("\nCommand Invalid");
      //  return 14;     // Incase of Invalid command //
      // }
   //}

   #ifdef mcm_addr1
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
   #endif

   #ifdef mcm_addr2
      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_2))
       {
        return 10;
       }
      else
       {
        return 14;
       }
   #endif

   #ifdef mcm_addr3

      if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3)))
       {
         printf("\nSubsystem validated");
         _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
         fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_301))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_302))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_303))
          {
           return 10;

           for(j=0; j<Wrpr_Cmd.cmd_data.numpkt; j++) // Looking for Jth parameter //
            {
             for(i=0; i<SMP_rawsize; i++) // comparing Jth against Ith parameter //
              {
               if(!_f_strcmp(Wrpr_Cmd.cmd_data.prmname[j],SenMon_Para[i]))
                {
                 cmpl++;
                 break;
                }
              }
            }// Checking of all parameters over here //

            if(cmpl == Wrpr_Cmd.cmd_data.numpkt)
             return 10;
            else
             return 13;   // Incase of Incomplete command //
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_304))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_305))
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
        return 14;        // Incase of Invalid command //
       }
  #endif

  #ifdef mcm_addr4

      if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_4)))
       {
         printf("\nSubsystem validated");
         _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
         fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_401))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_402))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_403))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_404))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_405))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_406))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_407))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_408))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_409))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_410))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_411))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_412))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_413))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_414))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_415))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_416))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_417))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_418))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_419))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_420))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_421))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_422))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_423))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_424))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_425))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_426))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_427))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_428))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_429))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_430))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_431))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_432))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_433))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_434))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_435))
          {
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_436))
          {
           return 10;

           for(j=0; j<Wrpr_Cmd.cmd_data.numpkt; j++) // Looking for Jth parameter //
            {
             for(i=0; i<SMP_rawsize; i++) // comparing Jth against Ith parameter //
              {
               if(!_f_strcmp(Wrpr_Cmd.cmd_data.prmname[j],SenMon_Para[i]))
                {
                 cmpl++;
                 break;
                }
              }
            }// Checking of all parameters over here //

            if(cmpl == Wrpr_Cmd.cmd_data.numpkt)
             return 10;
            else
             return 13;   // Incase of Incomplete command //
          }
          else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_437))
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
        return 14;        // Incase of Invalid command //
       }
  #endif

  #ifndef mcm_addr0
  #ifndef mcm_addr1
  #ifndef mcm_addr2
  #ifndef mcm_addr3
  #ifndef mcm_addr4
    return 100;     // Incase of Unknown error like, MCM address mismatch or Sending Sentinal command to Sigcon MCM. //
  #endif
  #endif
  #endif
  #endif
  #endif
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

  #ifdef mcm_addr0
  MCM_Resp.event = command_process_sen();
  #endif

  #ifdef mcm_addr1
   MCM_Resp.event = command_process_sig();
  #endif

  #ifdef mcm_addr2
   MCM_Resp.event = command_process_backend();
  #endif

  #ifdef mcm_addr3
  if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3)))
  {
   MCM_Resp.event = command_process_ofc();
  }
  #endif

  #ifdef mcm_addr4
   MCM_Resp.event = command_process_fps();
  #endif

  if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_0)))
  {
  MCM_Resp.event = command_process_sen();
  }
}

// ******** Processing the Sentinal command ******** //

int sen_mon()
{
int cnt=0, k;

    temp = ((((980 - rw[63])/196.0) * 1.035)/5)*100;
    sprintf(temps, "%5.2f", temp);

    for(k=0;k<5;k++)
    {
    if(rw[61]>144)
    {
     cnt++;
     Ms_delay(1);
    }
    }
    if(cnt>4)
    smoke_detector=1;
    else
    smoke_detector=0;

    cnt=0;
    for(k=0;k<5;k++)
    {
    if(rw[60]>144)
    {
     cnt++;
     Ms_delay(1);
    }
    }
    if(cnt>4)
    intruder_detector=1;
    else
    intruder_detector=0;

    if(temp>27 || smoke_detector==1 || intruder_detector==1)
    sprintf(state, "NOT OK");
    else
    sprintf(state, "OK");
}

int command_process_sen()
{
  printf("\nEntering command_process_sen");
  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_001))      // DoMon will monitor Temperature, RTC time, Status, Raw Values etc..//
  {
     MCM_Resp.resp_data.numpkt = 11;

     sec_tm = read_rtc();
     mktm(&rtc, sec_tm);
     sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

     _f_strcpy(MCM_Resp.resp_data.prmname[0], "time");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[0], time_ar);

     _f_strcpy(MCM_Resp.resp_data.prmname[1], "state");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[1], state);

     _f_strcpy(MCM_Resp.resp_data.prmname[2], "temp");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[2], temps); // This value should be filled in bg_mon function. //

     _f_strcpy(MCM_Resp.resp_data.prmname[3], "chan");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[3], "64");

    _f_strcpy(MCM_Resp.resp_data.prmname[4], "rawcnt");
     sprintf(MCM_Resp.resp_data.prmvalue[4], "%d", rw[63]);  // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[5], "Smoke Detector");
     sprintf(MCM_Resp.resp_data.prmvalue[5], "%d", smoke_detector);

     _f_strcpy(MCM_Resp.resp_data.prmname[6], "chan");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[6], "62");

    _f_strcpy(MCM_Resp.resp_data.prmname[7], "rawcnt");
     sprintf(MCM_Resp.resp_data.prmvalue[7], "%d", rw[61]);

    _f_strcpy(MCM_Resp.resp_data.prmname[8], "Intruder Detector");
     sprintf(MCM_Resp.resp_data.prmvalue[8], "%d", intruder_detector);

     _f_strcpy(MCM_Resp.resp_data.prmname[9], "chan");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[9], "61");

    _f_strcpy(MCM_Resp.resp_data.prmname[10], "rawcnt");
     sprintf(MCM_Resp.resp_data.prmvalue[10], "%d", rw[60]);

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_002))   // setbit will set portbits at 32 bit o/p //
  {
    // Do notihng, as currently nothing to set in Sentinal system !! //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_003))   // Init command will initalise Memory, Portbits, ADC and RTC //
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_004))   // reset
  {
     printf("\n MCM is given Reset : Socket Closed !!");
     sock_close(&Socket);
     exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_005))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
    printf("\n RTC set to: ,%s", Wrpr_Cmd.cmd_elem.timestamp);
  }

  return 12;     // This return value will be stored in MCM_Resp.event //
}

#ifdef mcm_addr1
// ******** Processing the SIGCON command ******** //

int command_process_sig()
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
  {
    lo_frq1 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lo_frq2 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set LO : %ld KHz %ld KHz Received..................",lo_frq1, lo_frq2);
    Set_LO_Ptrn(lo_frq1,0);
    Set_LO_Ptrn(lo_frq2,1);
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
    lor_frq1 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[0]);
    lor_frq2 = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set LOR : %ld KHz %ld KHz Received..................",lor_frq1, lor_frq2);
    Set_LOR_Ptrn(lor_frq1,0);
    Set_LOR_Ptrn(lor_frq2,1);
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

void Set_LO_Ptrn(unsigned long lo, int Ch)
{
 char LO_Arr[10], rem[10], Temp_Arr[10];
 int padding, cntr_1, i;

 unsigned long lo_frq;
 lo_frq = lo/1000;

 if(Ch==0)
   sprintf(LO1_Mon,"%ld %s",lo_frq,"MHz");
 else if(Ch==1)
   sprintf(LO2_Mon,"%ld %s",lo_frq,"MHz");

 for(cntr_1=0; cntr_1<9; cntr_1++)
	LO_Arr[cntr_1] = '0';

 printf("\nunsigned lo = %ld",lo);

 i=0;
  do
  {
   rem[i] = (lo%16)+48;
   if(rem[i]>57)
   rem[i] = rem[i]+7;
   lo = lo/16;
   i++;
  }while(lo>0);
  rem[i]= 0 ;

  for(cntr_1=0; cntr_1 < strlen(rem); cntr_1++)
  {
    Temp_Arr[i-1] = rem[cntr_1];
    i--;
  }
  Temp_Arr[cntr_1]= 0;

  printf("\nAfter converting to hex : %s",Temp_Arr);

  padding = 9 - strlen(Temp_Arr);

  strcpy(LO_Arr + padding, Temp_Arr);

  LO_Arr[0] = 'K';


 for(cntr_1=0; cntr_1 < strlen(LO_Arr); cntr_1++)
  {
    if((LO_Arr[cntr_1] > 96) && (LO_Arr[cntr_1] < 123))
    LO_Arr[cntr_1] = LO_Arr[cntr_1] - 32;
  }

 strcpy(Temp_Arr, LO_Arr);

 printf("\nSending over SPI (LO_Arr) : %s\n",LO_Arr);
 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);
 BitWrPortI(PEDR, &PEDRShadow, 0, Ch);

 for(i=0;i<10;i++);
 SPIWrite(LO_Arr,10);
 for(i=0;i<10;i++);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);
}

void Set_LOR_Ptrn(unsigned long lor, int Ch)
{
 char LOR_Arr[6];
 int i;

if(Ch==0)
   sprintf(LOR1_Mon, "%ld %s", lor, "MHz");
 else if(Ch==1)
   sprintf(LOR2_Mon, "%ld %s", lor, "MHz");

 printf("\nunsigned lo = %ld", lor);

    LOR_Arr[0] = 'R';
    LOR_Arr[1] = lor *2;
    LOR_Arr[2] = 0;
    LOR_Arr[3] = 0;
    LOR_Arr[4] = 0;
    LOR_Arr[5] = 0;

 printf("\nAfter converting to hex : %x",LOR_Arr[1]);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);
 BitWrPortI(PEDR, &PEDRShadow, 0, Ch);

 for(i=0;i<50;i++);
 SPIWrite(LOR_Arr,6);
 for(i=0;i<50;i++);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);
}


void Set_LORef()
{
printf("\nref_lo1 : %d", ref_lo1);
printf("\nref_lo2 : %d\n", ref_lo2);

if(ref_lo1>0 && ref_lo1>9 && ref_lo1<121)
Set_LOR_Ptrn(ref_lo1,0);
if(ref_lo2>0 && ref_lo2>9 && ref_lo2<121)
Set_LOR_Ptrn(ref_lo2,1);

ref_lo1=0;
ref_lo2=0;
}

void Set_LO()
{
printf("\nLO1 : %ld", LO1);
printf("\nLO2 : %ld\n", LO2);

if(LO1>0 && LO1>99 && LO1<1701)
Set_LO_Ptrn(LO1*1000,0);
if(LO2>0 && LO2>99 && LO2<1701)
Set_LO_Ptrn(LO2*1000,1);

LO1=0;
LO2=0;
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

  printf("\nMSB_2 LSB_2 MSB_1 LSB_1 : %x %x %x %x", MSB_2, LSB_2, MSB_1, LSB_1);
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

  // Set Most Significant Word and Least Significant Word on 32 Bit digital o/ps //

  WrPortI(PADR, &PADRShadow, LSB_1);
  BitWrPortI(PBDR, &PBDRShadow, 1, 2); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, MSB_1);
  BitWrPortI(PBDR, &PBDRShadow, 1, 3); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, LSB_2);
  BitWrPortI(PBDR, &PBDRShadow, 1, 4); // Latching value at LSB latch //

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

#endif

#ifdef mcm_addr2
// ******** Processing the backend command ******** //
int command_process_backend()
{
return 12;
}
#endif

#ifdef mcm_addr3
// ******** Processing the ofc command ******** //

void ofc_mon()
{
temp = ((((980 - rw[5])/196.0) * 1.035)/5)*100;
sprintf(temp1, "%5.2f", temp);

i1 = ((float)fabs(((((980 - rw[2]) /196.0) * 1.035) - (((980 - rw[3]) /196.0) * 1.035)) / 33)) * 1000;
i2 = ((float)fabs(((((980 - rw[8]) /196.0) * 1.035) - (((980 - rw[9]) /196.0) * 1.035)) / 33)) * 1000;
i3 = ((float)fabs(((((980 - rw[13])/196.0) * 1.035) - (((980 - rw[14])/196.0) * 1.035)) / 33)) * 1000;
i4 = ((float)fabs(((((980 - rw[30])/196.0) * 1.035) - (((980 - rw[31])/196.0) * 1.035)) / 33)) * 1000;

p1 = i1 * 0.16; p2 = i2 * 0.16; p3 = i3 * 0.16; p4 = i4 * 0.16;

if(p1>=1)
p1 = 10 * (float) log10 ((float) p1 );
if(p2>=1)
p2 = 10 * (float) log10 ((float) p2 );
if(p3>=1)
p3 = 10 * (float) log10 ((float) p3 );
if(p4>=1)
p4 = 10 * (float) log10 ((float) p4 );

sprintf(lt1i1,"%5.3f", i1);
sprintf(lt2i2,"%5.3f", i2);
sprintf(lt3i3,"%5.3f", i3);
sprintf(lt4i4,"%5.3f", i4);
sprintf(lt1p1,"%5.3f", p1);
sprintf(lt2p2,"%5.3f", p2);
sprintf(lt3p3,"%5.3f", p3);
sprintf(lt4p4,"%5.3f", p4);
}

int command_process_ofc()
{
int m=0, k, OFC_Attn1, OFC_Attn2;;

  printf("\nEntering command_process_ofc");
  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_301))      // DoMon will monitor Temperature, RTC time, Status, Raw Values etc..//
  {
     MCM_Resp.resp_data.numpkt = 107;

    _f_strcpy(MCM_Resp.resp_data.prmname[0],"time");

      sec_tm = read_rtc();
      mktm(&rtc, sec_tm);
      sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

    _f_strcpy(MCM_Resp.resp_data.prmvalue[0],time_ar);

     temp = ((((980 - rw[5])/196.0) * 1.035)/5)*100;
     sprintf(temp1, "%5.2f", temp);

    _f_strcpy(MCM_Resp.resp_data.prmname[1], "LT1NPS");
     sprintf(MCM_Resp.resp_data.prmvalue[1], "%s", vl[0]);
    _f_strcpy(MCM_Resp.resp_data.prmname[2], "LT1BV1");
     sprintf(MCM_Resp.resp_data.prmvalue[2], "%s", vl[3]);
    _f_strcpy(MCM_Resp.resp_data.prmname[3], "LT1BV2");
     sprintf(MCM_Resp.resp_data.prmvalue[3], "%s", vl[2]);
    _f_strcpy(MCM_Resp.resp_data.prmname[4], "LT1APC");
     sprintf(MCM_Resp.resp_data.prmvalue[4], "%s", vl[4]);
    _f_strcpy(MCM_Resp.resp_data.prmname[5], "LT1TMP");
     sprintf(MCM_Resp.resp_data.prmvalue[5], "%5.2f", temp);
    _f_strcpy(MCM_Resp.resp_data.prmname[6], "LT1AV1");
     sprintf(MCM_Resp.resp_data.prmvalue[6], "%s", vl[1]);

    _f_strcpy(MCM_Resp.resp_data.prmname[7], "LT2NPS");
     sprintf(MCM_Resp.resp_data.prmvalue[7], "%s", vl[6]);
    _f_strcpy(MCM_Resp.resp_data.prmname[8], "LT2BV1");
     sprintf(MCM_Resp.resp_data.prmvalue[8], "%s", vl[9]);
    _f_strcpy(MCM_Resp.resp_data.prmname[9], "LT2BV2");
     sprintf(MCM_Resp.resp_data.prmvalue[9], "%s", vl[8]);
    _f_strcpy(MCM_Resp.resp_data.prmname[10], "LT2APC");
     sprintf(MCM_Resp.resp_data.prmvalue[10], "%s", vl[10]);
    _f_strcpy(MCM_Resp.resp_data.prmname[11], "LT2AV1");
     sprintf(MCM_Resp.resp_data.prmvalue[11], "%s", vl[7]);

    _f_strcpy(MCM_Resp.resp_data.prmname[12], "LT3NPS");
     sprintf(MCM_Resp.resp_data.prmvalue[12], "%s", vl[11]);
    _f_strcpy(MCM_Resp.resp_data.prmname[13], "LT3BV1");
     sprintf(MCM_Resp.resp_data.prmvalue[13], "%s", vl[14]);
    _f_strcpy(MCM_Resp.resp_data.prmname[14], "LT3BV2");
     sprintf(MCM_Resp.resp_data.prmvalue[14], "%s", vl[13]);
    _f_strcpy(MCM_Resp.resp_data.prmname[15], "LT3APC");
     sprintf(MCM_Resp.resp_data.prmvalue[15], "%s", vl[15]);
    _f_strcpy(MCM_Resp.resp_data.prmname[16], "LT3AV1");
     sprintf(MCM_Resp.resp_data.prmvalue[16], "%s", vl[12]);

    _f_strcpy(MCM_Resp.resp_data.prmname[17], "LT4NPS");
     sprintf(MCM_Resp.resp_data.prmvalue[17], "%s", vl[16]);
    _f_strcpy(MCM_Resp.resp_data.prmname[18], "LT4BV1");
     sprintf(MCM_Resp.resp_data.prmvalue[18], "%s", vl[19]);
    _f_strcpy(MCM_Resp.resp_data.prmname[19], "LT4BV2");
     sprintf(MCM_Resp.resp_data.prmvalue[19], "%s", vl[18]);
    _f_strcpy(MCM_Resp.resp_data.prmname[20], "LT4APC");
     sprintf(MCM_Resp.resp_data.prmvalue[20], "%s", vl[20]);
    _f_strcpy(MCM_Resp.resp_data.prmname[21], "LT4AV1");
     sprintf(MCM_Resp.resp_data.prmvalue[21], "%s", vl[17]);

    _f_strcpy(MCM_Resp.resp_data.prmname[22], "LT1CURR");
     sprintf(MCM_Resp.resp_data.prmvalue[22], "%s", lt1i1);
    _f_strcpy(MCM_Resp.resp_data.prmname[23], "LT2CURR");
     sprintf(MCM_Resp.resp_data.prmvalue[23], "%s", lt2i2);
    _f_strcpy(MCM_Resp.resp_data.prmname[24], "LT3CURR");
     sprintf(MCM_Resp.resp_data.prmvalue[24], "%s", lt3i3);
    _f_strcpy(MCM_Resp.resp_data.prmname[25], "LT4CURR");
     sprintf(MCM_Resp.resp_data.prmvalue[25], "%s", lt4i4);

    _f_strcpy(MCM_Resp.resp_data.prmname[26], "LT1POW");
     sprintf(MCM_Resp.resp_data.prmvalue[26], "%s", lt1p1);
    _f_strcpy(MCM_Resp.resp_data.prmname[27], "LT2POW");
     sprintf(MCM_Resp.resp_data.prmvalue[27], "%s", lt2p2);
    _f_strcpy(MCM_Resp.resp_data.prmname[28], "LT3POW");
     sprintf(MCM_Resp.resp_data.prmvalue[28], "%s", lt3p3);
    _f_strcpy(MCM_Resp.resp_data.prmname[29], "LT4POW");
     sprintf(MCM_Resp.resp_data.prmvalue[29], "%s", lt4p4);

    _f_strcpy(MCM_Resp.resp_data.prmname[30], "RFCH1ATB1");
     sprintf(MCM_Resp.resp_data.prmvalue[30], "%s", vl[21]);
    _f_strcpy(MCM_Resp.resp_data.prmname[31], "RFCH1ATB2");
     sprintf(MCM_Resp.resp_data.prmvalue[31], "%s", vl[22]);
    _f_strcpy(MCM_Resp.resp_data.prmname[32], "RFCH1ATB3");
     sprintf(MCM_Resp.resp_data.prmvalue[32], "%s", vl[23]);
    _f_strcpy(MCM_Resp.resp_data.prmname[33], "RFCH1ATB4");
     sprintf(MCM_Resp.resp_data.prmvalue[33], "%s", vl[24]);
    _f_strcpy(MCM_Resp.resp_data.prmname[34], "RFCH1ATB5");
     sprintf(MCM_Resp.resp_data.prmvalue[34], "%s", vl[25]);
    _f_strcpy(MCM_Resp.resp_data.prmname[35], "RFCH1ATB6");
     sprintf(MCM_Resp.resp_data.prmvalue[35], "%s", vl[26]);

    _f_strcpy(MCM_Resp.resp_data.prmname[36], "RFCH2ATB1");
     sprintf(MCM_Resp.resp_data.prmvalue[36], "%s", vl[27]);
    _f_strcpy(MCM_Resp.resp_data.prmname[37], "RFCH2ATB2");
     sprintf(MCM_Resp.resp_data.prmvalue[37], "%s", vl[28]);
    _f_strcpy(MCM_Resp.resp_data.prmname[38], "RFCH2ATB3");
     sprintf(MCM_Resp.resp_data.prmvalue[38], "%s", vl[29]);
    _f_strcpy(MCM_Resp.resp_data.prmname[39], "RFCH2ATB4");
     sprintf(MCM_Resp.resp_data.prmvalue[39], "%s", vl[30]);
    _f_strcpy(MCM_Resp.resp_data.prmname[40], "RFCH2ATB5");
     sprintf(MCM_Resp.resp_data.prmvalue[40], "%s", vl[31]);
    _f_strcpy(MCM_Resp.resp_data.prmname[41], "RFCH2ATB6");
     sprintf(MCM_Resp.resp_data.prmvalue[41], "%s", vl[32]);

     _f_strcpy(MCM_Resp.resp_data.prmname[42], "Sys_Status");
     if(p1>9 || p2>9 || p3>9)
    _f_strcpy(MCM_Resp.resp_data.prmvalue[42], "RETURN LINK DOWN");
     else if(i1<55 || i1>60 || i2<55 || i2>60 || i3<55 || i3>60)
    _f_strcpy(MCM_Resp.resp_data.prmvalue[42], "NOT OK");
     else
    _f_strcpy(MCM_Resp.resp_data.prmvalue[42], "OK");

     for(k=43;k<107;k++)
    {
      sprintf(MCM_Resp.resp_data.prmname[k],"Ch%2d", m);
      sprintf(MCM_Resp.resp_data.prmvalue[k], "%d", rw[m]);
      m++;
    }

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_302))   // set attn will set portbits at 32 bit o/p //
  {
    OFC_Attn1 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
    OFC_Attn2 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set OFC Attn : %d dB %d dB ..................", OFC_Attn1, OFC_Attn2);
    Set_ofc_atten(OFC_Attn2, OFC_Attn1);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_303))   // Init command will initalise Memory, Portbits, ADC and RTC //
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_304))   // reset
  {
     printf("\n MCM is given Reset : Socket Closed !!");
     sock_close(&Socket);
     exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_305))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
    printf("\n RTC set to: ,%s", Wrpr_Cmd.cmd_elem.timestamp);
  }

  return 12;     // This return value will be stored in MCM_Resp.event //
}

void ofc_atten()
{
Set_ofc_atten(Ch2Att, Ch1Att);
}

void Set_ofc_atten(int ch2, int ch1)
{
 printf("Entering Set_OFC_attenuation: %d dB  %d dB\n ", ch1, ch2);

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
  command_process_ofc();
  sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
}

int ofc_alarm_handler()
{
if((((980 - rw[3])/196.0) * 1.035) > -1.051)       //Laser1 Bias voltage1 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[2])/196.0) * 1.035) > -2.653)       //Laser1 Bias voltage2 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[3])/196.0) * 1.035) < -1.122)       //Laser1 Bias voltage1 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[2])/196.0) * 1.035) < -3.222)       //Laser1 Bias voltage2 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[4])/196.0) * 1.035) > 2.40)       //Laser1 APC unlock
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 APC unlock");
form_alarm(3,1);
}
if((((980 - rw[5])/196.0) * 0.05175) >25)       //Laser1 temp. > 25
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser1 temp. is greater than 25 degree C");
form_alarm(4,2);
}

if((((980 - rw[9])/196.0) * 1.035) > -1.051)       //Laser2 Bias voltage1 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[8])/196.0) * 1.035) > -2.653)       //Laser2 Bias voltage2 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[9])/196.0) * 1.035) < -1.122)       //Laser2 Bias voltage1 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[8])/196.0) * 1.035) < -3.222)       //Laser2 Bias voltage2 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[10])/196.0) * 1.035) > 2.40)       //Laser2 APC unlock
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser2 APC unlock");
form_alarm(3,1);
}

if((((980 - rw[14])/196.0) * 1.035) > -1.051)       //Laser3 Bias voltage1 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[13])/196.0) * 1.035) > -2.653)       //Laser3 Bias voltage2 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[14])/196.0) * 1.035) < -1.122)       //Laser3 Bias voltage1 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[13])/196.0) * 1.035) < -3.222)       //Laser3 Bias voltage2 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[15])/196.0) * 1.035) > 2.40)       //Laser3 APC unlock
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser3 APC unlock");
form_alarm(3,1);
}

/*if((((980 - rw[19])/196.0) * 1.035) > -1.051)       //Laser4 Bias voltage1 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[18])/196.0) * 1.035) > -2.653)       //Laser4 Bias voltage2 high
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[19])/196.0) * 1.035) < -1.122)       //Laser4 Bias voltage1 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[18])/196.0) * 1.035) < -3.222)       //Laser4 Bias voltage2 low
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[20])/196.0) * 1.035) > 2.40)       //Laser4 APC unlock
{
_f_strcpy(MCM_Resp.MCM_alarm.description,"Laser4 APC unlock");
form_alarm(3,1);
}    */

}

#endif

#ifdef mcm_addr4
// ******** Processing the FPS command ******** //

int command_process_fps()
{
  printf("\nEntering Command_Process_FPS");
  memset(out_data,0x00,sizeof(out_data));

  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_401))        // null
  {
  printf("fps command: null \n");
  out_data[0] = 8;
  out_data[1] = 0;
  out_data[2] = 0;           // id_code
  out_data[3] = 0;           // NULL = 0
  out_data[4] = 0;
  out_data[5] = 0;           // arg_len
  out_data[6] = 0;
  out_data[7] = chksum(&out_data[0]);     // calculate checksum //
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_402))
  {
  printf("fps command: set turning point \n");
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 0; // set turning point = 0 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_403))
  {
  printf("fps command: set ramp down count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 1; // set ramp down count = 1 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_404))
  {
  printf("fps command: set lower RPM Limit \n");
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // set low rpm limit = 2 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

   else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_405))
  {
  printf("fps command: set brake down difference \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 3; // set brake down diff = 3 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_406))
  {
  printf("fps command: set ramp up count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 4; // set ramp up count = 4 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_407))
  {
  printf("fps command: set stop time count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 5; // set stop time count = 5 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_408))
  {
  printf("fps command: set max angle \n");
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 7; // set max angle = 7 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_409))
  {
  printf("fps command: set min angle \n");
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 8; // set min angle = 8 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_410))
  {
  printf("fps command: set max pwm count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 6; // set max pwm count = 6 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_411))
  {
  printf("fps command: read turnning point \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 0; // read_tpoint = 0 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_412))
  {
  printf("fps command: read ramp down count \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 1; // read_rampdcnt = 1 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_413))
  {
  printf("fps command: read lower rpm limit \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // read_low_rmp = 2 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_414))
  {
  printf("fps command: read brake down diff \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 3; // read_Brake_dd = 3 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_415))
  {
  printf("fps command: read ramp up count \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 4; // read_rampupcnt = 4 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_416))
  {
  printf("fps command: read stop time count \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 5; // read_stoptimecnt = 5 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_417))
  {
  printf("fps command: read Max angle \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 7; // read_MAX_angle = 7 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_418))
  {
  printf("fps command: read Min angle \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 8; // read_Min_angle = 8 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_419))
  {
  printf("fps command: read Max pwm count \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 6; // read_MAX_pwm_cnt = 6 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_420))
  {
  printf("fps command: read version \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 9; // read_version = 9 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_421))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_422))
  {
  printf("fps command: run to calibrate \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 0; // run to calibrate = 0 // sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_423))
  {
  printf("fps command: free run \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 1; // run free = 1 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // 270 or -10 direction of fps
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_424))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_425))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_426))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_427))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_428))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_429))
  {
  printf("fps command: reboot \n");
  out_data[0] = 8;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 5;  // reboot = 5  // main cmd
  out_data[4] = 0;
  out_data[5] = 0;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_430))
  {
  printf("fps command: stop \n");
  out_data[0] = 8;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 6;  // stop = 6  // main cmd
  out_data[4] = 0;
  out_data[5] = 0;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_431))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_432))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_433))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_434))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_435))
  {

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_436))
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_437))
  {
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_438))   // setttime will set RTC //
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
int i;

 serCparity(PARAM_MPARITY); //0x04...mark parity
 enable_485Tx();     // make rts\ high at 7404 ic output

 if( !(serCputc(mcm_addr)) ) // this is ONLY for FPS sys.
 printf("cmd_pkt...Serial C port trans buf is full or locked \n");

 serCparity(PARAM_SPARITY); //0x05...space parity

 for(i=0; i<out_data[0]; i++)
 printf("%2x \n",out_data[i]);

 serCwrite(out_data,out_data[0]);//original, for all mcm except fps sys.
 while(!((BitRdPortI(SCSR,2) == 0)&&(BitRdPortI(SCSR,3)== 0)) );

 disable_485Tx();       //make rts\ low
 memset(out_data,0x00,sizeof(out_data));
}

void ser2eth_comm(void)
{
 int resp_msg_cntr, temp, EncCount;
 int bytecntr_in;  //mcm cmd len including mcm addr byte
 int maxpktlen;    //holds maximum mcm cmd len
 char in_d[50], watchdogflag, overcurrentflag;
 long int Rpm;

 bytecntr_in = 0;
 resp_msg_cntr = 0;
 maxpktlen = 0;

maxpktlen = sizeof(in_d); //max cmd 50 bytes

serCrdFlush();

while((bytecntr_in = serCread(in_d,maxpktlen,MSSG_TMOUT)) <= 0);

if( (bytecntr_in > 50)|| (bytecntr_in < 0x0B) )  //null resp + addr byte = 0B
{//printf("pkt size error. \n");
 _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Packet Size error");
 return ;
 }   // exit if pkt_len is more than 50 bytes

if(in_d[0] != 0x0e)
{printf("NOT FPS 14 Packet.\n");
 _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"NOT FPS 14 Packet");
 return ;
 }   // exit if mcm addr is not 0e or 14 dec

// check status byte.
 resp_msg_cntr = 0;
 if(in_d[4] == 0x00)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Exec. OK");
 if(in_d[4] & 0x01)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS detected timeout");
 if(in_d[4] & 0x02)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS detected CS error");
 if(in_d[4] & 0x04)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS detected timeout");
 if(in_d[4] & 0x08)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS received unknown command");
 if(in_d[4] & 0x10)
  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS rejected command");
 if(in_d[4] & 0x20){
  if(!watchdogflag)
   _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS watchdog reset");
  watchdogflag = 1; }
  else
  watchdogflag = 0;

 if(in_d[4] & 0x40){
  if(!overcurrentflag)
   _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"FPS over current");
  overcurrentflag = 1;
  }
  else
  overcurrentflag = 0;

 if(in_d[5] == 0x01)   //1st_of_lp
  {
    if( (in_d[6] != 7) ||(in_d[7] != 0) ||(in_d[8] != 4) )
    {
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Resp Logical Pkt error");
     return ;
    }
    else
    {
     temp = in_d[9];
     switch(temp){
     case(0x42): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"280 Limit Hit");
                 break;
     case(0x43): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"-10 Limit Hit");
                 break;
                 }

     temp &= 0xfe;

     switch(temp){
     case(0x00): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Feed Calibrated and Idle");
                 break;
     case(0x40): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Feed uncalibrated");
                 break;
     case(0x08): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Busy - Preset Run");
                 break;
     case(0x10): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Busy - Free Run");
                 break;
     case(0x4a): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Busy - Passworded Run");
                 break;
     case(0xc0): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Busy - In Calibration");
                 break;

     case(0x28): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Jammed - Preset Run");
                 break;
     case(0x30): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Jammed - Free Run");
                 break;
     case(0xe0): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Jammed - In Calibration");
                 break;
     case(0x62): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Jammed - Passworded");
                 break;
     case(0x20): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Jammed ");
                 break;
     case(0xff): _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Ua0 OFFSET Processing");
                 break;
                 } //case ends
//calculate EncCount
     EncCount = ( in_d[10] + (in_d[11]<<8) ) *2;
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "EncCount = %d", EncCount);
//calculate Rpm
     Rpm = (in_d[12]*5*60)/2048*300;
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Rpm = %ld", Rpm);
    }//else
  }//if 1st lp ends  in_d[5]

 if(in_d[5] == 0x02)   //2nd_of_lp = main cmd, sub cmd,
  {
   if((in_d[15] >6)||(in_d[15] < 0))
    {
    _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"unknown command ");
    return;
    }

   if(in_d[15] == 0)  // main cmd = NULL
    {
     if(!(in_d[16] & 0x01))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"mode MAN ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"mode AUTO ");

     if(!(in_d[16] & 0x02))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"rgso OFF ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"rgso ON ");

     if(!(in_d[16] & 0x04))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"dir 270 ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"dir -10 ");

     if(!(in_d[16] & 0x08))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"fv OFF ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"fv ON ");

     if(!(in_d[16] & 0x10))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"t80l OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"t80l HIT ");

     if(!(in_d[16] & 0x20))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"t70l OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"t70l HIT ");

     if(!(in_d[16] & 0x40))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"n51 OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"n51 HIT ");

     if(!(in_d[16] & 0x80))
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"n101 OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"n101 HIT ");

//motor current in_d[17]
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Motor cur %.2f", (float)(in_d[17]/12.5));
/* to be implemented
//Brake current in_d[18]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "Brake cur %f", (float)(in_d[18]/127.5);
//CPU 5vdc      in_d[19]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "CPU 5vdc %f", (float)(in_d[19]/51.0);
// Env 5vdc     in_d[20]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "Enc 5vdc %f", (float)(in_d[10]/51.0);
*/
     }//if in_d[15] ends here
   }// if in_d[5] == 0 ie NULL cmd ends


   if(in_d[15] == 1)  // main cmd = SET
    {
    switch(in_d[16]) {
          case 0:
          temp = 2*(in_d[17]+(in_d[18]<<8));
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Turning Point, target: %d", temp); break;

          case 1:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Ramp Down Count, Slope: %d", in_d[17]*5); break;

          case 2:
          temp = (int) (in_d[18]/0.2048*1800.0/(in_d[17]*5) );
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Lower Ramp Limit, %d int %d", temp, in_d[17]*5); break;

          case 3:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Break Count Diff, %d ",(int) in_d[17]*2); break;

          case 4:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Ramp up Count, %d ", in_d[17]*5); break;

          case 5:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Stop Count, %d ", in_d[17]*5); break;

          case 6:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Max PWM Count, %x ", in_d[17]); break;

          case 7:
          temp = in_d[17]+(in_d[18]<<8);
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Max Angle, %d ", temp*2); break;

          case 8:
          temp = in_d[17]+(in_d[18]<<8);
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Min Angle, %d ", temp*2); break;
                    } // case ends here

     if(in_d[16] == 9)
      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Write Addr ");    // case 9
     else if(in_d[16] == 10)
      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Load Count ");    //case 10
    }// if in_d[15] == 1 ie SET cmd ends


   if(in_d[15] == 2)  // main cmd = READ
    {
         switch(in_d[16]) {
          case 0:
          temp = 2*(in_d[17]+(in_d[18]<<8));
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Turning Point, target: %d", temp); break;

          case 1:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Ramp Down Count, Slope: %d", in_d[17]*5); break;

          case 2:
          temp = (int) (in_d[18]/0.2048*1800.0/((in_d[17]*5)) );
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Lower Ramp Limit, %d int %d", temp, in_d[17]*5); break;

          case 3:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Break Count Diff, %d ",(int) in_d[17]*2); break;

          case 4:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Ramp up Count, %d ", in_d[17]*5); break;

          case 5:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Stop Count, %d ", in_d[17]*5); break;

          case 6:
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Max PWM Count, %x ", in_d[17]); break;

          case 7:
          temp = in_d[17]+(in_d[18]<<8);
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Max Angle, %d ", temp*2); break;

          case 8:
          temp = in_d[17]+(in_d[18]<<8);
          sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Read Min Angle, %d ", temp*2); break;
                    } // case ends here

     if(in_d[16] == 9)
      sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Read Version: %.1f ", (float) in_d[17]/10 );
     else if(in_d[16] == 10){
      temp = 2*(in_d[17]+(in_d[18]<<8));
      sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Read Ua0 position: %d ",temp);}

    }// if in_d[15] == 2 ie READ cmd ends

   if(in_d[15] == 3)  // main cmd = RUN
    {  switch(in_d[16]) {
          case 0:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to calibrate"); break;
          case 1:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run Free"); break;
          case 2:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Reset"); break;
          case 3:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Fine Tune"); break;
          case 4:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run Passworded"); break;
          case 5:
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Ua0"); break;
                       }
    }// if in_d[15] == 3 ie RUN cmd ends

   if(in_d[15] == 5)  // main cmd = reboot
    {
    _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Reboot");
    }// if in_d[15] == 5 ie reboot cmd ends

   if(in_d[15] == 6)  // main cmd = stop
    {
    _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Stop");
    }// if in_d[15] == 6 ie stop cmd ends

serCrdFlush();
printf("end of serial transmission... ser2eth \n\n");
}

#endif

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

  /*#ifdef mcm_addr1     // For Analog backend lab-test only.
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
  #endif   */
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