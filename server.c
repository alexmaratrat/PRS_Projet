#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>

#define RCVSIZE 1024
#define T_SIZE 1024
#define SIZE 100
#define MIN_PORT 1000
#define MAX_PORT 9999

extern int errno;



int main (int argc, char *argv[]) {
  if (argc!=1){
    perror("Error : Please write ./server");
  }


  // Get IP adress

  char host[256];
	char *IP;
	struct hostent *host_entry;
	int host_name;
	host_name = gethostname(host, sizeof(host)); //find the host name
	if(host_name == -1)
  {
    perror("gethostname");
    exit(1);
  }
	//check_host_name(hostname);
	host_entry = gethostbyname(host); //find host information
	if(host_entry == NULL){
    perror("gethostbyname");
    exit(1);
  }
	//check_host_entry(host_entry);
	IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
	printf("Current Host Name: %s\n", host);
	printf("Host IP: %s\n", IP);



  // printf("%s\n", argv[0]);
  struct sockaddr_in adresse2,adresse3;
  int port= 8080;
  int port_data = MIN_PORT;
  int valid= 1;
  socklen_t alen2= sizeof(adresse2);
  socklen_t alen3= sizeof(adresse3);
  char buffer[RCVSIZE];
  char buffer_data[RCVSIZE];

  float rtt;
  int fsize,max_seq;
  struct timespec start_time, end_time;

  //create socket
  int server_desc2 = socket(AF_INET, SOCK_DGRAM, 0);
  int server_data = socket(AF_INET,SOCK_DGRAM,0);

  //handle error
  if (server_desc2< 0) {
    perror("Cannot create socket 2\n");
    return -1;
  }
  if (server_data< 0) {
    perror("Cannot create data socket \n");
    return -1;
  }

  setsockopt(server_desc2, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
  setsockopt(server_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));



  adresse2.sin_family= AF_INET;
  adresse2.sin_port= htons(5050);
  adresse2.sin_addr.s_addr= htonl(INADDR_ANY);

  adresse3.sin_family= AF_INET;
  adresse3.sin_addr.s_addr= htonl(INADDR_ANY);


  // Binding socket

  if (bind(server_desc2, (struct sockaddr*) &adresse2, sizeof(adresse2)) == -1) {
    perror("Bind failed for sckt 2\n");
    close(server_desc2);
    return -1;
  }

  // Select

  fd_set fds;
  int activity;
  // int accept_done = 0;
  int *ack = NULL;
  char **segments= NULL;
  FILE *fp;
  char filename[20];
  char buffer_file[T_SIZE-6];
  char *c_temp = malloc(sizeof(char));
  struct timeval timeout;
  int file_sent = 0;
  int cwnd=1;
  int stop = 0;
  while (1) {
    printf("Hey\n");
    FD_ZERO(&fds);
    FD_SET(server_desc2,&fds);
    FD_SET(server_data,&fds);
    activity = select(server_data+1,&fds,NULL,NULL,NULL);

    if (activity<0) {
      printf("select error \n");
    }

    // Monitoring control scket
    if (FD_ISSET(server_desc2,&fds)) {
      int n;
      if ((n =recvfrom(server_desc2,buffer,sizeof buffer -1,0,(struct sockaddr*) &adresse2,&alen2))<0) {
        printf("Error rcvfrom\n");
      }else{
        printf("received in ctrl scket : %s\n",buffer);
        if (strncmp(buffer,"SYN",3)==0) {
          printf("SYN received\n");
          memset(buffer,'\0',sizeof(buffer));
          adresse3.sin_port = htons(MIN_PORT);

          while (bind(server_data, (struct sockaddr*) &adresse3, sizeof(adresse3)) <0 && port <= MAX_PORT)
          {
            port_data++;
            adresse3.sin_port = htons(port_data);
          }
          // if (bind(server_data, (struct sockaddr*) &adresse3, sizeof(adresse3)) == -1) {
          //   perror("Bind failed for data sckt \n");
          //   close(server_data);
          //   return -1;
          // }
          if (getsockname(server_data, (struct sockaddr *)&adresse3, &alen3) == -1)
          {
            perror("getsockname");
            return -1;
          }
          char s_port_data[10];
          sprintf(s_port_data,"%i",port_data);
          printf("Data port is %i\n",port_data);
          memset(buffer,'\0', sizeof(buffer));
          strcat(buffer,"SYN-ACK");
          strcat(buffer,s_port_data);
          buffer[11]='\0';
          printf("Control Socket : Sending SYN-ACK : %s\n",buffer);
          clock_gettime(CLOCK_MONOTONIC, &start_time);
          if ((n = sendto(server_desc2,buffer,strlen(buffer),0,(struct sockaddr*) &adresse2,sizeof(adresse2)))<0)
          {
            printf("Error %i sendto\n",n);
          }
          memset(buffer,'\0', sizeof(buffer));
          printf("Waiting for ACK...\n");


        }
        if (strncmp(buffer,"ACK",3)==0)
        {
          printf("Yo \n");
          clock_gettime(CLOCK_MONOTONIC, &end_time);
          printf("ACK received : %s\n",buffer);
          rtt = (end_time.tv_sec - start_time.tv_sec) * (1E9);
          printf("RTT 1 %f\n",rtt );
          rtt = (rtt + (end_time.tv_nsec - start_time.tv_nsec)) * (1E-9);

          printf("RTT: %f sec = %f usec\n", rtt, rtt * 1E9);
          timeout.tv_sec = end_time.tv_sec - start_time.tv_sec;
          timeout.tv_usec = (long int) (0.001*(end_time.tv_nsec - start_time.tv_nsec));






    }


    }

    if (FD_ISSET(server_data,&fds))
    {
      memset(buffer_data,'\0', sizeof(buffer_data));
      if ((recvfrom(server_data,buffer_data,sizeof buffer_data -1,0,(struct sockaddr*) &adresse3,&alen3))<0) {
        printf("Error rcvfrom\n");
      }
      printf("Received name file : %s \n",buffer_data);
      // timeout.tv_usec = (int) (rtt*pow(10,6));
      // printf("Timeout : %ld\n",timeout.tv_usec);

      if (setsockopt (server_data, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout)) < 0)
              printf("setsockopt failed\n");
      if (setsockopt (server_data, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof(timeout)) < 0)
              printf("setsockopt failed\n");
      FD_SET(server_data,&fds);
      activity = select(server_data+1,&fds,NULL,NULL,NULL);


        strcpy(filename,buffer_data);
        fp = fopen(filename, "r");
        fseek(fp,0,SEEK_END); // pointing to the end of file
        fsize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        max_seq = fsize/(RCVSIZE-6)+1;
        ack = (int*) calloc(max_seq,sizeof(int));
        printf("Tableau de ack \n");
        for(int i =0;i<max_seq;i++)
        {
          printf(" %i ",ack[i]);
        }

        // int seq = 0;
        char sq[12];
        // sprintf(sq,"%i",seq);
        // strcpy(sq,);
        // printf("sizeof qs : %li\n",sizeof(sq));
        // printf("fsize is : %i\n",fsize);
        printf("File divided in %i sequences\n", max_seq);
        if(fp==NULL)
        {
            perror("[-]Error in reading file.");
            exit(1);
        }

        memset(buffer_data,'\0', sizeof(buffer_data));

        // Extraction from file to 2D list of char[T_SIZE] with a length of max_seq
         segments= malloc(max_seq*sizeof(char*));

        for(int i=0;i<max_seq;i++)
        {
          segments[i]=malloc(T_SIZE*sizeof(char));
          // printf("fp is located at char : %li \n",ftell(fp));
          sprintf(sq,"%06i",i);
          // printf("Debug\n");
          strcat(segments[i],sq);
          for (int k = 0; k < T_SIZE-6; k++)
          {
            if (feof(fp)) // EOF
            {
              break;
            }
            *c_temp=fgetc(fp);
            strcat(segments[i],c_temp);
            memset(c_temp,'\0',sizeof(c_temp));
          }
          printf("Segment number %i is :  %s\n\n\n",i,segments[i]);
        }

        // Transmission
        int k =0;
        for(int i=k;i<k+cwnd+1;i++){
          if (strncmp(segments[i],max_seq,1)==0)
            break;
          if (ack[k]==1) // décalage de fenêtre vers la droite
            k++;

          // Lance un timer = RTT pour checker si ack du segment est reçu, si timeout, renvoi
          // clock_gettime(CLOCK_MONOTONIC, &timer_s);
          // int timeout=
          if(sendto(server_data, segments[i], T_SIZE, 0,(struct sockaddr*) &adresse3,sizeof(adresse3))==-1)
          {
            printf("Send failed for : %s \n\n",segments[i]);
          }
        }

        // printf("buffer_data size is %li and it is : %s, sizeof(buffer_data),buffer_data);
        if(sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse3,sizeof(adresse3))==-1)
        {
          printf("Send failed\n");
        }
        memset(buffer_data,'\0', sizeof(buffer_data));
        memset(buffer_file,'\0', sizeof(buffer_file));
        file_sent = 1;
      }



        // printf("n value : %i\n", n);


      // End of transmission
      // memset(buffer_data,'\0', sizeof(buffer_data));
      // strcpy(buffer_data,"FIN");
      // if(sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse3,sizeof(adresse3))==-1)
      // {
      //   printf("Send failed\n");
      // }
      // // Closing file then sockets
      // printf("Closing file \n");
      // fclose(fp);
    }
    stop++;
    if (stop==5) {
      break;
    }
  }
  // for(int i=0;i<max_seq;i++)
  // {
  //   free(segments[i]);
  //   free(ack);
  // }
free(c_temp);
close(server_desc2);
close(server_data);
return 0;
}
