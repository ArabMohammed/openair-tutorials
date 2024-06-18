#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct {
  uint64_t magic;          // Magic value (see defines above)
  uint32_t size;           // Number of samples per antenna to follow this header
  uint32_t nbAnt;          // Total number of antennas following this header
  // Samples per antenna follow this header,
  // i.e. nbAnt = 2 => this header+samples_antenna_0+samples_antenna_1
  // data following this header in bytes is nbAnt*size*sizeof(sample_t)
  uint64_t timestamp;      // Timestamp value of first sample
  uint32_t option_value;   // Option value
  uint32_t option_flag;    // Option flag
} samplesBlockHeader_t;
/*******************************************************/
/****************************************************/

int fullread(int fd, void *_buf, int count) {
  char *buf = _buf;
  int ret = 0;
  int l;

  while (count) {
    l = read(fd, buf, count);

    if (l <= 0)
      return -1;

    count -= l;
    buf += l;
    ret += l;
  }

  return ret;
}

/******************************************************/

void fullwrite(int fd, void *_buf, int count) {
  char *buf = _buf;
  int l;

  while (count) {
    l = write(fd, buf, count);

    if (l <= 0) {
      if (errno==EINTR)
        continue;

      if(errno==EAGAIN) {
        continue;
      } else {
        printf("Lost socket \n");
        exit(1);
      }
    } else {
      count -= l;
      buf += l;
    }
  }
}

/************************************************

void* server_start(short port) {
  char address[100];  // Assuming a reasonable size for the address string
  snprintf(address, sizeof(address), "tcp://*:%d",port);  // Construct address
  void *context = zmq_ctx_new();
  void *publisher = zmq_socket(context, ZMQ_PUB);
  int rc = zmq_bind (publisher, address);
  AssertFatal(rc == 0, "zmq_bind failed");
  return publisher ;
}
/*************************************************/
/**************************************************/
int server_start(short port) {
    printf("welcome in server \n");
    int listen_sock;
    int enable = 1;
    struct sockaddr_in addr;
    printf("ser 1 \n");
    // Create socket
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }
    printf("ser 2 \n");
    // Set socket options
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Set socket option failed");
        close(listen_sock);
        return -1;
    }
    printf("ser 3 \n");
    // Initialize sockaddr_in structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    printf("ser 4 \n");
    // Bind the socket to the port
    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(listen_sock);
        return -1;
    }
    printf("ser 5 \n");
    // Start listening for incoming connections
    if (listen(listen_sock, 5) < 0) {
        perror("Listen failed");
        close(listen_sock);
        return -1;
    }
    printf("ser 6 \n");
    // Accept a connection
    int client_sock;
    if ((client_sock = accept(listen_sock, NULL, NULL)) < 0) {
        perror("Accept failed");
        close(listen_sock);
        return -1;
    }
    printf("ser 7 \n");
    // Close the listening socket as it is no longer needed
    close(listen_sock);
    return client_sock;
}


/***********************************************************
 void* client_start(char *IP, short port) {
  void *context = zmq_ctx_new ();
  void *subscriber = zmq_socket (context, ZMQ_SUB);
  char address[100];  // Assuming a reasonable size for the address string
  snprintf(address, sizeof(address), "tcp://%s:%d",IP,port);  // Construct address
  if (zmq_connect(subscriber, address) == -1) {
        perror("zsock_connect failed");
  }
  return subscriber;
}
/**********************************************************/

int client_start(char *IP, short port) {
    printf("welcome in client \n");
    int sock;
    struct sockaddr_in addr;
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Initialize sockaddr_in structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, IP, &addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    bool connected = false;

    // Attempt to connect
    while (!connected) {
        // Attempt to connect to the server
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            connected = true;
            // Connection established
            // Uncomment the log line if needed
            // printf("rfsimulator: connection established\n");
        } else {
            // Print error message if connection fails
            perror("Connection failed");
            sleep(1); // Wait before retrying
        }
    }

    return sock;
}


/********************************************************/

enum  blocking_t {
  notBlocking,
  blocking
};
void setblocking(int sock, enum blocking_t active) {
  int opts;
  opts = fcntl(sock, F_GETFL);

  if (active==blocking)
    opts = opts & ~O_NONBLOCK;
  else
    opts = opts | O_NONBLOCK;

  fcntl(sock, F_SETFL, opts);
}

/**********************************************************/
int main(int argc, char *argv[]) {
  int fd=open("sendinfo.txt",O_RDONLY);
  printf("good1");
  //off_t fileSize=lseek(fd, 0, SEEK_END);
  int serviceSock=0;
  int fileSize=0;
  if (strcmp(argv[2],"server")==0) {
    printf("you are a server");
    serviceSock=server_start(atoi(argv[3]));
  } else {
    printf("you are a client");
    serviceSock=client_start(argv[2],atoi(argv[3]));
  }
  /*bool raw = false;
  if ( argc == 5 ) {
    raw=true;
  }
  samplesBlockHeader_t header;
  int bufSize=100000;
  void *buff=malloc(bufSize);
  uint64_t timestamp=0;
  const int blockSize=1920;
  // If fileSize is not multiple of blockSize*4 then discard remaining samples
  fileSize = (fileSize/(blockSize<<2))*(blockSize<<2);

  while (1) {
    //Rewind the file to loop on the samples
    if ( lseek(fd, 0, SEEK_CUR) >= fileSize )
      lseek(fd, 0, SEEK_SET);

    // Read one block and send it
    setblocking(serviceSock, blocking);

    if ( raw ) {
      header.magic=0;
      header.size=blockSize;
      header.nbAnt=1;
      header.timestamp=timestamp;
      timestamp+=blockSize;
      header.option_value=0;
      header.option_flag=0;
    } else {
      read(fd,&header,sizeof(header));
    }

    fullwrite(serviceSock, &header, sizeof(header));
    int dataSize=sizeof(int32_t)*header.size*header.nbAnt;

    if (dataSize>bufSize) {
      void *new_buff = realloc(buff, dataSize);

      if (new_buff == NULL) {
        free(buff);
      } else {
        buff = new_buff;
      }
    }

    read(fd,buff,dataSize);

    if (raw) // UHD shifts the 12 ADC values in MSB
      for (int i=0; i<header.size*header.nbAnt*2; i++)
        ((int16_t *)buff)[i]/=16;

    usleep(1000);
    printf("sending at ts: %lu, number of samples: %d \n",
           header.timestamp, header.size);
    fullwrite(serviceSock, buff, dataSize);
    // Purge incoming samples
    setblocking(serviceSock, notBlocking);
    int ret;

    do {
      char buff[64000];
      ret=read(serviceSock, buff, 64000);

      if ( ret<0 && !( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
        printf("error: %s\n", strerror(errno));
        exit(1);
      }
    } while ( ret > 0 ) ;
  }*/

  return 0;
}