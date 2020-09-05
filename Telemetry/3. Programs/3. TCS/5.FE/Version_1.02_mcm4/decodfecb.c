
#define IDLE_MODE 1
#define SCAN_MODE 2
#define MEAN_MODE 3
#define LIMITS_MODE 4

int noise_cal;
 unsigned char tmpbuf[64], stmpbuf[256];
 void copyTmpBuf(char *, char *, int);
 double convertdB (float , int );
 void decodeFEBox(int) ;
 void decodeCommonBox( );

 void decodeFEBox(int BoxNum)
 {
         int t, sub_band, wal_stat;
         float  vn1, vn2, vp1, vp2, vref1, vref2, vwf1, vwf2, tp_fe, tp_lna;
         int rf_stat=-999;

	   if((int) stmpbuf[2]>100 && (int) stmpbuf[4]>100)
             {
               if((int) stmpbuf[6]<100)noise_cal = 11;    // T-T-F NG=11 EHical On
               else noise_cal = 40;              // T-T-T NG=10 LOW CAL OFF
	     }

	   if((int) stmpbuf[2]<100 && (int) stmpbuf[4]<100)
             {
               if((int) stmpbuf[6]>100)noise_cal = 41;   // F-F-T : LOW CAL ON
	       else noise_cal = 10;             // F-F-F : EHiCal OFF
             }

	   if((int) stmpbuf[2]<100 && (int) stmpbuf[4]>100)
             {
               if((int) stmpbuf[6]<100) noise_cal = 21;  // F-T-F : NG=21 HICAL ON
	       else noise_cal = 30;            //  F-T-T : NG=30 MDE CAL OFF
	     }

           if((int) stmpbuf[2]>100 && (int) stmpbuf[4]<100)
             {
               if((int) stmpbuf[6]>100) noise_cal = 31;  // T-F-T NG=31 MED CAL ON
	       else noise_cal = 20;              // T-F-F NG=20 High Cal OFF
             }

        //   sprintf(tbuf,"FE_NGCAL"); tbuf[8]='\0';
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"FE_M_NGCAL");

           switch (noise_cal)
           {
               case 10 : sprintf(tbuf,"EHi OFF"); break;
               case 11 : sprintf(tbuf,"EHi ON"); break;
               case 20 : sprintf(tbuf,"Hi  OFF"); break;
               case 21 : sprintf(tbuf,"Hi  ON"); break;
               case 30 : sprintf(tbuf,"MED OFF"); break;
               case 31 : sprintf(tbuf,"MED ON"); break;
               case 40 : sprintf(tbuf,"LOW OFF"); break;
               case 41 : sprintf(tbuf,"LOW ON"); break;
               default : sprintf(tbuf,"Cal Unknown");
           }
            tbuf[strlen(tbuf)-1]='\0';
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%s",tbuf);
               resp_msg_cntr++;

           // At present only one RF ON/OFF status bit is there in the EXISTING ONLINE
	   if((int) stmpbuf[8] < 100 && (int) stmpbuf[9] > 100)rf_stat = 1;
	   else rf_stat = 0;

            _f_strcpy(&MCM_Resp.resp_data.prmname[resp_msg_cntr][0], "RF_ON_M");
            sprintf(&MCM_Resp.resp_data.prmvalue[resp_msg_cntr][0], "%d", rf_stat);
            resp_msg_cntr++;
                                   /*
            _f_strcpy(&MCM_Resp.resp_data.prmname[resp_msg_cntr][0], "RF_ON_M_CH2");
            sprintf(&MCM_Resp.resp_data.prmvalue[resp_msg_cntr][0], "%d", rf_stat);
            resp_msg_cntr++;     */


	  if(BoxNum == 5 || BoxNum == 4 || BoxNum == 3 )               // Only single band
	  {
           if((int) stmpbuf[10] < 100 && (int) stmpbuf[12] < 100)
           sub_band = 0;            // FF 0 1060 MHz
           else if((int) stmpbuf[10] < 100 && (int) stmpbuf[12] > 100)
           sub_band = 1;       // FT 1 1280 MHz
           else if((int) stmpbuf[10] > 100 && (int) stmpbuf[12] < 100)
           sub_band = 2;       // TF 2 1390 MHz
           else if((int) stmpbuf[10] > 100 && (int) stmpbuf[12] > 100)
           sub_band = 3;       // TT 3 1420 MHz
     }

            _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_FILTER_M");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "%d", sub_band);
            resp_msg_cntr++;
                                 /*
            _f_strcpy(&MCM_Resp.resp_data.prmname[resp_msg_cntr][0], "FE_FILTER_M_CH2");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "%d", sub_band);
            resp_msg_cntr++;   */



	   if((int) stmpbuf[16] < 100 && (int) stmpbuf[17] < 100)wal_stat = 0;
	   else wal_stat = 1;
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_WALSH_SW");
          sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "%d", wal_stat);
          resp_msg_cntr++;

           vwf1 = ((float)stmpbuf[16]*10)/(float)256;
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "WALSH_VOLT1_M");
          sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "%f", (vwf1 -5));
          resp_msg_cntr++;


	        vwf2 = ((float)stmpbuf[17]*10)/(float)256;
          _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "WALSH_VOLT2_M");
          sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr], "%f", (vwf2- 5));
           resp_msg_cntr++;


	   vn1 = ((float)stmpbuf[18]*10)/(float)256;
	   vn2 = ((float)stmpbuf[19]*10)/(float)256;
	   vp1 = ((float)stmpbuf[20]*10)/(float)256;
	   vp2 = ((float)stmpbuf[21]*10)/(float)256;
	   vref1 = ((float)stmpbuf[22]*10)/(float)256;
	   vref2 = ((float)stmpbuf[23]*10)/(float)256;

           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_N1");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", (vp1 - 5.0));
           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_N2");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  (vp2 - 5.0));
           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_P1");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  (vn1 - 5.0));
           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_P2");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  (vn2 - 5.0));
           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_R1");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", (vref1 - 5.0));
           _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_VOLT_R2");
           sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", (vref2 - 5.0));

