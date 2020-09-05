#use "rcm56xxw.lib"
void main()
{
	int Switch_1;

   //Initialize MiniCore hardware
   brdInit();

   printf("Press Switch to Light LED");
   while (1)
   {
      if (!BitRdPortI(PDDR, 1))	//Read S1
      {
      	BitWrPortI(PDDR, &PDDRShadow, 0, 0);
      }
      else
      {
      	BitWrPortI(PDDR, &PDDRShadow, 1, 0);
      }
   }
}