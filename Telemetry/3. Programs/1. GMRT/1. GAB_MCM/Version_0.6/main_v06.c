// ********************************************************************************** //
// ***** Well structured and web integrated program for In-House development MCM **** //
// ******************************** Version : V0.6 ********************************** //
// ****************************** Date : 12/11/2014 ********************************* //
// ************************* Author : Naresh M. Sisodiya **************************** //
// ** This version includes code for SENTINEL, FE, OF, GAB, FPS and MCM Self Test. ** //
// ********************************************************************************** //


#class    auto
#memmap   xmem

#use      "rcm43xx.lib"	             // Use with RCM 4300

#use      "adc_ads7870.lib"
#include  "define.c"
#include  "Init.c"
#include  "fe_bits.c"
#include  "gab_bits.c"
#use      "dcrtcp.lib"
#use      "http.lib"
#use      "spi.lib"
#use      "fat16.lib"


SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".html","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler)
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/",MCM_Mon_html),
   SSPEC_RESOURCE_XMEMFILE("/MCM_Mon.html",MCM_Mon_html),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Sum.xml",Mon_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Raw.xml",Mon_Raw_xml)
SSPEC_RESOURCETABLE_END


static tcp_Socket Socket;


void Ms_delay(unsigned int);
void cmd_exe();

void bg_mon();
void Rd_MCM_Clk();
void Read_RTC();

void Set_DMask(char *, char *);
void Set_SS();
void Set_FDB();
void Set_FDV();
void Set_RTC(char *);
void Set_LO(unsigned long, char);
void Set_RefLO(unsigned long, char);

int  atox(char *);


void sentinel_mon();
void sentinel_set();
void sentinal_init();
void sentinal_reset();
void sen_mon_sum();


void front_end_mon();
void front_end_set();
void front_end_init();
void front_end_reset();
void fe_mon_sum();


void fiber_optics_mon();
void fiber_optics_set();
void fiber_optics_init();
void fiber_optics_reset();
void of_mon_sum();


void back_end_mon();
void back_end_set();
void back_end_init();
void back_end_reset();
void be_mon_sum();

void fps_mon();
void fps_set();
void fps_init();
void fps_reset();
void fps_mon_sum();


void mcm_mon();
void mcm_set();
void mcm_init();
void mcm_reset();




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
   int i; // temperory used to print received structure //

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
   printf("\nHost hex address %lx\n",host);

   port = atoi(SERVER_PORT);

   printf("Attempting to open '%s' on port %u\n\r", SERVER_ADDR, port );

   while( ! tcp_open(&Socket, 0, host, port, NULL))
   {
     printf("\nUnable to open TCP session");
     tcp_tick(NULL);
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
   printf("\nSocket Established");

   if(sock_established(&Socket))
   {
    conn_flag = 1;
    read_bytes = 0;
    do{
       http_handler();
       if(sock_bytesready(&Socket) > 0)
       {
         read_bytes = sock_read(&Socket, &pc_cmd, pc_cmd_size);

         printf("\nMCM has read command of %d bytes",read_bytes);
         printf("\n Sequence Number : %ld",pc_cmd.Sequence_No);
         printf("\n Time Stamp : %s",pc_cmd.TimeStamp);
         printf("\n System Name : %s",pc_cmd.System_Name);
         printf("\n Operation Name : %s",pc_cmd.Operation_Name);

         for(i=0; i< pc_cmd.Total_Parameter; i++)
         {
          printf("\n Parameter Name : %s",pc_cmd.Parameter_Name[i]);
          if(strlen(pc_cmd.Parameter_Name[i])<5)
          printf("\t");
          printf("\t\t\t%s",pc_cmd.Argument_Ch1[i]);
          printf("\t%s",pc_cmd.Argument_Ch2[i]);
         }

         cmd_exe();  // Execution of command in this function //

         http_handler();

         bg_mon();

         write_bytes = sock_write(&Socket, &mcm_resp, mcm_resp_size);
         printf("\nMCM has sent response of %d bytes \n",write_bytes);

         http_handler();

         Struct_Init();
       }
       read_bytes = 0;
       yield;

       if(conn_flag == 0)
       {
         printf("Conn_flag = 0");
         break;
       }

       if(!strcmp(reset,"1"))
       {
         printf("\nreset received....");
         break;
       }

      }while(tcp_tick(&Socket));    // This loop continuously listens over socket for incoming command //
   }

   sock_close(&Socket);
   printf("\nConnection closed for port-%d",port);

   strcpy(reset,"0");

   Ms_delay(4000);

   return 0;
}


