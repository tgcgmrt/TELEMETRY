void Struct_Init();
void PortInit();
void ADC_Init();

void Struct_Init()
{
  int loop,i,j;

  Wrpr_Cmd.cmd_elem.seq = 0;
  Wrpr_Cmd.cmd_elem.version = 0.0;
  Wrpr_Cmd.cmd_elem.priority = 0;
  Wrpr_Cmd.cmd_elem.timeout = 0;
  for(loop=0; loop<DATALEN; loop++)
  {
    Wrpr_Cmd.cmd_elem.subsysid[loop]=0;
    Wrpr_Cmd.cmd_elem.cmd_name[loop]=0;
    Wrpr_Cmd.cmd_elem.timestamp[loop]=0;
  }
  for(loop=0; loop<8; loop++)
     Wrpr_Cmd.cmd_elem.cmd_id[loop]=0;

  MCM_Resp.MCM_alarm.id = 0;
  MCM_Resp.MCM_alarm.level = 0;
  for(loop=0; loop<DATALEN; loop++)
    MCM_Resp.MCM_alarm.description[loop]=0;

  MCM_Resp.code = 0;
  MCM_Resp.event = 0;

  MCM_Resp.resp_elem.seq = 0;
  MCM_Resp.resp_elem.version = 0.0;
  MCM_Resp.resp_elem.priority = 0;
  MCM_Resp.resp_elem.timeout = 0;
  for(loop=0; loop<DATALEN; loop++)
  {
    MCM_Resp.message[loop]=0;
    MCM_Resp.resp_elem.subsysid[loop]=0;
    MCM_Resp.resp_elem.cmd_name[loop]=0;
    MCM_Resp.resp_elem.timestamp[loop]=0;
  }
  for(loop=0; loop<8; loop++)
     MCM_Resp.resp_elem.cmd_id[loop]=0;


  Wrpr_Cmd.cmd_data.numpkt = 0;
  MCM_Resp.resp_data.numpkt = 0;
  for(i=0; i<MAXDATA ; i++)
  {
   for(j=0; j<DATALEN ; j++)
    {
       MCM_Resp.resp_data.prmname[i][j]=0;
       MCM_Resp.resp_data.prmvalue[i][j]=0;
       Wrpr_Cmd.cmd_data.prmname[i][j]=0;
       Wrpr_Cmd.cmd_data.prmvalue[i][j]=0;
    }
  }
  sprintf(LO1_Mon,"000 MHz");
  sprintf(LO2_Mon,"000 MHz");
  sprintf(LOR1_Mon,"000 MHz");
  sprintf(LOR2_Mon,"000 MHz");
  sprintf(CHsp1_Mon,"000 KHz");
  sprintf(CHsp2_Mon,"000 KHz");

  filter1=1;
  filter2=1;
  strcpy(source1,"Synthesizer");
  strcpy(source2,"Synthesizer");
  strcpy(lpf1,"100 MHZ");
  strcpy(lpf2,"100 MHZ");
  strcpy(path1,"Mixer");
  strcpy(in1,"RF");
  strcpy(path2,"Mixer");
  strcpy(in2,"RF");
  strcpy(Attn1_Mon,"31.5 dB");
  strcpy(Attn2_Mon,"31.5 dB");

  tLO1=LO1=000;
  tLO2=LO2=000;
  tLOR1=LOR1=10;   //Reference Freq in MHz
  tLOR2=LOR2=10;
  tCHsp1=CHsp1=10;  //step size in Khz
  tCHsp2=CHsp2=10;

  //tLO1=LO1=000;
  //tLO2=LO2=000;
  tLO3=LO3=000;
 // tLOR1=LOR1=10;   //Reference Freq in MHz
 // tLOR2=LOR2=10;
  tLOR3=LOR3=10;
 // tCHsp1=CHsp1=10;  //step size in Khz
 // tCHsp2=CHsp2=10;
  tCHsp=CHsp=10;

  strcpy(lsw,"0000");
  strcpy(msw,"0000");
  strcpy(mswT, msw);
  strcpy(lswT, lsw);
  strcpy(SS1_Mon,"Normal");
  strcpy(SS2_Mon,"Normal");
  strcpy(FDB_Mon,"On");
  strcpy(FDV_Mon,"1");
  strcpy(MCM_Clk,"60 MHz");

  reset1=0;
  Temp_SS =  2;
  Temp_FDB = 2;
  Temp_FDV = 1;
  strcpy(psw,"      ");

  for(i=0;i<70;i++)
  {
   rw[i]=0;
   strcpy(vl[i],"0");
  }

}

void PortInit()
{
  WrPortI(SPCR, &SPCRShadow, 0x84);       // Necessary statement to define Port A as normal Output //
  WrPortI(PADR, &PADRShadow, 0);

  WrPortI(PBDDR,&PBDDRShadow,0xFF);
  WrPortI(PBDR,&PBDRShadow,0xC0);        // Initialising Latch & Output Enable pins //

  //WrPortI(PCFR,&PCFRShadow,(PCFRShadow  & 0xF5) );   //set TxC for serial comm, TxD for SPI
  //WrPortI(PCALR, &PCALRShadow, 0x00);
  WrPortI(PCFR,&PCFRShadow,(PCFRShadow  & 0xFA) );
  WrPortI(PCDDR,&PCDDRShadow,(PCDDRShadow | 0x05) ); //pc0 and pc2 are output pins
  WrPortI(PCDR,&PCDRShadow,0x00 );

  //BitWrPortI(PEFR, &PEFRShadow, 1, 3);    // Pin PE3 work as alternet output //
  WrPortI(PEALR, &PEALRShadow, 0xC0);     // Pin PE3 set for SPI port D Serial Clock //
  WrPortI(PEDDR,&PEDDRShadow,0xFF);
  WrPortI(PEDR,&PEDRShadow,0xF0);

  BitWrPortI(PBDR, &PBDRShadow, 1, 6);
  BitWrPortI(PBDR, &PBDRShadow, 0, 5);
  WrPortI(PADR, &PADRShadow, 0x20);
  BitWrPortI(PBDR, &PBDRShadow, 1, 5);
  BitWrPortI(PBDR, &PBDRShadow, 0, 6);
}

// This function will initialise ADC by proper calibration //
void ADC_Init()
{
  char i;
  for(i=0; i<8; i++)
    anaInCalib(i, SINGLE, GAINSET, 0, 0.0, 255, 5.0);
}

