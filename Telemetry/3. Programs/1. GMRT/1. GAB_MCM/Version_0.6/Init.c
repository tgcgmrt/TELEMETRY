void Struct_Init();
void PortInit();
void ADC_Init();
void rd_MCM_addr();
void Init();

void Struct_Init()
{
  char i,j;

  pc_cmd.Sequence_No = 0;

  for(i=0; i<64;i++)
  pc_cmd.TimeStamp[i] = 0;

  for(i=0; i<16;i++)
  pc_cmd.System_Name[i] = 0;

  for(i=0; i<16;i++)
  pc_cmd.Operation_Name[i] = 0;

  for(i=0; i<32;i++)
  {
    for(j=0;j<16;j++)
    {
     pc_cmd.Parameter_Name[i][j] = 0;
     pc_cmd.Argument_Ch1[i][j] = 0;
     pc_cmd.Argument_Ch2[i][j] = 0;
    }
  }


  mcm_resp.Response_Type = 0;

  mcm_resp.Sequence_No = 0;

  for(i=0; i<64; i++)
  {
   mcm_resp.TimeStamp[i] = 0;
  }

  for(i=0; i<16;i++)
  {
   mcm_resp.System_Name[i] = 0;
  }

  for(i=0; i<64;i++)
  {
    for(j=0;j<8;j++)
    {
     mcm_resp.Mon_Raw[i][j] = 0;
    }
  }

  for(i=0; i<32;i++)
  {
    for(j=0;j<64;j++)
    {
     mcm_resp.Mon_Sum[i][j] = 0;
    }
  }

  for(i=0; i<32;i++)
  {
    for(j=0;j<64;j++)
    {
     mcm_resp.Response_Message[i][j] = 0;
    }
  }
}



// .... PortInit() initializes port pins as per application requirements .... //
/* It will initialize 16 digital outputs, 4 mux selection channels, Port C SPI
   Clock channel, Latch selection channels etc.     */

void PortInit()
{
  WrPortI(SPCR, &SPCRShadow, 0x84);       // Necessary statement to define Port A as normal Output //
  WrPortI(PADR, &PADRShadow, 0);

  BitWrPortI(PEDDR, &PEDDRShadow, 1, 0);  // Defining SPI_1, SPI_2 as output //
  BitWrPortI(PEDDR, &PEDDRShadow, 1, 1);

  BitWrPortI(PEDR, &PEDRShadow, 1, 0);    // Set Pin PE1, PE2 (Enable for SPI_1, SPI_2) as High //
  BitWrPortI(PEDR, &PEDRShadow, 1, 1);

  BitWrPortI(PEFR, &PEFRShadow, 1, 3);    // Pin PE3 work as alternet output //
  WrPortI(PEALR, &PEALRShadow, 0xC0);     // Pin PE3 set for SPI port D Serial Clock //

  BitWrPortI(PCFR, &PCFRShadow, 1, 0);    // Set Pin PC0 as alternate output : SPI_Tx_D //
  BitWrPortI(PCALR, &PCALRShadow, 0, 0);
  BitWrPortI(PCALR, &PCALRShadow, 0, 1);

  BitWrPortI(PBDDR, &PBDDRShadow, 1, 2);  // Defining Latch & Output Enable pins as output //
  BitWrPortI(PBDDR, &PBDDRShadow, 1, 3);
  BitWrPortI(PBDDR, &PBDDRShadow, 1, 4);
  BitWrPortI(PBDDR, &PBDDRShadow, 1, 5);
  BitWrPortI(PBDDR, &PBDDRShadow, 1, 6);

  BitWrPortI(PBDR, &PBDRShadow, 0, 2);    // Initialising Latch & Output Enable pins //
  BitWrPortI(PBDR, &PBDRShadow, 0, 3);
  BitWrPortI(PBDR, &PBDRShadow, 0, 4);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  BitWrPortI(PBDR, &PBDRShadow, 1, 6);

  BitWrPortI(PEDDR,&PEDDRShadow,1,4);   // Initialize Mux selection pins as output //
  BitWrPortI(PEDDR,&PEDDRShadow,1,5);
  BitWrPortI(PEDDR,&PEDDRShadow,1,6);
  BitWrPortI(PEDDR,&PEDDRShadow,1,7);

  BitWrPortI(PEDR,&PEDRShadow,0,4);    // Initializing Mux selection pins //
  BitWrPortI(PEDR,&PEDRShadow,0,5);
  BitWrPortI(PEDR,&PEDRShadow,0,6);
  BitWrPortI(PEDR,&PEDRShadow,0,7);
}



// This function will initialise ADC by proper calibration //
void ADC_Init()
{
  char i;
  for(i=0; i<8; i++);
    //anaInCalib(i, SINGLE, GAINSET, 0, 0.0, 2048, 5.0);
}