// Temperature Monitoring Aug 27, 2013 VBB  //
           tp_fe   = (float) (((float)stmpbuf[24]* 39.0625) - 5000.0)/80;
           tp_lna  = (float) (((float)stmpbuf[25]* 39.0625) - 5000.0)/80;

            _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "TEMP_FE_M");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f", tp_fe);
         //   printf(" %d %s %s \n", (resp_msg_cntr-1),MCM_Resp.resp_data.prmname[resp_msg_cntr-1],MCM_Resp.resp_data.prmvalue[resp_msg_cntr-1]);

            _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "TEMP_LNA_M");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  tp_lna);
          //  printf(" %d %s %s \n", (resp_msg_cntr-1),MCM_Resp.resp_data.prmname[resp_msg_cntr-1],MCM_Resp.resp_data.prmvalue[resp_msg_cntr-1]);

// FE Power Monitoring stmpbuf[27], stmpbuf[28] -- Check voltage and dB conversio?4/4/2014 Parikh //
	   vn1 = ((float)stmpbuf[27]*10)/(float)256;
	   vn2 = ((float)stmpbuf[28]*10)/(float)256;
            _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_DET_M_CH1");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  (vn1 - 5.0));
            _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "FE_DET_M_CH2");
            sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr++], "%5.2f",  (vn2 - 5.0));

          //  printf(" %d %s %s \n",(resp_msg_cntr-1), MCM_Resp.resp_data.prmname[resp_msg_cntr-1],MCM_Resp.resp_data.prmvalue[resp_msg_cntr-1]);

// decimal to voltage = -5+ ( 0.001*(float)pcBufRead[j]*39.0625); //
// printf("FEBOX Decode End>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
	 return;
}

