//
//  ShairportMgr.hpp
//  alsapipetest
//
//  Created by Vincent Moscaritolo on 10/13/22.
//

#pragma once

#include <mutex>
#include <utility>      // std::pair, std::make_pair
#include <string>       // std::string
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <utility>      // std::pair, std::make_pair
#include <fcntl.h>



#if defined(__APPLE__)
typedef struct _snd_mixer snd_mixer_t;
typedef struct _snd_mixer_elem snd_mixer_elem_t;
#else
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

#endif


using namespace std;

class ShairportMgr {
	
public:
	
	ShairportMgr();
	~ShairportMgr();
	
	bool begin(const char* path = "/tmp/shairport-sync-audio");
	bool begin(const char* path, int &error);
	
	void stop();
  	bool isConnected() ;

private:
	bool 				_isSetup = false;
	
	const char* 	_audioPath = NULL;

	
	bool  openAudioPipe( int &error);
	void  closeAudioPipe();
	
	int	 	_fd;		// audio pipe fd
	
	struct _snd_pcm *   	_pcm;

	void AudioReader();		// C++ version of thread
	// C wrappers for GPSReader;
	static void* AudioReaderThread(void *context);
	static void AudioReaderThreadCleanup(void *context);
	
	bool 			_isRunning = false;
	
	pthread_cond_t 		_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t 	_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_t				_TID;
	
};

