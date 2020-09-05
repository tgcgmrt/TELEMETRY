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
	int j;

   //Initialize TCP/IP, Web server, and reserve port 80
	sock_init_or_exit(1);
	http_init();
	tcp_reserveport(80);

	while (1)
	{
   	http_handler();	//Run as fast as possible!

      costate		//Throttle back the C code so we can drive the web server
      {
      	waitfor (DelayMs(100));  //delay 100 milliseconds between calls
      	printf("j = %d\r", j++);
      }

      costate		//Throttle back your function so we can drive the web server
      {
      	waitfor (DelayMs(400));	//delay 400 milliseconds between calls
      	//Your special function here
      }
	}
}


