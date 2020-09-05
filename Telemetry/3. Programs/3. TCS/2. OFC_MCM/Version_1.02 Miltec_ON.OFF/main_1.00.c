// **************************************************************************** //
// *******                    Program :OFCSNT_MCM                        ******** //
// *******                    Date : 19/11/2018                        ******** //
// *******                     Version : V1.01                         ******** //
// *******               Client for Existing Online Server             ******** //
// *******        HTTP Server & client for Remote Firmware Update.     ******** //
// *******     Remote Firmware Update (v1.0 & v2.0) using USER BLOCK   ******** //
// ******* set_attenuation function added from charu sir code date 28.8.20 *****//
// **************************************************************************** //

#class    auto
#memmap   xmem

#use      "rcm43xx.lib"	             // Use with RCM 4300
#use      "adc_ads7870.lib"
#include  "define.c"
#include  "init.c"
#include "firmware_update.c"
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
   SSPEC_RESOURCE_XMEMFILE("/Mon_check.xml",Mon_check_xml),
	SSPEC_RESOURCE_XMEMFILE("/MCM_Set.zhtml",MCM_Set_zhtml),
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END

static tcp_Socket OFC_Socket;

//.... Generating milisecond delay .... //

void Ms_delay(unsigned int delay)
{
	auto unsigned long delay_time;

	delay_time = MS_TIMER + delay;
   while((long)(MS_TIMER - delay_time) < 0 );
}

// .... Opening Server Socket at desired port address .... //

cofunc int OFC_mcm_clnt()
{
   int port1, write_bytes1, read_bytes1, wait_var1;
   longword host1;

   if(!(host1 = inet_addr(OFC_SERVER_ADDR)))
   {
      puts("\nCould not resolve host");
      exit(3);
   }
   printf("\nHost hex address %lx\n",host1);

   port1 = atoi(OFC_SERVER_PORT);

   printf("\nAttempting to open '%s' on port %u\n", OFC_SERVER_ADDR, port1 );

   while( ! tcp_open(&OFC_Socket, 0, host1, port1, NULL))
    {
     tcp_tick(NULL);
     printf("\nUnable to open TCP session");
    }

   wait_var1 = 0;

   while((!sock_established(&OFC_Socket) && sock_bytesready(&OFC_Socket)== -1))
    {
      http_handler();
      tcp_tick(NULL);
      wait_var1++;
      if(wait_var1 == 100)
      {
        tcp_open(&OFC_Socket, 0, host1, port1, NULL);
        wait_var1 = 0;
      }
      yield;
    }
   printf("\nOFC Socket Established ****************************************\n");

   if(sock_established(&OFC_Socket))
   {
    read_bytes1 = 0;
    do{
       http_handler();
       if(sock_bytesready(&OFC_Socket) > 0)
       {
         read_bytes1 = sock_read(&OFC_Socket, &OFC_Wrpr_Cmd, Wrpr_Cmd_Size);

         printf("\nMCM has read command of %d bytes",read_bytes1);

         if(OFC_form_inter_resp() == 10)    // Form intermediate response and send it over socket //
         {
           write_bytes1 = sock_write(&OFC_Socket, &OFC_MCM_Resp, MCM_Resp_Size);
           printf("\nMCM has sent intermediate response of %d bytes",write_bytes1);
           printf("\nIntermediate response :\n %d \n",OFC_MCM_Resp.event);
           http_handler();
           OFC_form_final_resp(SUCCESS,FINAL);   // Process the command, Form final response and send it over socket //
           write_bytes1 = sock_write(&OFC_Socket, &OFC_MCM_Resp, MCM_Resp_Size);
   		  printf("\nMCM has sent final response of %d bytes",write_bytes1);
   		  printf("\nFinal response :\n %d\n",OFC_MCM_Resp.event);
         }
         else
         {
   		 write_bytes1 = sock_write(&OFC_Socket, &OFC_MCM_Resp, MCM_Resp_Size);
         }
       }
       read_bytes1 = 0;
       yield;
      }while(tcp_tick(&OFC_Socket));    // This loop continuously listens over socket for incoming command //
   }
   http_handler();
   sock_close(&OFC_Socket);
	printf("\nConnection closed for port-%d",port1);

   return 0;
}