// ..... Main start .... //
main()
{
  auto unsigned long scn_dl, ping_dl, resp_dl;
  longword seq, tmp_seq, time_out;  // variables for pinging //
  seq = 0;

  brdInit();
  SPIinit();
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();
  Init();
  rd_MCM_addr();

  tcp_reserveport(80);

  while(1)
  {

   http_handler();

   costate    // This will open client socket at port 5000 //
    {
      wfd mcm_clnt();
    }


   costate    // This costatement run web server in parallel //
    {
      http_handler();
      tcp_tick(NULL);
    }


    costate    // This will do background monitoring in parallel at every SCAN_DELAY miliseconds //
    {
      scn_dl = MS_TIMER + SCAN_DELAY;

      Ms_delay(1);  // This delay is necessary for proper working of following loop. //

      while(((scn_dl - MS_TIMER) > 0) && ((scn_dl - MS_TIMER) < 900))
      {
        http_handler();
        yield;
      }

       bg_mon();
       Read_RTC();
    }


    costate    // Ping support added on 29/10/2013, seq & conn_flag shd be added //
    {
       ping_dl = MS_TIMER + 5000;
       Ms_delay(2);

       while(((ping_dl - MS_TIMER) > 0) && ((ping_dl - MS_TIMER) < 5000))
        {
          yield;
        }

	    if(!(_ping(inet_addr(SERVER_ADDR), seq++)))    // Sending first ping packet //
        printf("\nPing Send Sucessfully;  Sequence ID: %d;", seq);

       resp_dl = MS_TIMER + 2000;    //  Wait for first try ping reply  //
       Ms_delay(1);
       while(((resp_dl - MS_TIMER) > 0) && ((resp_dl - MS_TIMER) < 2000))
       {
          yield;
       }

       time_out=_chk_ping(inet_addr(SERVER_ADDR),&tmp_seq);    // Checking for first try ping reply //
		 if(time_out == 0xffffffff)
       {
         printf("\nFirst try ping reply not received....");
         seq--;
         if(!(_ping(inet_addr(SERVER_ADDR), seq++)))     // This is retry for ping if not received //
         {
           printf("\nSecond try Ping Send Sucessfully;  Sequence ID: %d;", seq);
         }

         resp_dl = MS_TIMER + 2000;    //  Wait for second try ping reply  //
         Ms_delay(1);
         while(((resp_dl - MS_TIMER) > 0) && ((resp_dl - MS_TIMER) < 2000))
         {
           yield;
         }

         time_out=_chk_ping(inet_addr(SERVER_ADDR),&tmp_seq);   // Checking for retry //
			if(time_out == 0xffffffff)
         {
           printf("\nSecond try Ping reply not received......");
           conn_flag = 0;
         }
       }
       else
       {
         printf("\nPing received..");
         conn_flag = 1;
       }
    }

  }

}



void cmd_exe()
{

  mcm_resp.Response_Type = 1;

  mcm_resp.Sequence_No = pc_cmd.Sequence_No;

  strcpy(mcm_resp.TimeStamp, pc_cmd.TimeStamp);

  strcpy(mcm_resp.System_Name, pc_cmd.System_Name);

  // strcpy(mcm_resp.Mon_Raw[0], "1000");

  strcpy(mcm_resp.Mon_Sum[0], "No summery !!");

  // strcpy(mcm_resp.Response_Message[0],"MCM received command");

  if(!strcmp(pc_cmd.System_Name,System_1))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : Sentinel");

    strcpy(webpg_mon.sys_name,"SENTINEL");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Sentinal Monitoring");

       sentinel_mon();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Sentinal Controlling");

       sentinel_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Sentinal Initialize");

       sentinal_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Sentinal Reset");

       sentinal_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"Sentinal Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }


  else if(!strcmp(pc_cmd.System_Name,System_2))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : Front End");

    strcpy(webpg_mon.sys_name,"FRONT END");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Front End Monitoring");

       front_end_mon();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Front End Controlling");

       front_end_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Front End Initialize");

       front_end_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Front End Reset");

       front_end_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"Front End Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }


  else if(!strcmp(pc_cmd.System_Name,System_3))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : Fiber Optics");

    strcpy(webpg_mon.sys_name,"FIBER OPTICS");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Fiber optics Monitoring");

       fiber_optics_mon();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Fiber optics Controlling");

       fiber_optics_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Fiber optics Initialize");

       fiber_optics_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Fiber optics Reset");

       fiber_optics_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"Fiber optics Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }


  else if(!strcmp(pc_cmd.System_Name,System_4))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : Back End");

    strcpy(webpg_mon.sys_name,"ANALOG BACK END");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Back End Monitoring");

       back_end_mon();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Back End Controlling");

       back_end_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Back End Initialize");

       back_end_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Back End Reset");

       back_end_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"Back End Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }


  else if(!strcmp(pc_cmd.System_Name,System_5))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : FPS");

    strcpy(webpg_mon.sys_name,"FPS SYSTEM");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : FPS Sys Monitoring");

       fps_mon();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : FPS Sys Controlling");

       fps_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : FPS Sys Initialize");

       fps_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : FPS Sys Reset");

       fps_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"FPS Sys Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }



  else if(!strcmp(pc_cmd.System_Name,System_6))
  {
    strcpy(mcm_resp.Response_Message[0],"System validated : Test Sys");

    strcpy(webpg_mon.sys_name,"TEST SYSTEM");

     if(!strcmp(pc_cmd.Operation_Name,Opr_1))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Test Sys Monitoring");

       mcm_mon();

     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_2))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Test Sys Controlling");

       mcm_set();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_3))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Test Sys Initialize");

        mcm_init();
     }

     else if(!strcmp(pc_cmd.Operation_Name,Opr_4))
     {
       strcpy(mcm_resp.Response_Message[1],"Command validated : Test Sys Reset");

        mcm_reset();
     }

     else
     {
       strcpy(mcm_resp.Response_Message[1],"Test Sys Command Invalid");

       mcm_resp.Total_Resp_Msg = 2;
     }
  }

  else
  {
     strcpy(mcm_resp.Response_Message[0],"System Invalid : ??");

     mcm_resp.Total_Resp_Msg = 1;
  }


}




// ****** Doing background monitoring tasks and event generation ****** //

void bg_mon()
{
  int channel, mux_sel, set_mux_sel;
  float tmp, volt;

  for(channel=0; channel<4; channel++)
  {
   for(mux_sel=0; mux_sel<16; mux_sel++)
    {
      set_mux_sel = mux_sel*16;

      WrPortI(PEDR, &PEDRShadow, ((RdPortI(PEDR)&0x0f) | set_mux_sel));

      mon_arr[channel*16 + mux_sel] = (anaIn(channel, SINGLE, GAINSET));
      sprintf(mcm_resp.Mon_Raw[channel*16 + mux_sel], "%d", mon_arr[channel*16 + mux_sel]);

      volt = (((980 - mon_arr[channel*16 + mux_sel])/196.0));
      sprintf(mon_vlt[channel*16 + mux_sel],"%5.2f",volt);

    }
  }

}