double convertdB ( float pval , int Box)
{
   double a[] =  { 3.613, -3589000000000000, 0, 122.1, -37.47 };
   double b[] =  { 4.353, -8.921, 2.454, 7.34, 2.077 };
   double c[] =  { 0.006815, 1.895, 0.000000000000000222, 1.898, 1.513 };
   double powerdB=0.0;


   pval = -5.0 + ( 0.001 * 39.0625 * pval); /* Voltage Conversion */

   if(Box) /* Common Box dBm conversion*/
   {
      powerdB = (double) ( a[0] * exp(-pow( ((pval - b[0])/c[0]),2) )+
                         a[1] * exp(-pow( ((pval - b[1])/c[1]),2) )+
                         a[3] * exp(-pow( ((pval - b[3])/c[3]),2) )+
                         a[4] * exp(-pow( ((pval - b[4])/c[4]),2) )
                       ) ;
   }
   return (double) powerdB ;
}

void decodeCommonBox()
{
  int i,j, pt=0;


#ifdef DEBUG
   printf(" Common Box decode >>>>>>>>>>>>>>>>>> resp_msg_cntr %d \n", resp_msg_cntr);
#endif
  for (i=0;i<2;i++)
  {


//--------FE
    if (  (int) stmpbuf[pt] < 100 && (int) stmpbuf[pt+2] > 100) Monparam[i*3] = 150;
    else if ((int) stmpbuf[pt+2] > 100  && (int) stmpbuf[pt+4] > 100 && (int) stmpbuf[pt+6] < 100) Monparam[i*3] = 235;
    else if ((int) stmpbuf[pt+2] < 100 &&  (int) stmpbuf[pt+4] > 100) Monparam[i*3] = 325;
    else if ((int) stmpbuf[pt+4] < 100 && (int) stmpbuf[pt+6] > 100) Monparam[i*3] = 1420;
    else if ((int) stmpbuf[pt+6] > 100 && (int) stmpbuf[pt+8] < 100) Monparam[i*3] = 610;
    else Monparam[i*3] = 50;
           //   printf(" BAND_SELECT_%s %d-  %d\n",ch, Monparam[i*3], i*3);

           if(i==0){ _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"BAND_SELECT_M_CH2"); }
           else {_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"BAND_SELECT_M_CH1"); }
       sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d",Monparam[i*3]);
       resp_msg_cntr++;

 //--------SOLATTEN
    pt+= 14;
    if ((int) stmpbuf[pt] > 100 &&  (int) stmpbuf[pt+2] < 100 && (int) stmpbuf[pt+4] < 100) Monparam[1+i*3] = 0;
    else if ((int) stmpbuf[pt] < 100 && (int) stmpbuf[pt+2] < 100  && (int) stmpbuf[pt+4] < 100) Monparam[1+i*3] = 14;
    else if ((int) stmpbuf[pt] > 100 && (int) stmpbuf[pt+2] > 100  && (int) stmpbuf[pt+4] > 100) Monparam[1+i*3] = 30;
    else if ((int) stmpbuf[pt] < 100 && (int) stmpbuf[pt+2] > 100  && (int) stmpbuf[pt+4] > 100) Monparam[1+i*3] = 44;
    else if ((int) stmpbuf[pt] > 100 && (int) stmpbuf[pt+2] < 100  && (int) stmpbuf[pt+4] > 100) Monparam[1+i*3] = -1;
            //     printf("SOL_ATTEN_%s %d\n",ch, Monparam[1+(i*3)]);

    if(i==0){ _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"SOL_ATTEN_M_CH2");}
    else  { _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"SOL_ATTEN_M_CH1");}
    sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d",Monparam[1+(i*3)]);
    resp_msg_cntr++;

 //----------CB_DET
    if(i==0){ _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"CB_DET_CH2");}
    else {_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"CB_DET_CH1");}
    pow_db=(double)convertdB((float)atof((char *)stmpbuf[pt+5]), 1);
    sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%g",pow_db );
    //sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",1.0 );
    resp_msg_cntr++;

    pt+= 6;          //2nd loop overwrts

    if(i==0) {
 //------------ SWAP
     if ((int) stmpbuf[pt] > 100) {Monparam[2] = 0;  }
     else {Monparam[2] = 1; }

      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr], "RF_SWAP");

       if(Monparam[2] == 0) { sprintf(tbuf,"%s","UNSWAP"); } else { sprintf(tbuf,"%s","SWAP"); }
       _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],tbuf);
        resp_msg_cntr++;
    }

