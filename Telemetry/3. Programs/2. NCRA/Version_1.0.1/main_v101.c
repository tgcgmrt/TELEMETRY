// ********************************************************************************** //
// ********************** Program : Client for 15m Wrapper Server ******************* //
// ****************************** Date : 11/06/2013 ********************************* //
// ******************************** Version : V1.01 ********************************* //
// ***** Compatible with RCM 4300, New MCM PCB included code for sigcon system ****** //
// ********** Both monitoring and controlling of sigcon system included ************* //
// *** Code is developed for latest PCB same which is being used for In House CMS *** //
// ********************************************************************************** //

// *************************  PORT PINS DESCRIPTION   ******************************* //
// ** Port A : 32 bit Data generation; ********************************************** //
// ** PB2, PB3, PB4, PB5 : For latching 8 bit Data;   PB6 : Latch Output Enable; **** //
// ** SPI D used -  PE0, PE1 : SPI Latch Enable;    PC0 : SPI_TX_D; PE3 : SPI_CLK; ** //
// ** PE4, PE5, PE6, PE7 : Mux Selection Pins; ************************************** //
// ********************************************************************************** //


#class    auto
#memmap   xmem

#use      "RCM43xx.lib"	             // Use with RCM 4300
#use      "adc_ads7870.lib"
#include  "define_15m_v101.c"
#include  "Init.c"
#use	    "dcrtcp.lib"
#use      "spi.lib"


static tcp_Socket Socket;

//*******************************************************************


#define ADMIN_USER "rahul"
#define ADMIN_PASS "rcm4300"

//#define BU_TEMP_USE_FAT
#define BU_TEMP_USE_SBF

#define  STDIO_DEBUG_SERIAL   SADR
#define	STDIO_DEBUG_BAUD		115200
#define	STDIO_DEBUG_ADDCR

#define USE_HTTP_UPLOAD

#define DISABLE_DNS

#define SSPEC_FLASHRULES

#ifdef BU_TEMP_USE_FAT
	// This value is required to enable FAT blocking mode for ZServer.lib
	#define FAT_BLOCK

	#define FAT_USE_FORWARDSLASH		// Necessary for zserver.

	#use "fat16.lib"							// Must use FAT before http/zserver
#endif

#define HTTP_MAXSERVERS 1

#define HTTP_SOCK_BUF_SIZE (12 * MAX_MTU)

#define TCP_BUF_SIZE   	4096    // By-Default TCP_BUF_SIZE : 4096 //
#define MAX_SOCKETS     2
//#define MAX_TCP_SOCKET_BUFFERS 2

#define USE_RABBITWEB 1
#define USE_HTTP_DIGEST_AUTHENTICATION 1

#use   "dcrtcp.lib"
#use   "http.lib"
#use   "board_update.lib"


int firmware_upload(HttpState * s);

#define ADMIN_GROUP	0x0002

#ximport  "upload.zhtml"    upload_zhtml

SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".htm", "text/html"),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".gif", "image/gif"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END

#define REALM "R&D"

SSPEC_RULETABLE_START
SSPEC_MM_RULE("/", REALM, ADMIN_GROUP, ADMIN_GROUP,
		SERVER_HTTP, SERVER_AUTH_DIGEST, NULL)
SSPEC_RULETABLE_END

int root_htm(HttpState *http)
{
	cgi_redirectto( http, "/fw/upload.zhtml");         /////////////////////////////////////////////////////
	return CGI_OK;
}

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
                    REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END



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

#ifdef BU_TEMP_USE_FAT
void unmount_all()
{
	int i, rc;

   for (i = 0; i < num_fat_devices * FAT_MAX_PARTITIONS;
   	i += FAT_MAX_PARTITIONS)
   {
      if (fat_part_mounted[i])
      {
         rc = fat_UnmountDevice(fat_part_mounted[i]->dev);
         if (rc < 0)
         {
            printf("Unmount Error on %c: %ls\n", 'A' + i, strerror(rc));
         }
      }
   }
}
#endif

