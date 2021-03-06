#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "floatfann.h"
#include "server.h"
#include "9DOF.h"
#include "LSM9DS0.h"

#define PORTNO 5000

#define NUMDATAPTS 151
#define PI 3.14159265358979
static volatile int run_flag = 1;
int shared_array_index = 0;
pthread_mutex_t lock;

struct Angle_Buffer{
    float pitchBuffer[150];
    float rollBuffer[150];
};
/*
typedef struct {
    CONNECTION *client[4];
} clients;
*/

typedef struct {
    CONNECTION *client;
    int index;
} clients;
int clientCount = 0;
//clientThreads->client[i]

struct Angle_Buffer Angle_Buffer_client_0;
struct Angle_Buffer Angle_Buffer_server;
struct Angle_Buffer Angle_Buffer_client_1;
struct Angle_Buffer Angle_Buffer_client_2;
struct Angle_Buffer Angle_Buffer_client_3;






void do_when_interrupted()
{
	run_flag = 0;
	pthread_mutex_destroy(&lock);
	printf("\n Threads exiting. Please wait for cleanup operations to complete...\n");
}

double timestamp()
{
	// calculate and return the timestamp
	struct timeval tv;
	double sec_since_epoch;

	gettimeofday(&tv, NULL);
	sec_since_epoch = (double) tv.tv_sec + (double) tv.tv_usec/1000000.0;

	return sec_since_epoch;
}

typedef struct client_data 
{
	char* clientIP;
	float accel_data_x[5];
	float accel_data_y[5];
	float accel_data_z[5];
	
	float gyro_data_x[5];
	float gyro_data_y[5];
	float gyro_data_z[5];
	
	
} client_data;

client_data sensors = {.clientIP = "ip"};

void* manage_9dof(void *arg) 
{
	NINEDOF *ninedof;
	double sec_since_epoch;

	mraa_init();
		
	//FILE *fp;
	//fp = fopen("./server_test_data.csv", "w");

	//write header to file "test_data.csv"
	//fprintf(fp, "time (epoch), accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, mag_x, mag_y, mag_z, temperature");
	//fclose(fp);
	
	// 9DOF sensor initialization
	ninedof = ninedof_init(A_SCALE_4G, G_SCALE_245DPS, M_SCALE_2GS);
	float angle_1;
	float angle_2;
	 int i=0;
	 //Angle_Buffer_server->pitchBuffer[0]=0;
		//Angle_Buffer_server->rollBuffer[0]=;
	 	for(i=0;i<150;i++)
	 	{
		// timestamp right before reading 9DOF data
		sec_since_epoch = timestamp();
		ninedof_read(ninedof);
		
		//store 9dof data into struct
		sensors.accel_data_x[4] = ninedof->accel_data.x;
		sensors.accel_data_y[4] = ninedof->accel_data.y;
		sensors.accel_data_z[4] = ninedof->accel_data.z;



		sensors.gyro_data_x[4] = ninedof->accel_data.x;
		sensors.gyro_data_y[4] = ninedof->accel_data.y;
		sensors.gyro_data_z[4] = ninedof->accel_data.z;
		angle_1= -2*atan2(ninedof->accel_data.x,sqrt(pow(ninedof->accel_data.x,2)+pow(ninedof->accel_data.y,2)+pow(ninedof->accel_data.z,2)))*(180/PI);
		angle_2=2*atan2(ninedof->accel_data.y,sqrt(pow(ninedof->accel_data.x,2)+pow(ninedof->accel_data.y,2)+pow(ninedof->accel_data.z,2)))*(180/PI);
		//you can put mag as the parameter too.
		//printf("here: index: %d\n", i);
		Angle_Buffer_server.pitchBuffer[i]=angle_1;
		//printf("here1\n");
		Angle_Buffer_server.rollBuffer[i]=angle_2;
		
		usleep(10000);
	}

	return NULL;
}

