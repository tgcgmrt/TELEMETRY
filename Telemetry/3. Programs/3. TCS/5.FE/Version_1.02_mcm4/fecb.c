
//const int MAXSTPRM = 9;
// const char MCMCARD1 = '5';
// const char MCMCARD2 = '4';

 // MCM 2 changed to 4 on Sep 20th, 2017-JPK
 far int MCMCARD1 = 5, MCMCARD2 = 4;

 typedef struct
 {
    int cmdCode;
    int freq_set[2];
    int solar_attn[2];
    int polSwap, noiseCal, ngCycle,ngFreq;
    int walshGrp, walshSW,walshPat,walshFreq;
    int rfcmSW, ngSW;
 } fecbset;

 typedef struct
 {
    int CmdNum ;
    int McmPow ;
    int feBoxNo[2];
    unsigned char Argument[100];
 } execcmd;

 typedef struct
 {
    int Length;
    char Msg[200];
 } serialBuff ;


far fecbset fecbSet;
far execcmd exeCmd;
far serialBuff wbuff;
serialBuff rbuff ;
far char *buf_ptr0;
far short bitD0[32], mcm4word[2];

far long mask_32[2]; // 32 bit Hex Mask up to 4 allowed
far int num_mask_32;   // Num of arg.


void getbitDO(int word)
{
  int i, mask, t_bit;

   for(i=7;i >= 0;)
   {
     mask = 1<<i;
     t_bit =  word & mask;            /* and operator */
     if( t_bit == 0 ) bitD0[i] = 0;
     else bitD0[i] = 1;
     i--;
   }
#ifdef JDEBUG
   printf("BIT : %d  ",word);
   for(i=0;i<7;i++) {
     printf("%d",bitD0[i]);
   }
   printf("\n");
#endif
}
/*
void getbitD32(int word)
{
  int i, mask, t_bit;

   for(i=31;i >= 0;)
   {
     mask = 1<<i;
     t_bit =  word & mask;            // and operator //
     if( t_bit == 0 ) bitD0[i] = 0;
     else bitD0[i] = 1;
     i--;
   }
#ifdef JDEBUG
   printf("BIT : %d  ",word);
   for(i=0;i<32;i++) {
     printf("%d",bitD0[i]);
   }
   printf("\n");
#endif

}
*/

int  mcmPow(int mcmSW)
{
    int FEDAT_ON, FEDAT_OFF;

//  FEDAT_OFF = fecbSet.walshSW + fecbSet.ngCycle ;
//  FEDAT_ON  = FEDAT_OFF + 16 ;

    exeCmd.McmPow = mcmSW;

    switch(mcmSW)
    {
       case 0 : // MCM OFF
/*
 #ifdef DEBUG                 // MCM 5 ON With Walsh and NoiseGen Settings
               printf("FEDAT_OFF = %02x\n",FEDAT_OFF);
 #endif
               sprintf(mask1,"70%02x",FEDAT_OFF);
               sprintf(mask2,"f0%02x",FEDAT_OFF);
*/


               sprintf(mask1,"%04x",mcm4word[1] & (~( 1 << 4)) );
               sprintf(mask2,"%04x",mcm4word[0] & (~( 1 << 4)) );
 #ifdef JDEBUG
               printf("mask1 %s ", mask1);
               printf("mask2: %s \n", mask2);
 #endif
               Mask32toArg(mask1, mask2);
               exeCmd.CmdNum = 7;
               FECB_CTRL(MCMCARD2);           //set dig mask 32 bits
               Ms_delay(200);
       break;

       case 1 : // MCM ON
             // MCM 5 OFF With Walsh and NoiseGen Settings
/*
 #ifdef DEBUG
               printf("FEDAT_OFF = %02x\n",FEDAT_OFF);
 #endif
               sprintf(mask1,"70%02x",FEDAT_OFF);
               sprintf(mask2,"f0%02x",FEDAT_OFF);
*/
               sprintf(mask1,"%04x",mcm4word[1] & (~( 1 << 4)) );
               sprintf(mask2,"%04x",mcm4word[0] & (~( 1 << 4)) );
#ifdef JDEBUG
               printf("mask1: %s ", mask1);
               printf("mask2: %s \n", mask2);
#endif
               Mask32toArg(mask1, mask2);
               exeCmd.CmdNum = 7;
               FECB_CTRL(MCMCARD2);           //set dig mask 32 bits
                 Ms_delay(500);
               // MCM 5 ON With Walsh and NoiseGen Settings
               //printf("FEDAT_ON = %02x\n",FEDAT_ON);
/*
               sprintf(mask2,"f0%02x",FEDAT_ON);
               sprintf(mask1,"70%02x",FEDAT_ON);
*/
               sprintf(mask1,"%04x",mcm4word[1] | ( 1 << 4) );
               sprintf(mask2,"%04x",mcm4word[0] | ( 1 << 4) );
#ifdef JDEBUG
               printf("mask1: %s ", mask1);
               printf("mask2: %s \n", mask2);
#endif
               Mask32toArg(mask1, mask2);
               exeCmd.CmdNum = 7;
               FECB_CTRL(MCMCARD2);
                Ms_delay(500);         //set dig mask 32 bits
      break;
   }

}

