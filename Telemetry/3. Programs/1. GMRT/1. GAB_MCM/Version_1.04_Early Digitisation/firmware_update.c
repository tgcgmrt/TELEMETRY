
/*********** Add following line in main program before adding .lib files **************
             #include "firmware_update.c"

             Add fwinit(); before http_init()in main() function

             Add following lines in RESOURCETABLE:
             SSPEC_RESOURCE_FUNCTION("/", root_htm),
   			 SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   			 SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   			 SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   			 REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
****************************************************************************************/

//#define BU_TEMP_USE_FAT		//store file on FAT filesystem
#define BU_TEMP_USE_SBF     //store file on serial boot flash
//#define BU_TEMP_USE_SECONDARY //store file on secondary part of serial boot flash reserved for power fail safe update.
// add BU_ENABLE_SECONDARY in defines tab.
#define  STDIO_DEBUG_SERIAL   SADR
#define	STDIO_DEBUG_BAUD		115200
#define	STDIO_DEBUG_ADDCR

#define USE_HTTP_UPLOAD		// Required to include upload code.
#define DISABLE_DNS			// No name lookups required
#define SSPEC_FLASHRULES   // Define this because we are using a static rule table.

#ifdef BU_TEMP_USE_FAT
	#define FAT_BLOCK       // Required to enable FAT blocking mode for ZServer.lib
	#define FAT_USE_FORWARDSLASH	// Necessary for zserver.
	#use "fat16.lib"					// Must use FAT before http/zserver
#endif

#define HTTP_MAXSERVERS 1
#define HTTP_SOCK_BUF_SIZE (12 * MAX_MTU)

#use   "dcrtcp.lib"
#use   "rcm43xx.lib"
#use   "http.lib"
#use   "http_client.lib"        // requires if firmware is placed on HTTP server
//#use   "ftp_client.lib"       // requires if firmware is placed on FTP server
#use   "board_update.lib"
#use   "idblock_api.lib"

void fwinit();
void update_webvars();
int  firmware_upload(HttpState * s);
void download_and_install1();
int  download_and_install( char *url);
void install_firmware();

#define ADMIN_GROUP	0x0002   // Define a group bit for updating resources...

#ximport  "upload.zhtml"    upload_zhtml
#ximport  "up_Sum.xml"      up_Sum_xml

// Uncomment below part if not present in main program file.
/*
SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END

SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".htm", "text/html"),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".gif", "image/gif"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END   */

#define REALM "MCM FWUP"
// Values for http_getState()
#define FW_UPLOAD_INIT			0	// Initial state, set by server (must be 0)
#define FW_UPLOAD_NORM			1	// Normal state (reading client data)
// http_setCond() state variables
#define FW_UPLOAD_ERROR			0	// Error code (<0)
#if HTTP_MAX_COND < 1
	#fatal "HTTP_MAX_COND must be >= 1 if using firmware_upload()"
#endif

SSPEC_RULETABLE_START
	   SSPEC_MM_RULE("/fw/", REALM, ADMIN_GROUP, ADMIN_GROUP,
		SERVER_HTTP, SERVER_AUTH_DIGEST, NULL)
SSPEC_RULETABLE_END

// redirects http://hostname/ to upload page for new firmware
int root_htm(HttpState *http)
{
	cgi_redirectto( http, "/upload.zhtml");
	return CGI_OK;
}

struct tm  rtc1;
unsigned long t01;

struct {
	char	running[40];		// information on the currently running firmware
	char	temp[40];			// information on the uploaded file
	int	show_temp;			// =0 if no temp. firmware available otherwise =1
   int	in_status;        // installation status
   int   up_status;        // upload status
	char	error[50];        // contains error message if any
	int	install;	         // flag to initiate install
   long  int fwlen;        // contains size of uploaded firmware
} firmware;
#web firmware
#web firmware.install
#web_update firmware.install install_firmware

char fwitime[10], fwidate[12], fwutime[10], fwudate[12];
#web fwitime
#web fwidate
#web fwutime
#web fwudate

char FIRMWARE_URL[50];
#web FIRMWARE_URL
#web_update FIRMWARE_URL download_and_install1

void* save_data[7];
unsigned int save_lens[7];

tcp_Socket demosock;

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
      printf("\nUnmount Error on %c: %ls\n", 'A' + i, strerror(rc));
      }
      }
   }
}
#endif

