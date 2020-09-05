#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH17/Dynamic_W02.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH17/my_data2.xml"    		my_data2_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

//Create a global variables and register them with RabbitWeb
int 	My_Data1,
		My_Data2,
      My_Data3,
      My_Data4;

#web My_Data1
#web My_Data2
#web My_Data3
#web My_Data4

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data2.xml", my_data2_xml)
SSPEC_RESOURCETABLE_END

void main()
{
   My_Data1 = 1;
   My_Data2 = 10;
   My_Data3 = 100;
   My_Data4 = 1000;

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   while (1) {
      http_handler();

      costate //Only update the data every 500 ms
      {
      	waitfor(DelayMs(500));
         My_Data1++;
      	My_Data2++;
      	My_Data3++;
      	My_Data4++;
      }
   }
}