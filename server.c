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

#define T_SIZE 1024 // Size of buffer transmitted (MTU)
#define MIN_PORT 1000
#define MAX_PORT 9999
#define GETSOCKETERRNO() (errno)
extern int errno;

int main (int argc, char *argv[]) {
  if (argc!=1){
    perror("Error : Please write ./server");
  }

  // Init variables //


  // Variables : get IP address
  char host[256];
	char *IP;
	struct hostent *host_entry;
	int host_name;

  // Variables : socket initialization
  struct sockaddr_in adresse_ctrl,adresse_data;
  int port_data = MIN_PORT;
  int valid= 1;
  socklen_t alen2= sizeof(adresse_ctrl);
  socklen_t alen3= sizeof(adresse_data);
  char buffer_ctrl[T_SIZE];
  char buffer_data[T_SIZE];




  // Variables : Congestion window and RTT
  float rtt;
  int fsize,max_seq;
  struct timespec start_time, end_time;
  char **ack = NULL;
  char **segments= NULL;
  struct timeval timeout;
  int file_sent = 0;
  int cwnd=1;
  int stop = 0;

  // Variables : reading file

  FILE *fp;
  char filename[20];
  char buffer_file[T_SIZE-6];
  char *c_temp = malloc(sizeof(char));

  fd_set fds;
  int activity;


  // Get IP adress
	host_name = gethostname(host, sizeof(host)); //find the host name
	if(host_name == -1)
  {
    perror("gethostname");
    exit(1);
  }
	host_entry = gethostbyname(host); //find host information
	if(host_entry == NULL){
    perror("gethostbyname");
    exit(1);
  }
	IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
	printf("Your IP: %s\n", IP);

  //create socket
  int server_ctrl = socket(AF_INET, SOCK_DGRAM, 0);
  int server_data = socket(AF_INET,SOCK_DGRAM,0);

  //handle error
  if (server_ctrl< 0) {
    perror("Cannot create control socket \n");
    return -1;
  }
  if (server_data< 0) {
    perror("Cannot create data socket \n");
    return -1;
  }

  // Init sockets options
  setsockopt(server_ctrl, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
  setsockopt(server_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse_ctrl.sin_family= AF_INET;
  adresse_ctrl.sin_port= htons(5050);
  adresse_ctrl.sin_addr.s_addr= htonl(INADDR_ANY);

  adresse_data.sin_family= AF_INET;
  adresse_data.sin_addr.s_addr= htonl(INADDR_ANY);


  // Binding socket
  if (bind(server_ctrl, (struct sockaddr*) &adresse_ctrl, sizeof(adresse_ctrl)) == -1) {
    perror("Bind failed for sckt 2\n");
    close(server_ctrl);
    return -1;
  }

  // Select

  while (1) {
    printf("Hey\n");
    FD_ZERO(&fds);
    FD_SET(server_ctrl,&fds);
    FD_SET(server_data,&fds);
    activity = select(server_data+1,&fds,NULL,NULL,NULL);

    if (activity<0) {
      printf("select error \n");
    }

    // 3-way handshake
    if (FD_ISSET(server_ctrl,&fds))
    {
      int n;
      if ((n =recvfrom(server_ctrl,buffer_ctrl,sizeof buffer_ctrl -1,0,(struct sockaddr*) &adresse_ctrl,&alen2))<0) {
        printf("Error rcvfrom\n");
      }else{
        printf("received in ctrl scket : %s\n",buffer_ctrl);
        if (strncmp(buffer_ctrl,"SYN",3)==0) {
          printf("SYN received\n");
          memset(buffer_ctrl,'\0',sizeof(buffer_ctrl));
          adresse_data.sin_port = htons(MIN_PORT);

          while (bind(server_data, (struct sockaddr*) &adresse_data, sizeof(adresse_data)) <0 && port_data <= MAX_PORT)
          {
            port_data++;
            adresse_data.sin_port = htons(port_data);
          }
          // if (bind(server_data, (struct sockaddr*) &adresse_data, sizeof(adresse_data)) == -1) {
          //   perror("Bind failed for data sckt \n");
          //   close(server_data);
          //   return -1;
          // }
          if (getsockname(server_data, (struct sockaddr *)&adresse_data, &alen3) == -1)
          {
            perror("getsockname");
            return -1;
          }
          char s_port_data[5];
          sprintf(s_port_data,"%i",port_data);
          printf("Data port is %i\n",port_data);
          memset(buffer_ctrl,'\0', sizeof(buffer_ctrl));
          strcat(buffer_ctrl,"SYN-ACK");
          strcat(buffer_ctrl,s_port_data);
          buffer_ctrl[11]='\0';
          printf("Control Socket : Sending SYN-ACK : %s\n",buffer_ctrl);
          clock_gettime(CLOCK_MONOTONIC, &start_time);
          if ((n = sendto(server_ctrl,buffer_ctrl,strlen(buffer_ctrl),0,(struct sockaddr*) &adresse_ctrl,sizeof(adresse_ctrl)))<0)
          {
            printf("Error %i sendto\n",n);
          }
          memset(buffer_ctrl,'\0', sizeof(buffer_ctrl));
          printf("Waiting for ACK...\n");


        }
        if (strncmp(buffer_ctrl,"ACK",3)==0)
        {
          printf("Yo \n");
          clock_gettime(CLOCK_MONOTONIC, &end_time);
          printf("ACK received : %s\n",buffer_ctrl);
          rtt = (end_time.tv_sec - start_time.tv_sec) * (1E9);
          printf("RTT 1 %f\n",rtt );
          rtt = (rtt + (end_time.tv_nsec - start_time.tv_nsec)) * (1E-9);

          printf("RTT: %f sec = %f usec\n", rtt, rtt * 1E9);
          timeout.tv_sec = end_time.tv_sec - start_time.tv_sec;
          timeout.tv_usec = (long int) (0.001*(end_time.tv_nsec - start_time.tv_nsec));
          timeout.tv_usec = 200*timeout.tv_usec;

    }


    }
  }

    // Receiving the namefile && transmission
    if (FD_ISSET(server_data,&fds))
    {
      memset(buffer_data,'\0', sizeof(buffer_data));
      if ((recvfrom(server_data,buffer_data,sizeof buffer_data -1,0,(struct sockaddr*) &adresse_data,&alen3))<0) {
        printf("Error rcvfrom\n");
      }
      printf("Received name file : %s \n",buffer_data);
      // timeout.tv_usec = (int) (rtt*pow(10,6));
      // printf("Timeout : %ld\n",timeout.tv_usec);

      // Setting a timeout for the data_socket
      if (setsockopt (server_data, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout)) < 0)
              printf("setsockopt failed\n");
      // if (setsockopt (server_data, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof(timeout)) < 0)
      //         printf("setsockopt failed\n");
      // FD_SET(server_data,&fds);
      // activity = select(server_data+1,&fds,NULL,NULL,NULL);

        // Reading file
        strcpy(filename,buffer_data);
        fp = fopen(filename, "r");
        fseek(fp,0,SEEK_END); // pointing to the end of file
        fsize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        max_seq = fsize/(T_SIZE-6)+1;
        char s_max_seq[6];
        sprintf(s_max_seq,"%i",max_seq);
        ack = malloc(cwnd*sizeof(char*)); // tableau de ack de taille de la fenetre de congestion
        printf("Tableau de ack \n");
        for(int i =0;i<cwnd;i++)
        {
          ack[i] = malloc(6*sizeof(char*));
          strncpy(ack[i],"X",1);
          printf("%s\n",ack[i]);
        }
        char sq[12];
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
          segments[i]=malloc(T_SIZE*sizeof(char*));
          // printf("fp is located at char : %li \n",ftell(fp));
          sprintf(sq,"%06i",i);
          // printf("%s is \n", sq);
          // printf("Debug\n");
          strncat(segments[i],sq,6);
          // printf("segment %i is %s\n",i, segments[i]);
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
          // printf("Segment number %i is :  %s\n\n\n",i,segments[i]);
          // printf("Reading file done\n", );





        }

        printf("Début transmission\n" );

        // Transmission
        int k =0; // last ack
        int seg=0;
        int file_sent =0;
        int end_while_test = 0;
        while (file_sent==0)
        {

          printf("k is %i\nMax seq is %i\n",k,max_seq);
          while (k!=max_seq-cwnd) {
            /* code */
            if (end_while_test==3) {
              break;
            }
            printf("Begin while\n");
            for(int i=0;i<+cwnd;i++)
            {
              printf("Check si décalage\n");
              // Si première case du tableau ackée --> décalage du tableau
              while(strncmp(ack[0],"X",1)!=0)
              {
                k++;
                printf("OUI\n" );
                for(int i =0;i<cwnd;i++)
                {
                  if (i==cwnd-1) {
                    break;
                  }
                  strncpy(ack[i],ack[i+1],6);
                  printf("%s\n",ack[i]);
                }
                // reset dernière case de la fênetre
                strncpy(ack[cwnd-1],"X",1);
                printf("%s\n",ack[cwnd-1]);
                // k++;
              }

              // Si case du buffer non validée --> transmission
              // printf("ack [i] : ");
              if (strncmp(ack[i],"X",1)==0)
              {
                // printf("Transmission\n" );
                printf("i vaut %i et k vaut %i\n",i,k);
                if(sendto(server_data, segments[k+i], T_SIZE, 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
                {
                  printf("Send failed for : %s \n\n",segments[i]);
                }
                printf("Sent segment : %i \n", k+i);
                memset(buffer_data,'\0', sizeof(buffer_data));
                if ((recvfrom(server_data,buffer_data,sizeof buffer_data -1,0,(struct sockaddr*) &adresse_data,&alen3))<0)
                {
                  printf("Error rcvfrom %d\n",GETSOCKETERRNO());
                } else {
                  // Traitement du ack reçu
                  printf("Ack ? : %s\n",buffer_data);
                  char *temp = strtok(buffer_data,"ACK");
                  // printf("TEMP IS %s\n", temp);
                  // printf("segment %i is %s\n",i,segments[i] );
                  if (strncmp(temp,segments[k+i],6)==0) {
                    printf("ça passe\n");
                    strncpy(ack[i],temp,6);
                    printf("ça passe\n");
                  }
                  // Comparaison entre numéro ACK reçu et séquence actuelle


                  printf("\n");
                  printf("Tableau de ack \n");
                  for(int i =0;i<cwnd;i++){
                    printf("%s\n",ack[i]);
                  }




                }              }



            }
            end_while_test++;
          }
          break;
        }


        // printf("buffer_data size is %li and it is : %s, sizeof(buffer_data),buffer_data);
        // if(sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
        // {
        //   printf("Send failed\n");
        // }
        // memset(buffer_data,'\0', sizeof(buffer_data));
        // memset(buffer_file,'\0', sizeof(buffer_file));
        // file_sent = 1;
      }


        // printf("n value : %i\n", n);


      // End of transmission
      // memset(buffer_data,'\0', sizeof(buffer_data));
      // strcpy(buffer_data,"FIN");
      // if(sendto(server_data, buffer_data, T_SIZE, 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
      // {
      //   printf("Send failed\n");
      // }
      // // Closing file then sockets
      // printf("Closing file \n");
      // fclose(fp);


    }

  // for(int i=0;i<max_seq;i++)
  // {
  //   free(segments[i]);
  //   free(ack);
  // }
free(c_temp);
close(server_ctrl);
close(server_data);
return 0;
}