void fwinit()
{
  int rc, uid, j;

  // Store network configuration data in User Block.
  // **********************************************************************

  strcpy(fwutime,"00:00:00");
  strcpy(fwudate,"00-00-0000");
  strcpy(fwitime,"00:00:00");
  strcpy(fwidate,"00-00-0000");

  save_data[0] = MY_IP_ADDR;
  save_data[1] = MY_NET_MASK;
  save_data[2] = MY_GATEWAY;
  save_data[3] = fwutime;
  save_data[4] = fwudate;
  save_data[5] = fwitime;
  save_data[6] = fwidate;

  save_lens[0] = 20;
  save_lens[1] = 20;
  save_lens[2] = 20;
  save_lens[3] = 20;
  save_lens[4] = 20;
  save_lens[5] = 20;
  save_lens[6] = 20;

  #ifdef MY_IP_ADDR
  writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
  #else
  readUserBlockArray(save_data, save_lens, 7, 0);
  #endif
  // **********************************************************************

  strcpy(ip,MY_IP_ADDR);
  strcpy(mask,MY_NET_MASK);
  strcpy(gateway,MY_GATEWAY);

  firmware.install   = 0;
  firmware.up_status = 0;

  for(j=0;j<50;j++)
  FIRMWARE_URL[j]=0;

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
	// unmount all FAT volumes on exit
   atexit( unmount_all);
#endif

   printf( "\nSetting up userid...\n");      // Create a user ID
   uid = sauth_adduser(ADMIN_USER, ADMIN_PASS, SERVER_HTTP);
   if (uid < 0)
   {
   	printf( "\nFailed to create userid, rc=%d\n", uid);
   	exit (uid);
	}

   sauth_setusermask(uid, ADMIN_GROUP, NULL);
   sauth_setwriteaccess(uid, SERVER_HTTP);
	printf( "\nInitialize RabbitWeb variables...\n");
	update_webvars();
}

// need to call this after upload completes...
void update_webvars()
{
	firmware_info_t		fi;
	int i, err;
   unsigned long t01;

   t01 = read_rtc();
   mktm(&rtc1,t01);

	// load information on the current firmware
	fiProgramInfo( &fi);
   sprintf( firmware.running, "%s v%u.%02x", fi.program_name,
            fi.version >> 8, fi.version & 0xFF);

	#ifdef BU_TEMP_USE_DIRECT_WRITE
		firmware.show_temp = 0;   	 // no temp firmware to show information about
	#else
		firmware.show_temp = 1;
	#endif

   // load information of firmware stored in temporary location
   err = buOpenFirmwareTemp( BU_FLAG_NONE);
   if (err)
   {
      printf("\nFirmware file not present\n");
      strcpy( firmware.error, "Firmware file not present");
      firmware.in_status = -1;
   }
   else
   {
	   i = 0;
	   do {
	      err = buGetInfo(&fi);
	      } while ( (err == -EBUSY) && (++i < 20) );

      if (err)
      {
      printf("\nFirmware file is currupted\n");
      strcpy( firmware.error, "Firmware file is currupted");
      //strcpy( firmware.temp, "Firmware file is currupted");
      firmware.in_status = -1;
      }
      else
      {
         sprintf( firmware.temp, "%s v%u.%02x", fi.program_name,
         fi.version >> 8, fi.version & 0xFF);
         // firmware is good, try to verify it
         err = buRewindFirmware();
         if (!err)
         {
            if(!buVerifyFirmwareBlocking())
            {
            firmware.in_status = 0;
            *firmware.error = '\0';
            }
            else
            {
            printf("\nError verifying firmware\n");
            strcpy( firmware.error, "Error verifying firmware");
            firmware.in_status = -1;
          }
       }
     }
   }
   while (buCloseFirmware() == -EBUSY);
}

void download_and_install1()
{
printf( "\n\nConnecting...");
download_and_install( FIRMWARE_URL);
FIRMWARE_URL[0]=' ';
}

int download_and_install( char *url)
{
	bu_download_t	dl;
	int result;

   printf( "\nURL : %s\n", url);

	result = buDownloadInit( &dl, &demosock, url);
   if (result)
   {
      printf( "\ncouldn't initiate download (error %d)\n", result);
      strcpy(firmware.error, "Error initiating download");
      firmware.up_status = -1;
   }

   else
   {
      printf( "\nDownloading......\n");
      while ((result = buDownloadTick( &dl)) == -EBUSY)
      {
         if (dl.filesize)
         {
            printf( " %lu/%lu (%u%%)\r", dl.bytesread, dl.filesize,
                     (int) (dl.bytesread * 100 / dl.filesize));
         }
         else
         {
            printf( " %lu bytes read\r", dl.bytesread);
         }
      }

      if (result == -ENODATA)
      {
         printf( "\ndownload canceled, file did not contain valid firmware\n");
         strcpy(firmware.error, "Download canceled (invalid firmware)");
         firmware.up_status = -1;
      }
      else if (result == 0)
      {
         t01 = read_rtc();
         mktm(&rtc1,t01);
         sprintf(fwutime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
         sprintf(fwudate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));
         writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
         printf( "\ndownload complete, %lu bytes\n", dl.bytesread);
         install_firmware();
         firmware.up_status = 1;
      }
      else
      {
         printf( "\ndownload canceled (error %d)\n", result);
         strcpy(firmware.error, "Download canceled");
         firmware.up_status = -1;
	      #ifdef BU_TEMP_USE_DIRECT_WRITE
	         printf( "\nattempting to restore boot firmware from RAM\n");
	         // There was an error downloading or installing the firmware,
	         // so we need to restore the boot firmware image from the copy
	         // of the program running in RAM.
	         result = buRestoreFirmware( 0);
	         if (result)
	         {
	            printf( "\nerror %d attempting to restore firmware\n", result);
               strcpy(firmware.error, "Error attempting to restore firmware");
	            // Firmware stored on the boot flash is corrupted and cannot be restored.
	         }
	         else
	         {
	            printf( "\nrestore complete\n");
	         }
	      #endif
      }
   }

   return result;
}