// ..... Main start .... //
main()
{
  auto unsigned long scn_dl;
  int m;
  char s[10];

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

  /*while(1)
  {
    gets(s);
    _f_strcpy(Wrpr_Cmd.cmd_elem.cmd_name, s);
    command_process_fps();
    Ms_delay(500);
  } */

  while(1)
  {
    http_handler();

    costate    // This will open client socket at port 3002 //
     {
       wfd OFC_mcm_clnt();
     }

    costate    // This will do background monitoring in parallel at every SCAN_DELAY miliseconds //
     {
       scn_dl = MS_TIMER + SCAN_DELAY;
       Ms_delay(1);  // This delay is necessary for proper working of following loop. //
       while(((scn_dl - MS_TIMER) > 0) && ((scn_dl - MS_TIMER) < 1000))
       yield;
       bg_mon();
       ofc_mon();
       sen_mon();
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

int OFC_cmd_validation()
 {
   int cmpl=0, i, j;
   char cmdstr1[64], cmdstr2[64];

   _f_strcpy(cmdstr1,OFC_Wrpr_Cmd.cmd_elem.cmd_name);
   fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);
   _f_strcpy(cmdstr2,OFC_Wrpr_Cmd.cmd_elem.subsysid);
   printf("\nSubsystem: %s", cmdstr2);

  if(!(_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_3)))
       {
         printf("\nSubsystem validated");
         //_f_strcpy(cmdstr1,OFC_Wrpr_Cmd.cmd_elem.cmd_name);
         //fprintf(stdout,"\nCommand received : %s\n",  cmdstr1);

         if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_301))
          {
           return 10;
          }
         else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_302))
          {
           return 10;
          }
         else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_303))
          {
           return 10;

           for(j=0; j<OFC_Wrpr_Cmd.cmd_data.numpkt; j++) // Looking for Jth parameter //
            {
             for(i=0; i<SMP_rawsize; i++) // comparing Jth against Ith parameter //
              {
               if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_data.prmname[j],SenMon_Para[i]))
                {
                 cmpl++;
                 break;
                }
              }
            }// Checking of all parameters over here //

            if(cmpl == OFC_Wrpr_Cmd.cmd_data.numpkt)
             return 10;
            else
             return 13;   // Incase of Incomplete command //
          }
         else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_304))
          {
           return 10;
          }
         else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_305))
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
 }

// ****** Forming intermediate response for all system commands ****** //

int OFC_form_inter_resp()
{
  int loop;

  printf("\nEntering OFC_form_inter_resp");

  OFC_MCM_Resp.code = OFC_cmd_validation();
  OFC_MCM_Resp.event = INTERMEDIATE;
  OFC_MCM_Resp.MCM_alarm.level=1;
  switch(OFC_MCM_Resp.code)
  {
    case 10 :  _f_strcpy(OFC_MCM_Resp.message,"Command received successfully");
               printf("\nCommand received successfully");
               break;
    case 12 :  _f_strcpy(OFC_MCM_Resp.message,"Command Syntax Error");
               printf("\nCommand Syntax Error");
               break;
    case 13 :  _f_strcpy(OFC_MCM_Resp.message,"Command Incomplete");
               printf("\nCommand Incomplete");
               break;
    case 14 :  _f_strcpy(OFC_MCM_Resp.message,"Command Invalid");
               printf("\nCommand Invalid");
               break;
    case 100:  _f_strcpy(OFC_MCM_Resp.message,"Error Unknown");
               printf("\nError Unknown");
               break;
    default :  break;
  };

 // ******** Write in to Tx_BasicFlds *********** //
  OFC_MCM_Resp.resp_elem.seq = OFC_Wrpr_Cmd.cmd_elem.seq;
  OFC_MCM_Resp.resp_elem.version = OFC_Wrpr_Cmd.cmd_elem.version;
  _f_strcpy(OFC_MCM_Resp.resp_elem.subsysid,OFC_Wrpr_Cmd.cmd_elem.subsysid);
  _f_strcpy(OFC_MCM_Resp.resp_elem.cmd_id,OFC_Wrpr_Cmd.cmd_elem.cmd_id);
  _f_strcpy(OFC_MCM_Resp.resp_elem.cmd_name,OFC_Wrpr_Cmd.cmd_elem.cmd_name);
  _f_strcpy(OFC_MCM_Resp.resp_elem.timestamp,OFC_Wrpr_Cmd.cmd_elem.timestamp);
  OFC_MCM_Resp.resp_elem.priority = OFC_Wrpr_Cmd.cmd_elem.priority;
  OFC_MCM_Resp.resp_elem.timeout = OFC_Wrpr_Cmd.cmd_elem.timeout;

  // ******** Write in to Tx_DataPkt *********** //
  OFC_MCM_Resp.resp_data.numpkt = 0;
  for(loop=0; loop<MAXDATA; loop++)
   {
     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[loop],"\0");
     _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[loop],"\0");
   }
  // ******** Write in to Tx_alarm *********** //
  OFC_MCM_Resp.MCM_alarm.id = 0;
  OFC_MCM_Resp.MCM_alarm.level = 0;
  _f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"\0");

  return OFC_MCM_Resp.code;
}
// ******** Forming final response ******** //