void install_firmware()
{
	int i, result;

	printf ("user clicked install/reboot button\n\n");

	#ifdef BU_TEMP_USE_DIRECT_WRITE
		// Firmware is already installed, just need to shut down HTTP server
		// and reboot.
      http_shutdown(0);
		exit(0);
	#endif

   result = buOpenFirmwareTemp( BU_FLAG_NONE);

	if (result)
	{
      sprintf (firmware.error, "Error %d opening firmware.", result);
      firmware.install = -1;
	}
	else
	{
	   result = buVerifyFirmwareBlocking();
	   if (result)
	   {
	      while (buCloseFirmware() == -EBUSY);
	      sprintf (firmware.error, "Error %d installing firmware.", result);
	      firmware.install = -1;
	   }
	   else
	   {
	      // shut down HTTP server and install
	      http_shutdown(0);
	      if (buInstallFirmware())
	      {
	         // on error, restart http server
            printf("Error installimg Firmware. Initializing HTTP.....\n");
	         http_init();
	      }
	      else
	      {
	         // Install was successful, reboot into new firmware.
            printf("Firmware installed. Rebooting........\n");
            forceWatchdogTimeout();
	         exit( 0);
	      }
	   }
	}
}

// need to call this after upload completes...
void update_webvars()
{
	firmware_info_t		fi;
	int i, err;

	// load information on the current firmware
	fiProgramInfo( &fi);
   sprintf( firmware.running, "%s v%u.%02x", fi.program_name,
      fi.version >> 8, fi.version & 0xFF);

	#ifdef BU_TEMP_USE_DIRECT_WRITE
		// no temp firmware to show information about
		firmware.show_temp = 0;
	#else
		firmware.show_temp = 1;
	#endif

   // load information on firmware stored in temporary location
   err = buOpenFirmwareTemp( BU_FLAG_NONE);

   if (!err)
   {
	   // buGetInfo is a non-blocking call, and may take multiple attempts
	   // before the file is completely open.
	   i = 0;
	   do {
	      err = buGetInfo(&fi);
	   } while ( (err == -EBUSY) && (++i < 20) );
   }
   if (err)
   {
      //sprintf( firmware.temp, "Error %d verifying firmware", err);
      firmware.show_install = -1;
   }
   else
   {
      sprintf( firmware.temp, "%s v%u.%02x", fi.program_name,
         fi.version >> 8, fi.version & 0xFF);
      // firmware is good, try to verify it
      err = buRewindFirmware();
      if (!err)
      {
         err = buVerifyFirmwareBlocking();
      }
      // if no errors, show a button (1) so user can try to install this
      // firmware, or show an error message that image is corrupted (-1)
      firmware.show_install = err ? -1 : 1;
      firmware.show_status = err ? -1 : 0;
   }
   while (buCloseFirmware() == -EBUSY);

	firmware.install = 0;
	*firmware.error = '\0';
}


// Firmware Upload function, based on http_defaultCGI from http.lib

// Values for http_getState()
#define FW_UPLOAD_INIT			0	// Initial state, set by server (must be 0)
#define FW_UPLOAD_NORM			1	// Normal state (reading client data)

// http_setCond() state variables
#define FW_UPLOAD_ERROR			0	// Error code (<0)
#if HTTP_MAX_COND < 1
	#fatal "HTTP_MAX_COND must be >= 1 if using firmware_upload()"
#endif

#ifndef HTTP_UPLOAD_SSL_SUPRESS_WARNING
#ifdef __SSL_LIB__
	#warns "HTTP Upload is not currently compatible with SSL. Make sure that"
	#warns "you are not using HTTP upload on SSL-secured ports. To eliminate"
	#warns "this warning, include '#define HTTP_UPLOAD_SSL_SUPRESS_WARNING'"
	#warns "in your program before the line '#use HTTP.LIB'."
#endif
#endif

