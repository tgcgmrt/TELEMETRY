#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"
#use "rcm56xxw.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH19/Dynamic_W05.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH19/my_data4.xml"    		my_data4_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

//Create global variables
int   DigIn_Port_A[8];
int   DigIn_Port_B[8];
int   DigIn_Port_C[8];
int   DigIn_Port_D[8];
int   DigIn_Port_E[8];

//Register arrays with RabbitWeb
#web  DigIn_Port_A
#web  DigIn_Port_B
#web  DigIn_Port_C
#web  DigIn_Port_D
#web  DigIn_Port_E

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data4.xml", my_data4_xml)
SSPEC_RESOURCETABLE_END

void main()
{
   int i;	//for loop index

   //Initialize MiniCore hardware
   brdInit();

   /////////////////////////////////////////////////////////////////////////
   // Configure Port B -- Leave PB0 and PB1 untouched (used for other purposes)
   // For use with Digitial I/O expansion board (Deluxe kit)
   /////////////////////////////////////////////////////////////////////////
   WrPortI(PBDDR, &PBDDRShadow, RdPortI(PBDDR) & 0x03); //Set PB2-7 as inputs

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   while (1) {
		http_handler();

      //Read status of parallel ports
      for (i=0;i<8;i++)
      {
      	DigIn_Port_A[i] = !BitRdPortI(PADR, i);
         DigIn_Port_B[i] = !BitRdPortI(PBDR, i);
         DigIn_Port_C[i] = !BitRdPortI(PCDR, i);
         DigIn_Port_D[i] = !BitRdPortI(PDDR, i);
         DigIn_Port_E[i] = !BitRdPortI(PEDR, i);
      }
   }
}