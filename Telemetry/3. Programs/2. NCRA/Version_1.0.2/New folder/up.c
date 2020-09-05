//*Add following line in main program after TCPCONFIG and before adding .lib files *****/
//            #include "up.c"
//*********** Add fwinit(); after http_init(); *****************************************/
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
	#define FAT_USE_FORWARDSLASH		   // Necessary for zserver.
	#use "fat16.lib"							// Must use FAT before http/zserver
#endif

// Only one server is really needed for the HTTP server as long as
// tcp_reserveport() is called on port 80.
#define HTTP_MAXSERVERS 1

// HTTP upload works better with a large buffer.  HTTP.LIB will xalloc
// HTTP_SOCK_BUF_SIZE bytes of xmem for each HTTP server socket
#define HTTP_SOCK_BUF_SIZE (12 * MAX_MTU)

#use   "dcrtcp.lib"
#use   "rcm43xx.lib"	             // Use with RCM 4300
#use   "http.lib"
#use   "board_update.lib"

// function prototype for cgi function to handle file upload
int firmware_upload(HttpState * s);
void install_firmware();
void fwinit();

// Define a group bit for updating resources...
#define ADMIN_GROUP	0x0002

#ximport  "upload.zhtml"    upload_zhtml

//**********Uncomment below MIMETABLE if not included in main application program*******//

/*SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".htm", "text/html"),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".gif", "image/gif"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END   */

#define REALM "UFW"

SSPEC_RULETABLE_START
	   SSPEC_MM_RULE("/fw/", REALM, ADMIN_GROUP, ADMIN_GROUP,
		SERVER_HTTP, SERVER_AUTH_DIGEST, NULL)
SSPEC_RULETABLE_END

// redirects http://hostname/ to upload page for new firmware
int root_htm(HttpState *http)
{
	cgi_redirectto( http, "/fw/upload.zhtml");
	return CGI_OK;
}

struct tm  rtc1;
unsigned long t01;

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

bbram char fwitime[10], fwidate[12], fwutime[10], fwudate[12], fwtime[10], fwdate[12];
#web fwitime
#web fwidate
#web fwutime
#web fwudate
#web fwtime
#web fwdate

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

   t01 = read_rtc();
   mktm(&rtc1,t01);
   sprintf(fwitime,"%02d:%02d:%02d",(rtc1.tm_hour),(rtc1.tm_min),rtc1.tm_sec);
   sprintf(fwidate,"%02d-%02d-%d",(rtc1.tm_mday),(rtc1.tm_mon),(1900+rtc1.tm_year));

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
      firmware.show_install = ( err ? -1 : 1 );
      firmware.show_status = ( err ? -1 : 0 );
   }
   while (buCloseFirmware() == -EBUSY);

	firmware.install = 0;
	*firmware.error = '\0';
}

void fwinit()
{
  int rc, i;
  char buf[20];
  int uid;

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

   // Ensure that user is a member of ADMIN_GROUP
   sauth_setusermask(uid, ADMIN_GROUP, NULL);

   // Also need to assign individual write access.
   sauth_setwriteaccess(uid, SERVER_HTTP);

   //printf( "Userid created successfully: use '%s' with password '%s'\n\n",
      //ADMIN_USER, ADMIN_PASS);

	printf( "Initialize RabbitWeb variables...\n\n");
	update_webvars();
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
   	// Initial state (FW_UPLOAD_INIT).
   	// Generate HTTP header
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

         printf("\n\nUpload completed\n\n");
         break;

      case CGI_ABORT:
      	// Server had to abort the connection.
			// Clean up resource if one was open.
			while (buTempClose() == -EBUSY);
         break;
   }

   return rc;
}