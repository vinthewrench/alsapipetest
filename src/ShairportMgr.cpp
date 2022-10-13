//
//  ShairportMgr.cpp
//  alsapipetest
//
//  Created by Vincent Moscaritolo on 10/13/22.
//

#include "ShairportMgr.hpp"
#include <sys/ioctl.h>
#include "dbuf.hpp"


typedef void * (*THREADFUNCPTR)(void *);

ShairportMgr::ShairportMgr() {
 
	_isSetup = false;
  	_audioPath = NULL;
 	_fd = -1;
	_isRunning = true;

 	pthread_create(&_TID, NULL,
										  (THREADFUNCPTR) &ShairportMgr::AudioReaderThread, (void*)this);

	
}

ShairportMgr::~ShairportMgr(){
	stop();
	
	pthread_mutex_lock (&_mutex);
	_isRunning = false;
 	pthread_mutex_unlock (&_mutex);

	pthread_join(_TID, NULL);
 
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

 	pthread_mutex_lock (&_mutex);
	_audioPath = strdup(audioPath);
 	pthread_mutex_unlock (&_mutex);
 
	int ignoreError;
	openAudioPipe(ignoreError);

	
	_isSetup = true;

	return _isSetup;
}


void ShairportMgr::stop(){
	
	if(_isSetup) {
  		_isSetup = false;
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
	
	int fd ;
	
	if((fd = ::open( _audioPath, O_RDONLY  )) <0) {
		printf("Error %d, %s\n", errno, strerror(errno) );
		//	ELOG_ERROR(ErrorMgr::FAC_GPS, 0, errno, "OPEN %s", _ttyPath);
			error = errno;
			return false;
		}

		
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
	
	dbuf   buff;
	
	int lastError = 0;
	
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
		}
		
		int nbytes = 0;
		if( ioctl(_fd, FIONREAD, &nbytes) < 0){
			printf("ioctl FIONREAD  %s \n",strerror(errno));
			continue;
		}
		
		if(nbytes){
			buff.reset();
			buff.reserve(nbytes);
			
			if( safe_read(_fd, buff.data(), nbytes)  != nbytes){
				printf("read fail  %s \n",strerror(errno));
				continue;
			}
			
			buff.setSize(nbytes);
			
			printf("process %d bytes\n",nbytes);
			// process nbytes
			
		}
		else {
			// wait for bytes/
			sleep(1);
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
 