void* handle_client(void *arg) //, float pitchBuffer[], float rollBuffer[])
{
        //float pitchBuffer[256], rollBuffer[256];
	//CONNECTION *client;
	clients *client_struct;
	//clients *clientVal = calloc(1,sizeof(CONNECTION*));
    	int n, client_socket_fd;
	char cmd[256]; //char buffer[256]
	float *buffer;
	buffer = calloc(NUMDATAPTS,sizeof(float));
	char ready_buf[10];
		
	//client = (CONNECTION *)arg;
	client_struct = (clients*)arg;
	client_socket_fd = client_struct->client->sockfd;
	//client_socket_fd = clientVal->client[clientCount]->sockfd;
	ioctl(client_socket_fd, FIONBIO, 0);  
	
	//memset(buffer, 0, 256);
	
	
	int index = client_struct->index;			//private variable - each thread will have its own copy. this is the array index of client_data's arrays in which this thread will store into 
	//pthread_mutex_lock(&lock);	//begin critical section
	//index = shared_array_index;	//assign this shared variable to the private index variable so now each thread keeps track of its own unique index	
	//shared_array_index += 1;	//increment the shared variable - first thread to access this variable gets index=0, second thread gets index=1, etc
	//pthread_mutex_unlock(&lock);	//end critical section
	
	
	//while (run_flag) {
		int i;
	    	memset(buffer, 0, 256);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "pitch");							
		//n = read(client_socket_fd, buffer, 10);		//read from socket to see if client ready
		n = read(client_socket_fd, ready_buf, 10);
		if (n < 0) {
			server_error("ERROR reading from socket");
			printf("client not ready\n");
		}
		else {
			//printf("client sent us: %s\n", buffer);
			if (strcmp(ready_buf, "ready")==0) {	//if we read that the client is ready
				cmd[strlen(cmd)] = '\0';							
				n = write(client_socket_fd, cmd, strlen(cmd));//write 'pitch' to socket to request pitch data
				if (n < 0) {
				    server_error("ERROR writing to socket");
				}
				printf("client is ready, sent pitch command\n");

				printf("Reading pitch data from client\n");
				n = read(client_socket_fd, buffer, NUMDATAPTS*4);
				printf("Post read from client\n");

				if(n < 0) {
				    //printf("HERE!\n");
				    server_error("ERROR reading from socket");
				    //return NULL;
				}

				if(buffer[0] == 0) {
				   /*if(buffer[1] == 0) { 
				       //read again
				       n = read(client_socket_fd, buffer, NUMDATAPTS*4);
				   }*/
				   switch(index) {
				       case 0:
				   	 printf("Received Pitch Data from Thread 0: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_0.pitchBuffer[i-1] = buffer[i];
				  	 }
					 break;
				  	case 1: 
					 printf("Received Pitch Data from Thread 1: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_1.pitchBuffer[i-1] = buffer[i];
				  	 }
					 break;
					case 2:
					 printf("Received Pitch Data from Thread 2: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_2.pitchBuffer[i-1] = buffer[i];
				  	 }
					 break;
					case 3:
					 printf("Received Pitch Data from Thread 3: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_3.pitchBuffer[i-1] = buffer[i];
				  	 }
					 break;
					default:
					 printf("I shouldn't be here! \n");
				   } // end of case statement
				}

				sprintf(cmd,"roll");
				cmd[strlen(cmd)] = '\0';
				n=write(client_socket_fd, cmd, strlen(cmd));
				printf("Sent client %s\n", cmd);
				if(n<0) {
				    server_error("ERROR writing to socket");
				}

				printf("----------------------------------------------\n");
				n = read(client_socket_fd, buffer, NUMDATAPTS*4);
				printf("Just read roll\n");
				if(buffer[0] == 1) {
				    /*if(buffer[1] == 0) {
					n = read(client_socket_fd, buffer, NUMDATAPTS*4);
				    }*/
				     switch(index) {
				       case 0:
				   	 printf("Received Roll Data from Thread 0: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_0.rollBuffer[i-1] = buffer[i];
				  	 }
					 break;
				  	case 1: 
					 printf("Received Roll Data from Thread 1: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_1.rollBuffer[i-1] = buffer[i];
				  	 }
					 break;
					case 2:
					 printf("Received Roll Data from Thread 2: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_2.rollBuffer[i-1] = buffer[i];
				  	 }
					 break;
					case 3:
					 printf("Received Roll Data from Thread 3: \n");
				  	 for(i=1;i<NUMDATAPTS;i++) {
				      		 printf("%f\n", buffer[i]);
				       		Angle_Buffer_client_3.rollBuffer[i-1] = buffer[i];
				  	 }
					 break;
					default:
					 printf("I shouldn't be here! Again... \n");
				   } // end of case statement
				}
				printf("END PITCH ROLL LOOP\n");
			}
		}
		
		/*
		// read what the client sent to the server and store it in "buffer"
		printf("Reading pitch from client\n");
		n = read(client_socket_fd, buffer, NUMDATAPTS*4); 	//51 floats at 4 bytes a piece	
		printf("Post Read from client\n");
		if(n < 0) {
		    //printf("HERE!\n");
		    server_error("ERROR reading from socket");
		    //return NULL;
		}

		if(buffer[0] == 0) {
		   if(buffer[1] == 0) { 
		       //read again
		       n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		   }
		   printf("Received Pitch Data: \n");
		   for(i=0;i<NUMDATAPTS;i++) {
		       printf("%f\n", buffer[i]);
		       Angle_Buffer_client.pitchBuffer[i-1] = buffer[i];
		   }
		}

		sprintf(cmd,"roll");
		cmd[strlen(cmd)] = '\0';
		n=write(client_socket_fd, cmd, strlen(cmd));
		printf("Sent client %s\n", cmd);
		if(n<0) {
		    server_error("ERROR writing to socket");
		}

		printf("----------------------------------------------\n");
		if(buffer[0] == 1) {
		    if(buffer[1] == 0) {
			n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		    }
		    printf("REceived Roll Data: \n");
		    for(i=0;i<NUMDATAPTS;i++) {
			printf("%f\n", buffer[i]);
			Angle_Buffer_client.rollBuffer[i-1] = buffer[i];
		    }
		}
		printf("END PITCH ROLL LOOP\n");
		*/



		//memset(cmd, 0, sizeof(cmd));

		//if n==0, ignore. dont worry about for now
		//increase data sample??
		

		//int bufferSize = sizeof(buffer)/4;
		//printf("buffer size: %d\n", bufferSize);
		/*
		for(i=0;i<NUMDATAPTS;i++) {
		    printf("%f\n", buffer[i]);
		}
		*/
		/*
		if(buffer[0] == 0) {
		    //FILE *fp;
		    //fp = fopen("train_data.txt", "wb");
		    printf("First Packet: Pitch Data\n");
		    //printf("raw buffer: %s\n", buffer); 
		    for(i=1; i < NUMDATAPTS; i++) {
			printf("%f\n", buffer[i]);
			Angle_Buffer_client.pitchBuffer[i-1] = buffer[i];
		    }
		    printf("\n");
		    sprintf(cmd,"r");
		    n = write(client_socket_fd, cmd, strlen(cmd));
		    if(n<0) {
			server_error("ERROR writing to socket");
			//return NULL;
		    }
		    n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		    if(n<0) {
			server_error("ERROR reading from socket");
			//return NULL;
		    }
		   */
		   /*
		    if(buffer[0] != 1) {
			printf("Error, incorrect packet ID\n");
			return NULL;
		    }*/
		    //fp = fopen("roll_data.txt", "wb");
