// **************************************************************************** //
// *******                  Program : Noise Gen PIU                    ******** //
// *******                    Date : 20/09/2015                        ******** //
// *******                     Version : V1.00                         ******** //
// *******        HTTP Server & client for Remote Firmware Update.     ******** //
// *******     Remote Firmware Update (v1.0 & v2.0) using USER BLOCK   ******** //
// *******                  Modified by Rahul Bhor.                    ******** //
// **************************************************************************** //

#class    auto
#memmap   xmem

#use      "rcm43xx.lib"	             // Use with RCM 4300
#use      "adc_ads7870.lib"
#include  "define.c"
#include  "firmware_update.c"
#include  "init.c"
#use	    "dcrtcp.lib"
#use  	 "http.lib"
#use      "spi.lib"

SSPEC_MIMETABLE_START
   SSPEC_MIME_FUNC(".zhtml","text/html",zhtml_handler),
   SSPEC_MIME_FUNC(".xml","text/xml",zhtml_handler),
   SSPEC_MIME(".html","text/html"),
   SSPEC_MIME(".jpg", "image/jpg"),
	SSPEC_MIME(".cgi", "")
SSPEC_MIMETABLE_END

SSPEC_RESOURCETABLE_START
   SSPEC_RESOURCE_XMEMFILE("/",homep_zhtml),
   SSPEC_RESOURCE_XMEMFILE("/homep.zhtml",homep_zhtml),
   //SSPEC_RESOURCE_XMEMFILE("/MCM_Set1.zhtml",MCM_Set1_zhtml),
   SSPEC_RESOURCE_XMEMFILE("/Mon_Sum.xml",Mon_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/gmrt.jpg",gmrt_jpg),
	SSPEC_RESOURCE_XMEMFILE("/MCM_Mon.zhtml",MCM_Mon_zhtml),
	SSPEC_RESOURCE_XMEMFILE("/calculations.zhtml",calculations_zhtml),
   SSPEC_RESOURCE_FUNCTION("/", root_htm),
   SSPEC_RESOURCE_XMEMFILE("/up_Sum.xml", up_Sum_xml),
   SSPEC_RESOURCE_XMEMFILE("/fw/upload.zhtml", upload_zhtml),
   SSPEC_RESOURCE_P_CGI("/fw/upload.cgi", firmware_upload,
   REALM, ADMIN_GROUP, 0x0000, SERVER_HTTP, SERVER_AUTH_DIGEST)
SSPEC_RESOURCETABLE_END

static tcp_Socket Socket;

//.... Generating milisecond delay .... //

void Ms_delay(unsigned int delay)
{
	auto unsigned long delay_time;

	delay_time = MS_TIMER + delay;
   while((long)(MS_TIMER - delay_time) < 0 );
}

int atox(char *mask)
{
  int hex_value;
  char msk[4];

  strcpy(msk, mask);

  printf("%s ",msk);

  if((msk[0] < 58) && (msk[0] > 47))
   msk[0] = msk[0] - '0';
  else if((msk[0] > 64) && (msk[0] < 71))
   msk[0] = msk[0] - 'A' + 10;
  else if((msk[0] > 96) && (msk[0] < 103))
   msk[0] = msk[0] - 'a' + 10;
  else
   msk[0] = 0;

  if((msk[1] < 58) && (msk[1] > 47))
   msk[1] = msk[1] - '0';
  else if((msk[1] > 64) && (msk[1] < 71 ))
   msk[1] = msk[1] - 'A' + 10;
  else if((msk[1] > 96) && (msk[1] < 103))
   msk[1] = msk[1] - 'a' + 10;
  else
   msk[1] = 0;

  hex_value = (((msk[0]) * 16) + (msk[1]));

  return hex_value;
}

__nodebug double pwm_init1(unsigned long frequency)
{
	auto double divisor;
	auto double base_frequency;

	base_frequency = (double)freq_divider * (double)PWM_FREQ_FACTOR;
	divisor = base_frequency/frequency - 1;

	if(divisor < 0)
	{
		divisor = 0;
	}
	else if(divisor > 255)
	{
		divisor = 255;
	}
	WrPortI(TAT9R, &TAT9RShadow, (char)divisor);
	if(divisor > 0)
	{
		return base_frequency/(divisor+1);
	}
	else
	{
		return base_frequency;
	}
}

/*cofunc int mcm_clnt()
{
   int wait_var;

   ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP, IFS_END);

   while (ifpending(IF_ETH0) == IF_COMING_UP)
         tcp_tick(NULL);

   wait_var = 0;

   while(1)
    {
      wait_var++;
      if(wait_var == 100)
      {
        wait_var = 0;
      }
      yield;
    }

   http_handler();

   return 0;
}  */

// ..... Main start .... //
main()
{
int i,j,a,b,c,o,k,j1;
int sr[11];
float rfi_atten_Mon1;
k=0;

  fwinit();
  brdInit();
  SPIinit();
  PortInit();
  sock_init();
  http_init();
  Struct_Init();
  ADC_Init();

  tcp_reserveport(80);

  ifconfig( IF_ETH0, IFS_DOWN, IFS_IPADDR,aton(MY_IP_ADDR), IFS_NETMASK,aton(MY_NET_MASK), IFS_ROUTER_SET,aton(MY_GATEWAY), IFS_UP, IFS_END);
  while (ifpending(IF_ETH0) == IF_COMING_UP)
  tcp_tick(NULL);

  sr[0]=1;
  sr[1]=0;
  sr[2]=0;
  sr[3]=0;
  sr[4]=0;
  sr[5]=0;
  sr[6]=0;
  sr[7]=0;
  sr[8]=0;
  sr[9]=0;
  sr[10]=0;

  while(1)
  {
    http_handler();
     /* costate    // This will open client socket at port 3000 //
     {
       wfd mcm_clnt();
     } */

    costate
      {
      	http_handler();
      	tcp_tick(NULL);
      }

     costate
   	{
   		if(atten_mode==3)
   	   {
   	   	strcpy(rfi_mode_mon, "Random Pulse noise attenuation");
         	a=0;
	         b=0;
	         c=0;

	         if(sr[0]==1)
	            a=1;
	         if(sr[7]==1)
	            b=1;
	         if(sr[10]==1)
	            c=1;

	         j1= (a ^ b);
	         j= (j1 ^ c);

	         for(i=10;i>0;i--)
	         {
	            sr[i]=sr[i-1];
	         }
	         sr[0]=j;

	         //printf("[%4d] >> ",k++);
	         for(i=10;i>=0;i--)
	         //printf("%d ",sr[i]);
	         //printf(", j=%d, a=%d, b=%d, c=%d \n",j, a, b, c);

	         o=((((((((sr[0]|(sr[1]<<1))|(sr[2]<<2))|(sr[3]<<3))|(sr[4]<<4))|(sr[5]<<5))|(sr[6]<<6))|(sr[7]<<7))|(sr[7]<<7));
            word32_1=~o;

            rfi_atten_Mon1=0;
            if(o&0x01==0x01)
            rfi_atten_Mon1=rfi_atten_Mon1+0.5;
            if(o&0x02==0x02)
            rfi_atten_Mon1=rfi_atten_Mon1+1;
            if(o&0x04==0x04)
            rfi_atten_Mon1=rfi_atten_Mon1+2;
            if(o&0x08==0x08)
            rfi_atten_Mon1=rfi_atten_Mon1+4;
            if(o&0x10==0x10)
            rfi_atten_Mon1=rfi_atten_Mon1+8;
            if(o&0x20==0x20)
            rfi_atten_Mon1=rfi_atten_Mon1+16;
            sprintf(rfi_atten_Mon, "%5.1f", rfi_atten_Mon1);
	         //printf("%x\n",o);
	         BitWrPortI(PBDR, &PBDRShadow, 1, 6);
	         BitWrPortI(PBDR, &PBDRShadow, 0, 2);
	         WrPortI(PADR, &PADRShadow, word32_1);
	         BitWrPortI(PBDR, &PBDRShadow, 1, 2);
	         BitWrPortI(PBDR, &PBDRShadow, 0, 6);

	         /*if(sr[0]==1 && sr[1]==0&& sr[1]==0 && sr[2]==0 && sr[3]==0 && sr[4]==0 && sr[5]==0 && sr[6]==0 && sr[7]==0)
	         {
	            if(sr[8]==0 && sr[9]==0 && sr[10]==0)
	            {
	               //printf("Repeated after %d\n",k);
	               while(!kbhit());
	            }
	         } */
	         waitfor (DelayMs(8));
	      }
	   }
  }
}

void Set_32bit()
{
  printf("Entering Set_32bit\n");

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 2);
  BitWrPortI(PBDR, &PBDRShadow, 0, 3);
  BitWrPortI(PBDR, &PBDRShadow, 0, 4);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);

  // Set Most Significant Word and Least Significant Word on 32 Bit digital o/ps //

  WrPortI(PADR, &PADRShadow, word32_1);
  BitWrPortI(PBDR, &PBDRShadow, 1, 2); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, word32_2);
  BitWrPortI(PBDR, &PBDRShadow, 1, 3); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, word32_3);
  BitWrPortI(PBDR, &PBDRShadow, 1, 4); // Latching value at LSB latch //

  WrPortI(PADR, &PADRShadow, word32_4);
  BitWrPortI(PBDR, &PBDRShadow, 1, 5); // Latching value at LSB latch //

  BitWrPortI(PBDR, &PBDRShadow, 0, 6); // Enable output of both latches at same time //

}

