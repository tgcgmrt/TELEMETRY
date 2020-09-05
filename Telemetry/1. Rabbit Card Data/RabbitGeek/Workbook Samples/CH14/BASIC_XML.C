#class auto

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH14/dogs.xml"    		dogs_xml

SSPEC_MIMETABLE_START
   SSPEC_MIME(".xml","text/xml")	//Here is my XML MIME type!
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/dogs.xml", dogs_xml) 		//Here is my XML file!
SSPEC_RESOURCETABLE_END

void main()
{
   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   while (1)
	{
   	http_handler();
   }
}