/*		    printf("Roll Buffer Data\n");
		    for(i=1;i<NUMDATAPTS;i++) {
			printf("%f\n", buffer[i]);
			Angle_Buffer_client.rollBuffer[i-1] = buffer[i];
		    }
		    printf("\n");

		}
		else if (buffer[0] == 1) {
		   //FILE *fp;
		   //fp = fopen("roll_data.txt","wb");
		   for(i=1; i < NUMDATAPTS; i++) {
		       Angle_Buffer_client.rollBuffer[i-1] = buffer[i];
		   }
		   memset(cmd, 0, sizeof(cmd));
		   sprintf(cmd,"received roll data");
		   n = write(client_socket_fd, cmd, strlen(cmd));
		   if(n<0) {
		       server_error("ERROR writing to socket");
		       //return NULL;
		   }
		   n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		   // an error has occurred
		   if (n < 0) {
			server_error("ERROR reading from socket");
			//return NULL;
		   }
*/			/*
		   if(buffer[0] != 0) {
		       printf("Error, incorrect packet ID\n");
	       	       //return NULL;
	       	   }
		   */
		   //FILE *fp = fopen("pitch_data.txt", "wb");
/*		   printf("Pitch Buffer Data: \n");
		   for(i=1;i<NUMDATAPTS;i++) {
		       printf("%f\n", buffer[i]);
		       Angle_Buffer_client.pitchBuffer[i-1] = buffer[i];
		   }

		}
		//error check to ensure packet label is provided
		else {
	    		printf("ERROR, incorrect  packet IDi. Neither packet had a 1 or 0 as first array element\n");
	    		return NULL;
		}		
*/
		// no data was sent, assume the connection was terminated
		if (n == 0) { 
			printf("%s has terminated the connection.\n", client_struct->client->ip_addr_str);
			//printf("%s has terminated the connection.\n",clientVal->client[clientCount]->ip_addr_str);
		    	return NULL;
		}
		/*
		for(i=0;i<NUMDATAPTS-1;i++) {
		    printf("%f\n", Angle_Buffer_client.pitchBuffer[i]);
		}
		*/

		/*if (n==0) {
		    printf("Waiting for client data\n");
		}*/

		// print the message to console
		//printf("%s says: %s ||| index: %d\n", client->ip_addr_str, buffer, index);
		
		// send an acknowledgement back to the client saying that we received the message
		
		//no need to send data back to client at this point
