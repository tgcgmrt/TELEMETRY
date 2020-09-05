// **************************************************************************** //
// *******                    Program : GAB_MCM                        ******** //
// *******                    Date : 20/08/2015                        ******** //
// *******                     Version : V1.02                         ******** //
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
#use      "spi.lib"

#nodebug

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
   #ifdef USE_HTTP_UPLOAD
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
   #endif
SSPEC_RESOURCETABLE_END

static tcp_Socket Socket;

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

  #ifdef USE_HTTP_UPLOAD
  fwinit();
  #endif
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

  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP, IFS_END);
  while (ifpending(IF_ETH0) == IF_COMING_UP)
         tcp_tick(NULL);

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
 char cmdstr1[20];
 int i, cmd_flag;

   _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
   fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);
   _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.subsysid);
   printf("\nSubsystem: %s", cmdstr1);

   cmd_flag=0;

   if( (!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3))) || (!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_4))) )
   {
      for(i=0;i<45;i++)
      {
      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[i]))
      cmd_flag=1;
      }

      if(cmd_flag==1)
      return 10;
      else
      return 12;
   }
   else
   return 14;

 /*  int cmpl=0, i, j;
   char cmdstr1[64], cmdstr2[64];

   _f_strcpy(cmdstr1,Wrpr_Cmd.cmd_elem.cmd_name);
   fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);
   _f_strcpy(cmdstr2,Wrpr_Cmd.cmd_elem.subsysid);
   printf("\nSubsystem: %s", cmdstr2);

  if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3)))
       {
         printf("\nSubsystem validated");

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

   if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_4)))
       {
         printf("\nSubsystem validated");

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
   }   */
 }

// ****** Forming intermediate response for all system commands ****** //

int form_inter_resp()
{
  int loop;

  printf("\nEntering OFC_form_inter_resp");

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
  printf("\nEntering form_final_resp\n");
  resp_msg_cntr = 0 ;
  resp_code = 10;
  MCM_Resp.code = resp_code;
  if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3)))
  MCM_Resp.event = command_process_ofcsnt();
  if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_4)))
  MCM_Resp.event = command_process_fps();
  MCM_Resp.resp_data.numpkt = resp_msg_cntr;
}

// ******** Processing the Sentinal command ******** //

void sen_mon()
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
    strcpy(smoke_detector,"Not Ok");
    else
    strcpy(smoke_detector,"Ok");

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
    strcpy(intruder_detector,"Not Ok");
    else
    strcpy(intruder_detector,"Ok");

    if(temp>27 || (!strcmp(smoke_detector,"Not Ok")) || (!strcmp(intruder_detector,"Not Ok")))
    sprintf(state, "NOT OK");
    else
    sprintf(state, "OK");
}

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

int command_process_ofcsnt()
{
int m=0, k, OFC_Attn1, OFC_Attn2;;

  printf("\nEntering command_process_ofc");
  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[0]))      // DoMon will monitor Temperature, RTC time, Status, Raw Values etc..//
  {
     resp_msg_cntr = 114;

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

     _f_strcpy(MCM_Resp.resp_data.prmname[107], "sntstate");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[107], state);

     _f_strcpy(MCM_Resp.resp_data.prmname[108], "snttemp");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[108], temps); // This value should be filled in bg_mon function. //

     _f_strcpy(MCM_Resp.resp_data.prmname[109], "ch64_temp");
      sprintf(MCM_Resp.resp_data.prmvalue[109], "%d", rw[63]);  // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[110], "SmokeDet");
     sprintf(MCM_Resp.resp_data.prmvalue[110], "%d", smoke_detector);

    _f_strcpy(MCM_Resp.resp_data.prmname[111], "ch62_smokedet");
     sprintf(MCM_Resp.resp_data.prmvalue[111], "%d", rw[61]);

    _f_strcpy(MCM_Resp.resp_data.prmname[112], "IntruDet");
     sprintf(MCM_Resp.resp_data.prmvalue[112], "%d", intruder_detector);

     _f_strcpy(MCM_Resp.resp_data.prmname[113], "ch61_intrdet");
     sprintf(MCM_Resp.resp_data.prmvalue[113], "%d", rw[60]);

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[1]))   // set attn will set portbits at 32 bit o/p //
  {
    OFC_Attn1 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
    OFC_Attn2 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set OFC Attn : %d dB %d dB ..................", OFC_Attn1, OFC_Attn2);
    Set_ofc_atten(OFC_Attn2, OFC_Attn1);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[2]))   // Init command will initalise Memory, Portbits, ADC and RTC //
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[3]))   // reset
  {
     printf("\n MCM is given Reset : Socket Closed !!");
     sock_close(&Socket);
     exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[4]))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
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
  command_process_ofcsnt();
  sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
}

