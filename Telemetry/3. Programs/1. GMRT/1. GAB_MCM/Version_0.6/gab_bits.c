char lpf[][15] = {"100 MHz", "200 MHz", "400 MHz", "Direct"};
char lpf_bits[] = {0x00, 0x01, 0x02, 0x03};

char source[][15] = {"Synthesizer", "SigGen"};
char src_bits[] = {0x00, 0x01};

char signal[][15] = {"Antenna RF", "VGA Cal", "Test Cal"};
char sig_bits[] = {0x00, 0x01, 0x02, 0x03};

char path[][15]  = {"Mixer", "Direct"};
char pth_bits[] = {0x00, 0x01};

char channel[][15] = {"1", "2"};
char chn_bits[] = {0x00, 0x01};

#define	lpf_arg_no	sizeof(lpf)/sizeof(lpf[0])
#define	src_arg_no	sizeof(source)/sizeof(source[0])
#define	sig_arg_no	sizeof(signal)/sizeof(signal[0])
#define	pth_arg_no	sizeof(path)/sizeof(path[0])
#define	chn_arg_no	sizeof(channel)/sizeof(channel[0])