int firmware_upload(HttpState * s)
{
	auto char * fname;		// Pointer to field (i.e. resource to upload) name
   auto int rc, offset, mode, wlen, error;

	if (!(mode = http_getState(s)))
	{
   	// Initial state (FW_UPLOAD_INIT).  Do things we want to do only
      // once at the start.
   	// First time through.  Generate HTTP header
      http_setState(s, FW_UPLOAD_NORM);		// Next state is "normal"
      http_setCond(s, FW_UPLOAD_ERROR, -1);	// Temp file hasn't been opened
      // Note: it is safe to use the getData buffer, since the first call to
      // the CGI will not have any incoming data in that buffer.
	   http_genHeader(s, http_getData(s), HTTP_MAXBUFFER, 200, NULL, 0,
      "<html><head><meta http-equiv=\"refresh\" content=\"2;url=upload.zhtml\"></head><body>");
      return CGI_SEND;	// Write string to client.
   }

   // Default return code.
   rc = CGI_OK;

   // Main switch depending on the current CGI action code
	switch (http_getAction(s))
	{
   	case CGI_START:
      	// Start of a new part of the multi-part data.
      	fname = http_getField(s);

         if (fname[0] != '/')
         {
         	http_skipCGI(s);	// Not a file: skip to next part.
         }
         else
         {
         	// Yes, it's a file to upload
				while ( (error = buTempCreate()) == -EBUSY);
            http_setCond(s, FW_UPLOAD_ERROR, error); // Save error code
            if (error < 0)
            {
               firmware.show_status = -1;
               http_skipCGI(s);	// Skip to next part after sending this buffer
      		}
            else
            {
            	// confirmation that something is happening.
              firmware.show_status = 1;
            }
            rc = CGI_SEND;
         }
         break;

   	case CGI_DATA:
      	// This is data to write to the resource.
         if (http_getCond(s, FW_UPLOAD_ERROR) < 0)
         {
         	// This should never happen (since we always skip data if there is
         	// an error opening the resource), but it doesn't hurt to check!
         	return 0;
			}

			offset = 0;
			do {
	         wlen = buTempWrite( http_getData(s) + offset,
	            http_getDataLength(s) - offset);
	         if (wlen == -EBUSY)
	         {
	         	// ignore BUSY error and try again
					wlen = 0;
	         }
	         else if (wlen < 0)
	         {
	            #ifdef BU_TEMP_USE_DIRECT_WRITE
	               // need to restore original firmware before continuing
	               if (wlen != -ENODATA)
	               {
	               	buRestoreFirmware( 0);
	               }
	            #endif
               firmware.show_status = -1;
	         }
	         offset += wlen;
			} while (offset != http_getDataLength(s));
         break;

   	case CGI_END:
      	// End of this file.  Close it.
			while (buTempClose() == -EBUSY);
      	http_setCond(s, FW_UPLOAD_ERROR, -1);	// No open resource now
         update_webvars();
         firmware.fwlen=http_getContentLength(s)/1024;
         firmware.show_status = 2;
			break;

      case CGI_EOF:
      	// Normal end of entire set of parts.

			if (firmware.show_install == -1)
			{
            firmware.show_status = -1;
			}

         // Send this last message, and we are done!
         rc = CGI_SEND_DONE;
         printf("\n\nUpload completed\n\n");
         break;

      case CGI_ABORT:
      	// Server had to abort the connection.
#ifdef VERBOSE
      	printf("HTTPU: ABORT CGI\n");
#endif
			// Clean up resource if one was open.
			while (buTempClose() == -EBUSY);
         break;
   }

   return rc;
}


//*************************************************************************************


//.... Generating milisecond delay .... //

void Ms_delay(unsigned int delay)
{
	auto unsigned long delay_time;

	delay_time = MS_TIMER + delay;
   while((long)(MS_TIMER - delay_time) < 0 );
}


int cmd_validation();
int form_inter_resp();
int command_process_sen();
int command_process_sig();
void form_final_resp(int, int);
void bg_mon();
void Sig_decode();
void Set_DMask(int, int, int, int);
void Set_RTC(char far *);
void Set_LO(unsigned long, int);
void Set_Attn(int, int);



// .... Opening Server Socket at desired port address .... //

