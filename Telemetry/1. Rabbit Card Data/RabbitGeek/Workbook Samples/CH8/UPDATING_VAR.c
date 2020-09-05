#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH8/mypage.zhtml"  mypage_zhtml

SSPEC_MIMETABLE_START
	SSPEC_MIME_FUNC(".zhtml", "text/html", zhtml_handler)
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
	SSPEC_RESOURCE_XMEMFILE("/", mypage_zhtml),
   SSPEC_RESOURCE_XMEMFILE("/index.zhtml", mypage_zhtml)
SSPEC_RESOURCETABLE_END

int My_Data;	//Create a global variable
#web My_Data	//Register it as a Web Variable

void main()
{
	My_Data = 2009;

   //Initialize TCP/IP, Web server, and reserve port 80
	sock_init_or_exit(1);
	http_init();
	tcp_reserveport(80);

	while (1)
	{
		http_handler();
      My_Data++;
	}
}

