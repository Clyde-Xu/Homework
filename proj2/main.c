#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOTAL_CARS	15
#define MAX_CARS	3
#define BLOCK		5
#define COLOR_YELLOW	"\x1B[33m"
#define COLOR_GREEN	"\x1B[32m"
#define COLOR_RESET	"\x1B[0m"

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t nor_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t han_cond = PTHREAD_COND_INITIALIZER;
static int onBridge = 0;
static int to_NORWICH = 0;
static int to_HANOVER = 0;

typedef enum
{
	TO_NORWICH,
	TO_HANOVER
}direction;


static void ArriveBridge(direction direct)
{
	int result = pthread_mutex_lock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_lock:%s\n", strerror(result));
		exit(1);
	}
	if(direct == TO_NORWICH){
		while(onBridge == MAX_CARS || onBridge < 0 || to_HANOVER - to_NORWICH > BLOCK){
			result = pthread_cond_wait(&nor_cond, &mtx);
			if(result != 0){
				fprintf(stderr, "pthread_cond_wait:%s\n", strerror(result));
				exit(1);
			}
		}
		onBridge++;
	}else{
		while(onBridge == -MAX_CARS || onBridge > 0 || to_NORWICH - to_HANOVER > BLOCK){
			result = pthread_cond_wait(&han_cond, &mtx);
			if(result != 0){
				fprintf(stderr, "pthread_cond_wait:%s\n", strerror(result));
				exit(1);
			}
		}
		onBridge--;
	}
	result = pthread_mutex_unlock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_unlock:%s\n", strerror(result));
		exit(1);
	}
}


static void OnBridge(direction direct)
{
	int result = pthread_mutex_lock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_lock:%s\n", strerror(result));
		exit(1);
	}
	if(direct == TO_NORWICH){
		printf(COLOR_YELLOW "%d" COLOR_RESET " cars on bridge towards " COLOR_YELLOW "NORWICH" COLOR_RESET "\n\n", onBridge);
	}else{
		printf(COLOR_GREEN "%d" COLOR_RESET " cars on bridge towards " COLOR_GREEN "HANOVER" COLOR_RESET "\n\n", -onBridge);
	}
	printf(COLOR_YELLOW "%d" COLOR_RESET " cars in total towards " COLOR_YELLOW "NORWICH" COLOR_RESET "\n", to_NORWICH);
	printf(COLOR_GREEN "%d" COLOR_RESET " cars in total towards " COLOR_GREEN "HANOVER" COLOR_RESET "\n", to_HANOVER);
	result = pthread_mutex_unlock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_unlock:%s\n", strerror(result));
		exit(1);
	}
	printf("==============================================\n");
	sleep(2);
}


static void ExitBridge(direction direct)
{
	int result = pthread_mutex_lock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_lock:%s\n", strerror(result));
		exit(1);
	}
	if(direct == TO_NORWICH){
		to_NORWICH--;
		onBridge--;
		if(onBridge == MAX_CARS - 1){
			result = pthread_cond_signal(&nor_cond);
			if(result != 0){
				fprintf(stderr, "pthread_cond_signal:%s\n", strerror(result));
				exit(1);
			}
		}
		else if(onBridge == 0){
			result = pthread_cond_broadcast(&han_cond);
			if(result != 0){
				fprintf(stderr, "pthread_cond_signal:%s\n", strerror(result));
				exit(1);
			}
		}
	}else{
		onBridge++;
		to_HANOVER--;
		if(onBridge == -(MAX_CARS - 1)){
			result = pthread_cond_signal(&han_cond);
			if(result != 0){
				fprintf(stderr, "pthread_cond_signal:%s\n", strerror(result));
				exit(1);
			}
		}
		else if(onBridge == 0){
			result = pthread_cond_broadcast(&nor_cond);
			if(result != 0){
				fprintf(stderr, "pthread_cond_signal:%s\n", strerror(result));
				exit(1);
			}
		}
	}
	result = pthread_mutex_unlock(&mtx);
	if(result != 0){
		fprintf(stderr, "pthread_mutex_unlock:%s\n", strerror(result));
		exit(1);
	}
	result = pthread_cond_signal(&cond);
	if(result != 0){
		fprintf(stderr, "pthread_cond_signal:%s\n", strerror(result));
		exit(1);
	}
}


static void *OneVehicle(void *cardirect)
{
	direction direct = (direction)cardirect;
	ArriveBridge(direct);
	OnBridge(direct);
	ExitBridge(direct);
	return NULL;
}


int main(void)
{
	int result;
	srand(time(NULL));
	pthread_t thread;
	for(;;){
		result = pthread_mutex_lock(&mtx);
		if(result != 0){
			fprintf(stderr, "pthread_mutex_lock:%s\n", strerror(result));
			exit(1);
		}
		while(to_NORWICH + to_HANOVER == TOTAL_CARS){
			result = pthread_cond_wait(&cond, &mtx);
			if(result != 0){
				fprintf(stderr, "pthread_cond_wait:%s\n", strerror(result));
				exit(1);
			}
		}
		while(to_NORWICH + to_HANOVER < TOTAL_CARS){
			direction direct;
			if(rand() % 2){
				direct = TO_NORWICH;
				to_NORWICH++;
				//printf("A car coming from NORWICH\n");
			}else{
				direct = TO_HANOVER;
				to_HANOVER++;
				//printf("A car coming from HANOVER\n");
			}
			result = pthread_create(&thread, NULL, OneVehicle, (void *)direct);
			if(result != 0){
				fprintf(stderr, "pthread_create:%s\n", strerror(result));
				exit(1);
			}
			result = pthread_detach(thread);
			if(result != 0){
				fprintf(stderr, "pthread_detach:%s\n", strerror(result));
				exit(1);
			}
		}
		result = pthread_mutex_unlock(&mtx);
		if(result != 0){
			fprintf(stderr, "pthread_mutex_unlock:%s\n", strerror(result));
			exit(1);
		}
	}
}