cofunc int mcm_clnt()
{
   int port, write_bytes, read_bytes, wait_var;
   longword host;

   ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR, aton(MY_IP_ADDR),IFS_NETMASK, aton(MY_NET_MASK),IFS_UP,IFS_END);

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
     printf("\nUnable to open TCP session");

   wait_var = 0;

   while((!sock_established(&Socket) && sock_bytesready(&Socket)== -1))
    {
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
    read_bytes = 0;
    do{

       if(sock_bytesready(&Socket) > 0)
       {
         read_bytes = sock_read(&Socket, &Wrpr_Cmd, Wrpr_Cmd_Size);

         printf("\nMCM has read command of %d bytes",read_bytes);

         if(form_inter_resp() == 10)    // Form intermediate response and send it over socket //
         {
           write_bytes = sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
           printf("\nMCM has sent intermediate response of %d bytes",write_bytes);

           form_final_resp(SUCCESS,FINAL);   // Process the command, Form final response and send it over socket //
           write_bytes = sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
   		  printf("\nMCM has sent final response of %d bytes",write_bytes);
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

   sock_close(&Socket);
	printf("\nConnection closed for port-%d",port);

   return 0;
}




// ..... Main start .... //
main()
{
  auto unsigned long scn_dl;
  int rc, i, uid;

  brdInit();
  SPIinit();
  PortInit();
  sock_init();
  Struct_Init();
  ADC_Init();

  #ifdef BU_TEMP_USE_FAT
	printf( "Initializing filesystem...\n");
	// Note: sspec_automount automatically initializes all known filesystems.
   rc = sspec_automount(SSPEC_MOUNT_ANY, NULL, NULL, NULL);
   if (rc)
   {
   	printf( "Failed to mount filesystem, rc=%d\n", rc);
   	exit (rc);
	}
   if (! fat_part_mounted[0])
   {
		printf("Couldn't mount A:\n");
		exit(-EIO);
   }

	// register a function to unmount all FAT volumes on exit
   atexit( unmount_all);
#endif

   printf( "Setting up userids...\n\n");
   // Create a user ID
   uid = sauth_adduser(ADMIN_USER, ADMIN_PASS, SERVER_HTTP);
   if (uid < 0)
   {
   	printf( "Failed to create userid, rc=%d\n", uid);
   	exit (uid);
	}

   // Ensure that that user is a member of ADMIN_GROUP
   sauth_setusermask(uid, ADMIN_GROUP, NULL);

   // Also need to assign individual write access.
   sauth_setwriteaccess(uid, SERVER_HTTP);

   //printf( "Userid created successfully: use '%s' with password '%s'\n\n",
      //ADMIN_USER, ADMIN_PASS);

	printf( "Initialize RabbitWeb variables...\n\n");
	update_webvars();

   http_init();
   tcp_reserveport(80);

  rd_MCM_addr();

  printf("\nsizeof(parseCMSCmd) : %d",sizeof(parseCMSCmd));
  printf("\nsizeof(devResponse) : %d",sizeof(devResponse));

  if(mcm_addr == 1)
  {
   BitWrPortI(PADR, &PADRShadow, 1, 0); // Useful for setting LO in sigcon //
   BitWrPortI(PADR, &PADRShadow, 0, 2);
   BitWrPortI(PADR, &PADRShadow, 0, 4);
   if_amp_vccp = 12;
   if_amp_vccn = 12;
   if_attn_vccp = 12;
   if_attn_vccn = 12;
   if_vdd = 5;
   vco2_vccp = 12;
   vco3_vccp = 12;
   vco2_vdd = 5;
   vco3_vdd = 5;
   amp_vdd = 5;
   ibob_vcc = 3.3;
   ibob_gnd = 0;
  }

  while(1)
  {
    costate    // This will open client socket at port 8000 //
     {
       wfd mcm_clnt();
     }

    costate    // This will do background monitoring in parallel at every SCAN_DELAY miliseconds //
     {
       scn_dl = MS_TIMER + SCAN_DELAY;
       Ms_delay(1);  // This delay is necessary for proper working of following loop. //
       while(((scn_dl - MS_TIMER) > 0) && ((scn_dl - MS_TIMER) < 8000))
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
   int cmpl,i,j;
   cmpl =  0;

   printf("\nEntering cmd_validation");
   if(mcm_addr == 0)
   {
      if(!(_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_0)))
       {
         printf("\nSubsystem validated");
         printf("\nCommand received : %s",Wrpr_Cmd.cmd_elem.cmd_name);

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_001))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_002))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_003))
          {
           printf("\nCommand Name Validated");
           return 10;

           for(j=0; j<Wrpr_Cmd.cmd_data.numpkt; j++) // Looking for Jth parameter //
            {
             for(i=0; i<SMP_rawsize; i++) // comparing Jth against Ith parameter //
              {
               if(!_f_strcmp(Wrpr_Cmd.cmd_data.prmname[j],SenMon_Para[i]))
                {
                 printf("\nCommand Name Validated");
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
           printf("\nCommand Name Validated");
           return 10;
          }
         else
          {
           printf("\nCommand Syntax Error");
           return 12;     // Incase of Syntax Error in command //
          }
       }
      else
       {
        printf("\nCommand Invalid");
        return 14;        // Incase of Invalid command //
       }
   }

   else if(mcm_addr == 1)
   {
      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_1))
       {
         printf("\nSubsystem validated");

         if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_101))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_103))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_104))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_105))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_106))
          {
           printf("\nCommand Name Validated");
           return 10;
          }
         else
          {
           printf("\nCommand Syntax Error");
           return 12;     // Incase of Syntax Error in command //
          }
       }
      else
       {
        printf("\nCommand Invalid");
        return 14;
       }
   }

   else if(mcm_addr == 2)
   {
      if(!_f_strcmp(Wrpr_Cmd.cmd_elem.subsysid,SYSTEM_2))
       {
        printf("\nCommand valid");
        return 10;
       }
      else
       {
        printf("\nCommand Invalid");
        return 14;
       }
   }

   else
   {
    printf("\nMCM Address not valid");
    return 100;     // Incase of Unknown error like, MCM address mismatch or Sending Sentinal command to Sigcon MCM. //
   }

 }