// **** After initialising port bits in PortInit(), 32 bit digital mask will be set here **** //

void Set_DMask(char * arg1, char * arg2)
{
  char i;
  int MSB_2, LSB_2, MSB_1, LSB_1;

  strcpy(msw,arg1);
  strcpy(lsw,arg2);

  LSB_2 = atox(msw + 2);
  MSB_2 = atox(msw);    // don't need any shift operation, because atox is done only for two poiter locations //

  LSB_1 = atox(lsw + 2);
  MSB_1 = atox(lsw);

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 2);
  BitWrPortI(PBDR, &PBDRShadow, 0, 3);
  BitWrPortI(PBDR, &PBDRShadow, 0, 4);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);

  //printf("\nMSB_2 LSB_2 MSB_1 LSB_1 : %02x %02x   %02x %02x", MSB_2, LSB_2, MSB_1, LSB_1);

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

  for(i=0; i<4; i++)
   msw[i] = toupper(msw[i]);
  for(i=0; i<4; i++)
   lsw[i] = toupper(lsw[i]);

}




// ***** Setting Spectrum Spreader ***** //

void Set_SS()
{

  if(SS == 0)
   {
     WrPortI(GCM1R,NULL,0x00);
     strcpy(SS1_Mon,"Off");
     strcpy(SS2_Mon,"Off");
     Ms_delay(10);
   }
  else if(SS == 1)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x40);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Normal");
     strcpy(SS2_Mon,"Normal");
   }
  else if(SS == 2)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x00);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Normal");
     strcpy(SS2_Mon,"Strong");
   }
  else if(SS == 3)
   {
     WrPortI(GCM1R,NULL,0x00);
     Ms_delay(10);
     WrPortI(GCM0R,NULL,0x80);
     WrPortI(GCM1R,NULL,0x80);
     strcpy(SS1_Mon,"Strong");
     strcpy(SS2_Mon,"Normal");
   }
}




// ***** Setting Frequency Doubler ***** //

void Set_FDB()
{

  if(FDB == 0)
   {
     WrPortI(GCDR,NULL,0x00);
     strcpy(FDB_Mon,"Off");
   }
  else if(FDB == 1)
   {
     WrPortI(GCDR,NULL,0x05);
     strcpy(FDB_Mon,"On");
   }
}




// ***** Setting Frequency Divider ***** //

void Set_FDV()
{

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
  else if(FDV == 4)
   {
     WrPortI(GCSR,NULL,0x19);
     strcpy(FDV_Mon,"4");
   }
  else if(FDV == 6)
   {
     WrPortI(GCSR,NULL,0x1D);
     strcpy(FDV_Mon,"6");
   }
  else if(FDV == 8)
   {
     WrPortI(GCSR,NULL,0x01);
     strcpy(FDV_Mon,"8");
   }
}




// ***** Reading MCM Clock Frequency ***** //

void Rd_MCM_Clk()
{

  MCM_Clk_Frq = (30.0*(FDB+1))/FDV;
  sprintf(MCM_Clk,"%5.2f MHz",MCM_Clk_Frq);
}




// ***** This function will set RTC ***** //

void Read_RTC()
{
  unsigned long t0;

  t0 = read_rtc();   // This function provides 32 bits RTC value in seconds //

  mktm(&rtc,t0);    // This function converts seconds into date & time, and fills them in rtc structure //

  sprintf(RTC_time,"%02d : %02d : %02d",rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
  sprintf(RTC_date,"%02d - %02d - %04d",rtc.tm_mday,rtc.tm_mon,(1900+rtc.tm_year));
}




// **** This function will set RTC to value provided within Init command **** //

void Set_RTC(char * rtc_str)
{
  char set_rtc[32], month1[5];
  int i;

  char month[][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

  strcpy(set_rtc,rtc_str);

  printf("\nRTC is set to : %s",rtc_str);
  printf("\nRTC is set to : %s",set_rtc);

  // Setting RTC is hard-coded, follows timestamp formate : dd-Mon-yyyy hh:mm:ss//

  rtc_str = set_rtc;

  set_rtc[2]=0;
  set_rtc[6]=0;
  set_rtc[11]=0;
  set_rtc[14]=0;
  set_rtc[17]=0;
  set_rtc[20]=0;


  rtc_str  = rtc_str + 18;
    rtc.tm_sec = atoi(rtc_str);            // storing second //

  rtc_str  = rtc_str - 3;
    rtc.tm_min = atoi(rtc_str);            // storing minute //

  rtc_str  = rtc_str - 3;
    rtc.tm_hour = atoi(rtc_str);           // storing hour //


  rtc_str  = rtc_str - 5;
   rtc.tm_year =  atoi(rtc_str) - 1900;    // storing year //


  rtc_str  = rtc_str - 4;
   for(i=0; i<12; i++)
    {
     if(!strcmp(rtc_str,month[i]))
      sprintf(month1, "%02d", i);      // storing month //
      rtc.tm_mon = atoi(month1);
    }

  rtc_str  = rtc_str - 3;
   rtc.tm_mday =  atoi(rtc_str);           // storing day //

  tm_wr(&rtc);								  	    // Writing to RTC //
  SEC_TIMER = mktime(&rtc);                // mktime() is required to read current value correctly using tm_read()//
}




// ***************************** Setting LO Frequency ***************************//
// ** FSW accepts frequency in KHz or MHz. As 500KHz is stepsize, KHz is used ** //
// ***************************************************************************** //

void Set_LO(unsigned long lo, char Ch)
{
  char LO_Arr[10], Temp_Arr[10],rem[10];
  int padding, cntr_1, i;


  for(cntr_1=0; cntr_1<9; cntr_1++)
	LO_Arr[cntr_1] = '0';

  printf("\n unsigned lo = %ld",lo);

  i=0;
  do
  {
   rem[i] = (lo%16)+48;
   if(rem[i]>57)
   rem[i] = rem[i]+7;
   lo = lo/16;
   i++;
  } while(lo>0);

  rem[i]= 0 ;

  for(cntr_1=0; cntr_1 < strlen(rem); cntr_1++)
  {
    Temp_Arr[i-1] = rem[cntr_1];
    i--;
  }
  Temp_Arr[cntr_1]= 0;

  printf("\n After converting to hex : %s",Temp_Arr);

  padding = 9 - strlen(Temp_Arr);

  strcpy(LO_Arr + padding, Temp_Arr);

  LO_Arr[0] = 'K';      // Writting 'K' to set frequency in KHz //


  for(cntr_1=0; cntr_1 < strlen(LO_Arr); cntr_1++)
  {
    if((LO_Arr[cntr_1] > 96) && (LO_Arr[cntr_1] < 123))
     LO_Arr[cntr_1] = LO_Arr[cntr_1] - 32;
  }


 strcpy(Temp_Arr, LO_Arr);

 printf("\n Sending over SPI (LO_Arr) : %s\n",LO_Arr);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);

 BitWrPortI(PEDR, &PEDRShadow, 0, Ch);

 for(i=0;i<500;i++);


 // **** Eventhough 72 bits (9 bytes) are required to send over SPI to set LO frequency ********************** //
 // **** We are purposefully sending 80 bits (10 bytes), so that FSW can get SPI data and set LO properly **** //
 // **** However this extra byte is not required while setting Reference LO ********************************** //

 SPIWrite(LO_Arr,10);

 for(i=0;i<500;i++);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);

}