void OFC_form_final_resp(int code, int event)
{
  //printf("\nEntering form_final_resp");
  OFC_resp_msg_cntr = 0 ;
  OFC_resp_code = 10;
  OFC_MCM_Resp.code = OFC_resp_code;
  OFC_MCM_Resp.event = command_process_ofcsnt();
  OFC_MCM_Resp.resp_data.numpkt = OFC_resp_msg_cntr;

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

int command_process_ofcsnt()
{
int m=0, k, OFC_Attn1, OFC_Attn2;;

  printf("\nEntering command_process_ofc");
  if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_301))      // DoMon will monitor Temperature, RTC time, Status, Raw Values etc..//
  {
     OFC_resp_msg_cntr = 114;

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[0],"time");
      sec_tm = read_rtc();
      mktm(&rtc, sec_tm);
      sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[0],time_ar);

     temp = ((((980 - rw[5])/196.0) * 1.035)/5)*100;
     sprintf(temp1, "%5.2f", temp);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[1], "LT1NPS");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[1], "%s", vl[0]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[2], "LT1BV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[2], "%s", vl[3]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[3], "LT1BV2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[3], "%s", vl[2]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[4], "LT1APC");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[4], "%s", vl[4]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[5], "LT1TMP");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[5], "%5.2f", temp);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[6], "LT1AV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[6], "%s", vl[1]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[7], "LT2NPS");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[7], "%s", vl[6]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[8], "LT2BV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[8], "%s", vl[9]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[9], "LT2BV2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[9], "%s", vl[8]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[10], "LT2APC");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[10], "%s", vl[10]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[11], "LT2AV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[11], "%s", vl[7]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[12], "LT3NPS");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[12], "%s", vl[11]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[13], "LT3BV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[13], "%s", vl[14]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[14], "LT3BV2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[14], "%s", vl[13]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[15], "LT3APC");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[15], "%s", vl[15]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[16], "LT3AV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[16], "%s", vl[12]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[17], "LT4NPS");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[17], "%s", vl[16]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[18], "LT4BV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[18], "%s", vl[19]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[19], "LT4BV2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[19], "%s", vl[18]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[20], "LT4APC");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[20], "%s", vl[20]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[21], "LT4AV1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[21], "%s", vl[17]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[22], "LT1CURR");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[22], "%s", lt1i1);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[23], "LT2CURR");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[23], "%s", lt2i2);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[24], "LT3CURR");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[24], "%s", lt3i3);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[25], "LT4CURR");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[25], "%s", lt4i4);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[26], "LT1POW");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[26], "%s", lt1p1);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[27], "LT2POW");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[27], "%s", lt2p2);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[28], "LT3POW");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[28], "%s", lt3p3);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[29], "LT4POW");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[29], "%s", lt4p4);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[30], "RFCH1ATB1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[30], "%s", vl[21]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[31], "RFCH1ATB2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[31], "%s", vl[22]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[32], "RFCH1ATB3");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[32], "%s", vl[23]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[33], "RFCH1ATB4");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[33], "%s", vl[24]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[34], "RFCH1ATB5");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[34], "%s", vl[25]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[35], "RFCH1ATB6");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[35], "%s", vl[26]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[36], "RFCH2ATB1");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[36], "%s", vl[27]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[37], "RFCH2ATB2");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[37], "%s", vl[28]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[38], "RFCH2ATB3");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[38], "%s", vl[29]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[39], "RFCH2ATB4");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[39], "%s", vl[30]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[40], "RFCH2ATB5");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[40], "%s", vl[31]);
    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[41], "RFCH2ATB6");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[41], "%s", vl[32]);

     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[42], "Sys_Status");
     if(p1>9 || p2>9 || p3>9)
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[42], "RETURN LINK DOWN");
     else if(i1<55 || i1>60 || i2<55 || i2>60 || i3<55 || i3>60)
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[42], "NOT OK");
     else
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[42], "OK");

     for(k=43;k<107;k++)
    {
      sprintf(OFC_MCM_Resp.resp_data.prmname[k],"Ch%2d", m);
      sprintf(OFC_MCM_Resp.resp_data.prmvalue[k], "%d", rw[m]);
      m++;
    }

     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[107], "sntstate");
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[107], state);

     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[108], "snttemp");
    _f_strcpy(OFC_MCM_Resp.resp_data.prmvalue[108], temps); // This value should be filled in bg_mon function. //

     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[109], "ch64_temp");
      sprintf(OFC_MCM_Resp.resp_data.prmvalue[109], "%d", rw[63]);  // This value should be filled in bg_mon function. //

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[110], "SmokeDet");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[110], "%d", smoke_detector);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[111], "ch62_smokedet");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[111], "%d", rw[61]);

    _f_strcpy(OFC_MCM_Resp.resp_data.prmname[112], "IntruDet");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[112], "%d", intruder_detector);

     _f_strcpy(OFC_MCM_Resp.resp_data.prmname[113], "ch61_intrdet");
     sprintf(OFC_MCM_Resp.resp_data.prmvalue[113], "%d", rw[60]);

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_302))   // set attn will set portbits at 32 bit o/p //
  {
    OFC_Attn1 =  _f_atoi(OFC_Wrpr_Cmd.cmd_data.prmvalue[0]);
    OFC_Attn2 =  _f_atoi(OFC_Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set OFC Attn : %d dB %d dB ..................", OFC_Attn1, OFC_Attn2);
    Set_ofc_atten(OFC_Attn2, OFC_Attn1);
  }

  else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_303))   // Init command will initalise Memory, Portbits, ADC and RTC //
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(OFC_Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_304))   // reset
  {
     printf("\n MCM is given Reset : Socket Closed !!");
     sock_close(&OFC_Socket);
     exit(0);
  }

  else if(!_f_strcmp(OFC_Wrpr_Cmd.cmd_elem.cmd_name,Cmd_305))   // setttime will set RTC //
  {
    Set_RTC(OFC_Wrpr_Cmd.cmd_elem.timestamp);
  }

  return 12;     // This return value will be stored in MCM_Resp.event //
}

