/*********** Add following line in main program before adding .lib files **************/
//           #include "firmware_update.c"
/*********** Add fwinit(); before http_init(); *****************************************/
/*********** Add this part in RESOURCETABLE of main program ***************************
             SSPEC_RESOURCE_FUNCTION("/", root_htm),
             SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
             SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
             REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
****************************************************************************************/

//#define BU_TEMP_USE_FAT				// use file on FAT filesystem
#define BU_TEMP_USE_SBF

#define  STDIO_DEBUG_SERIAL   SADR
#define	STDIO_DEBUG_BAUD		115200
#define	STDIO_DEBUG_ADDCR

#define USE_HTTP_UPLOAD		// Required for this demo, to include upload code.
#define DISABLE_DNS			// No name lookups required
#define SSPEC_FLASHRULES   // Define this because we are using a static rule table.

#ifdef BU_TEMP_USE_FAT
	// This value is required to enable FAT blocking mode for ZServer.lib
	#define FAT_BLOCK
	#define FAT_USE_FORWARDSLASH		// Necessary for zserver.
	#use "fat16.lib"							// Must use FAT before http/zserver
#endif

#define HTTP_MAXSERVERS 1

#define HTTP_SOCK_BUF_SIZE (12 * MAX_MTU)

#use   "dcrtcp.lib"
#use   "rcm43xx.lib"	             // Use with RCM 4300
#use   "http.lib"
#use   "http_client.lib"
#use   "ftp_client.lib"
#use   "board_update.lib"

int firmware_upload(HttpState * s);
void install_firmware();
void fwinit();

// Define a group bit for updating resources...
#define ADMIN_GROUP	0x0002

#ximport  "upload.zhtml"    upload_zhtml
#ximport  "up_Sum.xml"      up_Sum_xml

// Uncomment below part if not present in main program file.
/*SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".htm", "text/html"),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".gif", "image/gif"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END   */

#define REALM "MCM FU"
// Values for http_getState()
#define FW_UPLOAD_INIT			0	// Initial state, set by server (must be 0)
#define FW_UPLOAD_NORM			1	// Normal state (reading client data)
// http_setCond() state variables
#define FW_UPLOAD_ERROR			0	// Error code (<0)
#if HTTP_MAX_COND < 1
	#fatal "HTTP_MAX_COND must be >= 1 if using firmware_upload()"
#endif


void download_and_install1();
int download_and_install( char *url);
int install_firmware1();

SSPEC_RULETABLE_START
	   SSPEC_MM_RULE("/fw/", REALM, ADMIN_GROUP, ADMIN_GROUP,
		SERVER_HTTP, SERVER_AUTH_DIGEST, NULL)
SSPEC_RULETABLE_END

// redirects http://hostname/ to upload page for new firmware
int root_htm(HttpState *http)
{
	cgi_redirectto( http, "/fw/upload.zhtml");         /////////////////////////////////////////////////////
	return CGI_OK;
}

struct tm  rtc1;
unsigned long t01;

struct {
	char	running[40];		// information on the currently running firmware
	char	temp[40];			// information on the uploaded file
	int	show_temp;			// 1 == show info on temp firmware (set to 0 for
									// boards that only allow direct uploading)
	int   show_install;		// flag whether to show install button or not
									// 1 == show, 0 == don't show, -1 == show error
   int	show_status;
	char	error[80];
	int	install;	         // flag to initiate install
   long signed int fwlen;
} firmware;
#web firmware
#web firmware.install
#web_update firmware.install install_firmware

protected char fwitime[10], fwidate[12], fwutime[10], fwudate[12];
#web fwitime
#web fwidate
#web fwutime
#web fwudate

char FIRMWARE_URL[50];
#web FIRMWARE_URL
#web_update FIRMWARE_URL download_and_install1

int install_firmware1()
{
	firmware_info_t fi;
	int i, result, progress;

   printf( "verifying and installing new firmware\n");

   result = buOpenFirmwareTemp( BU_FLAG_NONE);
   if (!result)
   {
	   i = 0;
	   do {
	      result = buGetInfo( &fi);
	   } while ( (result == -EBUSY) && (++i < 20) );
   }

   if (result)
   {
      printf( "Error %d reading new firmware\n", result);
      strcpy(firmware.error, "Error reading new firmware");
      firmware.install = -1;
   }
   else
   {
      printf( "Found %s v%u.%02x...\n", fi.program_name,
              fi.version >> 8, fi.version & 0xFF);

      printf( "Attempting to install new version...\n");
      progress = 0;
      do
      {
         printf( "\r verify %u.%02u%%\r", progress / 100, progress % 100);
         result = buVerifyFirmware( &progress);
      } while (result == -EAGAIN);
      if (result)
      {
         printf( "\nError %d verifying firmware\n", result);
         strcpy(firmware.error, "Error verifying firmware");
         printf( "firmware image bad, installation aborted\n");
         firmware.install=-1;
      }
      else
      {
         printf( "verify complete, installing new firmware\n");
         result = buInstallFirmware();
         if (result)
         {
            printf( "!!! Error %d installing firmware !!!\n", result);
            strcpy(firmware.error, "Error installing firmware");
            firmware.install=-1;
         }
         else
         {
            t01 = read_rtc();
            mktm(&rtc1,t01);
            sprintf(fwitime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
            sprintf(fwidate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));

            printf( "\nInstall successful: rebooting.\n");
            firmware.install = 0;
            forceWatchdogTimeout();
            exit( 0);
         }
      }
   }

   // make sure firmware file is closed if there were any errors
	while (buCloseFirmware() == -EBUSY);
   return result;
}

