
char chksum(char *ary)
{
char temp=0, i=0;

  for(i=0; i< (ary[0] -1); i++)
   {
    temp += ary[i];
   }
  temp = ~temp;
  temp += 1;

  return temp;
}

/******* Add Check Sum function *********************
 * Add CS at the end of the buffer
 * CS does no include MCM Address                  */
void AddCksum()
{
  int length, i;
  char cs = 0;

  length = *(buf_ptr0+1) + (*(buf_ptr0+2)<<8);
  for(i=1; i<length; i++) cs += *(buf_ptr0+i);
  cs = ~cs+1;
  buf_ptr0[length] = cs;
}

/******* Init buffer function ***********************
 * Make buffer sub function
 * convenient for buffer implementation            */
void InitBuff(char buff[])
{
  int i, length;
  length = buff[1] + (buff[2]<<8);
  for (i=1; i<length; i++) *(buf_ptr0+i) = buff[i]; // First is MCM Address
}

void Convert(int offset, char * mask_x)
{
  int j;
  unsigned long Argument0, Argument1;
  unsigned char tab1[3], tab2[3];
  //printf("Entering in Convert\n");

  //memset(exeCmd.Argument,0x00,100); // Make null

  for (j=0;j<2;j++) {tab1[j]=mask_x[j];tab2[j]=mask_x[j+2];}     //"ffff" mask format
  tab1[2] = tab2[2] = '\0';       //strtol() converts until '\n'

 #ifdef DEBUG
  printf(" mask_x[0] %c mask_x[2] %c mask_x[1] %c mask_x[3] %c \n",
           mask_x[0], mask_x[2], mask_x[1], mask_x[3]);

  printf(" tab1 %s tab2 %s \n", tab1, tab2);
 #endif

  exeCmd.Argument[0+offset] =(unsigned char) _n_strtol((const char *)tab2, NULL, 16);     //3412 order   // Hex Conversion */
  exeCmd.Argument[1+offset] = (unsigned char) _n_strtol((const char *)tab1, NULL, 16);

 // Argument0 = _n_strtol((const char *)tab2, NULL, 16);     //3412 order   // Hex Conversion */
 // Argument1 = _n_strtol((const char *)tab1, NULL, 16);
  //printf("Argument0 = %ld \nArgument1 = %ld \n", Argument0, Argument1);


  /*for (j=0;j<2;j++)
  {
  exeCmd.Argument[offset*2+j] = tab2[j];
  exeCmd.Argument[offset*2+j] = tab1[j];
  }*/

  #ifdef DEBUG
  printf("Argument0 = %x  Argument1 = %x \n", exeCmd.Argument[0+offset],exeCmd.Argument[1+offset]);
  #endif
}

void Mask32toArg(char * mask1, char * mask2)
{
int j;

  for(j=0;j<sizeof(exeCmd.Argument);j++)
  exeCmd.Argument[j]=0x00;
   Convert(0, mask1);
   Convert(2, mask2);
}

void Mask64toArg(char * mask1, char * mask2, char * mask3, char * mask4)
{
int j;

  for(j=0;j<sizeof(exeCmd.Argument);j++)
  exeCmd.Argument[j]=0x00;
  Convert(0, mask1); Convert(2, mask2);
  Convert(4, mask3); Convert(6, mask4);
}

void set_dig_mask64()
{
 int i;
 char buff[18];
 //char buff[]={0,0x11,0,0,1,0,9,0,4,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0};

 buff[0]=0;
 buff[1]=0x11;
 buff[2]=0;
 buff[3]=0;
 buff[4]=1;
 buff[5]=0;
 buff[6]=9;
 buff[7]=0;
 buff[8]=4;
 buff[9]=0xff;
 buff[10]=0xff;
 buff[11]=0xff;
 buff[12]=0xff;
 buff[13]=0xff;
 buff[14]=0xff;
 buff[15]=0xff;
 buff[16]=0xff;
 buff[17]=0;

 for(i=0; i<8; i++)
 buff[9+i] = exeCmd.Argument[i];
 InitBuff(buff);
}

void set_dig_mask32()
{
 int i;
 char buff[14];
 //char buff[]={0,0xd,0,0,1,0,5,0,3,0xff,0xff,0xff,0xff,0};

 buff[0]=0;
 buff[1]=0xd;
 buff[2]=0;
 buff[3]=0;
 buff[4]=1;
 buff[5]=0;
 buff[6]=5;
 buff[7]=0;
 buff[8]=3;
 buff[9]=0xff;
 buff[10]=0xff;
 buff[11]=0xff;
 buff[12]=0xff;
 buff[13]=0;

 for(i=0; i<4; i++)
 buff[9+i] = exeCmd.Argument[i];
 InitBuff(buff);
}

