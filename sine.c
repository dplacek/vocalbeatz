#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FREQ 261  // middle C
#define SECONDS 5
#define SAMPLERATE 44100
#define CHANNELS 2
#define SAMPLESIZE 16
#define INTENSITY 5000

#define STEP (2 * M_PI) / (SAMPLERATE / FREQ)

void
ube2le(num, buf, len)
  unsigned int num;
  unsigned char *buf;
  int len;
{
  unsigned int tmp = num;
  int i;

  for (i = 0; i < len; i++) {
    buf[i] = num & 255;
    num = num >> 8;
  }
}

int
main(argc, argv)
  int argc;
  char **argv;
{
  FILE *f;
  unsigned char buf[256];
  unsigned int filesize, n;
  int i, j, k;

  /* riff header */
  f = fopen("sine.wav", "w");
  fwrite("RIFF", 4, 1, f);

  filesize = 24          /* fmt chunk */
    + SECONDS
    * SAMPLERATE
    * CHANNELS
    * SAMPLESIZE;
  ube2le(filesize, buf, 4);
  fwrite(buf, 4, 1, f);

  fwrite("WAVE", 4, 1, f);

  /* format chunk */
  fwrite("fmt ", 4, 1, f);
  ube2le(16, buf, 4);   /* chunk size */
  fwrite(buf, 4, 1, f);
  ube2le(1, buf, 2);    /* compression code */
  fwrite(buf, 2, 1, f);
  ube2le(CHANNELS, buf, 2);
  fwrite(buf, 2, 1, f);
  ube2le(SAMPLERATE, buf, 4);
  fwrite(buf, 4, 1, f);
  ube2le(SAMPLERATE * CHANNELS * SAMPLESIZE / 8, buf, 4);
  fwrite(buf, 4, 1, f);
  ube2le(CHANNELS * SAMPLESIZE / 8, buf, 2);
  fwrite(buf, 2, 1, f);
  ube2le(SAMPLESIZE, buf, 2);
  fwrite(buf, 2, 1, f);

  /* data chunk */
  fwrite("data", 4, 1, f);
  ube2le(filesize - 36, buf, 4);
  fwrite(buf, 4, 1, f);

  for (i = 0; i < SECONDS; i++) {
    for (j = 0; j < SAMPLERATE; j++) {
      n = sin(j * STEP) * INTENSITY;
      ube2le(n, buf, SAMPLESIZE / 8);
      for (k = 0; k < CHANNELS; k++)
        fwrite(buf, SAMPLESIZE / 8, 1, f);
    }
  }

done:
  fclose(f);
  return 0;
}