tcp_Socket demosock;

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
      firmware.install = -1;
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
         printf( "download canceled, file did not contain valid firmware\n");
         strcpy(firmware.error, "Download canceled (invalid firmware)");
         firmware.install = -1;
      }
      else if (result == 0)
      {
         printf( "download complete, %lu bytes\n", dl.bytesread);
         result = install_firmware1();
      }
      else
      {
         printf( "download canceled (error %d)\n", result);
         strcpy(firmware.error, "Download canceled");
         firmware.install = -1;
	      #ifdef BU_TEMP_USE_DIRECT_WRITE
	         printf( "attempting to restore boot firmware from RAM\n");
	         // There was an error downloading or installing the firmware,
	         // so we need to restore the boot firmware image from the copy
	         // of the program running in RAM.
	         result = buRestoreFirmware( 0);
	         if (result)
	         {
	            printf( "error %d attempting to restore firmware\n", result);
               strcpy(firmware.error, "Error attempting to restore firmware");
               firmware.install = -1;
	            // At this point, the firmware stored on the boot flash is
	            // corrupted and cannot be restored.
	         }
	         else
	         {
	            printf( "restore complete\n");
	         }
	      #endif
      }
   }

   return result;
}

void download_and_install1()
{
printf( "\n\nConnecting...");

t01 = read_rtc();
mktm(&rtc1,t01);
sprintf(fwutime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
sprintf(fwudate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));

download_and_install( FIRMWARE_URL);
FIRMWARE_URL[0]=' ';
}

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
		// Firmware is already installed, just need to shut down HTTP server and reboot.
      http_shutdown(0);
		exit(0);
	#endif

   result = buOpenFirmwareTemp( BU_FLAG_NONE);

	if (result)
	{
      strcpy(firmware.error, "Error opening firmware");
      firmware.install = -1;
	}
	else
	{
	   result = buVerifyFirmwareBlocking();
	   if (result)
	   {
	      while (buCloseFirmware() == -EBUSY);
	      strcpy(firmware.error, "Error installing firmware");
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
            firmware.install = -1;
	         http_init();
	      }
	      else
	      {
            t01 = read_rtc();
            mktm(&rtc1,t01);
            sprintf(fwitime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
            sprintf(fwidate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));

	         // Install was successful, reboot into new firmware.
            printf("Firmware installed. Rebooting........\n");
            firmware.install = 0;
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
   unsigned long t01;

   t01 = read_rtc();
   mktm(&rtc1,t01);

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
	   i = 0;
	   do {
	      err = buGetInfo(&fi);
	   } while ( (err == -EBUSY) && (++i < 20) );
   }
   if (err)
   {
      strcpy( firmware.temp, "Error verifying firmware");
      firmware.show_install = -1;
      firmware.install = -1;
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
      // if no errors, show a Install button (1)
      // or show an error message that image is corrupted (-1)
      firmware.show_install = ( err ? -1 : 1 );
      firmware.show_status = ( err ? -1 : 0);
      firmware.install = 0;
   }
   while (buCloseFirmware() == -EBUSY);
	*firmware.error = '\0';
}

void fwinit()
{
  int rc, uid, j;
  char buf[20];

  for(j=0;j<50;j++)
  {
  FIRMWARE_URL[j]=0;
  }

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

   printf( "Setting up userid...\n\n");
   // Create a user ID
   uid = sauth_adduser(ADMIN_USER, ADMIN_PASS, SERVER_HTTP);
   if (uid < 0)
   {
   	printf( "Failed to create userid, rc=%d\n", uid);
   	exit (uid);
	}

   sauth_setusermask(uid, ADMIN_GROUP, NULL);
   sauth_setwriteaccess(uid, SERVER_HTTP);

	printf( "Initialize RabbitWeb variables...\n\n");
	update_webvars();
}

// Firmware Upload function, based on http_defaultCGI from http.lib

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

         t01 = read_rtc();
         mktm(&rtc1,t01);
         sprintf(fwutime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
         sprintf(fwudate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));


         //strcpy(fwutime, time1);
         //strcpy(fwudate, date);
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