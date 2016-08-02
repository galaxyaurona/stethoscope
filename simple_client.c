 /*@file simple_client.c
  *
  * @brief This is very simple client that demonstrates the basic
  * features of JACK as they would be used by many applications.
  */
 
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <sndfile.h>
#include <signal.h>

#define NUM_INPUT_PORTS 1 
#define RING_BUFFER_SIZE 16384           /* ringbuffer size in frames */
jack_ringbuffer_t *rb;
jack_port_t *input_port;
jack_port_t *output_port_l;
jack_port_t *output_port_r;

jack_default_audio_sample_t **inputBufferArray;		// Array of pointers to input buffers
jack_ringbuffer_t *ringBuffer;

int running;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;   // Mutex to control ring buffer access
pthread_cond_t dataReady = PTHREAD_COND_INITIALIZER;      // Condition to signal data is available in ring buffer
 /**
  * The process callback for this JACK application.
  * It is called by JACK at the appropriate times.
  */

typedef struct _thread_info {
    pthread_t thread_id;
    SNDFILE *sf;
    jack_nframes_t duration;
    jack_nframes_t rb_size;
    jack_client_t *client;
    unsigned int channels;
    int bitdepth;
    char *path;
    volatile int can_capture;
    volatile int can_process;
    volatile int status;
} thread_info_t;

int
process (jack_nframes_t nframes, void *arg)
{
        jack_default_audio_sample_t *out_l = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_l, nframes);
        jack_default_audio_sample_t *out_r = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_r, nframes);
        jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port, nframes);
		
        memcpy (out_l, in, sizeof (jack_default_audio_sample_t) * nframes);
        memcpy (out_r, in, sizeof (jack_default_audio_sample_t) * nframes);
        
        int portNum;
		jack_nframes_t frameNum;


        // Iterate through input buffers, adding samples to the ring buffer
        for (frameNum=0; frameNum<nframes; frameNum++)
        {
        	
        	size_t written = jack_ringbuffer_write(ringBuffer, (void *) &in[frameNum], sizeof(jack_default_audio_sample_t));
        	if (written != sizeof(jack_default_audio_sample_t))
        	{
        		printf("Ringbuffer overrun\n");
        	}
        	
        }

        // Attempt to lock the threadLock mutex, returns zero if lock acquired
        if (pthread_mutex_trylock(&threadLock) == 0)
        {
        	// Wake up thread which is waiting for condition (should only be called after lock acquired)
        	pthread_cond_signal(&dataReady);

        	// Unlock mutex
        	pthread_mutex_unlock(&threadLock);
        }

        return 0;      
}
 
 /**
  * This is the shutdown callback for this JACK application.
  * It is called by JACK if the server ever shuts down or
  * decides to disconnect the client.
  */
void
jack_shutdown (void *arg)
{

        exit (1);
}

 void signalHandler(int sig)
{
  printf("Stopping\n");
  running = 0;
  pthread_cond_signal(&dataReady);

  // Unlock mutex
  pthread_mutex_unlock(&threadLock);
}

 int
 main (int argc, char *argv[])
 {
         jack_client_t *client;
         const char **ports;
 
         ringBuffer = jack_ringbuffer_create(1 * sizeof(jack_default_audio_sample_t) * RING_BUFFER_SIZE);
 
         /* try to become a client of the JACK server */
 
         if ((client = jack_client_new ("stethoscope")) == 0) {
                 fprintf (stderr, "jack server not running?\n");
                 return 1;
         }
 
         /* tell the JACK server to call `process()' whenever
            there is work to be done.
         */
 
         jack_set_process_callback (client, process, 0);
 
         /* tell the JACK server to call `jack_shutdown()' if
            it ever shuts down, either entirely, or if it
            just decides to stop calling us.
         */
 
         jack_on_shutdown (client, jack_shutdown, 0);
 
         /* display the current sample rate. 
          */
 
         printf ("engine sample rate: %" PRIu32 "\n",
                 jack_get_sample_rate (client));
 
         /* create two ports */
 
         input_port = jack_port_register (client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
         output_port_l = jack_port_register (client, "output_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
         output_port_r = jack_port_register (client, "output_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
 		
         /* tell the JACK server that we are ready to roll */
 
         if (jack_activate (client)) {
                 fprintf (stderr, "cannot activate client");
                 return 1;
         }
 
         /* connect the ports. Note: you can't do this before
            the client is activated, because we can't allow
            connections to be made to clients that aren't
            running.
         */
 
         if ((ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
                 fprintf(stderr, "Cannot find any physical capture ports\n");
                 exit(1);
         }
 
         if (jack_connect (client, ports[0], jack_port_name (input_port))) {
                 fprintf (stderr, "cannot connect input ports\n");
         }
 
         free (ports);
         
         if ((ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical & JackPortIsInput)) == NULL) {
                 fprintf(stderr, "Cannot find any physical playback ports\n");
                 exit(1);
         }
 		printf("num ports: %d\n",sizeof(ports)/sizeof(char));
        if (jack_connect (client, jack_port_name (output_port_l), ports[2]) 
        	 ) {
                fprintf (stderr, "cannot connect output ports l\n");
        }
        if (jack_connect (client, jack_port_name (output_port_r), ports[1]) 
        	 ) {
                fprintf (stderr, "cannot connect output port r\n");
        }
 
        free (ports);
 		pthread_mutex_lock(&threadLock);

 		SF_INFO info;
      	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
      	info.channels = 1;
      	info.samplerate = 44100;
      	SNDFILE *sndFile = sf_open("capture2.wav", SFM_WRITE, &info);

      	if (sndFile == NULL) {
        	fprintf(stderr, "Error writing wav file: %s\n", sf_strerror(sndFile));
        	exit(1);
      	}
      	size_t bytesPerFrame = sizeof(jack_default_audio_sample_t) * NUM_INPUT_PORTS;
      	jack_default_audio_sample_t *frameBuffer = (jack_default_audio_sample_t*) malloc(bytesPerFrame);
      	running = 1;
         /* Since this is just a toy, run for a few seconds, then finish */
 		signal(SIGINT, signalHandler);

 		do {
 			// Block this thread until dataReady condition is signalled
 			// The mutex is automatically unlocked whilst waiting
 			// After signal received, mutex is automatically locked
 			pthread_cond_wait(&dataReady, &threadLock);

 			// Check if there is enough bytes for a whole frame
 			while (jack_ringbuffer_read_space(ringBuffer) >= bytesPerFrame)
 			{
 				// Read a single frame of data
 				jack_ringbuffer_read(ringBuffer, (void *)frameBuffer, bytesPerFrame);

 				// Write data to file
 			   sf_writef_float(sndFile, (float*)frameBuffer, 1);
 			}
 		} while (running);

        printf("Closing\n");
      	jack_client_close(client);
      	sf_write_sync(sndFile);
      	sf_close(sndFile);
        exit (0);
 }
 
 