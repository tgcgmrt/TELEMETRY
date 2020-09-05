//Set input and output buffer
#define DINBUFSIZE  127
#define DOUTBUFSIZE 127

void main()
{
	int 			i,    	//for loop index
   				data;		//serial input data
	auto char 	cOut;		//serial output data
   data = 0;				//initialize variable

	serDopen(9600);		//open serial port at 9600 baud
	serDwrFlush();			//flush write buffer
	serDrdFlush();			//flush read buffer

	while(1)
   {
      //cycle output between a-z
      for (cOut='a';cOut<='z';++cOut)
		{
			serDputc (cOut);		//output data
         data = serDgetc();   //read and print data
			printf("Input = %c\n", data);
		}
	}
}