// ****** Forming intermediate response for all system commands ****** //

int form_inter_resp()
{
  int loop;

  printf("\nEntering form_inter_resp");

  MCM_Resp.code = cmd_validation();
  MCM_Resp.event = INTERMEDIATE;

  switch(MCM_Resp.code)
  {
    case 10 :  _f_strcpy(MCM_Resp.message,"Command received successfully");
               break;
    case 12 :  _f_strcpy(MCM_Resp.message,"Command Syntax Error");
               break;
    case 13 :  _f_strcpy(MCM_Resp.message,"Command Incomplete");
               break;
    case 14 :  _f_strcpy(MCM_Resp.message,"Command Invalid");
               break;
    case 100:  _f_strcpy(MCM_Resp.message,"Error Unknown");
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




// ******** Processing the Sentinal command ******** //

int command_process_sen()
{

  printf("\nEntering command_process_sen");
  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_001))      // DoMon will monitor Temperature, RTC time, Status, Raw Values etc..//
  {
     MCM_Resp.resp_data.numpkt = 5;

    _f_strcpy(MCM_Resp.resp_data.prmname[0],"time");

      sec_tm = read_rtc();
      mktm(&rtc, sec_tm);
      sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

    _f_strcpy(MCM_Resp.resp_data.prmvalue[0],time_ar);      // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[1],"state");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[1],"OK");         // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[2],"temp");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[2],temp);         // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[3],"chan");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[3],"1");          // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[4],"rawcnt");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[4],temp_raw);      // This value should be filled in bg_mon function. //

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_002))   // SetCmd will set portbits at 32 bit o/p //
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

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_004))
  {
     printf("\n MCM is given Reset : Socket Closed !!");
     sock_close(&Socket);
     exit(0);
  }

  return 12;     // This return value will be stored in MCM_Resp.event //
}




// ******** Processing the SIGCON command ******** //

int command_process_sig()
{
  unsigned long lo_frq;
  int IF_Attn1, IF_Attn2;

  printf("\nEntering Command Process");

  if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_101))    // Init command will initalise Memory, Portbits, ADC and RTC //
  {
    MCM_Resp.resp_data.numpkt = 20;

    _f_strcpy(MCM_Resp.resp_data.prmname[0],"time");

      sec_tm = read_rtc();
      mktm(&rtc, sec_tm);
      sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

    _f_strcpy(MCM_Resp.resp_data.prmvalue[0],time_ar);      // This value should be filled in bg_mon function. //

    _f_strcpy(MCM_Resp.resp_data.prmname[1],"state");
    _f_strcpy(MCM_Resp.resp_data.prmvalue[1],"Ok");         // This value should be filled in bg_mon function. //

    Sig_decode();

    return 99;   // This return value will be stored in MCM_Resp.event //
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_102))
  {
    lo_frq = _f_atol(Wrpr_Cmd.cmd_data.prmvalue[0]);
    printf("\nCommand for Set LO : %ld KHz Received..................",lo_frq);
    Set_LO(lo_frq, 0);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_103))
  {
    IF_Attn1 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[0]);
    IF_Attn2 =  _f_atoi(Wrpr_Cmd.cmd_data.prmvalue[1]);
    printf("\nCommand for Set IF Attn : %ddB %ddB ..................", IF_Attn1, IF_Attn2);
    Set_Attn(IF_Attn2, IF_Attn1);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_104))
  {

  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_105))
  {
    //Struct_Init();             // If this func will be called here, values in struct should be refilled in Final_Resp //
    PortInit();
    ADC_Init();
    Set_RTC(Wrpr_Cmd.cmd_elem.timestamp);
  }

  else if(!_f_strcmp(Wrpr_Cmd.cmd_elem.cmd_name,Cmd_106))
  {
    printf("\n MCM is given Reset : Socket Closed !!");
    sock_close(&Socket);
    exit(0);
  }

  else
  {
    printf("Nothing Matched, Received : %s",Wrpr_Cmd.cmd_elem.cmd_name);
  }

  return 12;
}



// ******** Forming final response ******** //

void form_final_resp(int code, int event)
{
  printf("\nEntering form_final_resp");

  MCM_Resp.code = code;
  if(mcm_addr == 0)
   MCM_Resp.event = command_process_sen();
  else if(mcm_addr == 1)
   MCM_Resp.event = command_process_sig();
}



// ******** Forming Alarm ******** //