/*
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%s sent the server: %s", client->ip_addr_str, buffer);
		n = write(client_socket_fd, cmd, strlen(cmd));
		if (n < 0) {
			server_error("ERROR writing to socket");
			return NULL;
		}
*/		

		/*Code below can be used when we know how much data we are sending to the server!*/	
		/*int size_buffer = sizeof(strtok(buffer,","));
		printf("Buffer size in tokens: %d\n", size_buffer);
		//char *p[sizeof(strtok(buffer,","))];
		char *p[size_buffer];
		if(sizeof(p)>0) {
		    p[0] = strtok(buffer,","); //tockenizes by comma
		    printf("First token: %s\n",p[0]);
		    int i;
		    for(i=0; i<size_buffer; i++) {
			p[i] = strtok(NULL,",");
			printf("Token %d: %s\n",i,p[i]);
		    }
		}*/

	  	/*From here to end of current block, we parse buffer received and then convert
		 *the tokens into floats, store the floats in the struct and then perform
		 *some rudimentary addition of the struct values as proof of concept
		 */
		//character array is stored in buffer
		/*
		char* p;
		p = strtok(buffer,",");
		if(p) {
    			//printf("\nFirst string: %f\n",atof(p));
			sensors.accel_data_x[index] = atof(p);
			//printf("First string: %f\n", sensors.accel_data_x[index]);
		}
		p = strtok(NULL,",");
		if(p) {
		    	sensors.accel_data_y[index] = atof(p);
    			//printf("Next string: %f\n",sensors.accel_data_y[index]);
		}*/
				
		//float sum;
	    //printf("SUM: %f\n", sensors.accel_data_x[index]+sensors.accel_data_y[index]);	
	    //printf("client_data contains: thread1=[%f, %f] thread2=[%f, %f], thread3=[%f, %f], thread4=[%f, %f], serverData_[%f, %f]\n\n", sensors.accel_data_x[0], sensors.accel_data_y[0], sensors.accel_data_x[1], sensors.accel_data_y[1], sensors.accel_data_x[2], sensors.accel_data_y[2], sensors.accel_data_x[3], sensors.accel_data_y[3], sensors.accel_data_x[4], sensors.accel_data_y[4]);
	    //run_flag = 0;
	//}

	close(client_socket_fd);
	//pthread_exit((void*)client);
	return NULL;
}


