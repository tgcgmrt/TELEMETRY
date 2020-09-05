//Add TCPIP library & HTTP library
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH16/JS_Loop.html" index_html

//Add a type of file to our list of supported types
SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html")
SSPEC_MIMETABLE_END

//Add the specific files as server resources
SSPEC_RESOURCETABLE_START
	SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
SSPEC_RESOURCETABLE_END

void main()
{
	sock_init_or_exit(1);	//start the TCP/IP connection
	http_init();				//initialize the web server
	tcp_reserveport(80);		//optionally reserve of port 80 to boost speed

	while (1)
	{
		http_handler();		//drive the web server
	}
}