void form_alarm(int Alrm_id, int Alrm_lvl)
{
 /* MCM_Resp.code = 20;
  MCM_Resp.event = 100;

  MCM_Resp.MCM_alarm.id = Alrm_id;
  MCM_Resp.MCM_alarm.level = Alrm_lvl;

  if(Alrm_id == 903)
  _f_strcpy(MCM_Resp.MCM_alarm.description,"Raw Count More than 500");
  if(Alrm_id == 613)
  _f_strcpy(MCM_Resp.MCM_alarm.description,"LO VCC+ Lower than 10V");

  MCM_Resp.resp_data.numpkt = 0;
  sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);    */
}



// ****** Doing background monitoring tasks and event generation ****** //

void bg_mon()
{
  int channel, mux_sel, set_mux_sel;
  float tmp;
  //printf("\nMonitoring Array :");
  for(channel=0; channel<4; channel++)
  {
   for(mux_sel=0; mux_sel<16; mux_sel++)
    {
      set_mux_sel = mux_sel*16;

      WrPortI(PEDR, &PEDRShadow, ((RdPortI(PEDR)&0x0f) | set_mux_sel));
      mon_arr[channel*16 + mux_sel] = (anaIn(channel, SINGLE, GAINSET));
      printf("%d:%x  ",channel*16 + mux_sel,mon_arr[channel*16 + mux_sel]);
    }
    printf("\n\n");
  }


  if(mcm_addr == 0)
  {
   itoa(mon_arr[0], temp_raw);

   tmp = ((1000 - mon_arr[0]) / 20.0);
   sprintf(temp,"%.2f",tmp);
  }
  else if(mcm_addr == 1)
  {
   Sig_decode();
  }
}



