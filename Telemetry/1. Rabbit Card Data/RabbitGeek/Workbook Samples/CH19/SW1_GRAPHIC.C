#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"
#use "rcm56xxw.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH19/Dynamic_W06.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH19/my_data5.xml"    		my_data5_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data5.xml", my_data5_xml)
SSPEC_RESOURCETABLE_END

int Switch_Time;
#web Switch_Time

void main()
{
   Switch_Time = 0;

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   //Initialize MiniCore hardware
   brdInit();

   while (1)
   {
      http_handler();

      costate
      {
      	waitfor(DelayMs(100));
         if (!BitRdPortI(PDDR, 1))
      	{
      		if (Switch_Time < 100)
            {
            	Switch_Time++;
            }
      	}
      	else if (Switch_Time > 0)
      	{
      		Switch_Time--;
      	}
      	printf("Switch_Time = %d  \r",Switch_Time);
      }
   }
}