void Set_atten_mode()
{
printf("RFI Attenuation Mode: %d \n", atten_mode);
if(atten_mode==1)
{
word32_4 = word32_4 & 0xFE;
word32_4 = word32_4 | 0x01;
strcpy(rfi_mode_mon, "Direct Connection to ADC");
}
if(atten_mode==2)
{
word32_4 = word32_4 & 0xFE;
strcpy(rfi_mode_mon, "Radiation Through Antenna");
}
Set_32bit();
}

void Set_time_mode()
{
if(time_mode==1)
{
strcpy(time_mode_mon, "Fix duty cycle");
}
else
{
strcpy(time_mode_mon, "Random time instances");
}
}

void Set_rfi_atten()
{
int Attn_val;
float rfi_atten_Mon1;

rfi_atten_Mon1 = (float) rfi_atten/2 - 0.5;
printf("Set_rfi_attenuation: %5.1f dB\n", rfi_atten_Mon1);
sprintf(rfi_atten_Mon, "%5.1f", rfi_atten_Mon1);

 BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 BitWrPortI(PBDR, &PBDRShadow, 0, 3);
 BitWrPortI(PBDR, &PBDRShadow, 0, 4);
 BitWrPortI(PBDR, &PBDRShadow, 0, 5);

 Attn_val= (int) (rfi_atten/2);
 WrPortI(PADR, &PADRShadow, 0x00);

 word32_1 = ~Attn_val;
 word32_1 = word32_1 << 1;

 if(rfi_atten%2==0)
 {
 word32_1 = word32_1 + 2;
 word32_1 = word32_1 & 0xFE;
 }

 else
 {
 word32_1 = word32_1 & 0xFE;
 word32_1 = word32_1 | 0x01;
 }
 word32_1 = word32_1 & 0x3F;
 Set_32bit();
}