void set_dig_mask16()
{
  int i;
  char buff[12];
  //char buff[]={0,0xb,0,0,1,0,3,0,2,0xff,0xff,0};
  buff[0]=0;
 buff[1]=0xb;
 buff[2]=0;
 buff[3]=0;
 buff[4]=1;
 buff[5]=0;
 buff[6]=3;
 buff[7]=0;
 buff[8]=2;
 buff[9]=0xff;
 buff[10]=0xff;
 buff[11]=0;

  for(i=0; i<2; i++)
  buff[9+i] = exeCmd.Argument[i];
  InitBuff(buff);
}

void set_anl_mask()
{
  int i;
  char buff[18];
  //char buff[]={0,0x11,0,0,1,0,9,0,1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0};
 buff[0]=0;
 buff[1]=0x11;
 buff[2]=0;
 buff[3]=0;
 buff[4]=1;
 buff[5]=0;
 buff[6]=9;
 buff[7]=0;
 buff[8]=1;
 buff[9]=0xff;
 buff[10]=0xff;
 buff[11]=0xff;
 buff[12]=0xff;
 buff[13]=0xff;
 buff[14]=0xff;
 buff[15]=0xff;
 buff[16]=0xff;
 buff[17]=0;

  for(i=0; i<8; i++)
  buff[9+i] = exeCmd.Argument[i];
  InitBuff(buff);
}

void set_mean_mode()
{
 char buff[12];
 //buff[0]={0,0xb,0,0,1,0,3,0,0,2,0,0};
 buff[0]=0;
 buff[1]=0xb;
 buff[2]=0;
 buff[3]=0;
 buff[4]=1;
 buff[5]=0;
 buff[6]=3;
 buff[7]=0;
 buff[8]=0;
 buff[9]=2;
 buff[10]=0;
 buff[11]=0;
 buff[10] = exeCmd.Argument[0];
 InitBuff(buff);
}

void feed_cntrl_old()
{
  char buff[12];
  //buff[]={0,0xa,0,0,3,0,2,0,0,0,0};
  buff[0]=0;
  buff[1]=0xa;
  buff[2]=0;
  buff[3]=0;
  buff[4]=3;
  buff[5]=0;
  buff[6]=2;
  buff[7]=0;
  buff[8]=0;
  buff[9]=0;
  buff[10]=0;
  buff[8] = exeCmd.Argument[0];
  buff[9] = exeCmd.Argument[0]>>8;//buff[9]=0;
  InitBuff(buff);
}

void feed_cntrl_new()
{
  int i;
  char buff[12];
  //buff[]={0,0xa,0,0,6,0,2,0,0,0,0};
  buff[0]=0;
  buff[1]=0xa;
  buff[2]=0;
  buff[3]=0;
  buff[4]=6;
  buff[5]=0;
  buff[6]=2;
  buff[7]=0;
  buff[8]=0;
  buff[9]=0;
  buff[10]=0;
  for(i=0; i<2; i++)
  buff[8+i] = exeCmd.Argument[i];
  InitBuff(buff);
}

void fe_box_mon()
{
  char buff[12], temp_arg[1];
  //buff[]={0,0xa,0,0,7,0,2,0,0,0,0};
  buff[0]=0;
  buff[1]=0xa;
  buff[2]=0;
  buff[3]=0;
  buff[4]=7;
  buff[5]=0;
  buff[6]=2;
  buff[7]=0;
  buff[8]=0;
  buff[9]=0;
  buff[10]=0;
  temp_arg[0]= exeCmd.Argument[0];
  buff[9] = atoi(temp_arg);
  InitBuff(buff);
}

  // exeCmd Far Memory Location hence not passing to function

