/*
DATA= pin 1 to pin 8
RS= pin 17
RW= GND
EN= pin 25
*/
#class auto
#use "RCM43xx.LIB"

void LCDINIT(void);
void LCDCOMMAND(unsigned char);
void LCDDATA(unsigned char);
//void MSDELAY(unsigned int);
void send_string(unsigned char *var);

void MSDELAY(int val)
{
int i,j;
for(i=0;i<val;i++)
{
for(j=0;j<600;j++)
{
}
}
}

void main(void)
{
WrPortI(SPCR, &SPCRShadow, 0x84);
MSDELAY(10);
WrPortI(PADR, &PADRShadow,0xFF);      //  FF on PA.
MSDELAY(10);
WrPortI(PBDDR, &PBDDRShadow, 0xFF);
MSDELAY(10);
WrPortI(PBDR, &PBDRShadow, 0x40);

WrPortI(PADR,&PADRShadow,0x00);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x3C);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(100);
BitWrPortI(PADR,&PADRShadow,0,0);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x10);          // RW=0
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR,&PADRShadow,1,2);
MSDELAY(10);
LCDINIT();
send_string("**LCD IS READY**");
LCDCOMMAND(0xC0);
send_string("****************");
while(1)
{
}
}

void LCDCOMMAND(unsigned char value)
{
WrPortI(PADR,&PADRShadow,value);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x04);          //  data
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 0,0);      // RS=0
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 1,1);      // EN=1
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 0,1);      // EN=0
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
}

void LCDDATA(unsigned char value)
{
WrPortI(PADR,&PADRShadow,value);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x04);          //  data
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 1,0);      // RS=1
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 1,1);      // EN=1
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
BitWrPortI(PADR, &PADRShadow, 0,1);      // EN=0
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x08);
MSDELAY(10);
WrPortI(PBDR,&PBDRShadow,0x00);
MSDELAY(10);
}


void LCDINIT(void)
{
LCDCOMMAND(0x38);     // initialization in 5X7 matrix ,8 data lines ,16X2 mode
MSDELAY(10);
LCDCOMMAND(0x0E);     //display on cursor off
MSDELAY(10);
LCDCOMMAND(0x01);     //clear LCD
MSDELAY(10);
LCDCOMMAND(0x06);     //increment curser(shift curser to right)
MSDELAY(10);
LCDCOMMAND(0x80);     //line 1,position 6
MSDELAY(10);
}

void send_string(unsigned char *var)
{
     while(*var)              //till string ends
       LCDDATA(*var++);  //send characters one by one
}


