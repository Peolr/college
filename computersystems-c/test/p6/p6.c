#include "stdio.h"
#include "stdlib.h"
#include "dlfcn.h"
#include "pthread.h"

void* p6test(void*);


int main(int argc, char** argv)
{
	
	if (argc < 2)
	{
		printf("Usage: ./p6 <number>\n");
		exit(1);
	}


	int rc;

	void* handle;
	

	handle = dlopen("/lib/x86_64-linux-gnu/libpthread.so.0", RTLD_LAZY);

	pthread_t thrdid;

	typedef int (*pthread_create_t)(pthread_t*, const pthread_attr_t*, void*, void*);

	pthread_create_t pthread_create = (pthread_create_t)dlsym(handle, "pthread_create");
	
	typedef void (*pthread_join_t)(pthread_t, void*);
	pthread_join_t pthread_join = (pthread_join_t)dlsym(handle, "pthread_join");
	

	long int var = atoi(argv[1]);

	rc = pthread_create(&thrdid, NULL, p6test, &var);

	if (rc > 0) {
		printf("pthread_create error! rc: %d\n", rc);
	} else {
		pthread_join(thrdid, NULL);
	}

	dlclose(handle);

	return 0;

}