void Sig_decode()
{
  char attn1[10], attn2[10];
  int i, ch1_attn, ch2_attn;
  ch1_attn = 0;
  ch2_attn = 0;

  //bg_mon();

  for(i=0;i<4;i++)
  {
   if((mon_arr[(2*i)+1]<1100) && (mon_arr[2*i]>2000))
     ch1_attn = ch1_attn + pow(2,i);
   if((mon_arr[(2*i)+9]<1100) && (mon_arr[(2*i)+8]>2000))
     ch2_attn = ch2_attn + pow(2,i);
  }
  printf("\nCh1_attn = %d, Ch2_attn = %d", ch1_attn, ch2_attn);

  _f_strcpy(MCM_Resp.resp_data.prmname[2],"attn1");
  sprintf(attn1,"%2d",ch1_attn);
  _f_strcpy(MCM_Resp.resp_data.prmvalue[2],attn1);

  _f_strcpy(MCM_Resp.resp_data.prmname[3],"attn2");
  sprintf(attn2,"%2d",ch2_attn);
  _f_strcpy(MCM_Resp.resp_data.prmvalue[3],attn2);

  printf("\nCh1_attn = %s, Ch2_attn = %s", attn1, attn2);

 /* if_vccp = 130 - mon_arr[8];
  if_vccp = (if_vccp * 2.4)/26;
  _f_strcpy(MCM_Resp.resp_data.prmname[4],"if-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[4],"%.2f",if_vccp);

  if_vccn = 130 - mon_arr[9];
  if_vccn = (if_vccn * 2.4)/26;
  _f_strcpy(MCM_Resp.resp_data.prmname[5],"if-vccn");
  sprintf(MCM_Resp.resp_data.prmvalue[5],"%.2f",if_vccn);

  if_vdd = 130 - mon_arr[10];
  if_vdd = if_vdd /26;
  _f_strcpy(MCM_Resp.resp_data.prmname[6],"if-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[6],"%.2f",if_vdd);


  _f_strcpy(MCM_Resp.resp_data.prmname[7],"sel_vco");
  if(mon_arr[16]<10 && mon_arr[17]>100 && mon_arr[18]>100)
  {
   printf("\nVCO-1 is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[7],"VCO-1");
  }
  else if(mon_arr[17]<10 && mon_arr[16]>100 && mon_arr[18]>100)
  {
   printf("\nVCO-2 is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[7],"VCO-2");
  }
  else if(mon_arr[18]<10 && mon_arr[16]>100 && mon_arr[17]>100)
  {
   printf("\nVCO-3 is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[7],"VCO-3");
  }
  else
  {
   printf("\nNo VCO is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[7],"Error");
  }


  _f_strcpy(MCM_Resp.resp_data.prmname[8],"vco");
  _f_strcpy(MCM_Resp.resp_data.prmvalue[8],"Locked");


  lo_vccp = 130 - mon_arr[22];
  lo_vccp = (lo_vccp * 2.4)/26;
  _f_strcpy(MCM_Resp.resp_data.prmname[9],"lo-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[9],"%.2f",lo_vccp);

  lo_vccn = 130 - mon_arr[23];
  lo_vccn = (lo_vccn * 2.4)/26;
  _f_strcpy(MCM_Resp.resp_data.prmname[10],"lo-vccn");
  sprintf(MCM_Resp.resp_data.prmvalue[10],"%.2f",lo_vccn);

  lo_vdd = 130 - mon_arr[24];
  lo_vdd = lo_vdd /26;
  _f_strcpy(MCM_Resp.resp_data.prmname[11],"lo-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[11],"%.2f",lo_vdd);    */

  if_amp_vccp = 1000 - mon_arr[22];
  if_amp_vccp = (if_amp_vccp * 2.79)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[4],"if-amp-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[4],"%.2f",if_amp_vccp);

  if_amp_vccn = 1000 - mon_arr[24];
  if_amp_vccn = (if_amp_vccn * 2.79)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[5],"if-amp-vccn");
  sprintf(MCM_Resp.resp_data.prmvalue[5],"%.2f",if_amp_vccn);

  if_attn_vccp = 1000 - mon_arr[23];
  if_attn_vccp = (if_attn_vccp * 2.79)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[6],"if-attn-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[6],"%.2f",if_attn_vccp);

  if_attn_vccn = 1000 - mon_arr[25];
  if_attn_vccn = (if_attn_vccn * 2.79)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[7],"if-attn-vccn");
  sprintf(MCM_Resp.resp_data.prmvalue[7],"%.2f",if_attn_vccn);

  if_vdd = 1000 - mon_arr[27];
  if_vdd = (if_vdd * 1.16) /200;
  _f_strcpy(MCM_Resp.resp_data.prmname[8],"if-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[8],"%.2f",if_vdd);



  _f_strcpy(MCM_Resp.resp_data.prmname[9],"sel_vco");
  if(mon_arr[36] > 1950)
  {
   printf("\nVCO-2 is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[9],"VCO-2");
  }
  else if(mon_arr[36]>950 && mon_arr[36]<1050)
  {
   printf("\nVCO-3 is selected");
   _f_strcpy(MCM_Resp.resp_data.prmvalue[9],"VCO-3");
  }
  else
  {
   printf("\nNo VCO is selected");
   //form_alarm(613,5);
   _f_strcpy(MCM_Resp.resp_data.prmvalue[9],"NO VCO SELECTED");
  }


  _f_strcpy(MCM_Resp.resp_data.prmname[10],"vco");
  if(mon_arr[28] < 370)
   _f_strcpy(MCM_Resp.resp_data.prmvalue[10],"VCO-2 LOCKED");
  else if(mon_arr[29] < 370)
   _f_strcpy(MCM_Resp.resp_data.prmvalue[10],"VCO-3 LOCKED");
  else
  {
   //form_alarm(614,5);
   _f_strcpy(MCM_Resp.resp_data.prmvalue[10],"NO VCO LOCKED");
  }


  vco2_vccp = 1000 - mon_arr[32];
  vco2_vccp = (vco2_vccp * 5.58)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[11],"vco2-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[11],"%.2f",vco2_vccp);

  vco3_vccp = 1000 - mon_arr[34];
  vco3_vccp = (vco3_vccp * 5.58)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[12],"vco3-vccp");
  sprintf(MCM_Resp.resp_data.prmvalue[12],"%.2f",vco3_vccp);

  vco2_vdd = 1000 - mon_arr[33];
  vco2_vdd = (vco2_vdd * 1.16)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[13],"vco2-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[13],"%.2f",vco2_vdd);

  vco3_vdd = 1000 - mon_arr[35];
  vco3_vdd = (vco3_vdd * 1.16)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[14],"vco3-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[14],"%.2f",vco3_vdd);

  amp_vdd = 1000 - mon_arr[37];
  amp_vdd = (amp_vdd * 1.39)/200;
  _f_strcpy(MCM_Resp.resp_data.prmname[15],"amp-vdd");
  sprintf(MCM_Resp.resp_data.prmvalue[15],"%.2f",amp_vdd);



  /*************************************************/

  _f_strcpy(MCM_Resp.resp_data.prmname[16],"ibob-vcc");
  ibob_vcc = (1000 - mon_arr[48])/200;
  sprintf(MCM_Resp.resp_data.prmvalue[16],"%.2f",ibob_vcc);



  _f_strcpy(MCM_Resp.resp_data.prmname[17],"adc-clk");
  if(mon_arr[49] < 700) // this is high
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[17],"ADC CLK OK : 200MHZ");
   }
  else
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[17],"ADC CLK NOT OK");
   }


  _f_strcpy(MCM_Resp.resp_data.prmname[18],"adc-overflow");
  if(mon_arr[50] < 700) // this is high
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[18],"ADC OVERFLOW");
   }
  else
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[18],"ADC INPUT POWER NORMAL");
   }


  _f_strcpy(MCM_Resp.resp_data.prmname[19],"qnt-overflow");
  if(mon_arr[51] < 700) // this is high
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[19],"QUANTIZER OVERFLOW");
   }
  else
   {
    _f_strcpy(MCM_Resp.resp_data.prmvalue[19],"QUANTIZER OUTPUT NORMAL");
   }


  _f_strcpy(MCM_Resp.resp_data.prmname[20],"ibob-gnd");
  ibob_gnd = (1000 - mon_arr[52])/200;
  sprintf(MCM_Resp.resp_data.prmvalue[20],"%.2f",ibob_gnd);

  /*************************************************/



  //sock_write(&Socket, &MCM_Resp, MCM_Resp_Size);
}



