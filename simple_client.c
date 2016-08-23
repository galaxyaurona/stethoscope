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




typedef struct _thread_info {
    pthread_t thread_id;
    SNDFILE *sf; // open in set up disk thread
    FILE * lockfile;
    char *lock_filename;
    jack_nframes_t duration; // duration in seconds
    jack_nframes_t rb_size;
    jack_client_t *client;
    unsigned int channels;
    int bitdepth;
    char *path;
    volatile int can_capture;
    volatile int can_process;
    volatile int status;
} jack_thread_info_t;

/* JACK data */
#define NUM_INPUT_PORTS 1 
jack_port_t *input_port;
jack_port_t *output_port_l;
jack_port_t *output_port_r;
const size_t sample_size = sizeof(jack_default_audio_sample_t);
static char lock_filename[30];
jack_default_audio_sample_t **inputBufferArray;   // Array of pointers to input buffers
jack_ringbuffer_t *ringBuffer;

int running;
#define RING_BUFFER_SIZE 16384   /* ringbuffer size in frames */
jack_ringbuffer_t *rb;
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;   // Mutex to control ring buffer access
pthread_cond_t dataReady = PTHREAD_COND_INITIALIZER;      // Condition to signal data is available in ring buffer
 long overruns = 0;
 /**
  * The process callback for this JACK application.
  * It is called by JACK at the appropriate times.
  */



// pipe input to output
int
process (jack_nframes_t nframes, void *arg)
{   
        jack_thread_info_t *info = (jack_thread_info_t *) arg; // passing thread info as argument
        // prevent auto start, let the disk_thread settle in
        if ((!info->can_process) || (!info->can_capture))
            return 0;
        
        // declare the data from input and output and get the memory allocated in a form of buffer
        jack_default_audio_sample_t *out_l = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_l, nframes);
        jack_default_audio_sample_t *out_r = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_r, nframes);
        jack_default_audio_sample_t *in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port, nframes);
    // pipe the data from input buffer to output buffer
        memcpy (out_l, in, sizeof (jack_default_audio_sample_t) * nframes);
        memcpy (out_r, in, sizeof (jack_default_audio_sample_t) * nframes);
        
        int portNum;
    jack_nframes_t frameNum;


        // Iterate through input buffers, adding samples to the ring buffer to be written to the file
        for (frameNum=0; frameNum<nframes; frameNum++)
        {
          
          size_t written = jack_ringbuffer_write(rb, (void *) &in[frameNum], sizeof(jack_default_audio_sample_t));
          if (written != sample_size) // if the amount of bytes written is larger than 
          {
            printf("Ringbuffer overrun\n");
                overruns++; // probably uneccessary , but just to signal the disk thread
          }
          
        }

        // Attempt to lock the threadLock mutex, returns zero if lock acquired, tell the disk 
        if (pthread_mutex_trylock(&threadLock) == 0) // if the disk thread already running, lock not available, 
        {   //don't wait, because disk thread will read all the data in the queue
          // Wake up thread which is waiting for condition (should only be called after lock acquired)
          pthread_cond_signal(&dataReady); // calling disk_thread, telling it there is work to do

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
  printf("Stopping in signal signalHandler\n");
  running = 0;
  pthread_cond_signal(&dataReady);

  // Unlock mutex
  pthread_mutex_unlock(&threadLock);
}

void *
disk_thread (void *arg)
{
    unsigned currentTime = (unsigned) time(NULL);
    jack_thread_info_t *info = (jack_thread_info_t *) arg;
    static jack_nframes_t total_captured = 0;
    jack_nframes_t samples_per_frame = info->channels;
    size_t bytes_per_frame = samples_per_frame * sample_size;
    void *framebuf = malloc (bytes_per_frame);

    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_mutex_lock (&threadLock);

    info->status = 0;

    while (1) {

        /* Write the data one frame at a time.  This is
         * inefficient, but makes things simpler. */
        while (info->can_capture &&
               (jack_ringbuffer_read_space (rb) >= bytes_per_frame)) {

            jack_ringbuffer_read (rb, framebuf, bytes_per_frame);

            if (sf_writef_float (info->sf, framebuf, 1) != 1) {
                char errstr[256];
                sf_error_str (0, errstr, sizeof (errstr) - 1);
                fprintf (stderr,
                     "cannot write sndfile (%s)\n",
                     errstr);
                info->status = EIO; /* write failed */
                goto done;
            }
                
            if (++total_captured >= info->duration) { // duration here is in frame, equal to 10 second * sample rate
                printf ("disk thread finished\n");
                goto done;
            }
        }

        /* wait until process() signals more data */
        pthread_cond_wait (&dataReady, &threadLock);
    }

 done:
    pthread_mutex_unlock (&threadLock);
    free (framebuf);
    printf(" removing %s\n", info->lock_filename);
    fclose(info->lockfile);
    // remove lockfile
    remove(lock_filename);
    return 0;
}