void ofc_atten()
{
Set_ofc_atten(Ch2Att, Ch1Att);
}
/*    commented out on28.8.2020 bbk
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
*/
// new function uplaoded as charu sir code 28.8.2020
void Set_ofc_atten(int ch2, int ch1)
{
 int mask_16bit=0;
// char port_val1, port_val2;

 printf("Entering Set_OFC_attenuation: %d dB  %d dB\n ", ch1, ch2);

 sprintf(Attn1_Mon, "%d dB", ch1);
 sprintf(Attn2_Mon, "%d dB", ch2);

 BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 BitWrPortI(PBDR, &PBDRShadow, 0, 3);
 BitWrPortI(PBDR, &PBDRShadow, 0, 4);
 BitWrPortI(PBDR, &PBDRShadow, 0, 5);

 WrPortI(PADR, &PADRShadow, 0x00);

// if((ch2 & 0x01) == 0x01)
// ch1 = ch1 | 0x40;
// if((ch2 & 0x02) == 0x02)
// ch1 = ch1 | 0x80;
 ch1 &= 0x0f;
 ch2 &= 0x0f;

 mask_16bit |= 0x00Bf; //0x3fH original 14oct2019 //

  //printf("mask _16bit = 0x%2x \n",mask_16bit); // D5-D0
 mask_16bit |= (ch1 <<8);
  //printf("mask _16bit = 0x%2x \n",mask_16bit); // ch1_val is D11-D8

 mask_16bit |= (ch2 <<12); // holds 16 bit control word.
  //printf("mask _16bit = 0x%2x \n",mask_16bit); //ch2_val is D15-D12
 printf("\n Mask for setting OF Attenuation for CH1 and CH2 is %4x Hex \n",mask_16bit);

 port_val1 = (char)(mask_16bit & 0xff);
 port_val2 = (char)((mask_16bit>> 8) & 0xff);

 WrPortI(PADR, &PADRShadow, port_val1);
 BitWrPortI(PBDR, &PBDRShadow, 1, 2);     // latch 1st latch ic

 //WrPortI(PADR, &PADRShadow, 0x00);
 //ch2 = ch2 >> 2;
 WrPortI(PADR, &PADRShadow, port_val2);
 BitWrPortI(PBDR, &PBDRShadow, 1, 3);     // latch 2nd latch ic

// WrPortI(PADR, &PADRShadow, 0x00);
 BitWrPortI(PBDR, &PBDRShadow, 1, 4);
 BitWrPortI(PBDR, &PBDRShadow, 1, 5);
 BitWrPortI(PBDR, &PBDRShadow, 0, 6);
}

