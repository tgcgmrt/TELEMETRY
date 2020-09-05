//Add TCPIP library & HTTP library
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH4/hello_world.html" index_html

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
	unsigned long time_0;	//used to store a time from MS_TIMER
	time_0 = 0;					//initialize to zero

   //Initialize TCP/IP, Web server, and reserve port 80
	sock_init_or_exit(1);
	http_init();
	tcp_reserveport(80);

	while(1)
	{
		printf("time elapsed = %lu\r",(MS_TIMER - time_0));
		http_handler();	//drive the web server
		time_0 = MS_TIMER;
	}
}

