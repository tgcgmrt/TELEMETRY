void Struct_Init();
void PortInit();
void ADC_Init();
void rd_MCM_addr();

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

  strcpy(LO1, "0600");
  strcpy(LO2, "0600");
  //strcpy(Temp_LO1, "0600 MHz");
  //strcpy(Temp_LO2, "0600 MHz");
  strcpy(ref_lo1, "010");
  strcpy(ref_lo2, "010");
  //sprintf(Temp_LOR1,"010 MHz");
  //sprintf(Temp_LOR2,"010 MHz");
  sprintf(LO1_Mon,"600 MHz");
  sprintf(LO2_Mon,"600 MHz");
  sprintf(LOR1_Mon,"10 MHz");
  sprintf(LOR2_Mon,"10 MHz");
  Temp_SS =  2;
  Temp_FDB = 2;
  Temp_FDV = 1;
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
  strcpy(SS1_Mon,"Normal");
  strcpy(SS2_Mon,"Normal");
  strcpy(FDB_Mon,"On");
  strcpy(FDV_Mon,"1");
  strcpy(MCM_Clk,"60 MHz");
  strcpy(msw,"0000");
  strcpy(lsw,"0000");
  db1=00.0;
  db2=00.0;
  strcpy(ip,MY_IP_ADDR);
  strcpy(mask,MY_NET_MASK);
  strcpy(gateway,MY_GATEWAY);
  strcpy(mswT, msw);
  strcpy(lswT, lsw);

  for(i=0;i<70;i++)
  {
   rw[i]=0;
   strcpy(vl[i],"0");
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


  strcpy(lsw,"0000");
  strcpy(msw,"0000");
  Set_DMask();


}

// This function will initialise ADC by proper calibration //
void ADC_Init()
{
  char i;
  for(i=0; i<8; i++)
    anaInCalib(i, SINGLE, GAINSET, 0, 0.0, 255, 5.0);
}


// This function will read MCM address //
void rd_MCM_addr()
{
  mcm_addr = 1;
}