// ******* Setting Reference LO *******//

void Set_RefLO(unsigned long reflo, char ch)
{
  char Ref_LO_Arr[5], i;

  //printf("\nReceived Ref LO : %ld   for channel : %d", reflo, ch);

  for(i=0; i<sizeof(Ref_LO_Arr); i++)
  {
    Ref_LO_Arr[i] = 0;
  }

  Ref_LO_Arr[0] = 0x52;          // Writting 'R' to set Reference LO //

  Ref_LO_Arr[1] = reflo * 2;     // Writting divisor value as required by FSW //

  BitWrPortI(PEDR, &PEDRShadow, 1, ch);

  BitWrPortI(PEDR, &PEDRShadow, 0, ch);

  for(i=0; i<100; i++);

  // As per FSW datasheet 5 bytes are required to send over SPI to set Ref LO **** //

  SPIWrite(Ref_LO_Arr, 5);

  for(i=0; i<100; i++);

  BitWrPortI(PEDR, &PEDRShadow, 1, ch);

}




// **** Converting String to Hex value **** //

int atox(char *msk)
{
  int hex_value;
  char mask[3];

  strcpy(mask,msk);

  if((mask[0] < 58) && (mask[0] > 47))
   mask[0] = mask[0] - '0';
  else if((mask[0] > 64) && (mask[0] < 71))
   mask[0] = mask[0] - 'A' + 10;
  else if((mask[0] > 96) && (mask[0] < 103))
   mask[0] = mask[0] - 'a' + 10;
  else
   mask[0] = 0;

  if((mask[1] < 58) && (mask[1] > 47))
   mask[1] = mask[1] - '0';
  else if((mask[1] > 64) && (mask[1] < 71 ))
   mask[1] = mask[1] - 'A' + 10;
  else if((mask[1] > 96) && (mask[1] < 103))
   mask[1] = mask[1] - 'a' + 10;
  else
   mask[1] = 0;


  hex_value = ((mask[0] * 16) + mask[1]);

  return hex_value;
}




/****************************************************************/
/****************** Tasks For Sentinel System *******************/
/****************************************************************/

