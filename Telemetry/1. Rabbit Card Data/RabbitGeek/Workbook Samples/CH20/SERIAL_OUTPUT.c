#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"
#use "rcm56xxw.lib"

#define CINBUFSIZE  31
#define COUTBUFSIZE 31

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH20/Dynamic_W07.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH20/my_serial_data.xml"    my_serial_data_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

//Create a global variable and register with RabbitWeb
char	Serial_Data[80];
#web  Serial_Data

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_serial_data.xml", my_serial_data_xml)
SSPEC_RESOURCETABLE_END

void main()
{
	char s[16];			//data buffer

   //Clear both strings
   strcpy(s,"");
   strcpy(Serial_Data,"");

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   serCopen(9600L);

   while (1)
   {
		http_handler();

      if ( (serCread(s, 2, 20)) > 0)
      {
         s[1] = 0;	//terminate with a NULL

         strcat(Serial_Data, s);	//Add new data to string

         //If string gets to big, dump the current data
         if (strlen(Serial_Data) > 75)
         {
         	strcpy(Serial_Data,"");
         }

         printf("%s\r", Serial_Data);
      }
   }
}