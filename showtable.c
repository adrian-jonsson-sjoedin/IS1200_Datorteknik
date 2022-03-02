void show_table(uint32_t avgtemp, uint32_t mintemp, uint32_t maxtemp)
{
char buf[32], buf2[32], buf3[32];
char *avgt, *mint, *maxt, *avgtt, *mintt, *maxtt;
avgt = fixed_to_string(avgtemp, buf);
avgtt = avgt + strlen(avgt);
mint = fixed_to_string(mintemp, buf2);
mintt = mint + strlen(mint);
maxt = fixed_to_string(maxtemp, buf3);
maxtt = maxt + strlen(maxt);


*avgtt++ = ' ';
*avgtt++ = 'insert unit';
*avgtt++ = ' ';
*avgtt++ = 'a';
*avgtt++ = 'v';
*avgtt++ = 'g';
*avgtt++ = '.';
*avgtt++ = 0;

*mintt++ = ' ';
*mintt++ = 'insert unit';
*mintt++ = ' ';
*mintt++ = 'm';
*mintt++ = 'i';
*mintt++ = 'n';
*mintt++ = '.';
*mintt++ = 0;

*maxtt++ = ' ';
*maxtt++ = 'insert unit';
*maxtt++ = ' ';
*maxtt++ = 'm';
*maxtt++ = 'a';
*maxtt++ = 'x';
*maxtt++ = '.';
*maxtt++ = 0;
while(!(button1 & 0x200) || !(buttons & 2) || !(buttons & 1) || buttons & 3)
{
display_string(0, avgtt);
display_string(1, mintt);
display_string(2, maxtt);
display_string(3, "exit - btn1");
display_update();
}

}
