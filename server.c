
//Code adapted from Workshop 6
//Author: Aneesh Chattaraj _826860
#include <errno.h>

#include <sys/sendfile.h>

#include <stdio.h>

#include <stdbool.h>

#include <stdlib.h>

#include <string.h>

#include <sys/stat.h>

#include <arpa/inet.h>

#include <fcntl.h>

#include <netdb.h>

#include <netinet/in.h>

#include <strings.h>

#include <sys/select.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <unistd.h>
//HTML files
char * page[7] = {
  "1_intro.html",
  "2_start.html",
  "3_first_turn.html",
  "4_accepted.html",
  "5_discarded.html",
  "6_endgame.html",
  "7_gameover.html"
};

static char
const *
const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char
const *
const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int
const HTTP_400_LENGTH = 47;
static char
const *
const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int
const HTTP_404_LENGTH = 45;
//constants
typedef enum {
  NO_PAGE = -1,
    INTRO = 0,
    START = 1,
    FIRST_TURN = 2,
    ACCEPTED = 3,
    DISCARDED = 4, ENDGAME = 5, GAMEOVER = 6
}
STATUS; //status of the webpage
//struct for players
typedef struct {
  int username_len;
  char * username;
  int sock_fd;
  char * guesses[100];

  int guess_no;

  int status;
}
Player;
int game = 0;
Player player1;

Player player2;
bool start = true; // toc check if the game should be in starting position or not
typedef enum {
  GET,
  POST,
  UNKNOWN
}
METHOD;

STATUS status1 = NO_PAGE;
static bool handle_http_request(int sockfd) {
  char buff[2048];
  int n = read(sockfd, buff, 2048);
  if (n <= 0) {
    if (n < 0)
      perror("read");
    else
      printf("socket %d close the connection\n", sockfd);
    return false;
  }

  // terminate the string
  buff[n] = 0;
  char * curr = buff;
  METHOD method = UNKNOWN;
  if (strncmp(curr, "GET ", 4) == 0) {
    curr += 4;
    method = GET;
  } else if (strncmp(curr, "POST ", 5) == 0) {
    curr += 5;
    method = POST;
  } else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0) {
    perror("write");
    return false;
  }

  while ( * curr == '.' || * curr == '/')
    ++curr;

  if ( * curr) {
    //GET
    if (method == GET) {
      if (strstr(buff, "start=") != NULL && start == false) {
        status1 = FIRST_TURN;

        if (sockfd == player1.sock_fd)
          player1.status = 1;
        else if (sockfd == player2.sock_fd)
          player2.status = 1;
      } else {
        status1 = INTRO;
        start = false;

      }
      struct stat st;
      stat(page[status1], & st);
      n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
      // send the header first
      if (write(sockfd, buff, n) < 0) {
        perror("write");
        return false;
      }
      // send the file
      int filefd = open(page[status1], O_RDONLY);
      do {
        n = sendfile(sockfd, filefd, NULL, 2048);
      }
      while (n > 0);
      if (n < 0) {
        perror("sendfile");
        close(filefd);
        return false;
      }
      close(filefd);
    } //POST
    else if (method == POST) 
    {
      char * username;
      int username_length;
      
      long size;
      if (start == false)
        status1 = START;
      else status1 = ENDGAME;
      struct stat st;
      if (strstr(buff, "user=") != NULL) {
        stat(page[status1], & st);
        username = strstr(buff, "user=") + 5;
        username_length = strlen(username);
       
        size = st.st_size;
        n = sprintf(buff, HTTP_200_FORMAT, size);

        if (player1.sock_fd == NULL) {
          player1.sock_fd = sockfd;
          player1.username = strdup(username);
          player1.username_len = username_length;
          player1.guess_no = 0;
        } else if (player2.sock_fd == NULL) {
          player2.sock_fd = sockfd;
          player2.username = strdup(username);
          player2.username_len = username_length;
          player2.guess_no = 0;
        }
      } else if (strstr(buff, "quit=Quit") != NULL) {
        //Quit button's separated from the function of other buttons
        if (start == true) {
          status1 = GAMEOVER;
          stat(page[status1], & st);
        } else {
          start = true;
          status1 = ENDGAME;
          stat(page[status1], & st);
        }
        n = sprintf(buff, HTTP_200_FORMAT, st.st_size);

        if (sockfd == player1.sock_fd) {
          player1.sock_fd = NULL;

        } else if (sockfd == player2.sock_fd) {
          player2.sock_fd = NULL;
        }
        n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
        if (write(sockfd, buff, n) < 0) {
          perror("write");
          return false;
        }

        int filefd = open(page[status1], O_RDONLY);
        n = read(filefd, buff, 2048);
        if (n < 0) {
          perror("read");
          close(filefd);
          return false;
        }
        close(filefd);
        stat(page[status1], & st);
        if (write(sockfd, buff, st.st_size) < 0) {
          perror("write");
          return false;
        }

      }//guess works fine but couldnt get the keywords to display
	 else if (strstr(buff, "guess=Guess") != NULL) {
        char * keyword = strstr(buff, "keyword=") + 8;
        int length = strlen(keyword);
        char * word = malloc(length - 12);
        strncpy(word, keyword, length - 12);
        if (sockfd == player1.sock_fd) {

          if (player2.status == 1) {
            status1 = ACCEPTED;
            player1.guesses[player1.guess_no] = word;
            player1.guesses[player1.guess_no][strlen(word)] = '\0';
            ++player1.guess_no;

            for (int j = 0; j < player2.guess_no; ++j) {

              if ((strcmp(player2.guesses[j], player1.guesses[player1.guess_no - 1]) == 0)) {
                game = 1;
                player1.guess_no = 0;
                player2.guess_no = 0;
                player1.status = 0;
                player2.status = 0;
                player1.sock_fd = NULL;
                player2.sock_fd = NULL;
                memset(player1.guesses, '\0', 50);
                memset(player2.guesses, '\0', 50);
                status1 = ENDGAME;
                start = true;

              }
            }
          } else status1 = DISCARDED;

        } else if (sockfd == player2.sock_fd) {
          if (player1.status == 1) {
            status1 = ACCEPTED;
            player2.guesses[player2.guess_no] = word;
            player2.guesses[player2.guess_no][strlen(word)] = '\0';
            player2.guess_no++;

            for (int j = 0; j < player1.guess_no; ++j) {
              if (strcmp(player1.guesses[j], player2.guesses[player2.guess_no - 1]) == 0) {
                game = 1;
                player1.guess_no = 0;
                player2.guess_no = 0;
                player1.status = 0;
                player2.status = 0;
                player1.sock_fd = NULL;
                player2.sock_fd = NULL;
                memset(player1.guesses, '\0', 50);
                memset(player2.guesses, '\0', 50);
                status1 = ENDGAME;
                start = true;
              }
            }
          } else status1 = DISCARDED;
        }
        stat(page[status1], & st);
        n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
      }

      if (write(sockfd, buff, n) < 0) {
        perror("write");
        return false;
      }
      // read the content of the HTML file
      int filefd = open(page[status1], O_RDONLY);
      n = read(filefd, buff, 2048);
      if (n < 0) {
        perror("read");
        close(filefd);
        return false;
      }
      close(filefd);
      //tried printing username using workshop method but failed/ program kept giving weird stuff
      if ((strlen(username) > 0) && (strstr(buff, "user=") != NULL)) {
        int p1, p2;
        for (p1 = size - 1, p2 = p1 ; p1 >= size - 25; --p1, --p2)
          buff[p1] = buff[p2];
        ++p2;
        
        buff[p2++] = ' ';
        ++p1;
        // copy the username
        strncpy(buff + p2, username, username_length + 1);

        if (write(sockfd, buff, size) < 0) {
          perror("write");
          return false;
        }
      } else {
        if (write(sockfd, buff, st.st_size) < 0) {
          perror("write");
          return false;
        }

      }
    } else {
      // never used
      fprintf(stderr, "no other methods supported");
    }
    // send 404
  } else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0) {
    perror("write");
    return false;
  }

  return true;
}

