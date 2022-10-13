//
//  ShairportMgr.cpp
//  alsapipetest
//
//  Created by Vincent Moscaritolo on 10/13/22.
//

#include "ShairportMgr.hpp"
#include <sys/ioctl.h>
#include "dbuf.hpp"

#define _PCM_  		"duplicate"


typedef void * (*THREADFUNCPTR)(void *);

ShairportMgr::ShairportMgr() {
 
	_isSetup = false;
  	_audioPath = NULL;
 	_fd = -1;
	_isRunning = true;

	pthread_create(&_outputProcessorTID, NULL,
								  (THREADFUNCPTR) &ShairportMgr::OutputProcessorThread, (void*)this);

 	pthread_create(&_audioReaderTID, NULL,
										  (THREADFUNCPTR) &ShairportMgr::AudioReaderThread, (void*)this);

	
}

ShairportMgr::~ShairportMgr(){
	stop();
	
	pthread_mutex_lock (&_mutex);
	_isRunning = false;
 	pthread_mutex_unlock (&_mutex);

	pthread_join(_audioReaderTID, NULL);
	pthread_join(_outputProcessorTID, NULL);

}



bool ShairportMgr::begin(const char* path){
	int error = 0;
	
	return begin(path, error);
}


bool ShairportMgr::begin(const char* audioPath,  int &error){

	
	if(isConnected())
		return true;
	
	_isSetup = false;
	
	if(_audioPath){
		free((void*) _audioPath); _audioPath = NULL;
	}

 
	
#if defined(__APPLE__)
#else

	int r;
	 
	r = snd_pcm_open(&_pcm, _PCM_,
								SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if( r < 0){
		error = r;
		
		printf("snd_pcm_open error %d, %s\n", errno, strerror(errno) );
		return false;
		
	}
	else {
		
		snd_pcm_nonblock(_pcm, 0);
		
		r = snd_pcm_set_params(_pcm,
									  SND_PCM_FORMAT_S16_LE,
									  SND_PCM_ACCESS_RW_INTERLEAVED,
									  2,
									  48000,
									  1,               // allow soft resampling
									  500000);         // latency in us

		
		if( r < 0){
			error = r;
			printf("snd_pcm_set_params error %d, %s\n", errno, strerror(errno) );
			return false;
			
		}
		
		
	}
#endif
	
	pthread_mutex_lock (&_mutex);
	_audioPath = strdup(audioPath);
	pthread_mutex_unlock (&_mutex);
 
//	int ignoreError;
//	openAudioPipe(ignoreError);

	_isSetup = true;

	return _isSetup;
}


void ShairportMgr::stop(){
	
	if(_isSetup) {
		closeAudioPipe();
  		_isSetup = false;
		
#if defined(__APPLE__)
#else

	 // Close device.
	 if (_pcm != NULL) {
		  snd_pcm_close(_pcm);
		 _pcm = NULL;
	 }
		
	 
#endif

		
		}
}


bool  ShairportMgr::isConnected() {
	bool val = false;
	
	pthread_mutex_lock (&_mutex);
	val = _fd != -1;
	pthread_mutex_unlock (&_mutex);
 
	return val;
};

bool  ShairportMgr::openAudioPipe( int &error){
	
	if(!_audioPath) {
		error = EINVAL;
		return false;
	}
	
	printf("openAudioPipe  %s\n", _audioPath);

	int fd ;
	
	if((fd = ::open( _audioPath, O_RDONLY  )) <0) {
		printf("Error %d, %s\n", errno, strerror(errno) );
		//	ELOG_ERROR(ErrorMgr::FAC_GPS, 0, errno, "OPEN %s", _ttyPath);
			error = errno;
			return false;
		}

		printf("Opened  %s\n", _audioPath);

		
		pthread_mutex_lock (&_mutex);
		_fd = fd;
 		pthread_mutex_unlock (&_mutex);
  
	return true;
}


void ShairportMgr::closeAudioPipe(){
	if(isConnected()){
		
		pthread_mutex_lock (&_mutex);

		close(_fd);
  		_fd = -1;
		pthread_mutex_unlock (&_mutex);
	}
}

// MARK: -  GPSReader thread

static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}

void ShairportMgr::AudioReader(){
	
 
	int lastError = 0;
	
  SampleVector samples;

	printf("AudioReader start \n");

	while(_isRunning){
		
		// if not setup // check back later
		if(!_isSetup){
			sleep(2);
			continue;
		}
		
		// is the port setup yet?
		if (! isConnected()){
			if(!openAudioPipe(lastError)){
				sleep(5);
				continue;
			}
			
			_output_buffer.flush();
		}
		
		int nbytes = 0;
		if( ioctl(_fd, FIONREAD, &nbytes) < 0){
			printf("ioctl FIONREAD  %s \n",strerror(errno));
			continue;
		}
		
		if(nbytes > 0){
		
//			printf("%d bytes avail \n",nbytes);

			int framesize = nbytes / 4;
			
			if (framesize > default_blockLength)
				framesize = default_blockLength;
			
			samples.resize(framesize);
	 
			if( safe_read(_fd, samples.data(), framesize*4)  != framesize*4){
				printf("read fail  %s \n",strerror(errno));
				continue;
			}
		 
	
			_output_buffer.push(move(samples));
	 				// process nbytes
			
		}
		else {
			// wait for bytes/
			usleep(10000);
 		}
	}
	
}
 

void* ShairportMgr::AudioReaderThread(void *context){
	ShairportMgr* d = (ShairportMgr*)context;

	//   the pthread_cleanup_push needs to be balanced with pthread_cleanup_pop
	pthread_cleanup_push(   &ShairportMgr::AudioReaderThreadCleanup ,context);
 
	d->AudioReader();
	
	pthread_exit(NULL);
	
	pthread_cleanup_pop(0);
	return((void *)1);
}

 
void ShairportMgr::AudioReaderThreadCleanup(void *context){
	//AudioReader* d = (AudioReader*)context;
 
	printf("cleanup AudioReader\n");
}
 


void ShairportMgr::OutputProcessor(){
	
	
	while(_isRunning){
		
		if(!_isSetup){
			usleep(200000);
			continue;
		}
		
		if (_output_buffer.queued_samples() == 0) {
			// The buffer is empty. Perhaps the output stream is consuming
			// samples faster than we can produce them. Wait until the buffer
			// is back at its nominal level to make sure this does not happen
			// too often.
			
			
			// revisit this..  the 2 is for stereo..
			
			_output_buffer.wait_buffer_fill(44100 * 2);
			
			
		}
		// Get samples from buffer and write to output.
		SampleVector samples =_output_buffer.pull();
		
// 	 printf("Output %ld samples\n", samples.size());

#if defined(__APPLE__)
#else
		// Write data.
		
		auto r = snd_pcm_writei(_pcm, samples.data(),  samples.size());
		if(r < 0) {
			printf("snd_pcm_writei Error %d, %s\n", r, strerror(r) );

		}
#endif

 
	}
}
  
 



void* ShairportMgr::OutputProcessorThread(void *context){
	ShairportMgr* d = (ShairportMgr*)context;

	//   the pthread_cleanup_push needs to be balanced with pthread_cleanup_pop
	pthread_cleanup_push(   &ShairportMgr::OutputProcessorThreadCleanup ,context);
 
	d->OutputProcessor();
	
	pthread_exit(NULL);
	
	pthread_cleanup_pop(0);
	return((void *)1);
}

 
void ShairportMgr::OutputProcessorThreadCleanup(void *context){
	//RadioMgr* d = (RadioMgr*)context;

 
//	printf("cleanup sdr\n");
}
