/**** Common BOX Settings ****/
char frq_band[][15]={"50 MHz","150 MHz","233 MHz","327 MHz","610 MHz","1420 MHz"};
int fb_bits[] = {0x0000, 0x0001, 0x0008, 0x000A, 0x0010, 0x0014};

char solar_attn[][15]={"0 dB","14 dB","30 dB","44 dB","Terminate"};
int sa_bits[] = {0x0180, 0x01E0, 0x0000, 0x0060, 0x00A0};

char polar[][15] = {"Unswap","Swap"};
int pl_bits[] = {0x0000, 0x0200};


/**** FE BOX Settings ****/
char band_sel[][15]={"Sub 1","Sub 2","Sub 3","Sub 4","Bypass"};
int bs_bits[] = {0x00, 0x40, 0x20, 0x60, 0x80};

char noise_cal[][15]={"Low","Medium","High","Extra High"};
int nc_bits[] = {0x09, 0x0A, 0x05, 0x06};

char rf[][15] = {"On","Off"};
int rf_bits[] = {0x00, 0x10};