void* manage_server(void *arg)
{
	CONNECTION *server;
	CONNECTION *client;
	int max_connections;
	int i;
	pthread_t tids[256];

	max_connections = 10;

	server = (CONNECTION *) server_init(PORTNO, 10);
	if ((int) server == -1){
		run_flag = 0;
	}

	i = -1;
	
	while(i < max_connections && run_flag) {
	    	//need four of line below in main
		//run while 1
		client = (CONNECTION*) server_accept_connection(server->sockfd);
		if ((int) client == -1) {
			printf("Latest child process is waiting for an incoming client connection.\n");
		}
		else {
			i++;
			pthread_create(&tids[i], NULL, handle_client, (void *)client);
		}
	}
	/*
	client = (CONNECTION*) server_accept_connection(server->sockfd);
	if ((int) client == -1) {
		printf("Latest child process is waiting for an incoming client connection.\n");
	}*/

	/*else {
	    pthread_create(&tids, NULL, handle_client, (void *)client);
	}*/
	/*
	//CONNECTION *client;
	int n, client_socket_fd;
	char cmd[256]; //char buffer[256]
	float *buffer;
	buffer = calloc(NUMDATAPTS,sizeof(float));
	
	client = (CONNECTION *)arg;
	client_socket_fd = client->sockfd;
	//memset(buffer, 0, 256);
	
	
	int index;			//private variable - each thread will have its own copy. this is the array index of client_data's arrays in which this thread will store into 
	pthread_mutex_lock(&lock);	//begin critical section
	index = shared_array_index;	//assign this shared variable to the private index variable so now each thread keeps track of its own unique index	
	shared_array_index += 1;	//increment the shared variable - first thread to access this variable gets index=0, second thread gets index=1, etc
	pthread_mutex_unlock(&lock);	//end critical section
	
	
	//while (run_flag) {
		// clear the buffer
		//memset(buffer, 0, 256);

		// read what the client sent to the server and store it in "buffer"
		n = read(client_socket_fd, buffer, NUMDATAPTS*4); //51 floats at 4 bytes a piece	
		if(n<0) {
		    server_error("ERROR reading from socket");
		    return NULL;
		}
		//int i;
		//int bufferSize = sizeof(buffer)/4;
		//printf("buffer size: %d\n", bufferSize);
		for(i=0;i<NUMDATAPTS;i++) {
		    printf("%f\n", buffer[i]);
		}
		
		if(buffer[0] == 0) {
		    //FILE *fp;
		    //fp = fopen("train_data.txt", "wb");
		    for(i=1; i < NUMDATAPTS; i++) {
			client->pitchBuffer[i-1] = buffer[i];
		    }
		    memset(cmd, 0, sizeof(cmd));
		    sprintf(cmd,"received pitch data");
		    n = write(client_socket_fd, cmd, strlen(cmd));
		    if(n<0) {
			server_error("ERROR writing to socket");
			return NULL;
		    }
		    n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		    if(n<0) {
			server_error("ERROR reading from socket");
			return NULL;
		    }
		    if(buffer[0] != 1) {
			printf("Error, incorrect packet ID\n");
			return NULL;
		    }
		    //fp = fopen("roll_data.txt", "wb");
		    for(i=1;i<NUMDATAPTS;i++) {
			client->rollBuffer[i-1] = buffer[i];
		    }

		}
		else if (buffer[0] == 1) {
		   //FILE *fp;
		   //fp = fopen("roll_data.txt","wb");
		   for(i=1; i < NUMDATAPTS; i++) {
		       client->rollBuffer[i-1] = buffer[i];
		   }
		   memset(cmd, 0, sizeof(cmd));
		   sprintf(cmd,"received roll data");
		   n = write(client_socket_fd, cmd, strlen(cmd));
		   if(n<0) {
		       server_error("ERROR writing to socket");
		       return NULL;
		   }
		   n = read(client_socket_fd, buffer, NUMDATAPTS*4);
		   // an error has occurred
		   if (n < 0) {
			server_error("ERROR reading from socket");
			return NULL;
		   }

		   if(buffer[0] != 0) {
		       printf("Error, incorrect packet ID\n");
	       	       return NULL;
	       	   }
		   //FILE *fp = fopen("pitch_data.txt", "wb");
		   for(i=1;i<NUMDATAPTS;i++) {
		       client->pitchBuffer[i-1] = buffer[i];
		   }

		}
		//error check to ensure packet label is provided
		else {
	    		printf("ERROR, incorrect  packet IDi. Neither packet had a 1 or 0 as first array element\n");
	    		return NULL;
		}		

		// no data was sent, assume the connection was terminated
		if (n == 0) { 
			printf("%s has terminated the connection.\n", client->ip_addr_str);
			return NULL;
		}
	
	*/

	if (i >= max_connections) {
		printf("Max number of connections reached. No longer accepting connections. Continuing to service old connections.\n");
	}

	//void *res;
	//pthread_join(tids, (void*)&res);
	
	
	for(; i >= 0; i--) {
		pthread_join(tids[i], NULL);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t manage_9dof_tid; //, manage_server_tid;
	int rc;
	fann_type *output;
	fann_type input[4];
	struct fann *ann;
	int k, max, patient_location;

	signal(SIGINT, do_when_interrupted);

	//initialize the mutex
	if (pthread_mutex_init(&lock, NULL)!=0)
	{
		printf("mutex init failed\n");
		return 1;
	}

	CONNECTION *server;
	pthread_t tids[256];

	//server thread
	server = (CONNECTION *) server_init(PORTNO, 10);
	if ((int) server == -1){
		run_flag = 0;
	}

	//clients *clientThreads = calloc(4,sizeof(CONNECTION*));	
	
	clients *clientThread_0 = calloc(1,sizeof(clients));
	//clients *clientThread_1 = calloc(1,sizeof(clients));
	//clients *clientThread_2 = calloc(1,sizeof(clients));
	//clients *clientThread_3 = calloc(1,sizeof(clients));

	clientThread_0->index = 0;
	//clientThread_1->index = 1;
	//clientThread_2->index = 2;
	//clientThread_3->index = 3;

	clientThread_0->client = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThread_1->client = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThread_2->client = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThread_3->client = (CONNECTION*) server_accept_connection(server->sockfd);

	//clientThreads->client[0] = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThreads->client[1] = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThreads->client[2] = (CONNECTION*) server_accept_connection(server->sockfd);
	//clientThreads->client[3] = (CONNECTION*) server_accept_connection(server->sockfd);

            float server_pitch_avg = 0;
	    float server_roll_avg = 0;
	    float server_pitch_sum = 0; 
	    float server_roll_sum = 0;
	    float client0_pitch_avg = 0;
	    float client0_roll_avg = 0;
	    float client0_pitch_sum = 0;
	    float client0_roll_sum = 0;
	    float client1_pitch_avg = 0;
	    float client1_roll_avg = 0;
	    float client1_pitch_sum = 0;
	    float client1_roll_sum = 0;
	    float client2_pitch_avg = 0;
	    float client2_roll_avg = 0; 
	    float client2_pitch_sum = 0;
	    float client2_roll_sum = 0;
	


		int i;
	while(1) {
	    pthread_create(&tids[0], NULL, handle_client, (void*)clientThread_0);
	    //pthread_create(&tids[1], NULL, handle_client, (void*)clientThread_1);
	    //pthread_create(&tids[2], NULL, handle_client, (void*)clientThread_2);
	    //pthread_create(&tids[3], NULL, handle_client, (void*)clientThread_3);
	    /*
	    pthread_create(&tids[0], NULL, handle_client, (void*)clientThreads->client[0]);
	    pthread_create(&tids[1], NULL, handle_client, (void*)clientThreads->client[1]);
	    pthread_create(&tids[2], NULL, handle_client, (void*)clientThreads->client[2]);
	    pthread_create(&tids[3], NULL, handle_client, (void*)clientThreads->client[3]);
	    */
	    //
	    clientCount = 0; //reset client count    
	    rc = pthread_create(&manage_9dof_tid, NULL, manage_9dof, NULL);
	    if (rc != 0) {
		fprintf(stderr, "Failed to create manage_9dof thread. Exiting Program.\n");
		exit(0);
	    }
	    
	    pthread_join(manage_9dof_tid, NULL);
	    pthread_join(tids[0], NULL);
	    //pthread_join(tids[1], NULL);
	    //pthread_join(tids[2], NULL);
	    //pthread_join(tids[3], NULL);
   	
	 
	     server_pitch_avg = 0;
	     server_roll_avg = 0;
	     server_pitch_sum = 0; 
	     server_roll_sum = 0;
	     client0_pitch_avg = 0;
	     client0_roll_avg = 0;
	     client0_pitch_sum = 0;
	     client0_roll_sum = 0;
	     client1_pitch_avg = 0;
	     client1_roll_avg = 0;
	     client1_pitch_sum = 0;
	     client1_roll_sum = 0;
	     client2_pitch_avg = 0;
	     client2_roll_avg = 0; 
	     client2_pitch_sum = 0;
	     client2_roll_sum = 0;
	    //float client3_pitch_avg, client3_roll_avg, client3_pitch_sum, client3_roll_sum;
	    //float client4_pitch_avg, client4_roll_avg, client4_pitch_sum, client4_roll_sum;

	    //float clients_pitch_avg[4], clients_roll_avg[4];	

	    
	   
	    for(i=0; i < 150; i++) {
		    server_pitch_sum += Angle_Buffer_server.pitchBuffer[i];
		    server_roll_sum += Angle_Buffer_server.rollBuffer[i];
		    client0_pitch_sum += Angle_Buffer_client_0.pitchBuffer[i];
		    client0_roll_sum += Angle_Buffer_client_0.rollBuffer[i];
		    client1_pitch_sum += Angle_Buffer_client_1.pitchBuffer[i];
		    client1_roll_sum += Angle_Buffer_client_1.rollBuffer[i];
		    client2_pitch_sum += Angle_Buffer_client_2.pitchBuffer[i];
		    client2_roll_sum += Angle_Buffer_client_2.rollBuffer[i];
		    //client3_pitch_sum += Angle_Buffer_client_3.pitchBuffer[i];
		    //client3_roll_sum += Angle_Buffer_client_3.rollBuffer[i];
	
	    }
	    		
	    
	    server_pitch_avg = (server_pitch_sum/150+90)/180;
	    server_roll_avg = (server_roll_sum/150+90)/180;
	    client0_pitch_avg = (client0_pitch_sum/150+90)/180;
	    client0_pitch_avg = (client0_pitch_sum/150+90)/180;
	    client1_pitch_avg = (client1_pitch_sum/150+90)/180;
	    client1_roll_avg = (client1_roll_sum/150+90)/180;
	    client2_pitch_avg = (client2_pitch_sum/150+90)/180;
	    client2_roll_avg = (client2_roll_sum/150+90)/180;
	    //client3_pitch_avg = (client3_pitch_sum/150+90)/180;
	    //client3_roll_avg = (client3_roll_sum/150+90)/180;
	    
	    //test the neural network
	    ann = fann_create_from_file("TEST.net");
	    
	    input[0] = server_pitch_avg;
	    input[1] = server_roll_avg;
	    input[2] = client0_pitch_avg;
	    input[3] = client0_roll_avg;
	    input[4] = client1_pitch_avg;
	    input[5] = client1_roll_avg;
	    input[6] = client2_pitch_avg;
	    input[7] = client2_roll_avg;
	    //input[8] = client3_pitch_avg;
	    //input[9] = client3_roll_avg;
	    

	    output = fann_run(ann, input);
	    max = output[0];
	    patient_location = 0;
	    for (k=1; k<7; k++) {
		    if (output[k] > max) {
			    max = output[k];
			    patient_location = k;
		    }
	    }
	    printf("Patient is at location %d\n", patient_location); 
	
	}

	/*	
	while(i < max_connections && run_flag) {
	    	//need four of line below in main
		//run while 1
		client = (CONNECTION*) server_accept_connection(server->sockfd);
		if ((int) client == -1) {
			printf("Latest child process is waiting for an incoming client connection.\n");
		}
		else {
			i++;
			pthread_create(&tids[i], NULL, handle_client, (void *)client);
		}
	}
	*/


	//create threads
/*	rc = pthread_create(&manage_9dof_tid, NULL, manage_9dof, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create manage_9dof thread. Exiting Program.\n");
		exit(0);
	}

	//dpnt need stuff below
	rc = pthread_create(&manage_server_tid, NULL, manage_server, NULL);
	if (rc != 0) {
		fprintf(stderr, "Failed to create thread. Exiting program.\n");
		exit(0);
	}
*/
	//pthread_join(manage_9dof_tid, NULL);
	//pthread_join(manage_server_tid, NULL);
	
	/*	
	float server_pitch_avg, server_roll_avg, client_pitch_avg, client_roll_avg;
        float server_pitch_sum = 0, server_roll_sum = 0, client_pitch_sum = 0, client_roll_sum = 0;
	
	int i = 0;

	for(; i < 150; i++) {
		server_pitch_sum += Angle_Buffer_server.pitchBuffer[i];
		server_roll_sum += Angle_Buffer_server.rollBuffer[i];
		client_pitch_sum += Angle_Buffer_client.pitchBuffer[i];
		client_roll_sum += Angle_Buffer_client.rollBuffer[i];
	}	    

	server_pitch_avg = (server_pitch_sum/150+90)/180;
	server_roll_avg = (server_roll_sum/150+90)/180;
	client_pitch_avg = (client_pitch_sum/150+90)/180;
	client_roll_avg = (client_roll_sum/150+90)/180;


	//test the neural network
	ann = fann_create_from_file("TEST.net");
	
	input[0] = server_pitch_avg;
	input[1] = server_roll_avg;
	input[2] = client_pitch_avg;
	input[3] = client_roll_avg;
	
	output = fann_run(ann, input);
	max = output[0];
	patient_location = 0;
	for (k=1; k<4; k++)
	{
		if (output[k] > max)
		{
			max = output[k];
			patient_location = k;
		}
	}
	printf("Patient is at location %d\n", patient_location);
	
	
	////////////training stuff////////////
	//FILE *fp;
	//fp = fopen("./train_data.txt", "a");
	//fprintf(fp, "250\t3\t5\n");
	//printf("%f\t%f\t%f\t%f\n", server_pitch_avg, server_roll_avg, client_pitch_avg, client_roll_avg);
	//fprintf(fp,"%f\t%f\t%f\t%f\n", server_pitch_avg, server_roll_avg, client_pitch_avg, client_roll_avg);
	//fprintf(fp,"%d\t%d\t%d\t%d\n", 0,0,0,1);
	//fclose(fp);
*/
	printf("\n...cleanup operations complete. Exiting main.\n");

	return 0;
} 
