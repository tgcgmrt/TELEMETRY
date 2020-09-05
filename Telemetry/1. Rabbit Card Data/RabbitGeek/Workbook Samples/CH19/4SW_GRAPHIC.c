#class auto

#define USE_RABBITWEB 1

#memmap xmem
#use "dcrtcp.lib"
#use "http.lib"
#use "rcm56xxw.lib"

//You will need to set this path to point at your HTML file
#ximport "C:/RabbitGeek/Workbook Samples/CH19/Dynamic_W06.html"    	index_html
#ximport "C:/RabbitGeek/Workbook Samples/CH19/my_data5.xml"    		my_data5_xml

SSPEC_MIMETABLE_START
	SSPEC_MIME(".html", "text/html"),
   SSPEC_MIME_FUNC(".xml","text/xml", zhtml_handler)
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/", index_html),
   SSPEC_RESOURCE_XMEMFILE("/index.html", index_html),
   SSPEC_RESOURCE_XMEMFILE("/my_data5.xml", my_data5_xml)
SSPEC_RESOURCETABLE_END

int Switch_Time[4];
#web Switch_Time

int Read_Switches()
{
	int switch_data;

   //Read entire switch in one pass
   switch_data = RdPortI(PBDR);

   if (!(switch_data & 0x04))	//Test PB4 	(Digital I/O board S1)
   {
   	printf("PB2 is hot\n");
   }

   if (!(switch_data & 0x05))	//Test PB5	(Digital I/O board S2)
   {
   	printf("PB3 is hot\n");
   }

   if (!(switch_data & 0x06))	//Test PB6	(Digital I/O board S3)
   {
   	printf("PB4 is hot\n");
   }

   if (!(switch_data & 0x07))	//Test PB7	(Digital I/O board S4)
   {
   	printf("PB5 is hot\n");
   }
}

void main()
{
   //Initialize Data Array
   Switch_Time[0] = Switch_Time[1] = Switch_Time[2] = Switch_Time[3] = 0;

   //Initialize TCP/IP, Web server, and reserve port 80
   sock_init_or_exit(1);
   http_init();
   tcp_reserveport(80);

   //Initialize MiniCore hardware
   brdInit();

   /////////////////////////////////////////////////////////////////////////
   // Configure Port B -- Leave PB0 and PB1 untouched (used for other purposes)
   // For use with Digitial I/O expansion board (Deluxe kit)
   /////////////////////////////////////////////////////////////////////////
   WrPortI(PBDDR, &PBDDRShadow, RdPortI(PBDDR) & 0x03); //Set PB2-7 as inputs


   while (1)
   {
      http_handler();

      costate
      {
      	waitfor(DelayMs(100));
         Read_Switches();


/*
         if (!BitRdPortI(PDDR, 1))
      	{
      		if (Switch_Time < 100)
            {
            	Switch_Time++;
            }
      	}
      	else if (Switch_Time > 0)
      	{
      		Switch_Time--;
      	}
      	printf("Switch_Time = %d  \r",Switch_Time);
*/
      }
   }
}