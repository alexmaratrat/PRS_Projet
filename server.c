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
	if(host_name == -1){perror("gethostname");exit(1);}
	//check_host_name(hostname);
	host_entry = gethostbyname(host); //find host information
	if(host_entry == NULL){perror("gethostbyname");exit(1);}
	//check_host_entry(host_entry);
	IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
	printf("Current Host Name: %s\n", host);
	printf("Host IP: %s\n", IP);



  // printf("%s\n", argv[0]);
  struct sockaddr_in adresse, adresse2,client1,adresse3;
  int port= 8080;
  int port_data = MIN_PORT;
  int valid= 1;
  socklen_t alen1= sizeof(adresse);
  socklen_t alen2= sizeof(adresse2);
  socklen_t alen3= sizeof(adresse3);
  char buffer[RCVSIZE];
  char buffer_data[RCVSIZE];

  float rtt;
  int fsize,max_seq;
  struct timespec start_time, end_time;

  //create socket
  int server_desc = socket(AF_INET, SOCK_STREAM, 0);
  int server_desc2 = socket(AF_INET, SOCK_DGRAM, 0);
  int server_data = socket(AF_INET,SOCK_DGRAM,0);
  int client_desc1;


  //handle error
  if (server_desc < 0) {
    perror("Cannot create socket 1\n");
    return -1;
  }
  if (server_desc2< 0) {
    perror("Cannot create socket 2\n");
    return -1;
  }
  if (server_data< 0) {
    perror("Cannot create data socket \n");
    return -1;
  }

  setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
  setsockopt(server_desc2, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
  setsockopt(server_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));



  adresse.sin_family= AF_INET;
  adresse.sin_port= htons(port);
  adresse.sin_addr.s_addr= htonl(INADDR_ANY);

  adresse2.sin_family= AF_INET;
  adresse2.sin_port= htons(5050);
  adresse2.sin_addr.s_addr= htonl(INADDR_ANY);

  adresse3.sin_family= AF_INET;
  adresse3.sin_addr.s_addr= htonl(INADDR_ANY);


  //initialize socket
  if (bind(server_desc, (struct sockaddr*) &adresse, sizeof(adresse)) == -1) {
    perror("Bind failed for sckt 1\n");
    close(server_desc);
    return -1;
  }

  if (bind(server_desc2, (struct sockaddr*) &adresse2, sizeof(adresse2)) == -1) {
    perror("Bind failed for sckt 2\n");
    close(server_desc2);
    return -1;
  }



  //listen to incoming clients
  if (listen(server_desc, 0) < 0) {
    printf("Listen failed 1\n");
    return -1;
  }

  // Select

  fd_set fds;
  int activity;

  int accept_done = 0;
  while (1) {
    printf("Hey\n");
    FD_ZERO(&fds);
    FD_SET(server_desc,&fds);
    FD_SET(server_desc2,&fds);
    FD_SET(server_data,&fds);
    activity = select(server_data+1,&fds,NULL,NULL,NULL);

    if (activity<0) {
      printf("select error \n");
    }

    if (FD_ISSET(server_desc,&fds)) {
      if ((client_desc1 = accept(server_desc, (struct sockaddr*)&client1, &alen1))<0) {
        printf("Pbm connect client1\n");
      }else{
        int msgSize = read(client_desc1,buffer,RCVSIZE);
        printf("Connected ");

        while (msgSize > 0) {
          // printf("%i\n",msgSize);
          write(client_desc1,buffer,msgSize);
          printf("%s\n",buffer );
          printf("%i",strcmp(buffer,"stop"));

          if (strcmp(buffer,"stop")==0) {
            close(client_desc1);
            memset(buffer,0,RCVSIZE);
            printf("%s",buffer);
            close(client_desc1);
            // if (p==0) {
            //   printf("This process (pid :%i )\n",getpid());
            //   exit(0);
            // }

          }else{
            memset(buffer,0,RCVSIZE);
            msgSize = read(client_desc1,buffer,RCVSIZE);
          }
      }
    }
  }
    if (FD_ISSET(server_desc2,&fds)) {
      int n;
      if ((n =recvfrom(server_desc2,buffer,sizeof buffer -1,0,(struct sockaddr*) &adresse2,&alen2))<0) {
        printf("Error rcvfrom\n");
      }else{
        // buffer[n] = "";
        printf("received in ctrl scket : %s\n",buffer);
        // printf("%i\n", strlen(buffer));
        // printf("%i\n", strcmp(buffer,"stop\0\0"));
        if (strncmp(buffer,"SYN",3)==0) {
          printf("SYN received\n");
          memset(buffer,0,sizeof(buffer));
          adresse3.sin_port = htons(MIN_PORT);

          while (bind(server_data, (struct sockaddr*) &adresse3, sizeof(adresse3)) <0 && port <= MAX_PORT) {
            port_data++;
            adresse3.sin_port = htons(port_data);
          }
          // if (bind(server_data, (struct sockaddr*) &adresse3, sizeof(adresse3)) == -1) {
          //   perror("Bind failed for data sckt \n");
          //   close(server_data);
          //   return -1;
          // }
          if (getsockname(server_data, (struct sockaddr *)&adresse3, &alen3) == -1) {
            perror("getsockname");
            return -1;
          }

          char *s = malloc(40);
          char s_port_data[10];
          sprintf(s_port_data,"%d",port_data);
          strcpy(s,s_port_data);
          // strcat()
          printf("Data port is %i\n",port_data);
          bzero(buffer, RCVSIZE);
          // char *buffer = malloc(80);
          // char *ack = malloc(40);
          // strcpy(ack,"SYN-ACK");
          strcat(buffer,"SYN-ACK");
          strcat(buffer,s_port_data);
          printf("buffer akhi IS : %s\n",buffer);


          clock_gettime(CLOCK_MONOTONIC, &start_time);
          if ((n = sendto(server_desc2,buffer,strlen(buffer),0,(struct sockaddr*) &adresse2,sizeof(adresse2)))<0) {
            printf("Error %i sendto\n",n);
          }
        }
        if (strncmp(buffer,"ACK",3)==0) {
          printf("ACK received : %s\n",buffer);


        }
        if (strncmp(buffer,"stop",4)==0) {
          printf("Going to stop \n" );
          close(server_desc2);
          exit(0);
        }


    }


    }

    if (FD_ISSET(server_data,&fds)) {
      // printf("Data socket\n");
      bzero(buffer_data,RCVSIZE);
      if ((recvfrom(server_data,buffer_data,sizeof buffer_data -1,0,(struct sockaddr*) &adresse3,&alen3))<0) {
        printf("Error rcvfrom\n");
      }
      printf("received in data socket: %s \n",buffer_data);
      if (strncmp(buffer_data,"ACK",3)==0) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        printf("ACK received\n");
        rtt = (end_time.tv_sec - start_time.tv_sec) * (1E9);
        rtt = (rtt + (end_time.tv_nsec - start_time.tv_nsec)) * (1E-9);

        printf("RTT: %f sec = %f usec\n", rtt, rtt * 1E9);

        // strcpy(buffer_data,"ACK");
        // if ((sendto(server_data,buffer_data,strlen(buffer_data),0,(struct sockaddr*) &adresse3,sizeof(adresse3)))<0) {
        //   printf("Error %i sendto\n");
        // }
      }

      int n;
      FILE *fp;
      char filename[20];
      strcpy(filename,buffer_data);
      fp = fopen(filename, "r");
      fseek(fp,0,SEEK_END); // pointing to the end of file
      fsize = ftell(fp);
      fseek(fp,0,SEEK_SET);
      max_seq = fsize/(RCVSIZE-6)+1;
      int seq = 22;
      printf("File divided in %i sequences\n", max_seq);
      if(fp==NULL)
      {
          perror("[-]Error in reading file.");
          exit(1);
      }
      char buffer_file[T_SIZE-6];
      while(fgets(buffer_file,T_SIZE-6,fp)!=NULL)
      {
        bzero(buffer_data,RCVSIZE);
        sprintf(buffer_data,"%d",seq);
        strcat(buffer_data,buffer_file);
        printf("buffer_data : %s\n", buffer_data);
        n = sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse3,sizeof(adresse3));
        printf("n value : %i\n", n);

      }
      // End of transmission
      bzero(buffer_data,RCVSIZE);
      strcpy(buffer_data,"FIN");
      n = sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse3,sizeof(adresse3));
      // Closing file then sockets
      printf("Closing file \n");
      fclose(fp);
      // printf("%s\n",buffer_data);


    }

    // if(p==0){
    //   printf("In child process connected\n");
    // } else {
    //   printf("In parent process connected\n");
    // }
    // // printf("Value of accept is:%d\n", client_desc);
  }


close(server_desc);
return 0;
}