// **** After initialising port bits in PortInit(), 32 bit digital mask will be set here **** //

void Set_DMask(int MSB_2, int LSB_2,int MSB_1, int LSB_1)
{
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



// **** This function will set RTC to value provided within Init command **** //

void Set_RTC(char far * rtc_str)
{
  _f_strcpy(RTC_time,rtc_str);

  printf("\nRTC is set to : %s",rtc_str);
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


void Set_LO(unsigned long lo, int Ch)
{
 char LO_Arr[10], Temp_Arr[10],rem[10];
 int padding, cntr_1, i;

 BitWrPortI(PBDR, &PBDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 3);

 // Selecting the VCO first, then sending SPI pattern //
 if(lo>=110000 && lo<=249500)
 {

  BitWrPortI(PADR, &PADRShadow, 1, 4);
  BitWrPortI(PADR, &PADRShadow, 0, 5);
  BitWrPortI(PADR, &PADRShadow, 0, 6);
 }
 else if(lo>=250000 && lo<=599500)
 {
  BitWrPortI(PADR, &PADRShadow, 0, 4);
  BitWrPortI(PADR, &PADRShadow, 1, 5);
  BitWrPortI(PADR, &PADRShadow, 0, 6);
 }
 else if(lo>=600000 && lo<=1700000)
 {
  BitWrPortI(PADR, &PADRShadow, 0, 4);
  BitWrPortI(PADR, &PADRShadow, 0, 5);
  BitWrPortI(PADR, &PADRShadow, 1, 6);
 }

 BitWrPortI(PBDR, &PBDRShadow, 1, 3); // Latching value at LSB latch //



 BitWrPortI(PBDR, &PBDRShadow, 0, 6); // Enabling output of both Latches //



 // Code for sending SPI pattern starts (Enable:PC1, Data:PC2 Clk:PE7)//
 // All SPI pins are initialised in PortInit() //

 for(cntr_1=0; cntr_1<9; cntr_1++)
	LO_Arr[cntr_1] = '0';

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

  strcpy(LO_Arr+padding, Temp_Arr);

  LO_Arr[0] = 'K';


 for(cntr_1=0; cntr_1 < strlen(LO_Arr); cntr_1++)
  {
    if((LO_Arr[cntr_1] > 96) && (LO_Arr[cntr_1] < 123))
     LO_Arr[cntr_1] = LO_Arr[cntr_1] - 32;
  }


 strcpy(Temp_Arr, LO_Arr);
   printf("\nLO_Arr : %s\n",LO_Arr);


 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);

 BitWrPortI(PEDR, &PEDRShadow, 0, Ch);


 for(i=0;i<10;i++);

 SPIWrite(LO_Arr,10);

 for(i=0;i<10;i++);

 BitWrPortI(PEDR, &PEDRShadow, 1, Ch);
}


void Set_Attn(int ch2, int ch1)
{
 char Attn_bits;

 Attn_bits = 0;

 BitWrPortI(PBDR, &PBDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);

 Attn_bits = ch1 & 0x0f;

 ch2 = ch2 << 4;

 Attn_bits = (Attn_bits | (ch2 & 0xf0));

 printf("\nAttn_bits : 0x%x",Attn_bits);
 WrPortI(PADR, &PADRShadow, Attn_bits);

 BitWrPortI(PBDR, &PBDRShadow, 1, 2); // Latching value at LSB latch //
 BitWrPortI(PBDR, &PBDRShadow, 0, 6); // Enabling output of both Latch //
}