void FECB_CTRL(int MCMADDR )
{
  int i, j;
  char buff[12];


   for(j=0;j<sizeof(wbuff.Msg);j++) { wbuff.Msg[j]=0x00; }

   wbuff.Length = 0;

   buf_ptr0 = (far char *)&wbuff.Msg ;

   buf_ptr0[0] = MCMADDR;
   //("------------- FECB_CTRL started %d \n",buf_ptr0[0]);

  switch(exeCmd.CmdNum)
  {
       case 1 :  // NULL
                 //buff[]={0,8,0,0,0,0,0,0,0};
                 buff[0]=0;
                 buff[1]=8;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=0;
                 buff[5]=0;
                 buff[6]=0;
                 buff[7]=0;
                 buff[8]=0;
                 InitBuff(buff);
                 break;
       case 2 : // IDL Mode
                 //buff[]={0,0xa,0,0,1,0,2,0,0,0,0};
                 buff[0]=0;
                 buff[1]=0xa;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=1;
                 buff[5]=0;
                 buff[6]=2;
                 buff[7]=0;
                 buff[8]=0;
                 buff[9]=0;
                 buff[10]=0;
                 InitBuff(buff);
                 break;
       case 3 : // Scan Mode
                 //buff[]={0,0xa,0,0,1,0,2,0,0,1,0};
                 buff[0]=0;
                 buff[1]=0xa;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=1;
                 buff[5]=0;
                 buff[6]=2;
                 buff[7]=0;
                 buff[8]=0;
                 buff[9]=1;
                 buff[10]=0;
                 InitBuff(buff);
                 break;
       case 4 : // Mean Mode -- Unused.
                 set_mean_mode();
                 break;
       case 5 : // Set AnalogMask
                set_anl_mask();
                break;
       case 6 : // Set Digital 16 bit Mask
                set_dig_mask16();
                break;
       case 7 : // Set Digital32 bit Mask
                set_dig_mask32();
                break;
       case 8 : // Set Digital64 bit Mask
                set_dig_mask64();
                break;
       case 9 : // TX Read Anl Mask
                //buff[]={0,9,0,0,2,0,1,0,0,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=0;
                 buff[9]=0;
                InitBuff(buff);
                break;
      case 10 : // TX Read Dig Mask16
                 //buff[]={0,9,0,0,2,0,1,0,1,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=1;
                 buff[9]=0;
                 InitBuff(buff);
                 break;
      case 11 : //  TX Read Dig Mask32
                 //buff[]={0,9,0,0,2,0,1,0,2,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=2;
                 buff[9]=0;
                 InitBuff(buff);
                 break;
      case 12 : // TX Read Dig Mask64
                 //char buff[]={0,9,0,0,2,0,1,0,5,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=5;
                 buff[9]=0;
                 InitBuff(buff);
                 break;
      case 13 : // TX Read Version
                 //buff[]={0,9,0,0,2,0,1,0,3,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=3;
                 buff[9]=0;
                 InitBuff(buff);
                 break;
      case 14 : // TX Read Mode
                 //buff[]={0,9,0,0,2,0,1,0,4,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=2;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=4;
                 buff[9]=0;
                 InitBuff(buff);
                 break;
      case 15 : // TX Reboot
                 //buff[]={0,8,0,0,5,0,0,0,0};
                 buff[0]=0;
                 buff[1]=8;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=5;
                 buff[5]=0;
                 buff[6]=0;
                 buff[7]=0;
                 buff[8]=0;
                 InitBuff(buff);
                 break;
      case 16 : // TX Feed Mcm
                feed_cntrl_old(&exeCmd.Argument);
                break;
      case 17 : // TX Feed Mcm
                feed_cntrl_new(&exeCmd.Argument);
                break;
      case 18 : //TX FE Box monitor
                fe_box_mon(&exeCmd.Argument);
                break;
      case 19 : //  TX Common Box Monitor
                //buff[]={0,9,0,0,7,0,1,0,1,0};
                 buff[0]=0;
                 buff[1]=9;
                 buff[2]=0;
                 buff[3]=0;
                 buff[4]=7;
                 buff[5]=0;
                 buff[6]=1;
                 buff[7]=0;
                 buff[8]=1;
                 buff[9]=0;
                InitBuff(buff);
                break;
   }

    AddCksum();
    wbuff.Length = *(buf_ptr0+1) + (*(buf_ptr0+2)<<8) + 1;

  #ifdef DEBUG
        printf(" # Buf length: %d COMMAND %d\n", wbuff.Length,exeCmd.CmdNum);
        for (i=0; i<wbuff.Length; i++)
        printf("%x ", *(buf_ptr0+i));

        printf("\n");
  #endif


    writeSerial(MCMADDR);
    readSerial();

    fprintf(stderr,"#DATA received from mcmdriver: %d \n", rbuff.Length );
    if(resp_code == 10 )
    {
        if(rbuff.Length > 1 )
        decodeResponse();
    //    printf("FECB_CTRL finished\n");
        return ;
    }
    //else return ;
}

