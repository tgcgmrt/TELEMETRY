#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"
#use "rcm56xxw.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH18/Dynamic_W04.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH18/my_data3.xml"    		my_data3_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

//Create a global variable and register with RabbitWeb
int   Switch_1;
#web  Switch_1

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data3.xml", my_data3_xml)
SSPEC_RESOURCETABLE_END

void main()
{
   //Initialize MiniCore hardware
   brdInit();

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   while (1)
   {
		http_handler();
      Switch_1 = !BitRdPortI(PDDR, 1);	//Read S1
   }
}