void sentinel_mon()
{

  // Code for Back End Monitoring and Decoding //

  sprintf(mcm_resp.Response_Message[2],"Sentinel Monitoring Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void sentinel_set()
{
  char i,j;
  char Para_invalid[15];

  for(i=0; i < pc_cmd.Total_Parameter; i++)
  {
   for(j=0; j < SEN_Para_No; j++)
   {
    if(!strcmp(pc_cmd.Parameter_Name[i], SEN_Para[j]))
    {
      switch(j)
      {
        case 0 :  sprintf(mcm_resp.Response_Message[2],"Digital Mask : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);

                  strcpy(webpg_mon.sys_para[0],"Digital Mask");
                  strcpy(webpg_mon.arg1[0],pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.arg2[0],pc_cmd.Argument_Ch2[i]);

                  Set_DMask(pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);  // Setting 32 bit digital Mask for Sentinel Settings //

                  break;

        default : break;
      }
      Para_invalid[i] = 0;
      break;
    } // if end
    else
    {
      Para_invalid[i] = 1;
    }
   }  // for - j end
  }   // for - i end

  for(i=0; i < SEN_Para_No; i++)
  {
   if(Para_invalid[i])
   {
     sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
   }
  }
  mcm_resp.Total_Resp_Msg =  SEN_Para_No + 2;
}


void sentinal_init()
{
  Set_RTC(pc_cmd.TimeStamp);
  sprintf(mcm_resp.Response_Message[2],"Sentinel Initialized");

  mcm_resp.Total_Resp_Msg = 3;

}


void sentinal_reset()
{
  sprintf(mcm_resp.Response_Message[2],"Sentinel Reset done");
  mcm_resp.Total_Resp_Msg = 3;

}


void sen_mon_sum()
{

}




/****************************************************************/
/****************** Tasks For Front End System ******************/
/****************************************************************/

void front_end_mon()
{

  // Code for Back End Monitoring and Decoding //

  sprintf(mcm_resp.Response_Message[2],"Front End Monitoring Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void front_end_set()
{
 char Para_invalid[15];
 char i,j,k;

 int cb_lsw1, cb_lsw2, fe_lsb, fe_band;
 char cb_lsb[3], cb_msb[3], cb_lsw[5], cb_msw[5];
 char fe_lsw[5], fe_msw[5];

 cb_lsw1 = 0;
 cb_lsw2 = 0;
 fe_lsb = 0;

 strcpy(cb_msw,"0000");
 strcpy(fe_lsw,"0000");
 strcpy(fe_msw,"0000");

 for(i=0; i < pc_cmd.Total_Parameter; i++)
  {
   for(j=0; j < FE_Para_No; j++)
   {

    tcp_tick(&Socket);

    if(!strcmp(pc_cmd.Parameter_Name[i], FE_Para[j]))
    {
      switch(j)
      {
        case 0 :  sprintf(mcm_resp.Response_Message[2],"Band Selected : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[0],"Band Selection");
                  sprintf(webpg_mon.arg1[0],"%s",pc_cmd.Argument_Ch1[i]);
                  sprintf(webpg_mon.arg2[0],"%s",pc_cmd.Argument_Ch2[i]);

                  for(k=0; k<6; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], frq_band[k])) )
                     {
                        cb_lsw1 = cb_lsw1 | fb_bits[k];
                        fe_band = k+1;
                        break;
                     }
                  }

                  for(k=0; k<6; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], frq_band[k])) )
                     {
                        cb_lsw2 = cb_lsw2 | fb_bits[k];
                        break;
                     }
                  }
                  break;

        case 1 :  sprintf(mcm_resp.Response_Message[3],"Solar Attenuation : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[1],"Solar Attenuation");
                  sprintf(webpg_mon.arg1[1],"%s",pc_cmd.Argument_Ch1[i]);
                  sprintf(webpg_mon.arg2[1],"%s",pc_cmd.Argument_Ch2[i]);

                  for(k=0; k<5; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], solar_attn[k])) )
                     {
                        cb_lsw1 = cb_lsw1 | sa_bits[k];
                        break;
                     }
                  }

                  for(k=0; k<5; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], solar_attn[k])) )
                     {
                        cb_lsw2 = cb_lsw2 | sa_bits[k];
                        break;
                     }
                  }

        				break;

        case 2 :  sprintf(mcm_resp.Response_Message[4],"Channel : %s",pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.sys_para[2],"Channel");
                  strcpy(webpg_mon.arg1[2],pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.arg2[2]," ");

                  for(k=0; k<2; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], polar[k])) )
                     {
                        cb_lsw1 = cb_lsw1 | pl_bits[k];
                        break;
                     }
                  }

                  for(k=0; k<2; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], polar[k])) )
                     {
                        cb_lsw2 = cb_lsw2 | pl_bits[k];
                        break;
                     }
                  }
                  break;

        case 3 :  sprintf(mcm_resp.Response_Message[5],"Sub Band Selected : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[3],"Sub-Band Selected");
                  sprintf(webpg_mon.arg1[3],"%s",pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.arg2[3]," ");

                  for(k=0; k<4; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], band_sel[k])) )
                     {
                        fe_lsb = fe_lsb | bs_bits[k];
                        break;
                     }
                  }
                  break;

        case 4 :  sprintf(mcm_resp.Response_Message[6],"RF Power : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[4],"RF Power");
                  strcpy(webpg_mon.arg1[4],pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.arg2[4]," ");

                  for(k=0; k<2; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], rf[k])) )
                     {
                        fe_lsb = fe_lsb | rf_bits[k];
                        break;
                     }
                  }
                  break;

       case 5 :  sprintf(mcm_resp.Response_Message[7],"Calibration Noise : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                 strcpy(webpg_mon.sys_para[5],"Calibration Noise");
                 strcpy(webpg_mon.arg1[5],pc_cmd.Argument_Ch1[i]);
                 strcpy(webpg_mon.arg2[5]," ");

                  for(k=0; k<4; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], noise_cal[k])) )
                     {
                        fe_lsb = fe_lsb | nc_bits[k];
                        break;
                     }
                  }

                  break;

       default : break;

      }

      Para_invalid[i] = 0;
      break;
    } // if end
    else
    {
      Para_invalid[i] = 1;
      sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
    }
   }  // for - j end

  }   // for - i end


  //-------- Common Box should be set before setting the FE Box -------- //

  Set_DMask("0000","0000"); //**** Initialize 32 bit digital port as 0000 0000 ****//

  sprintf(cb_lsb,"%02x",(cb_lsw1 & 0x00FF));
  sprintf(cb_msb,"%02x",((cb_lsw1 & 0xFF00)>>8));


  //printf("\nChannel 1  ##  cb_msb : %s     cb_lsb : %s", cb_msb, cb_lsb);

  sprintf(cb_lsw, "07%s",cb_lsb);
  Set_DMask(cb_msw,cb_lsw);
  sprintf(cb_lsw, "00%s",cb_lsb);
  Set_DMask(cb_msw,cb_lsw);

  sprintf(cb_lsw, "08%s",cb_msb);
  Set_DMask(cb_msw,cb_lsw);
  sprintf(cb_lsw, "00%s",cb_msb);
  Set_DMask(cb_msw,cb_lsw);


  tcp_tick(&Socket);


  sprintf(cb_lsb,"%02x",(cb_lsw2 & 0x00FF));
  sprintf(cb_msb,"%02x",((cb_lsw2 & 0xFF00)>>8));

  //printf("\nChannel 2  ##  cb_msb : %s     cb_lsb : %s", cb_msb, cb_lsb);

  sprintf(cb_lsw, "09%s",cb_lsb);
  Set_DMask(cb_msw,cb_lsw);
  sprintf(cb_lsw, "00%s",cb_lsb);
  Set_DMask(cb_msw,cb_lsw);

  sprintf(cb_lsw, "0A%s",cb_msb);
  Set_DMask(cb_msw,cb_lsw);
  sprintf(cb_lsw, "00%s",cb_msb);
  Set_DMask(cb_msw,cb_lsw);


  //-------- Setting the FE Box -------- //

  sprintf(fe_lsw, "%02x%02x", fe_band, fe_lsb);
  Set_DMask(fe_msw,fe_lsw);

  sprintf(fe_lsw, "00%02x", fe_lsb);
  Set_DMask(fe_msw,fe_lsw);

  strcpy(msw, fe_msw);
  strcpy(lsw, fe_lsw);


  /* for(i=0; i < FE_Para_No; i++)
  {
   if(Para_invalid[i])
   {
     sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
   }
  } */

  mcm_resp.Total_Resp_Msg =  FE_Para_No + 2;
}