int main(int argc, char * argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s ip port\n", argv[0]);
    return 0;
  }
  printf("Image tagger running at IP %s on port %s \n",argv[1],argv[2] );

  // create TCP socket which only accept IPv4
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // reuse the socket if possible
  int
  const reuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, & reuse, sizeof(int)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // create and initialise address we will listen on
  struct sockaddr_in serv_addr;
  bzero( & serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  // if ip parameter is not specified
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  // bind address to socket
  if (bind(sockfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // listen on the socket
  listen(sockfd, 5);

  // initialise an active file descriptors set
  fd_set masterfds;
  FD_ZERO( & masterfds);
  FD_SET(sockfd, & masterfds);
  // record the maximum socket number
  int maxfd = sockfd;

  while (1) {
    // monitor file descriptors
    fd_set readfds = masterfds;
    if (select(FD_SETSIZE, & readfds, NULL, NULL, NULL) < 0) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    // loop all possible descriptor
    for (int i = 0; i <= maxfd; ++i)
      // determine if the current file descriptor is active
      if (FD_ISSET(i, & readfds)) {
        // create new socket if there is new incoming connection request
        if (i == sockfd) {
          struct sockaddr_in cliaddr;
          socklen_t clilen = sizeof(cliaddr);
          int newsockfd = accept(sockfd, (struct sockaddr * ) & cliaddr, & clilen);
          if (newsockfd < 0)
            perror("accept");
          else {
            // add the socket to the set
            FD_SET(newsockfd, & masterfds);
            // update the maximum tracker
            if (newsockfd > maxfd)
              maxfd = newsockfd;
            // print out the IP and the socket number
            char ip[INET_ADDRSTRLEN];
            printf(
              "new connection from %s on socket %d\n",
              // convert to human readable string
              inet_ntop(cliaddr.sin_family, & cliaddr.sin_addr, ip, INET_ADDRSTRLEN),
              newsockfd
            );
          }
        }
        // a message is sent from the client
        else if (!handle_http_request(i)) {
          close(i);
          FD_CLR(i, & masterfds);
        }
      }
  }
  return 0;
}
