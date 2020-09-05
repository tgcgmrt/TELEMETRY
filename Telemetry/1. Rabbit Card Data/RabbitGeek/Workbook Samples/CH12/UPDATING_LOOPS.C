#define USE_RABBITWEB 1

#memmap xmem

#use "dcrtcp.lib"
#use "http.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH12/mypage4.zhtml"  mypage4_zhtml

SSPEC_MIMETABLE_START
	SSPEC_MIME_FUNC(".zhtml", "text/html", zhtml_handler),SSPEC_MIME(".html", "text/html")
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
	SSPEC_RESOURCE_XMEMFILE("/", mypage4_zhtml),
	SSPEC_RESOURCE_XMEMFILE("/index.zhtml", mypage4_zhtml)
SSPEC_RESOURCETABLE_END

int My_Data[5];	//Create a global array
#web My_Data		//Register entire array

void main()
{
	int i;

   My_Data[0] = 2009;
	My_Data[1] = 2010;
	My_Data[2] = 2011;
	My_Data[3] = 2012;
	My_Data[4] = 2013;

   //Initialize TCP/IP, Web server, and reserve port 80
	sock_init_or_exit(1);
	http_init();
	tcp_reserveport(80);

	while (1)
	{
      	http_handler();
         for (i=0;i<5;i++)
         {
         	My_Data[i]++;
         }
	}
}