void install_firmware()
{
	int i, result, progress;

	printf ("\nuser clicked install/reboot button\n\n");

	#ifdef BU_TEMP_USE_DIRECT_WRITE
		// Firmware is already installed, just need to shut down HTTP server and reboot.
      http_shutdown(0);
		exit(0);
	#endif

   result = buOpenFirmwareTemp( BU_FLAG_NONE);

	if (result)
	{
      printf("Error opening firmware");
      strcpy(firmware.error, "Error opening firmware");
      firmware.in_status = -1;
	}
	else
	{
    progress = 0;
      do
      {
         printf( "\rverify %u.%02u%% completed\r", progress / 100, progress % 100);
         result = buVerifyFirmware( &progress);
      } while (result == -EAGAIN);
         printf( "\rverify %u.%02u%% completed\n", 100, 00);

      if (result)
      {
         printf( "\nError %d verifying firmware\n", result);
         strcpy(firmware.error, "Error verifying firmware");
         printf( "\nfirmware image bad, installation aborted\n");
         firmware.in_status = -1;
      }

	   else
	   {
	      http_shutdown(0);       	      // shut down HTTP server and install
	      if (buInstallFirmware())
	      {
	         // on error, restart http server
            printf("\nError installing Firmware. Initializing HTTP.....\n");
            strcpy(firmware.error, "Error installing firmware");
            firmware.in_status = -1;
	         http_init();
	      }
	      else
	      {
            t01 = read_rtc();
            mktm(&rtc1,t01);
            sprintf(fwitime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
            sprintf(fwidate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));
            writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
	         // Install was successful, reboot into new firmware.
            printf("\nFirmware installed. Rebooting........\n");
            firmware.install = 0;
            firmware.in_status = 0;
	      }
	   }
	}
   // make sure firmware file is closed if there were any errors
	while (buCloseFirmware() == -EBUSY);
   forceWatchdogTimeout();
   //exit( 0);
}

// Firmware Upload function, based on http_defaultCGI from http.lib
int firmware_upload(HttpState * s)
{
	auto char * fname;		// Pointer to field (i.e. resource to upload) name
   auto int rc, offset, mode, wlen, error;

	if (!(mode = http_getState(s)))
	{
      http_setState(s, FW_UPLOAD_NORM);		// Next state is "normal"
      http_setCond(s, FW_UPLOAD_ERROR, -1);	// Temp file hasn't been opened
	   http_genHeader(s, http_getData(s), HTTP_MAXBUFFER, 200, NULL, 0,
      "<html><head><meta http-equiv='refresh' content='0;url=upload.zhtml'></head><body>Uploading</body></html>");
      return CGI_SEND;	// Write string to client.
   }

   rc = CGI_OK;         // Default return code.

   // Main switch depending on the current CGI action code
	switch (http_getAction(s))
	{
   	case CGI_START:        // Start of a new part of the multi-part data.
      	fname = http_getField(s);

         if (fname[0] != '/')
         {
         	http_skipCGI(s);	// Not a file: skip to next part.
         }
         else
         {
				while ( (error = buTempCreate()) == -EBUSY);
            http_setCond(s, FW_UPLOAD_ERROR, error); // Save error code
            if (error < 0)
            {
               firmware.up_status = -1;
               http_skipCGI(s);	// Skip to next part after sending this buffer
      		}
            rc = CGI_SEND;
         }
      break;

   	case CGI_DATA:
      	// This is data to write to the resource.
         if (http_getCond(s, FW_UPLOAD_ERROR) < 0)
         {
         	// Skip data if there is an error opening the resource
         	return 0;
			}

			offset = 0;
			do {
	         wlen = buTempWrite( http_getData(s) + offset,
	                http_getDataLength(s) - offset);
	         if (wlen == -EBUSY)
	         {
					wlen = 0;     // ignore BUSY error and try again
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
               firmware.up_status = -1;
	         }
	         offset += wlen;
			} while (offset != http_getDataLength(s));
      break;

   	case CGI_END:             // End of this file.  Close it.
			while (buTempClose() == -EBUSY);
      	http_setCond(s, FW_UPLOAD_ERROR, -1);	// No open resource now
         update_webvars();
         firmware.fwlen=http_getContentLength(s)/1024;
      break;

      case CGI_EOF:             // Normal end of entire set of parts.
         rc = CGI_SEND_DONE;
         firmware.up_status = 1;
         t01 = read_rtc();
   		mktm(&rtc1,t01);
   		sprintf(fwutime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
   		sprintf(fwudate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));
         writeUserBlockArray(0, (const void * const *) save_data, save_lens, 7);
         printf("\nUpload completed\n");
      break;

      case CGI_ABORT:
      	// Server had to abort the connection. Clean up resource if one was open.
			while (buTempClose() == -EBUSY);
      break;
   }
   return rc;
}