void front_end_init()
{
  Set_RTC(pc_cmd.TimeStamp);
  sprintf(mcm_resp.Response_Message[2],"Front End Initialized");
  mcm_resp.Total_Resp_Msg = 3;
}


void front_end_reset()
{
  sprintf(mcm_resp.Response_Message[2],"Front End Reset Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void fe_mon_sum()
{

}




/****************************************************************/
/**************** Tasks For Fiber Optics System *****************/
/****************************************************************/

void fiber_optics_mon()
{

  // Code for Back End Monitoring and Decoding //

  sprintf(mcm_resp.Response_Message[2],"Fiber Optics Monitoring Done");

  sprintf(mcm_resp.Response_Message[3],"Voltage(+12V) : %s", mon_vlt[0]);
  sprintf(mcm_resp.Response_Message[4],"Voltage(-5.0V) : %s", mon_vlt[1]);
  sprintf(mcm_resp.Response_Message[5],"Voltage(-3.1V) : %s", mon_vlt[2]);
  sprintf(mcm_resp.Response_Message[6],"Voltage(-1.1V) : %s", mon_vlt[3]);
  sprintf(mcm_resp.Response_Message[7],"Voltage(GND) : %s", mon_vlt[4]);

  mcm_resp.Total_Resp_Msg = 8;
}


void fiber_optics_set()
{
  char i,j;
  char Para_invalid[15];
  int OF_Bits;
  char OF_MSW[5], OF_LSW[5];

  for(i=0; i < pc_cmd.Total_Parameter; i++)
  {
   for(j=0; j < OF_Para_No; j++)
   {
    if(!strcmp(pc_cmd.Parameter_Name[i], OF_Para[j]))
    {
      switch(j)
      {
        case 0 :  sprintf(mcm_resp.Response_Message[2],"RF Attenuation : %s %s",pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[0],"RF Attenuation");
                  sprintf(webpg_mon.arg1[0],"%s dB",pc_cmd.Argument_Ch1[i]);
                  sprintf(webpg_mon.arg2[0],"%s dB",pc_cmd.Argument_Ch2[i]);

                  OF_Bits = _f_atoi(pc_cmd.Argument_Ch2[i]);
                  printf("\nFirst Hex Value of OF_Bits is : %x", OF_Bits);
                  OF_Bits = OF_Bits << 6;
                  printf("\nHex Value After Shifting by 6 is : %x", OF_Bits);
                  OF_Bits = OF_Bits + _f_atoi(pc_cmd.Argument_Ch1[i]);
                  printf("\nFinal Hex Value of OF_Bits is : %x", OF_Bits);

                  strcpy(OF_MSW,"0000");
                  sprintf(OF_LSW,"%04x",OF_Bits);
                  printf("String OF_LSW = %s",OF_LSW);

                  Set_DMask(OF_MSW,OF_LSW);  // Setting 32 bit digital Mask for OF Settings //
                  break;

        default : break;
      }
      Para_invalid[i] = 0;
      break;
    } // if end
    else
    {
      Para_invalid[i] = 1;
    }
   }  // for - j end
  }   // for - i end

  for(i=0; i < OF_Para_No; i++)
  {
   if(Para_invalid[i])
   {
     sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
   }
  }
  mcm_resp.Total_Resp_Msg =  OF_Para_No + 2;
}


void fiber_optics_init()
{
  Set_RTC(pc_cmd.TimeStamp);
  sprintf(mcm_resp.Response_Message[2],"Fiber Optics Initialized");
  mcm_resp.Total_Resp_Msg = 3;

}


void fiber_optics_reset()
{
  sprintf(mcm_resp.Response_Message[2],"Fiber Optics Reset Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void of_mon_sum()
{

}




/****************************************************************/
/****************** Tasks For Back End System *******************/
/****************************************************************/

void back_end_mon()
{

  // Code for Back End Monitoring and Decoding //

  sprintf(mcm_resp.Response_Message[2],"Back End Monitoring Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void back_end_set()
{
  char i, j, k;
  char Para_invalid[15];

  unsigned long gab_long, temp_long1, temp_long2;
  char gab_dmask[10], hold_mask[10];

  unsigned long lo1, lo2, reflo1, reflo2;
  float attn1, attn2;
  int fltr1, fltr2;
  int lpf1, lpf2;


  gab_long = 0;


  for(i=0; i < pc_cmd.Total_Parameter; i++)
  {
   temp_long1 = 0;
   temp_long2 = 0;

   for(j=0; j < BE_Para_No; j++)
   {

    if(!strcmp(pc_cmd.Parameter_Name[i], BE_Para[j]))
    {
      switch(j)
      {
        case 0 :  lo1 = atol(pc_cmd.Argument_Ch1[i]);
                  lo2 = atol(pc_cmd.Argument_Ch2[i]);

                  Set_LO(lo1,0);
                  Set_LO(lo2,1);

                  sprintf(mcm_resp.Response_Message[2],"LO SET : %ld KHz, %ld KHz", lo1, lo2);
                  strcpy(webpg_mon.sys_para[0],"LO Frequency");
    					sprintf(webpg_mon.arg1[0],"%ld KHz",lo1);
    					sprintf(webpg_mon.arg2[0],"%ld KHz",lo2);
                  break;

        case 1 :  reflo1 = atol(pc_cmd.Argument_Ch1[i]);
                  reflo2 = atol(pc_cmd.Argument_Ch2[i]);

                  Set_RefLO(reflo1,0);
                  Set_RefLO(reflo2,1);

                  sprintf(mcm_resp.Response_Message[3],"Ref LO SET : %ld MHz, %ld MHz", reflo1, reflo2);
                  strcpy(webpg_mon.sys_para[1],"Reference LO");
    					sprintf(webpg_mon.arg1[1],"%ld MHz",reflo1);
    					sprintf(webpg_mon.arg2[1],"%ld MHz",reflo2);
                  break;

        case 2 :  sprintf(mcm_resp.Response_Message[4],"Attenuation SET : %s , %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[2],"Attenuation");
    					strcpy(webpg_mon.arg1[2],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[2],pc_cmd.Argument_Ch2[i]);

                  attn1 = atof(pc_cmd.Argument_Ch1[i]);
					   attn2 = atof(pc_cmd.Argument_Ch2[i]);

                  temp_long1 = temp_long1 | (~(int)((attn1 + 0.1) * 2) & 0x3F);
                  temp_long2 = ( temp_long2 | (~(int)((attn2 + 0.1) * 2)) & 0x3F) << 15;

                  //printf("\nIn Attn temp_long2 : %08lx", temp_long2);

                  gab_long = gab_long | temp_long1 | temp_long2;

					   break;

        case 3 :  sprintf(mcm_resp.Response_Message[5],"Filter SET : %s , %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[3],"Filter");
    					strcpy(webpg_mon.arg1[3],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[3],pc_cmd.Argument_Ch2[i]);

                  fltr1 = atoi(pc_cmd.Argument_Ch1[i]);
				      fltr2 = atoi(pc_cmd.Argument_Ch2[i]);

                  temp_long1 = (temp_long1 | fltr1) << 9;
                  temp_long2 = (temp_long2 | fltr2) << 24;

                  gab_long = gab_long | temp_long1 | temp_long2;

				      break;

        case 4 :  sprintf(mcm_resp.Response_Message[6],"LPF SET : %s , %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[4],"Low pass Filter");
    					strcpy(webpg_mon.arg1[4],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[4],pc_cmd.Argument_Ch2[i]);

                  for(k=0; k < lpf_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], lpf[k])) )
                     {
                        temp_long1 = (temp_long1 | lpf_bits[k]) << 12;
                        break;
                     }
                  }

                  for(k=0; k < lpf_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], lpf[k])) )
                     {
                        temp_long2 = (temp_long2 | lpf_bits[k]) << 27;
                        break;
                     }
                  }

                  gab_long = gab_long | temp_long1 | temp_long2;

				      break;

        case 5 :  sprintf(mcm_resp.Response_Message[7],"Source: %s %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[5],"Source");
    					strcpy(webpg_mon.arg1[5],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[5],pc_cmd.Argument_Ch2[i]);

                  for(k=0; k < src_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], source[k])) )
                     {
                        temp_long1 = (temp_long1 | src_bits[k]) << 30;
                        break;
                     }
                  }

                  for(k=0; k < src_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], source[k])) )
                     {
                        temp_long2 = (temp_long2 | src_bits[k]) << 31;
                        break;
                     }
                  }

                  gab_long = gab_long | temp_long1 | temp_long2;

    					break;

        case 6 :  sprintf(mcm_resp.Response_Message[8],"Signal : %s %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[6],"Signal");
    					strcpy(webpg_mon.arg1[6],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[6],pc_cmd.Argument_Ch2[i]);

                  for(k=0; k < sig_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], signal[k])) )
                     {
                        temp_long1 = (temp_long1 | sig_bits[k]) << 6;
                        break;
                     }
                  }

                  for(k=0; k < sig_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], signal[k])) )
                     {
                        temp_long2 = (temp_long2 | sig_bits[k]) << 21;
                        break;
                     }
                  }

                  gab_long = gab_long | temp_long1 | temp_long2;
				      break;

        case 7 :  sprintf(mcm_resp.Response_Message[9],"Path : %s %s", pc_cmd.Argument_Ch1[i], pc_cmd.Argument_Ch2[i]);
                  strcpy(webpg_mon.sys_para[7],"Path");
    					strcpy(webpg_mon.arg1[7],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[7],pc_cmd.Argument_Ch2[i]);

                  for(k=0; k < pth_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch1[i], path[k])) )
                     {
                        temp_long1 = (temp_long1 | pth_bits[k]) << 8;
                        break;
                     }
                  }

                  for(k=0; k < pth_arg_no; k++)
                  {
                     if( !(_f_strcmp(pc_cmd.Argument_Ch2[i], path[k])) )
                     {
                        temp_long2 = (temp_long2 | pth_bits[k]) << 23;
                        break;
                     }
                  }

                  gab_long = gab_long | temp_long1 | temp_long2;
				      break;

        case 8 :  sprintf(mcm_resp.Response_Message[10],"Channel : %s", pc_cmd.Argument_Ch1[i]);
                  strcpy(webpg_mon.sys_para[8],"Channel");
    					strcpy(webpg_mon.arg1[8],pc_cmd.Argument_Ch1[i]);
    					strcpy(webpg_mon.arg2[8]," ");

                  // Do nothing, as of now no bit is allocated to this parameter //

				      break;

        default : break;
      }
      Para_invalid[i] = 0;
      break;
    } // if end
    else
    {
      Para_invalid[i] = 1;
    }
   }  // for - j end

   //printf("\ntemp_long1 : %08lx;  temp_long2 : %08lx", temp_long1, temp_long2);

  }   // for - i end

  sprintf(gab_dmask, "%08lx", gab_long);   // Converting long to string //

  strcpy(hold_mask, gab_dmask);   // Storing upper word in msw //
  hold_mask[4] = 0;
  strcpy(msw, hold_mask);

  strcpy(hold_mask, (gab_dmask + 4));  // Storing lower word in lsw //
  strcpy(lsw, hold_mask);

  Set_DMask(msw, lsw);  // Setting 32 bit digital Mask for GAB settings //

  printf("\n gab_long : %08x", gab_long);

  printf("\n GAB MSW : %s    LSW : %s", msw, lsw);


  for(i=0; i < BE_Para_No; i++)
  {
   if(Para_invalid[i])
   {
     sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
   }
  }
  mcm_resp.Total_Resp_Msg =  BE_Para_No + 2;
}


