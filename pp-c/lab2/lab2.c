/*
 *Author: Hunter Esler
 *Course: CSCI 4330
 *Lab number: Lab 2
 *Purpose: This lab will run the player and dragon "game" simulation described in the lab2.html
 *Due date: February 8, 2018
 * */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//player object with id and type of player
typedef struct {
	int id;
	int type;
	int alive;//player alive?
} player;

//custom queue for player
typedef struct {
	int size;//size of list
	int length;//length of list (# of elements)
	int a;//a's in list
	int h;//h's in list
	int alive_a;
	int alive_h;
	player* last;//not used
	player** list;//array of pointers of player
} player_queue;

//Translation for player type 0, 1, and 2 respectively
const char type_trans[3] = {'A', 'H', 'D'};
int cur_dragon = 0;//current dragon position
int epochs = 5;//number of epochs to do
const int lobbyQueueSize = 32;
player_queue* lobby;
//Number of continents
const int MAX = 4;
//Continent queues
const int continentQueueSize = 6;//size of queue..... needs 6 for dragon
player_queue* continents[4];//continent array
int thread_done;//done threads with current task;
pthread_mutex_t cm;//child mutex
pthread_mutex_t mm;//main mutex
pthread_cond_t cv;//child condition
pthread_cond_t cmv;//main condition

//Allocates space for a player and returns created player of id and type
player* newPlayer(int id, int type) {
	player* p = (player*)malloc(sizeof(player));//1
	p->id = id;
	p->type = type;
	p->alive = 1;
	return p;
}

//Allocates space for a new queue and returns created queue
player_queue* newQueue(int size) {
	player_queue* q =(player_queue*) malloc(sizeof(player_queue));//2
	q->list = calloc(size, sizeof(player*));//3
	q->size = size;
	q->length = 0;
	q->a = 0;
	q->h = 0;
	q->alive_a = 0;
	q->alive_h = 0;
	for (int i = 0; i < size; i++) {
		q->list[i] = NULL;
	}
	return q;
}

//removes player from queue and returns player (selective pop)
player* removePlayer(player_queue* q, int id) {
	
	player* p;

	//no data in queue
	if (q->length < 1) {
		return NULL;
	}

	int f = 0;//found id
	
	//shift eveybody to the left after finding culprit (player with id)
	for (int i = 0; i < q->length; i++) {
		if (q->list[i]->id == id) {
			p = q->list[i];
			f++;
		}
		if (f > 0) {
			if (i + 1 < q->size) {
				if (!(q->list[i+1]!=NULL && q->list[i+1]->type==2))
					q->list[i] = q->list[i+1];
			} else {
				//at last item.
				q->list[i] = NULL;
			}
		}
	}

	//nothing found
	if (f < 1)
		return NULL;
	//make sure to update length
	q->length--;

	//update alive and number
	if (p->type==0) {
		q->a--;
		if (q->alive_a > 0)
			q->alive_a--;
	} else if (p->type==1) {
		q->h--;
		if (q->alive_h > 0)
			q->alive_h--;
	}

	return p;
}

//pops first in first out and returns player pointer.. uses removePlayer for no repeated code
player* popQueue(player_queue* q) {
	if (q->length < 1)
		return NULL;

	return removePlayer(q, q->list[0]->id);
}


//Inserts a player pointer into the queue, assumes you already checked queue length with canInsert
void insertQueue(player_queue* q, player* p) {
	if (p->type == 0) {
		q->a++;
		//assume alive now
		q->alive_a++;
		
	} else if (p->type==1) {
		q->h++;
		q->alive_h++;
	}
	p->alive=1;
	q->list[q->length] = p;
	q->length++;
}

//sends dead to lobby.. LOCK mutex first
void cleanDead(player_queue* q) {
	int numdead = q->alive_a + q->alive_h;
	int* dead = calloc(numdead,sizeof(int));//4
	int c = 0;
	int i;
	for (i = 0; i < q->length; i++) {
		if (q->list[i]->alive < 1) {
			dead[c]=q->list[i]->id;
			c++;
		}
	}
	for (i = 0; i < c; i++) {
		insertQueue(lobby, removePlayer(q, dead[i]));
	}
	free(dead);//4
}

//is there room in the queue? 1 for true 0 for false
int canInsert(player_queue* q) {
	return q->length < q->size ? 1 : 0;
}