void
setup_disk_thread (jack_thread_info_t *info)
{
    SF_INFO sf_info;
    int short_mask;
    // decleare filename here
    unsigned currentTime = (unsigned) time(NULL);
    char sound_filename[30];
    sprintf(sound_filename, "records/capture_%d.wav", currentTime  );
         

    sprintf(lock_filename, "records/capture_%d.lock", currentTime  );
        
    
    info->path = sound_filename;
    info->lock_filename = lock_filename;
    sf_info.samplerate = jack_get_sample_rate (info->client); // dynamically get sample rate
    sf_info.channels = info->channels;
    short_mask = SF_FORMAT_PCM_16;
    sf_info.format = SF_FORMAT_WAV|short_mask;

    if ((info->sf = sf_open (info->path, SFM_WRITE, &sf_info)) == NULL) {
        char errstr[256];
        sf_error_str (0, errstr, sizeof (errstr) - 1);
        fprintf (stderr, "cannot open sndfile \"%s\" for output (%s)\n", info->path, errstr);
        jack_client_close (info->client);
        exit (1);
    }

    if (info->duration == 0) {
        info->duration = JACK_MAX_FRAMES; // JACK max frame is a constant defined in library
    } else {
        info->duration *= sf_info.samplerate;
    }

    info->can_capture = 0;
    // create a lockfile here ,open lockfile to write, just to lock it, or create it
    info->lockfile = fopen(info->lock_filename,"wb" );

    pthread_create (&info->thread_id, NULL, disk_thread, info);
}




// to run the thread info
void
run_disk_thread (jack_thread_info_t *info)
{
    info->can_capture = 1;
    pthread_join (info->thread_id, NULL);
    sf_close (info->sf);
  

    if (overruns > 0) {
        fprintf (stderr,
             "jackrec failed with %ld overruns.\n", overruns);
        fprintf (stderr, " try a bigger buffer than -B %"
             PRIu32 ".\n", info->rb_size);
        info->status = EPIPE;
    }
}

void
setup_ports (jack_thread_info_t *info) // 1 channcel, only need other
{

    /* Allocate data structures that depend on the number of ports. */
    jack_port_t **ports;

    rb = jack_ringbuffer_create (1 * sample_size * info->rb_size);

    /* When JACK is running realtime, jack_activate() will have
     * called mlockall() to lock our pages into memory.  But, we
     * still need to touch any newly allocated pages before
     * process() starts using them.  Otherwise, a page fault could
     * create a delay that would force JACK to shut us down. */

    memset(rb->buf, 0, rb->size);

    input_port = jack_port_register (info->client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    output_port_l = jack_port_register (info->client, "output_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    output_port_r = jack_port_register (info->client, "output_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    // connect input ports to output ports
     if ((ports = jack_get_ports (info->client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
             fprintf(stderr, "Cannot find any physical capture ports\n");
             exit(1);
     }
    
     if (jack_connect (info->client, ports[0], jack_port_name (input_port))) {
             fprintf (stderr, "cannot connect input ports\n");
     }
    
     free (ports);
     
     if ((ports = jack_get_ports (info->client, NULL, NULL, JackPortIsPhysical & JackPortIsInput)) == NULL) {
             fprintf(stderr, "Cannot find any physical playback ports\n");
             exit(1);
     }
    
    if (jack_connect (info->client, jack_port_name (output_port_l), ports[2]) 
         ) {
            fprintf (stderr, "cannot connect output ports l\n");
    }
    if (jack_connect (info->client, jack_port_name (output_port_r), ports[1]) 
         ) {
            fprintf (stderr, "cannot connect output port r\n");
    }
     
    free (ports);

    info->can_process = 1;      /* process() can start, now */
}


 int
 main (int argc, char *argv[])
 {       


         jack_client_t *client;
    
 
         ringBuffer = jack_ringbuffer_create(1 * sizeof(jack_default_audio_sample_t) * RING_BUFFER_SIZE);
 
         /* try to become a client of the JACK server */
 
         if ((client = jack_client_new ("stethoscope")) == 0) { // with the name stethoscope, if the result is equal to 0, it mean that the server cannot be create
                 fprintf (stderr, "jack server not running?\n");
                 return 1;
         }
         // create thread info
        jack_thread_info_t thread_info;
         /* tell the JACK server to call `process()' whenever
            there is work to be done.
         */

        // initialize disk info
        memset (&thread_info, 0, sizeof (thread_info)); // initialize disk info     
        thread_info.duration = 10; // 10 seconds duration for the record
        thread_info.rb_size = RING_BUFFER_SIZE;
        thread_info.bitdepth = 16; // 16 bit default
        thread_info.client = client; // assign the client to the opened client
        thread_info.can_process = 0;
        thread_info.channels = 1; // mono input
        setup_disk_thread (&thread_info);
        // set process callback to our process function, pass thread info along
         jack_set_process_callback (client, process, &thread_info);
 
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
        setup_ports(&thread_info);
        run_disk_thread (&thread_info);

         /* Since this is just a toy, run for a few seconds, then finish */
    signal(SIGINT, signalHandler);

	/*while (2>1) {
		digitalWrite(2,thread_info.can_capture);
	}*/

    printf("Closing\n");
    jack_client_close(client);

    jack_ringbuffer_free (rb);
    exit (0);
 }
 
 