void form_alarm(int Alrm_id, int Alrm_lvl)
{
  OFC_MCM_Resp.code = 10;
  OFC_MCM_Resp.event = 100;

  OFC_MCM_Resp.MCM_alarm.id = Alrm_id;
  OFC_MCM_Resp.MCM_alarm.level = Alrm_lvl;
  _f_strcpy(OFC_Wrpr_Cmd.cmd_elem.cmd_name,"domon");
  command_process_ofcsnt();
  sock_write(&OFC_Socket, &OFC_MCM_Resp, MCM_Resp_Size);
}

void ofc_alarm_handler()
{
if((((980 - rw[3])/196.0) * 1.035) > -1.051)       //Laser1 Bias voltage1 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[2])/196.0) * 1.035) > -2.653)       //Laser1 Bias voltage2 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[3])/196.0) * 1.035) < -1.122)       //Laser1 Bias voltage1 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[2])/196.0) * 1.035) < -3.222)       //Laser1 Bias voltage2 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[4])/196.0) * 1.035) > 2.40)       //Laser1 APC unlock
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 APC unlock");
form_alarm(3,1);
}
if((((980 - rw[5])/196.0) * 0.05175) >25)       //Laser1 temp. > 25
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser1 temp. is greater than 25 degree C");
form_alarm(4,2);
}

if((((980 - rw[9])/196.0) * 1.035) > -1.051)       //Laser2 Bias voltage1 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[8])/196.0) * 1.035) > -2.653)       //Laser2 Bias voltage2 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[9])/196.0) * 1.035) < -1.122)       //Laser2 Bias voltage1 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[8])/196.0) * 1.035) < -3.222)       //Laser2 Bias voltage2 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser2 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[10])/196.0) * 1.035) > 2.40)       //Laser2 APC unlock
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser2 APC unlock");
form_alarm(3,1);
}

if((((980 - rw[14])/196.0) * 1.035) > -1.051)       //Laser3 Bias voltage1 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[13])/196.0) * 1.035) > -2.653)       //Laser3 Bias voltage2 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[14])/196.0) * 1.035) < -1.122)       //Laser3 Bias voltage1 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[13])/196.0) * 1.035) < -3.222)       //Laser3 Bias voltage2 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser3 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[15])/196.0) * 1.035) > 2.40)       //Laser3 APC unlock
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser3 APC unlock");
form_alarm(3,1);
}

/*if((((980 - rw[19])/196.0) * 1.035) > -1.051)       //Laser4 Bias voltage1 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage1 high");
form_alarm(1,1);
}
if((((980 - rw[18])/196.0) * 1.035) > -2.653)       //Laser4 Bias voltage2 high
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage2 high");
form_alarm(1,1);
}
if((((980 - rw[19])/196.0) * 1.035) < -1.122)       //Laser4 Bias voltage1 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage1 low");
form_alarm(2,1);
}
if((((980 - rw[18])/196.0) * 1.035) < -3.222)       //Laser4 Bias voltage2 low
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser4 Bias voltage2 low");
form_alarm(2,1);
}
if((((980 - rw[20])/196.0) * 1.035) > 2.40)       //Laser4 APC unlock
{
_f_strcpy(OFC_MCM_Resp.MCM_alarm.description,"Laser4 APC unlock");
form_alarm(3,1);
}    */

}

