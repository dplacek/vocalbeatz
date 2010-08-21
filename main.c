#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
  double avg;
  int *samples;
} data;

typedef struct {
  int start;
  int stop;
  int peak;
  int valley;
  int direction;
  int diff;
} wave;

typedef struct {
  void *ptr;
  void *next;
} link;

unsigned int
ule2be(buf, len)
  unsigned char *buf;
  int len;
{
  unsigned int result = 0;
  unsigned int tmp;
  int i;

  for (i = 0; i < len; i++) {
    tmp = buf[i];
    result |= (tmp << ((i & 7) << 3));
  }
  return result;
}

int
le2be(buf, len)
  unsigned char *buf;
  int len;
{
  unsigned int tmp;
  int result, bits;

  bits = len * 8;
  tmp = ule2be(buf, len);
  if ((tmp >> (bits - 1)) == 1) {
    tmp &= ((1 << bits) - 1);
    tmp ^= ((1 << bits) - 1);
    result = -tmp;
  }
  else
    result = tmp;
  return result;
}

int
main(argc, argv)
  int argc;
  char **argv;
{
  FILE *f;
  unsigned char buf[2048];
  int res, filesize, chunksize, numchannels, samplerate, numsamples,
      avgbytes, blockalign, sigbits, sigbytes, sample, flag, i, j, n,
      *prev_samples = NULL;
  link **heads = NULL, **tails = NULL, *newlink = NULL;
  wave *curwave = NULL, *newwave = NULL;

  f = fopen(argv[1], "r");

  /* read the initial header */
  res = fread(buf, 4, 1, f);
  if (strncmp("RIFF", buf, 4) != 0) {
    fprintf(stderr, "File format not recognized.\n");
    goto done;
  }

  res = fread(buf, 4, 1, f); /* file size */
  filesize = ule2be(buf, 4);
  printf("Size: %d\n", filesize);

  res = fread(buf, 4, 1, f);
  if (strncmp("WAVE", buf, 4) != 0) {
    fprintf(stderr, "File format not recognized.\n");
    goto done;
  }

  /* format chunk */
  res = fread(buf, 4, 1, f);
  if (strncmp("fmt ", buf, 4) != 0) {
    fprintf(stderr, "Format chunk wasn't first.\n");
    goto done;
  }

  res = fread(buf, 4, 1, f);
  chunksize = ule2be(buf, 4);
  if (chunksize > 16) {
    fprintf(stderr, "There are extra format bytes: %d.\n", n);
    goto done;
  }

  res = fread(buf, 2, 1, f); /* compression code */
  n = ule2be(buf, 2);
  if (n != 1) {
    fprintf(stderr, "This audio is compressed.\n");
    goto done;
  }

  res = fread(buf, 2, 1, f); /* number of channels */
  numchannels = ule2be(buf, 2);
  printf("Channels: %d\n", numchannels);

  res = fread(buf, 4, 1, f); /* sample rate */
  samplerate = ule2be(buf, 4);
  printf("Sample Rate: %d\n", samplerate);

  res = fread(buf, 4, 1, f); /* average bytes per second */
  avgbytes = ule2be(buf, 4);
  printf("Average Bytes Per Second: %d\n", avgbytes);

  res = fread(buf, 2, 1, f); /* block align */
  blockalign = ule2be(buf, 2);
  printf("Block Align: %d\n", blockalign);

  res = fread(buf, 2, 1, f); /* significant bits per sample */
  sigbits = ule2be(buf, 2);
  sigbytes = sigbits / 8;
  printf("Significant Bits Per Sample: %d (%d bytes)\n", sigbits, sigbytes);

  /* data chunk! */
  res = fread(buf, 4, 1, f);
  if (strncmp("data", buf, 4) != 0) {
    fprintf(stderr, "Data chunk wasn't second.\n");
    goto done;
  }

  res = fread(buf, 4, 1, f);
  chunksize = ule2be(buf, 4);
  printf("Data size: %d\n", chunksize);

  numsamples = chunksize / (sigbytes * numchannels);
  printf("Number of Samples: %d\n", numsamples);

  /* initialize wave linked list */
  heads = (link **)malloc(sizeof(link *) * numchannels);
  tails = (link **)malloc(sizeof(link *) * numchannels);
  for (i = 0; i < numchannels; i++) {
    curwave = (wave *)malloc(sizeof(wave) * numchannels);
    curwave->peak   = INT_MIN;
    curwave->valley = INT_MAX;
    curwave->direction = 0;

    heads[i] = (link *)malloc(sizeof(link));
    tails[i] = (link *)malloc(sizeof(link));
    heads[i]->ptr = tails[i]->ptr = (void *)curwave;
    heads[i]->next = tails[i]->next = NULL;
  }

  prev_samples = (int *)malloc(sizeof(int) * numchannels);
  for (i = 0; i < numsamples; i++) {
    if (i % (samplerate / 4) == 0) {
      printf("=== Second %f ===\n", (float)i / samplerate);
    }
    for (j = 0; j < numchannels; j++) {
      curwave = (wave *)tails[j]->ptr;

      res = fread(buf, sigbytes, 1, f);
      sample = le2be(buf, sigbytes);
      flag = 0;
      if (i > 0) {
        switch(curwave->direction) {
          case -1:
            if (sample > prev_samples[j]) {
              curwave->valley = prev_samples[j];
              curwave->direction = 1;
              flag = curwave->peak != INT_MIN;
            }
            break;
          case 0:
            if (sample != prev_samples[j])
              curwave->direction = sample > prev_samples[j] ? 1 : -1;
            break;
          case 1:
            if (sample < prev_samples[j]) {
              curwave->peak = prev_samples[j];
              curwave->direction = -1;
              flag = curwave->valley != INT_MAX;
            }
            break;
        }
        if (flag) {
          curwave->diff = curwave->peak - curwave->valley;
          curwave->stop = i-1;
          /*
          printf("Channel %d wave: %09d-%09d %+06d:%+06d (%05d%s)\n",
              j, curwave->start, curwave->stop, curwave->valley,
              curwave->peak, curwave->diff, curwave->diff > 1000 ? "(HUGE)" : "");
          */

          newwave = (wave *)malloc(sizeof(wave) * numchannels);
          newwave->start  = i;
          newwave->peak   = INT_MIN;
          newwave->valley = INT_MAX;
          newwave->direction = 0;

          newlink = (link *)malloc(sizeof(link));
          newlink->ptr = (void *)newwave;
          newlink->next = NULL;

          tails[j]->next = (void *)newlink;
          tails[j] = newlink;
        }
      }
      prev_samples[j] = sample;
    }
    for (j = 0; j < numchannels; j++)
      printf("\t% 05d", prev_samples[j]);

    printf("\n");
  }

done:
  fclose(f);
  return 0;
}