void back_end_init()
{
  Set_RTC(pc_cmd.TimeStamp);

  sprintf(mcm_resp.Response_Message[2],"Back End Initialized");
  mcm_resp.Total_Resp_Msg = 3;
}


void back_end_reset()
{
  sprintf(mcm_resp.Response_Message[2],"Back End Reset Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void be_mon_sum()
{

}




/****************************************************************/
/************** Tasks For Feed Positioning System ***************/
/****************************************************************/

void fps_mon()
{
  sprintf(mcm_resp.Response_Message[2],"MCM Monitoring Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void fps_set()
{
  sprintf(mcm_resp.Response_Message[2],"FPS Setting Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void fps_init()
{
  Set_RTC(pc_cmd.TimeStamp);

  sprintf(mcm_resp.Response_Message[2],"FPS Initialized");
  mcm_resp.Total_Resp_Msg = 3;
}


void fps_reset()
{
  sprintf(mcm_resp.Response_Message[2],"FPS Reset Done");
  mcm_resp.Total_Resp_Msg = 3;
  exit(0);
}


void fps_mon_sum()
{

}




/****************************************************************/
/********************* Tasks For Self Test **********************/
/****************************************************************/

void mcm_mon()
{

  sprintf(mcm_resp.Response_Message[2],"MCM Monitoring Done");
  mcm_resp.Total_Resp_Msg = 3;
}


void mcm_set()
{
  char i, j;
  char Para_invalid[15];


  for(i=0; i < pc_cmd.Total_Parameter; i++)
  {
   for(j=0; j < MCM_Para_No; j++)
   {
    if(!strcmp(pc_cmd.Parameter_Name[i], MCM_Para[j]))
    {
      switch(j)
      {
        case 0 : 	SS = atoi(pc_cmd.Argument_Ch1[i]);

                  Set_SS();

                  if(SS == 0)
    			       strcpy(mcm_resp.Response_Message[2],"Spectrum Spreader Set : Off    Off");

                  else if(SS == 1)
    			   	 strcpy(mcm_resp.Response_Message[2],"Spectrum Spreader Set : Normal    Normal");

                  else if(SS == 2)
    			   	 strcpy(mcm_resp.Response_Message[2],"Spectrum Spreader Set : Normal    Strong");

                  else if(SS == 3)
    			   	 strcpy(mcm_resp.Response_Message[2],"Spectrum Spreader Set : Strong    Normal");

                  else
                   strcpy(mcm_resp.Response_Message[2],"Spectrum Spreader : Argument Invalid");

                  break;

        case 1 :  FDB = atoi(pc_cmd.Argument_Ch1[i]);

        				Set_FDB();

        				if(FDB == 0)
					    strcpy(mcm_resp.Response_Message[3],"Frequency Doubler : Off");

        				else if(FDB == 1)
					    strcpy(mcm_resp.Response_Message[3],"Frequency Doubler : On");

                  else
                   strcpy(mcm_resp.Response_Message[3],"Frequency Doubler : Argument Invalid");

					   break;

        case 2 :  FDV = atoi(pc_cmd.Argument_Ch1[i]);

        				Set_FDV();

				      sprintf(mcm_resp.Response_Message[4],"Frequency Divided By : %d",FDV);

				      break;

        default : break;
      }
      Para_invalid[i] = 0;
      break;

    } // if end
    else
    {
      Para_invalid[i] = 1;
    }
   }  // for - j end

  }   // for - i end


  for(i=0; i < MCM_Para_No; i++)
  {
   if(Para_invalid[i])
   {
     sprintf(mcm_resp.Response_Message[i+2],"Parameter : %s Invalid",pc_cmd.Parameter_Name[i]);
   }
  }


  Rd_MCM_Clk();

  sprintf(mcm_resp.Response_Message[MCM_Para_No + 2],"Clock Frequency : %5.2f MHz",MCM_Clk_Frq);

  mcm_resp.Total_Resp_Msg =  MCM_Para_No + 3;

}


void mcm_init()
{
  Set_RTC(pc_cmd.TimeStamp);

  sprintf(mcm_resp.Response_Message[2],"MCM Initialized");
  mcm_resp.Total_Resp_Msg = 3;
}


void mcm_reset()
{
  sprintf(mcm_resp.Response_Message[2],"MCM Reset Done");
  mcm_resp.Total_Resp_Msg = 3;
  exit(0);
}