// ****** MCM reset function will reset MCM card
void MCM_Reset()
{
    printf("\n\n****************************\n MCM Reset !!\n****************************\n");
    sock_close(&OFC_Socket);
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
//Turn ON and OFF MILTEC PC using PS_ON
void Set_MON()
{ char temp;

//printf("Entering Miltec PC POWER ON/OFF ..Set_FDB()\n");

  if(!(MON))
   MON = Temp_MON;

//printf("OFF \t FDB1 \t control port value 0x%x FDB = %x \n",port_val1,FDB);
  if(MON == 1)
   {
     //original FDB code
     //WrPortI(GCDR,NULL,0x00);
     //strcpy(FDB_Mon,"Off");
/*
//printf("OFF \t FDB1 \t control port value 0x%x FDB = %x \n",port_val1,FDB);
     port_val1 |= 0x40;
//printf("after \t OFF \t FDB1 \t control port value 0x%x FDB = %x \n",port_val1,FDB);

	 BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
 	 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 	 BitWrPortI(PBDR, &PBDRShadow, 0, 3);
 	 BitWrPortI(PBDR, &PBDRShadow, 0, 4);
 	 BitWrPortI(PBDR, &PBDRShadow, 0, 5);

 	 WrPortI(PADR, &PADRShadow, port_val1);
    BitWrPortI(PBDR, &PBDRShadow, 1, 2);     // latch 1st latch ic

    BitWrPortI(PBDR, &PBDRShadow, 1, 4);
    BitWrPortI(PBDR, &PBDRShadow, 1, 5);
    BitWrPortI(PBDR, &PBDRShadow, 0, 6);
*/

/*
//commented out on 9 jan 2020
    port_val1 |= 0x40;   // bit is set to high
    //port_val1 &= 0xBF;   // bit is set to low
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    //Ms_delay(500);  //using switch

    port_val1 &= 0xBF;
    //port_val1 |= 0x40;   // bit is set to high
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    Ms_delay(5);  //using switch

    port_val1 |= 0x40;   // bit is set to high
    //port_val1 &= 0xBF;   // bit is set to low
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    //Ms_delay(500);  //using switch
*/

//code : 9jan 2020
    port_val1 &= 0xBF;   // D6 bit is set to low
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    port_val1 |= 0x40;   // D6 bit is set to high
    port_val1 &= 0x7f;   // D7 bit is set to low

    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    Ms_delay(1000);  //using switch


    port_val1 &= 0xBF;   // D6 bit is set to low
    port_val1 |= 0x80;   // D7 bit is set to high
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

   }
  else if(MON == 2)
   {
/*
//printf("ON \t FDB2 \t control port value 0x%x FDB = %x \n",port_val1,FDB);
     port_val1 &= 0xBF;
//printf("after \t ON \t FDB1 \t control port value 0x%x FDB = %x \n",port_val1,FDB);

     BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
     BitWrPortI(PBDR, &PBDRShadow, 0, 2);
     BitWrPortI(PBDR, &PBDRShadow, 0, 3);
     BitWrPortI(PBDR, &PBDRShadow, 0, 4);
     BitWrPortI(PBDR, &PBDRShadow, 0, 5);

     WrPortI(PADR, &PADRShadow, port_val1);
     BitWrPortI(PBDR, &PBDRShadow, 1, 2);     // latch 1st latch ic

     BitWrPortI(PBDR, &PBDRShadow, 1, 4);
     BitWrPortI(PBDR, &PBDRShadow, 1, 5);
     BitWrPortI(PBDR, &PBDRShadow, 0, 6);

     //original FDB code
     //WrPortI(GCDR,NULL,0x05);
     //strcpy(FDB_Mon,"On");
*/

/*
//commented out on 9 jan 2019
    port_val1 |= 0x40;
    //port_val1 &= 0xBF;
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    //Ms_delay(500);  //using switch

    port_val1 &= 0xBF;
    //port_val1 |= 0x40;
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    Ms_delay(500);  //using switch

    port_val1 |= 0x40;
    //port_val1 &= 0xBF;
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15
*/

    //code : 9jan 2020
    port_val1 &= 0xBF;   // D6 bit is set to low
    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    port_val1 |= 0x40;   // D6 bit is set to high
    port_val1 &= 0x7f;   // D7 bit is set to low

    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15

    Ms_delay(1000);  //using switch


    port_val1 &= 0xBF;   // D6 bit is set to low
    port_val1 |= 0x80;   // D7 bit is set to high

    BitWrPortI(PBDR,&PBDRShadow,1,6); //set PB6 high to disable OE\ of U14   - 09july2016
	 BitWrPortI(PBDR,&PBDRShadow,0,2); //set PB2 low to disable latch1 U14 D00-07
    BitWrPortI(PBDR,&PBDRShadow,0,3); //set PB3 low to disable latch2 U13 D08-15

    WrPortI(PADR,&PADRShadow,port_val1);   // write first byte of data
    BitWrPortI(PBDR,&PBDRShadow,1,2); //set PB2 high to enable latch U14 D00-07

    BitWrPortI(PBDR,&PBDRShadow,0,6); //set PB6 low to OE\ of latch U14 D00-15
   }
 Temp_MON = MON;
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
  #ifdef USE_HTTP_UPLOAD
  writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
  #endif
  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP,IFS_END);

  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);
  }

}