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
  strcpy(Attn1_Mon,"00 dB");
  strcpy(Attn2_Mon,"00 dB");
  strcpy(lt1i1,"0.00");
  strcpy(lt2i2,"0.00");
  strcpy(lt3i3,"0.00");
  strcpy(lt4i4,"0.00");
  strcpy(lt1p1,"0.00");
  strcpy(lt2p2,"0.00");
  strcpy(lt3p3,"0.00");
  strcpy(lt4p4,"0.00");
  strcpy(temp1,"00.00");

  strcpy(lsw,"0000");
  strcpy(msw,"0000");
  strcpy(mswT, msw);
  strcpy(lswT, lsw);
  strcpy(SS1_Mon,"Normal");
  strcpy(SS2_Mon,"Normal");
  strcpy(FDB_Mon,"On");
  strcpy(FDV_Mon,"1");
  strcpy(MCM_Clk,"60 MHz");
  strcpy(temps,"00.00");
  smoke_detector=0;
  intruder_detector=0;
  strcpy(state,"OK");
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

  WrPortI(PCFR,&PCFRShadow,(PCFRShadow  & 0xFC) );   //set TxC for serial comm
  WrPortI(PCALR, &PCALRShadow, 0x00);
  WrPortI(PCDDR,&PCDDRShadow,(PCDDRShadow | 0x04) ); //set pc2 as output pin pc3 as input pin
  WrPortI(PCDR,&PCDRShadow,0xFF );

  BitWrPortI(PEFR, &PEFRShadow, 1, 3);    // Pin PE3 work as alternet output //
  WrPortI(PEALR, &PEALRShadow, 0xC0);     // Pin PE3 set for SPI port D Serial Clock //
  WrPortI(PEDDR,&PEDDRShadow,0xFF);
  WrPortI(PEDR,&PEDRShadow,0xF3);
}

// This function will initialise ADC by proper calibration //
void ADC_Init()
{
  char i;
  for(i=0; i<8; i++)
    anaInCalib(i, SINGLE, GAINSET, 0, 0.0, 255, 5.0);
}

void serial_init()
{

if (serCopen(9600L))
printf("Serial C Port opened \n");
else
printf("Serial C port problem\n");

serCdatabits(PARAM_8BIT);  //by default databits are 8
serCparity(PARAM_MPARITY); //0x04...mark parity
serCflowcontrolOff();
}