//try to kill player of type in queue
void killPlayer(player_queue* q, int type) {
	//checking if player is part of queue then updating alive value
	for (int i = 0; i < q->length; i++) {
		if (q->list[i]->type==type && q->list[i]->alive > 0) {
			player* p = q->list[i];
			if (p->type==0) {
				q->alive_a--;
			} else if (p->type==1){
				q->alive_h--;
			}
			p->alive = 0;
			return;//finished
		}
	}
}

//freeing queue memory by clearing players and then the list and then queue itself
void freeQueue(player_queue* q) {
	int i;
	for (i = 0; i < q->length; i++) {
		free(q->list[i]);//1
	}
	free(q->list);//3
	free(q);//2
}

int createThreads(pthread_t continent[], int num); //creates the threads in a separate function
void printWorld();//prints current status of world.
//What each child will run
void* child_thread(void* me) {
	int id = *((int*)me);
	int i, a, h, d;
	player_queue* q = continents[id]; 

	player* temp[2];//holding 2 live players

	//struct continenent_queue cq;
	while (epochs > 0) {
		
		a = 0;
		h = 0;
		d = (q->list[5] != NULL) ? 1 : 0;
		//if (cur_dragon == id) {
		//	d = 1;
		//}
		
		a = q->a;
		h = q->h;

		temp[0] = NULL;
		temp[1] = NULL;

		//killing logic
		if (a > h) {
			int t = a / 2;
			if (t > h)
				t = h;
			for (i = 0; i < t; i++) {
				killPlayer(q, 1);//killing h player
			}
		}
		if (h > a) {
			int t = h / 2;
			if (t > a)
				t = a;
			for (i = 0; i < t; i++) {
				killPlayer(q, 0);//killing a player
			}
		}

		//dragon killing
		if (d > 0) {
			if (q->alive_a > 0) {
				killPlayer(q, 0);
			} else {
				killPlayer(q, 1);
			}
		}

		//add dead players to lobby
		pthread_mutex_lock(&cm);
			cleanDead(q);
		pthread_mutex_unlock(&cm);
		//grab first 2 alive and put them in temp
		if (q->length > 0) {
			temp[0] = q->list[0];
		}
		if (q->length > 1) {
			temp[1] = q->list[1];
		}

		//printing world after this
		pthread_mutex_lock(&cm);
			thread_done++;
			if (thread_done >= 4) {
				pthread_cond_signal(&cmv);
			}
			pthread_cond_wait(&cv, &cm);
		pthread_mutex_unlock(&cm);
		
		//pop our living(at least 2)
		for (i = 0; i < 2; i++) {
			popQueue(q);
		}

		//ready for next phase
		pthread_mutex_lock(&cm);
			thread_done++;
			if (thread_done>=4) {
				pthread_cond_signal(&cmv);
			}
			pthread_cond_wait(&cv, &cm);
		pthread_mutex_unlock(&cm);

		//find out what is next continent
		int id2 = id + 1;
		if (id2 > 3)
			id2 = 0;
		for (i = 0; i < 2; i++) {
			if (temp[i] != NULL)
				insertQueue(continents[id2], temp[i]);
		}


		//ready for next phase
		pthread_mutex_lock(&cm);
			thread_done++;
			if (thread_done>=4) {
				pthread_cond_signal(&cmv);
			}
			pthread_cond_wait(&cv, &cm);
		
			//grabbing from lobby
			while (q->length < 5) {
				insertQueue(q, popQueue(lobby));
			}

			//"transporting" dragon
			if (d > 0) {
				printf("Transferring from thread %d\n", id);
				id2 = id - 1;
				if (id2 < 0)
					id2 = 3;
				continents[id2]->list[5] = q->list[5];
				q->list[5] = NULL;
			}

			//getting ready for next cycle
		
			thread_done++;
			if (thread_done>=4) {
				pthread_cond_signal(&cmv);
			}
			pthread_cond_wait(&cv, &cm);
			
		pthread_mutex_unlock(&cm);


	}
}

int ids[4];//ids of slaves