// This function will read MCM address //
void rd_MCM_addr()
{
 mcm_addr = 0;
}



void Init()
{
  char i,j;

  sprintf(webpg_mon.ant,"C00");
  sprintf(webpg_mon.ip_addr,MY_IP_ADDR);

  strcpy(webpg_mon.sys_name," ");

  strcpy(reset,"0");

  for(i=0; i<10; i++)
  {
    for(j=0; j<20; j++)
    {
     webpg_mon.sys_para[i][j] = 0;
     webpg_mon.arg1[i][j] = 0;
     webpg_mon.arg2[i][j] = 0;
    }
  }

 /***********************************
  if(mcm_addr==0) // Sentinel
  {
    sprintf(webpg_mon.sys_name,"SENTINEL");

    strcpy(webpg_mon.sys_para[0],"Digital Mask");
    strcpy(webpg_mon.arg1[0],"0000");
    strcpy(webpg_mon.arg2[0],"0000");
  }

  else if(mcm_addr==1)  // Front End
  {
    sprintf(webpg_mon.sys_name,"FRONT END");

    strcpy(webpg_mon.sys_para[0],"Calibration Noise");
    strcpy(webpg_mon.arg1[0],"Medium");
    strcpy(webpg_mon.arg2[0],"Medium");

    strcpy(webpg_mon.sys_para[1],"RF");
    strcpy(webpg_mon.arg1[1],"Off");
    strcpy(webpg_mon.arg2[1],"Off");

    strcpy(webpg_mon.sys_para[2],"Filter");
    strcpy(webpg_mon.arg1[2],"1");
    strcpy(webpg_mon.arg2[2],"1");

    strcpy(webpg_mon.sys_para[3],"Filter L-Band");
    strcpy(webpg_mon.arg1[3],"1060 MHz");
    strcpy(webpg_mon.arg2[3],"1060 MHz");

    strcpy(webpg_mon.sys_para[4],"Band Selection");
    strcpy(webpg_mon.arg1[4],"150 MHz");
    strcpy(webpg_mon.arg2[4],"150 MHz");

    strcpy(webpg_mon.sys_para[5],"Solar Attenuation");
    strcpy(webpg_mon.arg1[5],"14 dB");
    strcpy(webpg_mon.arg2[5],"14 dB");

    strcpy(webpg_mon.sys_para[6],"Channel");
    strcpy(webpg_mon.arg1[6],"Unswap");
    strcpy(webpg_mon.arg2[6]," ");
  }

  else if(mcm_addr==2)  // Fiber Optics
  {
    sprintf(webpg_mon.sys_name,"FIBER OPTICS");

    strcpy(webpg_mon.sys_para[0],"RF Attenuation");
    strcpy(webpg_mon.arg1[0],"10 dB");
    strcpy(webpg_mon.arg2[0],"10 dB");
  }

  else if(mcm_addr==3)  // Analog Back End
  {
    sprintf(webpg_mon.sys_name,"ANALOG BACK END");

    strcpy(webpg_mon.sys_para[0],"LO Frequency");
    strcpy(webpg_mon.arg1[0],"600 MHz");
    strcpy(webpg_mon.arg2[0],"600 MHz");

    strcpy(webpg_mon.sys_para[1],"Attenuation");
    strcpy(webpg_mon.arg1[1],"12.0 dB");
    strcpy(webpg_mon.arg2[1],"22.5 dB");

    strcpy(webpg_mon.sys_para[2],"Filter");
    strcpy(webpg_mon.arg1[2],"1");
    strcpy(webpg_mon.arg2[2],"6");

    strcpy(webpg_mon.sys_para[3],"Low Pass Filter");
    strcpy(webpg_mon.arg1[3],"0");
    strcpy(webpg_mon.arg2[3],"1");

    strcpy(webpg_mon.sys_para[4],"Source");
    strcpy(webpg_mon.arg1[4],"SigGen");
    strcpy(webpg_mon.arg2[4]," ");

    strcpy(webpg_mon.sys_para[5],"Signal");
    strcpy(webpg_mon.arg1[5],"Antenna");
    strcpy(webpg_mon.arg2[5]," ");

    strcpy(webpg_mon.sys_para[6],"Path");
    strcpy(webpg_mon.arg1[6],"Direct");
    strcpy(webpg_mon.arg2[6]," ");

    strcpy(webpg_mon.sys_para[7],"Channel");
    strcpy(webpg_mon.arg1[7],"1");
    strcpy(webpg_mon.arg2[7]," ");
  }
 ******************************************/


  strcpy(SS1_Mon,"Normal");
  strcpy(SS2_Mon,"Normal");
  strcpy(FDB_Mon,"On");
  strcpy(FDV_Mon,"1");
  strcpy(MCM_Clk,"60 MHz");
  strcpy(msw,"0000");
  strcpy(lsw,"0000");
}



