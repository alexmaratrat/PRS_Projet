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
  char buffer_ctrl[15];
  char buffer_data[T_SIZE];




  // Variables : Congestion window and RTT
  float rtt, speed;
  int fsize,nb_seq;
  struct timespec rtt_start, rtt_stop,timer_start,timer_stop;
  long *ack = NULL;
  char **segments= NULL;
  struct timeval timeout;
  int file_sent = 0;
  int cwnd;
  int stop = 0;
  int indice =0;
  int last_ack_indice =-1;
  int last_ack=-1;

  // Variables : reading file

  FILE *fp;
  char filename[20];
  char buffer_file[T_SIZE-6];
  char *c_temp = malloc(sizeof(char));
  double inter;
  int rest_bytes;
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
          printf("Length of buffer_ctrl is %li :Control Socket : Sending SYN-ACK : %s\n",strlen(buffer_ctrl),buffer_ctrl);
          clock_gettime(CLOCK_MONOTONIC, &rtt_start);
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
          clock_gettime(CLOCK_MONOTONIC, &rtt_stop);
          printf("ACK received : %s\n",buffer_ctrl);
          rtt = (rtt_stop.tv_sec - rtt_start.tv_sec) * (1E9);
          printf("RTT 1 %f\n",rtt );
          rtt = (rtt + (rtt_stop.tv_nsec - rtt_start.tv_nsec)) * (1E-9);

          printf("RTT: %f sec = %f usec\n", rtt, rtt * 1E9);
          timeout.tv_sec = rtt_stop.tv_sec - rtt_start.tv_sec;
          timeout.tv_usec = (long int) (0.001*(rtt_stop.tv_nsec - rtt_start.tv_nsec));
          timeout.tv_usec = 350*timeout.tv_usec;

    }


    }
  }

    // Receiving the namefile && transmission
    if (FD_ISSET(server_data,&fds))
    {
      printf("In FD SET\n");
      memset(buffer_data,0, sizeof(buffer_data));
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
        fp = fopen(filename, "rb");
        fseek(fp,0,SEEK_END); // pointing to the end of file
        fsize = ftell(fp);
        printf("File size is %i \n",fsize);
        fseek(fp,0,SEEK_SET);
        inter = (double)fsize/(T_SIZE-6);
        nb_seq = (int) ceil(inter);;
        cwnd =15;
        ack = malloc(cwnd*sizeof(long*)); // tableau de ack de taille de la fenetre de congestion
        printf("Tableau de ack \n");
        for(int i =0;i<cwnd;i++)
        {
          ack[i] = -5;
          // strncpy(ack[i],"X",1);
          printf("%li\n",ack[i]);
        }
        char sq[12];
        printf("File divided in %i sequences\n", nb_seq);
        printf("Hello.??\n" );
        if(fp==NULL)
        {
            perror("[-]Error in reading file.");
            exit(1);
        }
        printf("Hello.??\n" );

        memset(buffer_data,'\0', sizeof(buffer_data));

        printf("Hello.??\n" );


        // Extraction from file to 2D list of char[T_SIZE] with a length of nb_seq
        segments= calloc(nb_seq,sizeof(char*));
        printf("Hello.??\n" );

        for(int i=1;i<(nb_seq+1);i++)
        {
          if (i == (nb_seq))
          {
            printf("Hey\n");
            // printf("LAST SEQ\n" );
            segments[i]=calloc((T_SIZE),sizeof(char*));
            // printf("fp is located at char : %li \n",ftell(fp));
            sprintf(sq,"%06i",i);
            // printf("%s is \n", sq);
            // printf("Debug\n");
            memcpy(&segments[i][0],sq,6);
            strncat(segments[i],sq,6);
            rest_bytes = fsize%(T_SIZE-6);
            printf("rest_bytes is : %i\n",rest_bytes);
            for (int k = 0; k < rest_bytes+1; k++)
            {
              *c_temp=fgetc(fp);
              if (feof(fp)) // EOF
              {
                printf("EOF REACHED\n");
                break;
              }
              memcpy(&segments[i][6+k],c_temp,1);
              // strncat(segments[i],c_temp,1);
              memset(c_temp,'\0',sizeof(char));
            }
            printf("segment %i is %s\n",i, segments[i]);

          }
          else
          {
            segments[i]=malloc(T_SIZE*sizeof(char*));
            // printf("fp is located at char : %li \n",ftell(fp));
            sprintf(sq,"%06i",i);
            // printf("%s is \n", sq);
            // printf("Debug\n");
            // strncat(segments[i],sq,6);
            memcpy(&segments[i][0],sq,6);
            // printf("segment %i is %s\n",i, segments[i]);
            for (int k = 0; k < T_SIZE-6; k++)
            {
              int temp = fgetc(fp);
              sprintf(c_temp,"%c",temp);
              // *c_temp=fgetc(fp);
              // if (temp==EOF) // EOF
              // {
              //   break;
              // }
              // strncat(&segments[i],c_temp,1);
              memcpy(&segments[i][6+k],c_temp,1);
              memset(c_temp,'\0',sizeof(char));

            }
          }

          // printf("Segment number %i is :  %s\n\n\n",i,segments[i]);
          // printf("Reading file done\n", );
        }

        printf("\n\n DEBUT " );
// Transmission
        int k =1; // last ack
        int seg=0;
        file_sent =0;
        int end_while_test = 0;

        // Starting timer
        clock_gettime(CLOCK_MONOTONIC, &timer_start);
        while (file_sent==0)
        {

          // printf("k is %i\nMax seq is %i\n",k,nb_seq);
          printf("\n");
          printf("\n");

          while (k!=nb_seq+1)
          {
            // if (end_while_test==3) {
            //   break;
            // }
            // Transmission
            printf("Transmission\n");
            for(int i=0;i<cwnd;i++)
            {
              // printf("Check si d??calage\n");
              // Si premi??re case du tableau ack??e --> d??calage du tableau


              // Si case du buffer non valid??e --> transmission
              // printf("ack [i] : ");
              if (ack[i]<=-4)
              {
                if ((i+k)>=nb_seq) {
                  printf("Last segment reached\n");
                  if(sendto(server_data, segments[nb_seq], rest_bytes, 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
                  {
                    printf("Send failed for : %s \n\n",segments[nb_seq]);
                  }
                  printf("Last segment sent\n");
                  break;
                }
                else
                {
                  ack[i]=-1;
                  // printf("Transmission\n" );
                  printf("i vaut %i et k vaut %i\n",i,k);
                  // printf("Segment number %i is sent and it is :  %s\n\n\n",(k+i+1),segments[k+i+1]);
                  // Starting timer

                  if(sendto(server_data, segments[k+i], T_SIZE, 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
                  {
                    printf("Send failed for : %s \n\n",segments[i+k]);
                  }
                  // printf("Segment number %i is sent and it is : %s\n",(k+i), segments[i+k]);
                  printf("Segment number %i is sent\n",(k+i));

                  memset(buffer_data,'\0', sizeof(buffer_data));
                }


              } // ICI
              // R??ception ACK
            }
            printf("\n");
            // R??ception et MAJ des ACKS
            last_ack_indice=-1;
            printf("R??ception et MAJ des ACKS\n");
            for(int i=0;i<cwnd;i++)
            {
              if (k==nb_seq+1) {

                // printf("Last segment reached\n");
                // if(sendto(server_data, segments[nb_seq], (rest_bytes+6), 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
                // {
                //   printf("Send failed for : %s \n\n",segments[i+k]);
                // }
                // // printf("Segment number %i is sent and it is : %s\n",(k+i), segments[i+k]);
                // printf("Segment number %i is sent\n",(k+i));
                // memset(buffer_data,'\0', sizeof(buffer_data));
                file_sent=1;
                break;
              }
              else
              {
                if ((recvfrom(server_data,buffer_data,9,0,(struct sockaddr*) &adresse_data,&alen3))<0)
                {
                  printf("Error rcvfrom %d\n",GETSOCKETERRNO());
                  if (last_ack_indice==-1 ) {
                    ack[0]--;
                  }
                  if (last_ack_indice>=0 && last_ack_indice<(cwnd-1)) {
                    ack[last_ack_indice+1]--;
                  }

                } else {
                  printf("ACK received\n");
                  char *temp = strtok(buffer_data,"ACK");
                  printf("Tableau de ack avant changement:   ");
                  for(int i =0;i<cwnd;i++){
                    printf(" %li ",ack[i]);
                    // printf("i : %i\n",i);
                  }
                  printf("\n" );
                  long num_ack = strtol(temp,NULL,10);
                  if (num_ack>=last_ack) {
                    last_ack=num_ack;
                    // printf("\nnum_ack vaut %li\n",num_ack);
                    printf("i vaut %i et k vaut %i\n",i,k);
                    printf("Je compare l'ack re??u (num_ack) qui est %li au segment %i\n",num_ack,i+k);
                    // Equivalent num??ro segment dans la fen??tre
                    indice = (num_ack)-k;
                    if (indice>last_ack_indice) {
                      last_ack_indice=indice;
                    }
                    printf("Last last_ack_indice is %i \n",last_ack_indice);
                    // Comparaison entre num??ro ACK re??u et segment actuel
                    if (ack[indice]!=0) {
                      ack[indice]=0;
                      for(int i=0;i<indice;i++){
                        ack[i]=0;
                      }
                    }
                    else{
                      ack[indice+1]--;
                    }
                    printf("Tableau de ack apr??s changement:   ");
                    for(int i =0;i<cwnd;i++){
                      printf(" %li ",ack[i]);
                      // printf("i : %i\n",i);
                    }
                    printf("\n\n");
                    memset(buffer_data,0,9);
                  }
                }
                // D??calage fen??tre pour les i acks contigus


              }




            }
            printf("Tableau de ack avant d??calage ??ventuel:   ");
            for(int i =0;i<cwnd;i++){
              printf(" %li ",ack[i]);
              // printf("i : %i\n",i);
            }
            if (last_ack_indice!=-1 && file_sent==0) {
              for(int i = last_ack_indice;i<cwnd;i++){
                ack[i-last_ack_indice]=ack[i];
                ack[i]=-4;
              }
              k+=last_ack_indice+1;
            }
            printf("k vaut %i Tableau de ack apr??s d??calage ??ventuel:   ",k);
            for(int i =0;i<cwnd;i++){
              printf(" %li ",ack[i]);
              // printf("i : %i\n",i);
            }
            printf("\n\n");
            end_while_test++;
          }
          clock_gettime(CLOCK_MONOTONIC, &timer_stop);
          printf("FIN WHILE, fichier envoy??\n");
          printf("segment 0 is %s\n",segments[0]);
          file_sent=1;
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
      if(file_sent==1)
      {
        memset(buffer_data,0, sizeof(buffer_data));
        strcpy(buffer_data,"FIN");
        if(sendto(server_data, buffer_data, sizeof(buffer_data), 0,(struct sockaddr*) &adresse_data,sizeof(adresse_data))==-1)
        {
          printf("Send failed\n");
        }
        // Closing file then sockets
        printf("Closing file \n");
        fclose(fp);
        break;
      }



    }

  // for(int i=0;i<nb_seq;i++)
  // {
  //   free(segments[i]);
  //   free(ack);
  // }
printf("File size is %i \n",fsize);
speed = (timer_stop.tv_sec - timer_start.tv_sec) * (1E9);
speed = (speed+ (timer_stop.tv_nsec - timer_start.tv_nsec)) * (1E-9);

printf("Time elapsed is : %f sec\n So speed is %i ko/s\n", speed, (int)(fsize*0.001/speed));
printf("Num??ro seq max est %i\n",nb_seq);
free(c_temp);
close(server_ctrl);
close(server_data);
return 0;
}
