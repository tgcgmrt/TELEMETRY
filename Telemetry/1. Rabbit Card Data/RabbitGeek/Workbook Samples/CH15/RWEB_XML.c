#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH15/Dynamic_W01.html"    		index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH15/my_data.xml"    				my_data_xml

//Create global variable and register it with RabbitWeb
int 	My_Data;
#web My_Data

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data.xml", my_data_xml)
SSPEC_RESOURCETABLE_END

void main()
{
   My_Data = 1;

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   while (1) {
      http_handler();
      My_Data++;	//Keep changing My_Data
   }
}