//------------------ VCC_M
        Monvoltbis[i*3] = ((float)atof((char *)stmpbuf[pt+2])*10)/(float)256 -5;
        if(i==0){_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VCC_M_CH2");}
        else{_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VCC_M_CH1");}
        sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",Monvoltbis[i*3]);
        resp_msg_cntr++;

//------------------ VEE_M
        Monvoltbis[1+i*3] = ((float)atof((char *)stmpbuf[pt+3])*10)/(float)256 -5;
        if(i==0){_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VEE_M_CH2");}
        else {_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VEE_M_CH1");}
        sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",Monvoltbis[i*3]);
        resp_msg_cntr++;

 //------------------ VREF_M
        Monvoltbis[2+i*3] = ((float)atof((char *)stmpbuf[pt+4])*10)/(float)256 -5;
        if(i==0){_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VREF_M_CH2");}
        else {_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"VREF_M_CH1");}
        sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",Monvoltbis[i*3]);
        resp_msg_cntr++;

  //------------------ RF_POW_M
         if(i==0){_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"RF_POW_M_CH2");}
         else  {_f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"RF_POW_M_CH1");}
        sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",atof((char *)stmpbuf[pt+5]) );
        resp_msg_cntr++;

 // ---------------- CB_TEMP
     if (i == 0 ) {
         _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"CB_TEMP");
         sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%4.2f",  (( (float)stmpbuf[52] * 39.0625) - 5000.0 )/80.0 );
          resp_msg_cntr++;
     }

    pt+=8;


    }

    return;

 // printf(" Common Box decode End >>>>>>>>>>>>>>>>>>>>[ %d ]>>>>>>>>>>>>>\n",resp_msg_cntr);

}

void copyTmpBuf(char *tmpbuf, char *rptr, int siz)
{
   int i;
    for(i=0;i< sizeof(tmpbuf);i++) tmpbuf[i]=0;
         //  printf("==>>>");
    for (i = 0; i < siz; i++) { tmpbuf[i] = rptr[i];
   //  printf (" %x ", tmpbuf[i] );
    }
    printf("\n");
}

void decodeResponse()
{
  int i, rawDataLen, remRespLen, nCmdResps=0, cnt=0, cmdRespLen, nbLog, log1Len, log2Len, tempSize;
  //unsigned char cmdStat, lowCmd, lowSubCmd;
  int cmdStat, lowCmd, lowSubCmd;
  int mcmnb = 5, bothMcms = 0;


  //char *rbuf_ptr ;
  int rcmd = 0, RxCmdNum = -1;
  int FEboxNum;
  char tmp1, tmp2;
  char *cmdName[] = { "setMCMAddr", "NullCmd", "SetIdleMode", "SetScanMode", "SetMeanMode", "setAnalogMask", "setDigMask16Bits", "setDigMask32Bits",\
		      "setDigMask64Bits", "ReadAnalogMask", "ReadDigMask16Bits", "ReadDigMask32Bits", "ReadDigMask64Bits", "ReadVersion", "ReadMode",\
		      "Reboot", "FEControlOld", "FEControlNew", "FE-BoxMon", "CommonBoxMon", "PowerDown" };

   //rbuf_ptr = &rbuff.Msg;
   rawDataLen = remRespLen = rbuff.Length ;


 // printf("++++++++++ START %d \n", resp_msg_cntr);

  _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"time");      // VAR0
  sec_tm = read_rtc();
  mktm(&rtc, sec_tm);
 // sprintf(time_ar,"%d:%d:%d",rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d:%d:%d", rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
   resp_msg_cntr++ ;


  // fprintf(stderr, " Response Message Length is %d \n", rawDataLen);

        // Decoding Logic
        while( remRespLen > 0 )
        {
                RxCmdNum = (-1);

		if((int)rbuff.Msg[cnt] != 5)
		{ cnt++; remRespLen--; continue; }

		nCmdResps++;
		if(nCmdResps > 1)   // Next Packet
		{ if(mcmnb != (int)rbuff.Msg[cnt])
		  {	bothMcms++;            }//To check whether both MCMs are used or only one
		}

// 0 is MCM CArd ADDRESS
		//mcmnb = atoi((char *)rbuff.Msg[cnt++]);
  //    printf("mcmnb = %d, cnt = %d \n", (int)rbuff.Msg[cnt], cnt);
      mcmnb = (int)rbuff.Msg[cnt++];

#ifdef DEBUG
  	printf("No Of Response %d, MCMCard %d\n",  nCmdResps, mcmnb);
#endif

                _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"totalresp");
                 sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d",nCmdResps); resp_msg_cntr++;  // VAR1


                _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"mcmcard");
                 sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d",mcmnb); resp_msg_cntr++;  // VAR2

