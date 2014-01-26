/* audio.[ch]
 *
 * part of the Systems Programming 2005 assignment
 * (c) Vrije Universiteit Amsterdam, BSD License applies
 * contact info : wdb -_at-_ few.vu.nl
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define swap_short(x)		(x)
#define swap_long(x)		(x)

#include <stdint.h>
#include <sys/soundcard.h>
#include "audio.h"

/* Definitions for the WAVE format */

#define RIFF		"RIFF"	
#define WAVEFMT		"WAVEfmt"
#define DATA		0x61746164
#define PCM_CODE	1

typedef struct _waveheader {
  char		main_chunk[4];	/* 'RIFF' */
  uint32_t	length;		/* filelen */
  char		chunk_type[7];	/* 'WAVEfmt' */
  uint32_t	sc_len;		/* length of sub_chunk, =16 */
  uint16_t	format;		/* should be 1 for PCM-code */
  uint16_t	chans;		/* 1 Mono, 2 Stereo */
  uint32_t	sample_fq;	/* frequence of sample */
  uint32_t	byte_p_sec;
  uint16_t	byte_p_spl;	/* samplesize; 1 or 2 bytes */
  uint16_t	bit_p_spl;	/* 8, 12 or 16 bit */ 

  uint32_t	data_chunk;	/* 'data' */
  uint32_t	data_length;	/* samplecount */
} WaveHeader;

int aud_readinit (char *filename, int *sample_rate, 
		  int *sample_size, int *channels ) 
{
  /* Sets up a descriptor to read from a wave (RIFF). 
   * Returns file descriptor if successful*/
  int fd;
  WaveHeader wh;
		  
  if (0 > (fd = open (filename, O_RDONLY))){
    fprintf(stderr,"unable to open the audiofile\n");
    return -1;
  }

  read (fd, &wh, sizeof(wh));

  if (0 != bcmp(wh.main_chunk, RIFF, sizeof(wh.main_chunk)) || 
      0 != bcmp(wh.chunk_type, WAVEFMT, sizeof(wh.chunk_type)) ) {
    fprintf (stderr, "not a WAVE-file\n");
    errno = 3;// EFTYPE;
    return -1;
  }
  if (swap_short(wh.format) != PCM_CODE) {
    fprintf (stderr, "can't play non PCM WAVE-files\n");
    errno = 5;//EFTYPE;
    return -1;
  }
  if (swap_short(wh.chans) > 2) {
    fprintf (stderr, "can't play WAVE-files with %d tracks\n", wh.chans);
    return -1;
  }
	
  *sample_rate = (unsigned int) swap_long(wh.sample_fq);
  *sample_size = (unsigned int) swap_short(wh.bit_p_spl);
  *channels = (unsigned int) swap_short(wh.chans);	
	
  fprintf (stderr, "%s chan=%u, freq=%u sample-size=%u\n", 
	   filename, *channels, *sample_rate, *sample_size);
  return fd;
}

pa_simple *aud_writeinit (int sample_rate, int sample_size, int channels) 
{
  /* Sets up the audio device params. 
   * Returns device file descriptor if successful*/
  pa_simple *s;
  pa_sample_spec ss;

  printf("requested chans=%d, freq=%d sample-size=%d\n", 
	 channels, sample_rate, sample_size);
	
  ss.rate = sample_rate;
  ss.channels = channels;
  switch (sample_size) {
  case 8:
    ss.format = PA_SAMPLE_U8;
    break;
  case 16:
    ss.format = PA_SAMPLE_S16NE;
    break;
  default:
    fprintf(stderr,"Unsupported sample size %d\n",sample_size);
    return NULL;
  }

  s = pa_simple_new(NULL,               // Use the default server.
		    "SysProg",          // Our application's name.
		    PA_STREAM_PLAYBACK,
		    NULL,               // Use the default device.
		    "SysProg",          // Description of our stream.
		    &ss,                // Our sample format.
		    NULL,               // Use default channel map
		    NULL,               // Use default buffering attributes.
		    NULL                // Ignore error code.
		    );
  if (!s) {
    fprintf(stderr,"Error opening audio device\n");
    return NULL;
  }
  return s;
}


int aud_write(pa_simple *audio_device, const char *buffer, int size) {
  return pa_simple_write(audio_device,buffer,size,NULL);
}