void ofc_alarm_handler()
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

// ******** Processing the FPS command ******** //
      // NOTE : FPS CONSTRAINT  : COMMAND NO 4 SHALL NOT BE GIVEN
int command_process_fps()
{
  int arg_fpscnt=0;

  printf("\nEntering Command_Process_FPS");
  memset(out_data,0x00,sizeof(out_data));
 // _f_strcpy(Wrpr_Cmd.cmd_elem.cmd_name, "domon");

  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[5]))        // null/domon
  {
  printf("\nfps command: null \n");
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
  return 99;
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[6]))
  {
  printf("\nfps command: set turning point \n");

  arg_fpscnt = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2 ;

  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 0; // set turning point = 0 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[7]))
  {
  printf("\nfps command: set ramp down count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 1; // set ramp down count = 1 // sub cmd
  out_data[8] = (int) _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/5 ;  // time count
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[8]))
  {
  printf("\nfps command: set lower RPM Limit \n");  //expected values are like 700 550
  arg_fpscnt = (int) ((15 * 140625)/(160 * _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]))) ;
  /*   teleset-abc
  x = 550; y = 0;        //time interval 0?
  CurrentCmd.Argument[0] = x/5;
  CurrentCmd.Argument[1] = (int) (y*0.2048*x/1800.0);
  */
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // set low rpm limit = 2 // sub cmd
  out_data[8] = arg_fpscnt;
  out_data[9] = 15;
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

   else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[9]))
  {
  printf("\nfps command: set brake down difference \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 3; // set brake down diff = 3 // sub cmd
  out_data[8] = (int) _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2;   // Encoder Pulses/2
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[10]))
  {
  printf("\nfps command: set ramp up count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 4; // set ramp up count = 4 // sub cmd
  out_data[8] = (int) _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/5;  // TMcount/5
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[11]))
  {
  printf("\nfps command: set stop time count \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 5; // set stop time count = 5 // sub cmd
  out_data[8] = (int) _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/ 5;   // timecount
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[12]))
  {
  printf("\nfps command: set max angle \n");
  arg_fpscnt = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 7; // set max angle = 7 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[13]))
  {
  printf("\nfps command: set min angle \n");
  arg_fpscnt = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 1;  // set = 1  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 8; // set min angle = 8 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[14]))
  {
  printf("\nfps command: set max pwm count \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[15]))
  {
  printf("\nfps command: read turnning point \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[16]))
  {
  printf("\nfps command: read ramp down count \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[17]))
  {
  printf("\nfps command: read lower rpm limit \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[18]))
  {
  printf("\nfps command: read brake down diff \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[19]))
  {
  printf("\nfps command: read ramp up count \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[20]))
  {
  printf("\nfps command: read stop time count \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[21]))
  {
  printf("\nfps command: read Max angle \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[22]))
  {
  printf("\nfps command: read Min angle \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[23]))
  {
  printf("\nfps command: read Max pwm count \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[24]))
  {
  printf("\nfps command: read version \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[25]))
  {
  printf("\nfps command: read UA0 angle \n");
  out_data[0] = 9;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 2;  // read = 2  // main cmd
  out_data[4] = 0;
  out_data[5] = 1;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 0x0a; // read_UA0_angle = 10// sub cmd
  out_data[8] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();    }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[26]))
  {
  printf("\nfps command: run to calibrate \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[27]))
  {
  printf("\nfps command: free run \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 1; // run free = 1 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // 0=270 or 1=-10 direction of fps
  out_data[9] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[28]))
  {
  printf("\nfps command: UA0CALIB \n");
  out_data[0] = 0x0a;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 2;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 5; // UA0CALIB=5 // sub cmd
  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // 0=270 or 1=-10 direction of fps
  out_data[9] = chksum(&out_data[0]);     // calculate checksum

  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[29]))
  {
  printf("\nfps command run to C preset \n");
  arg_fpscnt = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
//  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
//  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[30]))
  {
    printf("\nfps command run to D preset \n");
   // (512*NINT(CPARM(1)))/18 + (8500-(28.44*NINT(CPARM(2))))

  arg_fpscnt = (int)((_f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]) * 28.44)+ (8500 - (28.44 * 270)));
  // 270 is hardcoded.
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
 // out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
 // out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[31]))
  {
  printf("\nfps command run C fine tune \n");
  arg_fpscnt = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0])/2 ; // Encoder pulse
  out_data[0] = 0x0c;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 4;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 3; // run fine = 3 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
  out_data[10] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[2]);     //
  out_data[11] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[32]))
  {
  printf("\nfps command run D fine tune \n");
  arg_fpscnt = (int)((_f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]) * 28.44)+ (8500 - (28.44 * 270)));
  out_data[0] = 0x0c;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 4;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 3; // run fine = 3 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
  out_data[10] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[2]);     //
  out_data[11] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[33]))
  {
  printf("\nfps command: reboot \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[34]))
  {
  printf("\nfps command: stop \n");
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[35]))
  {
  printf("\nfps command: load fps position \n");
  fps_def_cnt[0] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]); // 610
  fps_def_cnt[1] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]); // 150
  fps_def_cnt[2] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[2]); // 1420
  fps_def_cnt[3] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[3]); // 325
  //transmit();
  resp_code = 10;
  //ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[36]))
  {
  printf("\nfps command: Move FPS to 150 \n");
  arg_fpscnt = fps_def_cnt[1]/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
//  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
//  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[37]))
  {
  printf("\nfps command: Move FPS to 375 \n");
  arg_fpscnt = fps_def_cnt[3]/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
//  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
//  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[38]))
  {
  printf("\nfps command: Move FPS to 700 \n");
  arg_fpscnt = fps_def_cnt[0]/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
//  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
//  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[39]))
  {
  printf("\nfps command: Move FPS to 1420 \n");
  arg_fpscnt = fps_def_cnt[2]/2 ; // Encoder pulse
  out_data[0] = 0x0b;
  out_data[1] = 0;
  out_data[2] = 0;
  out_data[3] = 3;  // run = 3  // main cmd
  out_data[4] = 0;
  out_data[5] = 3;  // lsb arg length
  out_data[6] = 0;
  out_data[7] = 2; // run to preset = 2 // sub cmd
  out_data[8] = (int ) arg_fpscnt & 0x00ff ;
  out_data[9] = (int ) arg_fpscnt >> 8;
//  out_data[8] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);      // tgt angle
//  out_data[9] = _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);      // tgt angle
  out_data[10] = chksum(&out_data[0]);     // calculate checksum
  transmit();
  ser2eth_comm();
  }

  /*else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[40]))
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  } */

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[41]))
  {
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    exit(0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd[42]))   // setttime will set RTC //
  {
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
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

 //dly_rts();
 //add delay if nessesary
 //for(i=0; i<1687; i++);       //(1687 for lab) adjust delay for holding rts high for data transmission
  disable_485Tx();       //make rts\ low
  memset(out_data,0x00,sizeof(out_data));
  serCwrFlush();
}

void ser2eth_comm(void)
{
 int i, temp, EncCount, l;
 int bytecntr_in;  //mcm cmd len including mcm addr byte
 int maxpktlen;    //holds maximum mcm cmd len
 char watchdogflag, overcurrentflag;
 long int Rpm;

 char *par_name[TVAR] = { "current_encoder", "target_encoder","current_angle","target_angle", "current_rpm",
 "mode", "fps_version", "ramp_dntime", "ramp_uptime","low_rpm_lmt", "low_rpm_time",
 "brk_cnt_diff", "step_slew_timecnt","fine_tune_pwm", "max_pwm", "max_angle","min_angle",
 "ua0_angle","over_current","commerrors","cmd_fail","timeout_cnt","chksum_cnt","reset_cnt",
 "fps_control_info", "fps_motor_temp","motor_curr","domon_stat", "turning_pt","stop_cnt" };


 bytecntr_in = 0;
 resp_msg_cntr = 0;
 maxpktlen = 0;
 l = 0;

 memset(in_d,0x00,sizeof(in_d));

// ------------ Read Packet --------------//

maxpktlen = sizeof(in_d)-1; //max cmd 50 bytes
//serCrdFlush();

//BitWrPortI(PBDR, &PBDRShadow, 1, 7); // PB7=1 (RE\=0, DE=1) will disable Tx & enable Rx.
//disable_485Tx();

while( ((bytecntr_in = serCread(in_d,maxpktlen,MSSG_TMOUT)) <= 5) && (l++ < 1000));

//while((bytecntr_in = serCread(in_d,maxpktlen,MSSG_TMOUT)) <= 5);
//bytecntr_in = serCread(&in_d,maxpktlen,MSSG_TMOUT);

printf("\nbytecntr_in: %d\n", bytecntr_in);
printf("in_d: \n");
for(i=0;i<bytecntr_in;i++)
{
printf("[%2d] : %x \n", i, in_d[i]);
}
//serCrdFlush();
//serCwrFlush();
BitWrPortI(PBDR, &PBDRShadow, 0, 7); // PB7=0 (RE\=1, DE=0) will enable Tx & disable Rx.

i=0;
while(in_d[i]!=0x0e)
{
i=i+1;
if(i>5)
break;
}

   if(bytecntr_in <= 0 )
   {
   _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"cmdstatus");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"No Answer from FPS");
   printf("\nNo Answer from FPS");
   resp_msg_cntr++;
   resp_code = 11;
   return;
   }
  //---------- cmdstatus ------------------------//
 _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"cmdstatus");

if( (bytecntr_in > 50)|| (bytecntr_in < 0x0B) )  //null resp + addr byte = 0B
{//printf("pkt size error. \n");
 _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Packet Size error");
 printf("\nPacket Size error");
  resp_msg_cntr++;
  resp_code = 11;
 return ;
}   // exit if pkt_len is more than 50 bytes

if(in_d[i+0] != 0x0e)
{
printf("NOT FPS 14 Packet.\n");
 sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr],"NOT FPS 14 Packet %d - %s" ,bytecntr_in,in_d);
 resp_msg_cntr++;
 resp_code = 11;
 return ;
}   // exit if mcm addr is not 0e or 14 dec

// check status byte.

 if(in_d[i+4] == 0x00)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Exec OK");
 if(in_d[i+4] & 0x01)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS detected timeout");
 if(in_d[i+4] & 0x02)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS detected CS error");
 if(in_d[i+4] & 0x04)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS received unknown command");
 if(in_d[i+4] & 0x08)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS received unknown command");
 if(in_d[i+4] & 0x10)
  _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS received correct Pkt but rejected cmd");
 if(in_d[i+4] & 0x20)
  {
    if(!watchdogflag)
    {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS watchdog reset");
    watchdogflag = 1;
    }
    else
    {
    watchdogflag = 0;
    }
  }
 if(in_d[i+i+4] & 0x40)
 {
    if(!overcurrentflag)
    {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"FPS over current");
    overcurrentflag = 1;
    }
    else
    {
    overcurrentflag = 0;
    }
  }
  resp_msg_cntr++; // cmdstatus over

 if( in_d[i+5] >= 0x01)   //1st_of_lp
  {
      // --------------fps_state --------------------------------//
     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"fps_state");

    if( (in_d[i+6] != 7) ||(in_d[i+7] != 0) ||(in_d[i+8] != 4) )  // in_d[i+6+7] packet length, in_d[i+8] is fixed 4
    {
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Resp Logical Pkt error");
     resp_msg_cntr++;
     resp_code = 11;
     return ;
    }
    else
    {

     temp = in_d[i+9];
     switch(temp)
     {
     case(0x42): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"280 Limit Hit");
                 break;
     case(0x43): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"-10 Limit Hit");
                 break;
     }

     temp &= 0xfe;

     switch(temp)
     {
     case(0x00): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Feed Calibrated and Idle");
                 break;
     case(0x40): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Feed uncalibrated");
                 break;
     case(0x08): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Busy - Preset Run");
                 break;
     case(0x10): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Busy - Free Run");
                 break;
     case(0x4a): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Busy - Passworded Run");
                 break;
     case(0xc0): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Busy - In Calibration");
                 break;

     case(0x28): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Jammed - Preset Run");
                 break;
     case(0x30): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Jammed - Free Run");
                 break;
     case(0xe0): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Jammed - In Calibration");
                 break;
     case(0x62): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Jammed - Passworded");
                 break;
     case(0x20): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Jammed ");
                 break;
     case(0xff): _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Ua0 OFFSET Processing");
                 break;
     } //case ends
        resp_msg_cntr++;
        //------------ fps_state end here -----------------------//

             //calculate EncCount
     EncCount = ( in_d[i+10] + (in_d[i+11]<<8) ) *2;
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr], "current_encoder");
     sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%d", EncCount);

//calculate Rpm
     Rpm = (in_d[i+12]*5*60)/2048*300;
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr], "current_rpm");
     sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%ld", Rpm);
    }//else
  }//if 1st lp ends  in_d[i+5]


 if(in_d[i+5] == 0x02)   //2nd_of_lp = main cmd, sub cmd,
  {
    // ---------   fpscmd ---------------------------//
   sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr], "fpscmd");

   if((in_d[i+15] >6)||(in_d[i+15] < 0))
    {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"unknown cmd");
    resp_msg_cntr++;
    resp_code = 11;
    return;
    }


    // commented part is written by CPK.
   /*if(in_d[i+15] == 0)  // main cmd = NULL
    {
     sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "domon"); // NULL changed to domon

    _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"FPS Status");

     if(!(in_d[i+16] & 0x01))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"mode MAN ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"mode AUTO ");

     if(!(in_d[i+16] & 0x02))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"rgso OFF ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"rgso ON ");

     if(!(in_d[i+16] & 0x04))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"dir 270 ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"dir -10 ");

     if(!(in_d[i+16] & 0x08))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"fv OFF ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"fv ON ");

     if(!(in_d[i+16] & 0x10))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"t80l OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"t80l HIT ");

     if(!(in_d[i+16] & 0x20))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"t70l OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"t70l HIT ");

     if(!(in_d[i+16] & 0x40))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"n51 OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"n51 HIT ");

     if(!(in_d[i+16] & 0x80))
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"n101 OK ");
     else
     _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++],"n101 HIT ");

//motor current in_d[i+17]
     sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr], "Motor cur");
     sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%.2f", (float)(in_d[i+17]/12.5));
// to be implemented
//Brake current in_d[i+18]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "Brake cur %f", (float)(in_d[i+18]/127.5);
//CPU 5vdc      in_d[i+19]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "CPU 5vdc %f", (float)(in_d[i+19]/51.0);
// Env 5vdc     in_d[i+20]
     sprintf(MCM_Resp.message[resp_msg_cntr++], "Enc 5vdc %f", (float)(in_d[i+10]/51.0);

     }*///if in_d[i+15] ends here

  }//  2nd_of_lp end here.



     // NOTE READ and SET Sub-Command IDs Assumed identical

   if(in_d[i+15] == 1)  // main cmd = SET
    {
       sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "SET");
    }

   if(in_d[i+15] == 2)  // main cmd = READ
    {
       sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "READ");
    }

   if ( in_d[i+15] == 1 || in_d[i+15] == 2 )
   {
     switch(in_d[i+16])
     {
          case 0:
          temp = 2*(in_d[i+17]+(in_d[i+18]<<8));
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_TNG_PNT");
          sprintf(par_value[TNG_PT],"%d",temp);
          // sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Turning Point, target: %d", temp);
          break;

          case 1:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_RMP_DNCNT");
          sprintf(par_value[RAMP_DNTIME],"%d",in_d[i+17]*5);
          //sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Ramp Down Count, Slope: %d", in_d[i+17]*5);
          break;

          case 2:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_LRPM_LMT");
          temp = (int) (in_d[i+18]/0.2048*1800.0/(in_d[i+17]*5) );
          sprintf(par_value[LOW_RPM_LMT],"%d",temp);
          // sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Lower Ramp Limit, %d int %d", temp, in_d[i+17]*5);
          break;

          case 3:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_BRKCNT_DIFF");
          sprintf(par_value[BRK_CNT_DIFF],"%d",(int) in_d[i+17]*2);
          //sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Break Count Diff, %d ",(int) in_d[i+17]*2);
          break;

          case 4:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_RMP_UPCNT");
           sprintf(par_value[RAMP_UPTIME],"%d",in_d[i+17]*5);
         // sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Ramp up Count, %d ", in_d[i+17]*5);
          break;

          case 5:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_STOP_CNT");
          sprintf(par_value[STOP_CNT],"%d",in_d[i+17]*5);
          //sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Stop Count, %d ", in_d[i+17]*5);
          break;

          case 6:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_MAX_PWM");
            sprintf(par_value[MAX_PWM],"%d",in_d[i+17]);
          //sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Max PWM Count, %x ", in_d[i+17]);
          break;

          case 7:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_MAX_ANG");
          temp = in_d[i+17]+(in_d[i+18]<<8);
          sprintf(par_value[MAX_ANG],"%d",temp * 2);
          //sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Max Angle, %d ", temp*2);
          break;

          case 8:
          _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_MIN_ANG");
          temp = in_d[i+17]+(in_d[i+18]<<8);
          sprintf(par_value[MIN_ANG],"%d",temp * 2);
         // sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr++], "Set Min Angle, %d ", temp*2);
           break;

          case 9:
           _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_VERSION");
           sprintf(par_value[FPS_VER],"%f",(float) in_d[i+17]/10);

          case 10 :
           _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_UA0_ANG");
           temp = 2*(in_d[i+17]+(in_d[i+18]<<8));
           sprintf(par_value[UA0_ANG],"%d",temp);

     } // case ends here
    /* TBD ---  10/9/2015
     if(in_d[i+16] == 9)
      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Write Addr ");    // case 9
     else if(in_d[i+16] == 10)
      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Load Count ");    //case 10
    }// if in_d[i+15] == 1 ie SET cmd ends
     */
  } else if(in_d[i+15] == 3)  // main cmd = RUN
   {
    sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "Run");
    switch(in_d[i+16])
    {
          case 0: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_CALIBRATE");break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to calibrate"); break;
          case 1: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_FREE");break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run Free"); break;
          case 2: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_TO_RESET");break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Reset"); break;
          case 3: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_TO_FINETUNE");  break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Fine Tune"); break;
          case 4: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_PASSWORD");  break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run Passworded"); break;
          case 5: _f_strcat( MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"_TO_UA0");  break;
         // _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr++],"Run to Ua0"); break;
      }
   } else if(in_d[i+15] == 5)  // main cmd = reboot
   {
     sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "Reboot");
   } else if(in_d[i+15] == 6)  // main cmd = stop
   {
    // if in_d[i+15] == 5 ie reboot cmd ends
      sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "Stop");
   }// if in_d[i+15] == 6 ie stop cmd ends
   resp_msg_cntr++;
   //---------------fpscmd end


   for (i = 0;i < TVAR; i++)
   {
      sprintf(MCM_Resp.resp_data.prmname[resp_msg_cntr],"%s",par_name[i]);
      sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%s",par_value[i]);
      resp_msg_cntr++;
   }

/*for(i=0;i<resp_msg_cntr;i++)
printf("\n[%2d] : %s",i, MCM_Resp.resp_data.prmvalue[i]); */

printf("end of serial transmission... ser2eth \n\n");
resp_code = 10;

}

// ****** MCM reset function will reset MCM card
void MCM_Reset()
{
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&Socket);
    exit(0);
}

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
     FDV_Mon=1;
   }
  else if(FDV == 2)
   {
     WrPortI(GCSR,NULL,0x0D);
     FDV_Mon=2;
   }
  else if(FDV == 3)
   {
     WrPortI(GCSR,NULL,0x19);
     FDV_Mon=4;
   }
  else if(FDV == 4)
   {
     WrPortI(GCSR,NULL,0x1D);
     FDV_Mon=6;
   }
  else if(FDV == 5)
   {
     WrPortI(GCSR,NULL,0x01);
     FDV_Mon=8;
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
char RTC_time[20];

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
  #ifdef USE_HTTP_UPLOAD
  writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
  #endif
  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP,IFS_END);

  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);
  }

}