// 1 and 2 is PACKET SIZE
		//cmdRespLen = atoi((char *)rbuff.Msg[cnt]) + (atoi((char *)rbuff.Msg[cnt+1])<<8);

      cmdRespLen = (int)rbuff.Msg[cnt] + (((int)rbuff.Msg[cnt+1]) << 8) ;
#ifdef DEBUG
      printf(" [%d] %d + %d = CMDRESPLEN %d \n", cnt, (int)rbuff.Msg[cnt] ,(int) rbuff.Msg[cnt+1], cmdRespLen);
#endif
      cnt += 2;	// cnt points to byte "0" -- next to "response packet size" bytes
		cnt += 1; 	// cnt points to cmd status
// 4 Packet Command Status
		cmdStat = (int)rbuff.Msg[cnt];
#ifdef DEBUG
      printf("cmdStat = %d, cnt = %d \n", cmdStat, cnt);
#endif
      _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"cmdstat"); // VAR3

		switch (cmdStat)
		{
		   case 0 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"MCM Command OK"); resp_code=10; break;
		   case 1 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"MCM TimeOut");  break;
		   case 2 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Command Check Sum Error");  break;
		   case 4 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Command unknown");  break;
		   case 8 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Packet too large");  break;
		   case 16 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Answer Message truncated");break;
         default : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"cmstate_blank");
      }
                 resp_msg_cntr++;
                 // Command Failed send diagnostic
                 if( cmdStat != 0 )
                 {   return ; }
                 // Points to "Number of Logical Packets" bytes
                 cnt++;
                 //cnt++;    //Added by RDB.....................
                 nbLog = (int)rbuff.Msg[cnt];
#ifdef DEBUG
                 printf("*********** nbLog = %d, cnt = %d \n", nbLog, cnt);
                 printf(" Number of logical packets in the response message : %d \n", nbLog);
