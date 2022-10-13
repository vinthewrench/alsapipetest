//
//  main.cpp
//  alsapipetest
//
//  Created by Vincent Moscaritolo on 10/13/22.
//

#include <iostream>
#include "ShairportMgr.hpp"
#include "CommonDefs.hpp"


int main(int argc, const char * argv[]) {
	
	
	ShairportMgr _sp;
	
	
	try {
		int error = 0;
	
		const char* path_audio =  "/tmp/shairport-sync-audio";
 
		if(!_sp.begin(path_audio, error))
			throw Exception("failed to setup Audio Reader.  error: %d", error);

		while(true){
			sleep(100);
		}
		
		_sp.stop();
		
	}
	
	catch ( const Exception& e)  {
		
		// display error on fail..
		
		printf("\tError %d %s\n\n", e.getErrorNumber(), e.what());
	}
	catch (std::invalid_argument& e)
	{
		// display error on fail..
		
		printf("EXCEPTION: %s ",e.what() );
	}

	return 0;
}