int main()
{
	//threads
	pthread_t continent[MAX];
	//thread ids
	int nums[MAX];
	//counter variables
	int i = 0;
	int c = 0;
	int x = 0;
	//done lobbies filled
	int done_fill = 0;
	//number of threads user asks for
	int num_threads = MAX + 1;
	player* dragon = newPlayer(-1, 2);

	pthread_mutex_init(&cm, NULL);
	pthread_mutex_init(&mm, NULL);
	pthread_cond_init(&cv, NULL);
	pthread_cond_init(&cmv, NULL);

	//LOAD players.dat
	FILE *players;
	players = fopen("players.dat", "r");
	if (players == NULL) {
		fprintf(stderr, "Can't open players.dat\n");
		exit(1);
	}

	//initialize ids
	for (i = 0; i < 4; i++) {
		ids[i] = i;
	}

	//initialize queues
	lobby = newQueue(lobbyQueueSize);
	for (i = 0; i < 4; i++) {
		continents[i] = newQueue(continentQueueSize);
	}

	i = 0;
	while ((c = fgetc(players)) != EOF)
	{
		char ch = (char) c;
		for (x = 0; x < 3; x++) {
			if (type_trans[x] == ch) {
				if (x != 2)
				{
					insertQueue(lobby, newPlayer(i, x));
					break;
				}
			}
		}
		//dragon
		if (c >= 48 && c <=51) {
			continents[c-48]->list[5] = dragon;//adding dragon to queue....
		}
			
		i++;
		
	}
	
	//LOAD Players into continents
	while (lobby->length >= 4 && done_fill < 4) {//5 available
		for (i = 0; i < 5; i++)
		{
			player* p = popQueue(lobby);
			insertQueue(continents[done_fill], p);
		//	printf("%d %d\n",done_fill, p.id);
		}
		done_fill++;
	}

	printf("Initial setup:\n");
	printWorld();
	thread_done = 0;
	int created_threads = createThreads(continent, MAX);

	while (epochs>0) {
		pthread_mutex_lock(&mm);
			pthread_cond_wait(&cmv, &mm);
			epochs--;
			thread_done = 0;
			pthread_cond_broadcast(&cv);
			//waiting to pop temp players
			pthread_cond_wait(&cmv, &mm);
			thread_done = 0;
			pthread_cond_broadcast(&cv);
			//waiting to shift players
			pthread_cond_wait(&cmv, &mm);
			thread_done = 0;
			pthread_cond_broadcast(&cv);
			//waiting to add players from lobby
			pthread_cond_wait(&cmv, &mm);
			thread_done = 0;
			
			printf("\n\nThe world after Epoch %d\n\n", 5 - epochs);
			printWorld();
			pthread_cond_broadcast(&cv);
		pthread_mutex_unlock(&mm);
	}
	
	printf("\n\nFinal world:\n\n");
	printWorld();
	pthread_join(continent[0], NULL);
	pthread_join(continent[1], NULL);
	pthread_join(continent[2], NULL);
	pthread_join(continent[3], NULL);

	//freeing allocations
	//queues
	freeQueue(lobby);
	for (i = 0; i < 4; i++) {
		freeQueue(continents[i]);
	}
	free(dragon);
	
	pthread_cond_destroy(&cmv);
	pthread_cond_destroy(&cv);
	pthread_mutex_destroy(&mm);
	pthread_mutex_destroy(&cm);
	return 0;
}

void printWorld() {
	int size;//size of queue for continent player
	int i;
	int x;
	printf("%10.10s %10.10s %10.10s\n","Player","Type","Continent");
	for (i = 0; i < 4; i++) {
		size = continents[i]->length;
		for (x = 0; x < size; x++) {
			printf("%10d %10.10c %10d\n", continents[i]->list[x]->id, type_trans[continents[i]->list[x]->type], i);
		}
		if (continents[i]->list[5] != NULL) {
			printf("%10.10s %10.10c %10d\n", "dragon", ' ', i);
		}
	}
	size = lobby->length;
	for (x = 0; x < size; x++) {
		printf("%10d %10.10c %10.10s\n",lobby->list[x]->id, type_trans[lobby->list[x]->type], "queue");
	}

}

int createThreads(pthread_t continent[], int num) {
	int error;
	int created_threads = 0;
	for(int i = 0; i < num; i++) {
		error = pthread_create(&(continent[i]), NULL, &child_thread, (void*)(&(ids[i])));
		if (error != 0) {
			printf("\nThread %d cannot be created: [error %d]\n", i, error);
			return 0;
		} else {
			created_threads++;
		}
	}
	return created_threads;
}
