#use "rcm56xxw.lib"
void main()
{
	int Switch_1;

   //Initialize MiniCore hardware
   brdInit();

   while (1)
   {
      Switch_1 = !BitRdPortI(PDDR, 1);	//Read S1
      printf("Switch 1 = %d\r",Switch_1);
   }
}