void Set_nf_atten()
{
int Attn_val2;
float nf_atten_Mon1;

nf_atten_Mon1 = (float) nf_atten/2 - 0.5;
printf("Set_nf_attenuation: %5.1f dB\n", nf_atten_Mon1);
sprintf(nf_atten_Mon, "%5.1f", nf_atten_Mon1);

 BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);
 BitWrPortI(PBDR, &PBDRShadow, 0, 2);
 BitWrPortI(PBDR, &PBDRShadow, 0, 3);
 BitWrPortI(PBDR, &PBDRShadow, 0, 4);
 BitWrPortI(PBDR, &PBDRShadow, 0, 5);

 Attn_val2= (int) (nf_atten/2);
 WrPortI(PADR, &PADRShadow, 0x00);

 word32_2 = ~Attn_val2;
 word32_2 = word32_2 << 1;

 if(nf_atten%2==0)
 {
 word32_2 = word32_2 + 2;
 word32_2 = word32_2 & 0xFE;
 }

 else
 {
 word32_2 = word32_2 & 0xFE;
 word32_2 = word32_2 | 0x01;
 }
 word32_2 = word32_2 & 0x3F;
 Set_32bit();
}

void Set_total_time()
{
long t_total_mon1;

t_total_mon1 = t_total * 8;
printf("Set Total Time: %ld ms\n", t_total_mon1);
sprintf(t_total_mon, "%ld", t_total_mon1);

word32_3 = t_total;
word32_4 = word32_4 & 0x3F;
word32_4 = word32_4 | 0x40;
Set_32bit();
}

void Set_on_time()
{
printf("Set ON Time: %d us\n", t_on);
sprintf(t_on_mon, "%d", t_on);

word32_3 = t_on;
word32_4 = word32_4 & 0x3F;
word32_4 = word32_4 | 0x80;
Set_32bit();
}

void Set_pwm()
{
unsigned long  a_freq, s_freq;
float duty_c, freq1;
int rc, pwm_options;

pwm_options = PWM_USEPORTC;
pwm_options |= PWM_SPREAD;

a_freq = s_freq = 0;

printf("ON time : %ld\n", t_on);
printf("total time : %ld\n", t_total);
duty_c = (float) t_on / t_total;
printf("Duty cycle : %f\n", duty_c);

freq1 = (float) 1 / t_total;
s_freq = (unsigned long)freq1 * 1000000;
//st_freq = s_freq * 0.69444;       // error correction
printf("Given freq. : %ld\n", s_freq);

a_freq = (unsigned long) pwm_init1(s_freq);
printf("Actual freq. : %ld\n", a_freq);

rc = pwm_set(0, (int)(duty_c * 1024.0), pwm_options);

if(rc==0)
printf("success !\n");
}