int fecbCtrlCmd()
{
   int add_fe[2], i , j, b,k ;
   int data1[2], data2[2], data3[2];
   char mask[4][8];

   int freq_band[]= {50,  150, 235, 325, 610,  1060, 1170, 1280, 1390, 1420, -1};
   int p_data11[] = {0x0, 0x1, 0x8, 0xa, 0x10, 0x14, 0x14, 0x14, 0x14, 0x14};
   int p_data32[] = {0x0, 0x0, 0x0, 0x0, 0x0,  0x00, 0x40, 0x20, 0x60, 0x90};
   int p_add[]    = {0x1, 0x2, 0x3, 0x4, 0x5,  0x6,  0x6,  0x6,  0x6, 0x6};

   int freq_uband[]= {50, 150,  235, 290,  350,  410,  470,  325,  600,  685,  725,  770,  850 , 1060, 1170, 1280, 1390, 1420 , -1};
   int p_udata11[] = {0x0, 0x1, 0x8, 0xa,  0xa,  0xa,  0xa,  0xa,  0x10, 0x10, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14, 0x14, 0x14 };
   int p_udata32[] = {0x0, 0x0, 0x0, 0x60, 0x20, 0x40, 0x0,  0x80, 0x60 ,0x20, 0x80, 0x40, 0x00, 0x00, 0x40, 0x20, 0x60, 0x90 };
   int p_uadd []   = {0x1, 0x2, 0x3, 0x4,  0x4,  0x4,  0x4,  0x4,  0x5 ,  0x5, 0x5,  0x5,   0x5,  0x6,  0x6,  0x6,  0x6, 0x6  };

   int sol_attr[] = {   0,   14,   30,   44,  -1};
   int p_data12[] = {0x80, 0xe0, 0x00, 0x60, 0xa0};
   int p_data21[] = {0x01, 0x01, 0x00, 0x00, 0x00};

   int ns_cal[]   = {   0,    1,    2,    3, -1};
   char *N_CAL[]   = {"E-HI","HI","MED","LOW","RF-OFF"};
   int p_data31[] = {0x06, 0x05, 0x0a, 0x09, 0x15};
   short digWord[32];

   typedef enum { INIT=0, FNULL=1, SETRFSYS=2,FESDOMON=3, SEL_FEBOX=4, SEL_UFEBOX=5, RFSWAP=6, RFATTN=7, RFON=8, CBTERM=9, \
                  RFTERM=10, RFNGSET=11, RFNGCAL=12 ,SETWALSH=13, SETWALSHGRP=14, SETURFSYS=15, RFCM_SW=16, RAWMON=17, RESTORE=18, \
                  SETTIME=19, REBOOTMCM=20, SET32BIT=30 ,TCMD=22 } FesEnum;

     resp_msg_cntr = 0 ;

#ifdef JDEBUG
     printf("\n\nfecb -->   %d\n", fecbSet.cmdCode);
#endif

   switch (fecbSet.cmdCode)
   {
        case FNULL : // if(exeCmd.McmPow ==0 )
                       FECB_CTRL(MCMCARD2);
                       mcmPow(1); exeCmd.CmdNum = 1;FECB_CTRL(MCMCARD1);
              break ;

        case SETRFSYS : case SETURFSYS :                // Set RF, upgraded RF
	           // MCM ON
        for(i=0;i<32;i++) { digWord[i]=0; }

        for (j=0; j<2; j++)
        {
                 // Frequency Setting
          if ( fecbSet.cmdCode == SETRFSYS ) {
                for(i=0;( freq_band[i] != (-1) && fecbSet.freq_set[j] != freq_band[i]); i++);
                if( freq_band[i] == - 1 )
                { return ( (-1) * SETRFSYS ); }
                data1[0] = p_data11[i];
                data3[1] = p_data32[i];
                add_fe[j] = p_add[i];
                exeCmd.feBoxNo[j] = (int) p_add[i];
          } else if (fecbSet.cmdCode == SETURFSYS )  {
                for(i=0;( freq_uband[i] != (-1) && fecbSet.freq_set[j] != freq_uband[i]); i++);
                if( freq_band[i] == - 1 )
                { return ((-1) * SETURFSYS);}
                data1[0] = p_udata11[i];
                data3[1] = p_udata32[i];
                add_fe[j] = p_uadd[i];
                exeCmd.feBoxNo[j] = (int) p_uadd[i];
         }


 #ifdef JDEBUG
        printf("\n ===> FEBox Number CH[%d] = %d \n", j, exeCmd.feBoxNo[j]);
 #endif
          //Control word for setting SOLAR ATTENUATOR in the common box
            for (i=0; sol_attr[i]!=-1 && fecbSet.solar_attn[j]!=sol_attr[i]; i++) {;}
                data1[1] = p_data12[i];  // p_data11 + p_data12
                data2[0] = p_data21[i];

           // If polarization swap
           if (fecbSet.polSwap != 0) {data2[1] = 0x02; }else {data2[1]=0x0;}     // p_data21 + p_data22 , p_data22 is not declared

          //  control word for setting NOISE CAL LEVEL in front end box.
            for (i=0; fecbSet.noiseCal != ns_cal[i]; i++) {;}
            data3[0] = p_data31[i];  // p_data32 +  p_data31

            if (fecbSet.freq_set[1] == 0) exeCmd.feBoxNo[++j] = 0;

           // ----------New logic for MCM 4, SEP 20, 2017 ---------//
           // NG Cycle bit D0,1
             getbitDO(fecbSet.ngCycle); for (b=0;b<2;b++) { digWord[b + (j*16)] = bitD0[b]; }
           // NG Freq bit D2,3
             getbitDO(fecbSet.ngFreq); for (b=0;b<2;b++) { digWord[ 2 + b + (j*16)] = bitD0[b]; }
           // FE SEL D4
              digWord[4 + (j * 16) ] = fecbSet.rfcmSW;
           // Walsh Pattern D5,6,7
             getbitDO(fecbSet.walshPat); for (b=0;b<3;b++) { digWord[ 5 + b + (j*16)] = bitD0[b]; }
           // Walsh Freq D8,9
             getbitDO(fecbSet.walshFreq); for (b=0;b<2;b++) { digWord[ 8 + b + (j*16)] = bitD0[b]; }
           // Spare 10,11
             digWord[10 + (j*16)] = digWord[11 + (j*16)] = 0;
           // Walsh Group B12,13,14 freezed to value-1?
             digWord[12 + (j*16)] = digWord[13 + (j*16)] = digWord[14 + (j*16)] = 1;
           // bit D15 - EN
             if(j == 1) digWord[15 + (j*16)] = 1;
             else digWord[15 + (j*16)] = 0 ;

          } // for j end

#ifdef JDEBUG
       printf(" INITIALIZING WORDS");
#endif
          mcm4word[0] = 0 ;  mcm4word[1] = 0;

          for(i = 0 ; i < 16; i++)
          {
             if(digWord[i+16]) mcm4word[0] = mcm4word[1] | (1 << i);
             if(digWord[i]) mcm4word[1] = mcm4word[0] | (1 << i );
          }

          printf(" MCM4WORD1 ");
          for(i=0 ; i<16;i++)   {
             if( (mcm4word[1] & ( 1 << i )) ) { b = 1 ; }
             else b = 0 ;
             printf( "%d", b);
          }
          printf("\n");

          printf(" MCM4WORD0 ");
          for(i= 0 ; i<16;i++)  {
             if( (mcm4word[0] & ( 1 << i )) ) { b = 1 ; }
             else b = 0 ;
             printf( "%d", b);
          }
          printf("\n");

          printf(" %04xX %04xX ======> mcm4word 0 and 1 \n", (mcm4word[0] & (~(1 << 4)) ), (mcm4word[1] & (~(1 <<4))) );

              data1[0] = data1[0] + data1[1];
              data2[0] = data2[0] + data2[1];
              data3[0] = data3[0] + data3[1];

           mcmPow(1);

             //  Frequency, Solar Attenuator, Nose cal and Swap control
                  sprintf(mask1, "07%02x",data1[0]); sprintf(mask2, "00%02x",data1[0]);
                  sprintf(mask3, "08%02x",data2[0]); sprintf(mask4, "00%02x",data2[0]);
                  //printf("07%02x ",data1[0]); printf("00%02x ",data1[0]); printf("08%02x ",data2[0]); printf("00%02x \n",data2[0]);
#ifdef JDEBUG
                  printf("mask1: %s ", mask1);
                  printf("mask2: %s ", mask2);
                  printf("mask3: %s ", mask3);
                  printf("mask4: %s \n", mask4);
#endif
                  Mask64toArg(mask1, mask2, mask3, mask4);
                  exeCmd.CmdNum = 8; FECB_CTRL(MCMCARD1);  // set dig mask 64 bits

                 if (fecbSet.freq_set[1] == 0) j = 0;
                 else j = 1;
                 sprintf(mask1, "09%02x",data1[0]); sprintf(mask2, "00%02x",data1[0]);
                 sprintf(mask3, "0A%02x",data2[0]); sprintf(mask4, "00%02x",data2[0]);
                 //printf("09%02x ",data1[j]); printf("00%02x ",data1[j]); printf("0A%02x ",data2[j]); printf("00%02x \n",data2[j]);
#ifdef JDEBUG
                 printf("mask1: %s ", mask1);
                 printf("mask2: %s ", mask2);
                 printf("mask3: %s ", mask3);
                 printf("mask4: %s \n", mask4);
#endif
                 Mask64toArg(mask1, mask2, mask3, mask4);
                 exeCmd.CmdNum = 8; FECB_CTRL(MCMCARD1);  // set dig mask 64 bits

               //
                 for (i=1; i<add_fe[0]; i++) {
                   sprintf(mask1, "%02x16", i); sprintf(mask2, "0016");
#ifdef JDEBUG
                   printf("mask1: %s ", mask1);
                   printf("mask2: %s \n", mask2);
#endif
                   Mask32toArg(mask1, mask2);
                   exeCmd.CmdNum = 7; FECB_CTRL(MCMCARD1);           //set dig mask 32 bits
                 }

                 for (i=add_fe[0]+1; i<7; i++) {
                   sprintf(mask1, "%02x16", i); sprintf(mask2, "0016");
#ifdef JDEBUG
                   printf("mask1: %s ", mask1);
                   printf("mask2: %s \n", mask2);
#endif
                   Mask32toArg(mask1, mask2);
                   exeCmd.CmdNum = 7; FECB_CTRL(MCMCARD1);           //set dig mask 32 bits
                 }

                 for (j=0;j<1;j++) {
                   sprintf(mask1, "%02x%02x",add_fe[j],data3[0]);   //21/3 jk
                   sprintf(mask2, "00%02x",data3[0]);
#ifdef JDEBUG
                   printf("mask1: %s ", mask1);
                   printf("mask2: %s \n", mask2);
#endif
                   Mask32toArg(mask1, mask2);
                   exeCmd.CmdNum = 7; FECB_CTRL(MCMCARD1);           //set dig mask 32 bits
                   if (fecbSet.freq_set[1] == 0) j++;
                 }

	        //  MCM OFF
             mcmPow(0);

                break;

       case RFNGSET :

              if(fecbSet.ngSW < 0 ) return ERROR;

             if(exeCmd.McmPow == 0 ) mcmPow(1);

/*
              switch (fecbSet.ngCycle)
              {
                  case 0 :  sprintf(mask1, "7018"); sprintf(mask2, "f018"); // NGOFF
                        break;
                  case 1 :  sprintf(mask1, "7019"); sprintf(mask2, "f019"); // NGON 25 %
                        break;
                  case 2 :  sprintf(mask1, "701a"); sprintf(mask2, "f01a"); // NGON 50 %
                        break;
                  case 3 :  sprintf(mask1, "701b"); sprintf(mask2, "f01b"); // NGON 100 %
                        break;
              }
*/

           // NG Cycle bit D0,1
             for(i = 0;i<2;i++)
             { getbitDO((int)fecbSet.ngCycle); for (b=0;b<2;b++) { digWord[b + (i*16)] = bitD0[b]; }
             }

             for(i= 0 ;i<2;i++) {
             if(digWord[i+16]) mcm4word[0] = mcm4word[1] | (1 << i );
             if(digWord[i]) mcm4word[1] = mcm4word[0] | (1 << i );
             }

               sprintf(mask1,"%04x",mcm4word[1] | ( 1 << 4) );
               sprintf(mask2,"%04x",mcm4word[0] | ( 1 << 4) );
#ifdef JDEBUG
               printf("mask1: %s ", mask1);
               printf("mask2: %s \n", mask2);
#endif
               Mask32toArg(mask1, mask2);
               exeCmd.CmdNum = 7; FECB_CTRL(MCMCARD1);           //set dig mask 32 bits
            //  Copy(-1, Mcm2, SPEINDX);

              break;

       case RFCM_SW :
               mcmPow(fecbSet.rfcmSW);
               break;

       case REBOOTMCM :
                     exeCmd.CmdNum = 15; FECB_CTRL(MCMCARD1);
                     break;

       case FESDOMON  :

                 if(exeCmd.McmPow == 0 ) mcmPow(1);

                   exeCmd.CmdNum = 19; /*  Copy(1, Mcm1, MONINDX);   */      // Common box monitor
                   FECB_CTRL(MCMCARD1); //? 2 or 5 JPK
                    costate
                    {
                         waitfor (DelayMs(100));
                    }

                   for (i=0;i<1;i++)
                   {
                     if (exeCmd.feBoxNo[i] != 0) {
 #ifdef JDEBUG
                       printf(" \n============ FEBOX NO %d =====\n", exeCmd.feBoxNo[i]);
 #endif
                       exeCmd.Argument[0] = exeCmd.feBoxNo[i];  // FeBox Number!
                       exeCmd.CmdNum = 18;                                            //FE Box Mon
                       FECB_CTRL(MCMCARD1); // 5 JPK
                       costate
                       {
                         waitfor (DelayMs(100));
                       }
                     }
                   }
                   break;

             case SET32BIT  :
               // for(k=0; k < num_mask_32; k++)
               // {
                       /* for(i=0;i<32;i++) { digWord[i]=0; }
                        getbitD32(mask_32[k]); for (b=0;b<32;b++) { digWord[b] = bitD0[b]; }

                         mcm4word[0] = 0 ;  mcm4word[1] = 0;
                         for(i = 0 ; i < 16; i++)
                         {
                            if(digWord[i+16]) mcm4word[0] = mcm4word[1] | (1 << i);
                            if(digWord[i]) mcm4word[1] = mcm4word[0] | (1 << i );
                         }

                         sprintf(mask1,"%04x",mcm4word[1] & (~( 1 << 4)) );
                         sprintf(mask2,"%04x",mcm4word[0] & (~( 1 << 4)) );
                       */
                          sprintf(mask1,"%04x",mask_32[1]);
                          sprintf(mask2,"%04x",mask_32[0]);

                         printf("mask1 %s ", mask1);
                         printf("mask2: %s \n", mask2);

                         Mask32toArg(mask1, mask2);
                         exeCmd.CmdNum = 7;
                         FECB_CTRL(MCMCARD1);           //set dig mask 32 bits
                         Ms_delay(200);

                 //}
                   break;

       }

 }