//-----------------------------------------------//

nodebug
void enable_485Tx( void )
{
#asm
	push	ip							;save off IP state
	ipset	1							;set interrupt priority to level 1
	//ld		a,(PDDRShadow)		;get copy of shadow reg V2 - PD4 as RTS
   ld		a,(PBDRShadow)			;get copy of shadow reg V3 - PB7 as RTS
	and		0x7F					;set bit PB7 to LOW to make RTS\ high v3 PB7// v2 PD4 // old PC1 high
	ld		(PBDRShadow),a			;update shadow reg
	ioi	ld	(PBDR),a				;set PE0 high
  	;IOWRITE_A(PBDR)            // commented on 21 July 2014
	pop	ip							;restore IP to the previous state
  ;	ret
#endasm
}

nodebug
void disable_485Tx( void )
{
#asm
	push	ip							;save off IP state
	ipset	1							;set interrupt priority to level 1
	ld		a,(PBDRShadow)			;get copy of shadow reg
	or		0x80						;V3 set bit PB7 HIGH to make RTS\ low; v20xEF //clear bit PC1  old ver
	ld		(PBDRShadow),a			;update shadow reg
	ioi	ld	(PBDR),a				;set PB7low
  	;IOWRITE_A(PBDR)           // commented on 21 July 2014
	pop	ip							;restore IP to the previous state
 	;ret
#endasm
}

void writeSerial(int MCMADDR)
{
int j;


  mcm_addr = MCMADDR;

 #ifdef DEBUG
 for(j=0; j<wbuff.Length; j++)
 printf("%2x ",(char)buf_ptr0[j]);
 #endif

 serCparity(PARAM_MPARITY); //0x04...mark parity
 enable_485Tx();     // make rts\ high at 7404 ic output

 if( !(serCputc(mcm_addr)) )
  printf("cmd_pkt...Serial C port trans buf is full or locked \n");

 costate
  {
//  waitfor (DelayMs(5000));
    waitfor (DelayMs(50));
  }
 serCparity(PARAM_SPARITY); //0x05...space parity

 serCwrite(&buf_ptr0[1],(wbuff.Length-1));// Check 2nd argument - whether its length
   while(!((BitRdPortI(SCSR,2) == 0)&&(BitRdPortI(SCSR,3)== 0)) );

   disable_485Tx();       //make rts\ low
   serCwrFlush();
  //serCrdFlush();
  costate
  {
    waitfor (DelayMs(100));
  }
}

void readSerial(void)
{
 int i=0, l=0, bytecntr_in=0, maxpktlen;    //holds maximum mcm cmd len
 int sleep;
 for(i=0;i<sizeof(rbuff.Msg);i++)
 rbuff.Msg[i]=0x00;
 rbuff.Length = 0;
 maxpktlen = sizeof(rbuff.Msg)-1;

   i=0;

   serCread(&rbuff.Msg[0],1,MSSG_TMOUT);
   while( rbuff.Msg[0] != 0x5 && rbuff.Msg[0] != 0x2  && i < 100 )    //-- 1000
   {
     serCread(&rbuff.Msg[0],1,MSSG_TMOUT);
       // sleep=1000; while( sleep > 0 ) { sleep = sleep - 1; }
     //--  printf(" %d ", (int) rbuff.Msg[0]);
     i++;
   }
 #ifdef DEBUG
   printf(">>>>>>>>>>>>>>>>>Rec Addr: %x \n", rbuff.Msg[0]);
 #endif         

while((bytecntr_in = serCread(&rbuff.Msg[1],maxpktlen,MSSG_TMOUT)) <= 5 && l++<100);
   rbuff.Length =  bytecntr_in+1;

#ifdef DEBUG
  printf("\nbytecntr_in: %d\n", bytecntr_in);
  printf("rbuff.Msg: \n");
  for(i=1;i<bytecntr_in;i++)
  {
    printf("[%2d] : %x \n", i, rbuff.Msg[i]);
  }      printf("end of serial transmission... ser2eth \n\n") ;
#endif

//serCrdFlush(); // fflush read buffer
BitWrPortI(PBDR, &PBDRShadow, 0, 7); // PB7=0 (RE\=1, DE=0) will enable Tx & disable Rx.


if ( rbuff.Length <= 0 )
{ resp_code = 11; }
else { resp_code = 10; }

}