#endif
                 _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"totallogpkt");  // VAR4
                  sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%d",nbLog);
                  resp_msg_cntr++;
 #ifdef DEBUG
                 printf("totallogpkt %d \n",nbLog);
 #endif
                 if(nbLog == 1) RxCmdNum = 1;
                 cnt++;
                 if( nbLog > 0 )
                 {
                    tempSize = 0;
                    // 6, 7 First Logical Packet Size
                    log1Len = (int)rbuff.Msg[cnt] + ( ((int)rbuff.Msg[cnt+1]) << 8) ;
  //                  printf("<<<<<<<<<<<<< log1Len = %d, cnt = %d \n", log1Len, cnt);
                    tempSize+=2;
                    cnt += 2 ; // Pointing to the byte next to "logical 1 packet Size byte". 8, 9

                    // if log1Len == 4 Ideal; BLANK-8, 9
                     tempSize+=2;
                     cnt += 2 ; // Pointing to MCM Mode byte of logical packet one
//                     printf("tempSize = %d \n", tempSize);
                    // 10 Location for 1- scan, 2- Mean, 3-Limit?
            if(log1Len > tempSize)
               {

                       _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"mcmmode");      // VAR5
                       switch( (int)rbuff.Msg[cnt])
                       {
		         case 1 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Scan mode");
                                   resp_msg_cntr++; cnt++; tempSize++;
                                   break;
		         case 2 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Mean mode");
                                   resp_msg_cntr++; cnt += 2; tempSize += 2;
                                   break;
		         case 3 : _f_strcpy(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"Limits mode");
                                   resp_msg_cntr++; cnt++; tempSize++;
                                   break;
               default : break;
                }

                       _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"anamask"); // 8 bit analog Mask   VAR6
                        for(i=0;i<8;i++) {
	                           MCM_Resp.resp_data.prmvalue[resp_msg_cntr][i] = (char) rbuff.Msg[cnt];
                              cnt++;
                        }
                              MCM_Resp.resp_data.prmvalue[resp_msg_cntr][i] = '\0'; resp_msg_cntr++;
                            tempSize += 8;
  #ifdef DEBUG
	                printf("Scan data (length: %d) \n", log1Len - tempSize);
  #endif
                       _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],"scandata"); // 64 data points  VAR7
	                for (i = 0; i < (log1Len - tempSize) && (i < 63);  i++) { // Size -13 or -14 i.e. tempSize
	                   MCM_Resp.resp_data.prmvalue[resp_msg_cntr][i] = (char) rbuff.Msg[cnt];
 	                   fprintf(stderr, "[ %x ]", MCM_Resp.resp_data.prmvalue[resp_msg_cntr][i]);
	                   cnt++; tempSize++;
                   }   MCM_Resp.resp_data.prmvalue[resp_msg_cntr][i]='\0';  resp_msg_cntr++;

                      }
                  }

		  if (nbLog == 2)
		  {
                        // Second Packet Length
			tempSize = 0;
//         printf( "  [%d] %d + [%d] %d log2Len %d \n", cnt, (int) rbuff.Msg[cnt], cnt+1, (int)rbuff.Msg[cnt+1], log2Len);
			log2Len = (int) rbuff.Msg[cnt] + ( ((int)rbuff.Msg[cnt+1]) <<8) ;
			cnt += 2;	//Pointing to the byte next to packet-size-byte
			tempSize += 2;

                        // Low and Lower Sub Command
			lowCmd = (int)rbuff.Msg[cnt];
			lowSubCmd = (int) rbuff.Msg[cnt+1]; // SecPAckStart + 3
 #ifdef DEBUG
         printf(" LOWCMD %d LSUBCMD %d \n", lowCmd, lowSubCmd);
 #endif
       	cnt += 2;
			tempSize += 2;
         printf( "LOG2LEN %d tempSize %d \n", log2Len, tempSize);

                        switch( lowCmd )
                        {
                            case 1 :  switch(lowSubCmd)   // SetProcess
                                      {
                                       case 0 :  rcmd = (int) rbuff.Msg[cnt];
                                                 if( rcmd = 0 ) RxCmdNum = 2;
                                                 else if ( rcmd = 1 ) RxCmdNum = 3;
                                                 else if ( rcmd = 2 ) RxCmdNum = 4;
                                                 sprintf(tmpbuf,"%s","true"); // tmpbuf shall not be null;
                                                 cnt++; tempSize++;
                                                 break;
                                       case 1 :  RxCmdNum = 5;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],8);
                                                 cnt += 8; tempSize += 8 ;
                                                 break;
                                       case 2 :  RxCmdNum = 6;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],2);
                                                 cnt += 2; tempSize += 2 ;
                                                 break;
                                       case 3 :  RxCmdNum = 7;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],4);
                                                 cnt += 4; tempSize += 4 ;
                                                 break;
                                       case 4 :  RxCmdNum = 8;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],8);
                                                 cnt += 8; tempSize += 8 ;
                                                 break;
                                       default : break;
                                      }
                                  break;

                            case 2 :  switch(lowSubCmd)   // ReadProcess
                                      {
                                       case 0 :  RxCmdNum = 9;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],8);
                                                 cnt += 8; tempSize += 8 ;
                                                 break;
                                       case 1 :  RxCmdNum = 10;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],2);
                                                 cnt += 2; tempSize += 2 ;
                                                 break;
                                       case 2 :  RxCmdNum = 11;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],4);
                                                 cnt += 4; tempSize += 4 ;
                                                 break;
                                       case 5 :  RxCmdNum = 12;
                                                 copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],8);
                                                 cnt += 8; tempSize += 8 ;
                                                 break;
                                       case 3 :  RxCmdNum = 13;
                                                 tmp1 = ( (int) rbuff.Msg[cnt] & 0xf ) + 0x30;
                                                 tmp2 = ( ((int)rbuff.Msg[cnt] >> 4) & 0xf ) + 0x30;
                                                 sprintf(tmpbuf,"%c.%c",tmp2,tmp1);
                                                 cnt++; tempSize++;
                                                 break;
                                       case 4 :  RxCmdNum = 14;
                                                 switch( (int) rbuff.Msg[cnt] )
                                                 {
		                                   case 0 : sprintf(tmpbuf,"%s","IDLE");
                                                             break;
		                                   case 1 : sprintf(tmpbuf,"%s","SCAN");
                                                             break;
		                                   case 2 : sprintf(tmpbuf,"%s","MEAN");
                                                             break;
		                                   default : sprintf(tmpbuf,"%s","LIMITS");
                                                             break;
                                                 }
                                                 cnt++; tempSize++;
                                                 break;
                                      }
                                      break;

                            case 5 : RxCmdNum = 15;
                                   break;
                            case 8 : RxCmdNum = 20;
                                   break;

                            case 3 : RxCmdNum = 16;
                                    copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],2);
                                    cnt += 2; tempSize += 2 ;
                                   break;
                            case 6 : RxCmdNum = 17;
                                    copyTmpBuf(tmpbuf,(char *)rbuff.Msg[cnt],2);
                                    cnt += 2; tempSize += 2 ;
                                   break;

                            case 7 : // Main CB and FE Monitor
				                         if(lowSubCmd)
                                     {  RxCmdNum = 19;  // CommonBox
#ifdef DEBUG
                                        printf("\n=> Packet Buf CommonBox %d, cnt %d \n", (log2Len - tempSize), cnt);
#endif
                                        copyTmpBuf(stmpbuf,(char *)&rbuff.Msg[cnt],(log2Len -tempSize));
                                        stmpbuf[(log2Len-tempSize)+1]='\0';
                                        decodeCommonBox(&stmpbuf[0]);
                                     } else {
                                        RxCmdNum = 18; // FE BOX Mon
                                        FEboxNum = (int)rbuff.Msg[cnt++]; cnt++; tempSize++;
                                        copyTmpBuf(stmpbuf,(char *)rbuff.Msg[cnt],(log2Len -tempSize));
#ifdef DEBUG
                                         printf("\n=> Packet Buf FE Box %d, cnt %d \n", (log2Len - tempSize), cnt);
#endif
                                         stmpbuf[(log2Len - tempSize) + 1]='\0';
                                         decodeFEBox(FEboxNum);
                                     }
                                        cnt += (log2Len -tempSize) ; tempSize += (log2Len - tempSize)  ;

                                   break;

                            default : break;
                        }
                  }

                  if(RxCmdNum >  0 && RxCmdNum != 15 && RxCmdNum != 20 ) {
                   //    sprintf(MCM_Resp.resp_data.prmvalue[resp_msg_cntr],"%s",tmpbuf);  // copying first tmpbuf of Data  VAR8
                        sprintf(tmpbuf,"%s",cmdName[RxCmdNum]);
                  //     _f_strcpy(MCM_Resp.resp_data.prmname[resp_msg_cntr],tmpbuf); // Command Name
                       printf(" %s \n",tmpbuf);
                    //    resp_msg_cntr++;
                  }
			cnt++;		// Added for checksum -- If next response is there, pointing to that
			remRespLen -= (cmdRespLen+1);
 #ifdef DEBUG
  			printf("Command length: %d, rem. len; %d\n", cmdRespLen, remRespLen);
			printf("\n----------------------------////////\n");
 #endif
       }
       resp_code = 10;
 // printf("Decode Resp. End >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");

 }

