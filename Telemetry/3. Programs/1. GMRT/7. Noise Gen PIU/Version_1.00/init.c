void Struct_Init();
void PortInit();
void ADC_Init();

void Struct_Init()
{
  atten_mode=0;
  time_mode=0;
  rfi_atten=0;
  t_on=16;
  nf_atten=0;
  t_total=1;
  strcpy(rfi_atten_Mon, "31.5");
  strcpy(nf_atten_Mon, "31.5");
  strcpy(t_on_mon, "16");
  strcpy(t_total_mon, "16");
  strcpy(rfi_mode_mon, "Direct Connection to ADC");
  strcpy(time_mode_mon, "Fix duty cycle");
  //Set_pwm();

  //word32_1=0x2B;
  //word32_2=0x2F;
  //word32_3=0x00;
  //word32_4=0xA1;
  word32_1=0x00;
  word32_2=0x00;
  word32_3=0x00;
  word32_4=0x81; //ton=1, ttotal=0, reset=0, control pin=1
  Set_32bit();
  Ms_delay(1000);
  Set_on_time();
  Ms_delay(1000);
  Set_total_time();
  Ms_delay(1000);
  Set_on_time();
  Ms_delay(1000);
  Set_total_time();
}

void PortInit()
{
  WrPortI(SPCR, &SPCRShadow, 0x84);       // Necessary statement to define Port A as normal Output //
  WrPortI(PADR, &PADRShadow, 0);

  WrPortI(PBDDR,&PBDDRShadow,0xFF);
  WrPortI(PBDR,&PBDRShadow,0xC0);        // Initialising Latch & Output Enable pins //

  WrPortI(PCFR,&PCFRShadow,(PCFRShadow  & 0xF5) );   //set TxC for serial comm, TxD for SPI
  WrPortI(PCALR, &PCALRShadow, 0x00);
  WrPortI(PCDDR,&PCDDRShadow,(PCDDRShadow | 0x05) ); //pc0 and pc2 are output pins
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

