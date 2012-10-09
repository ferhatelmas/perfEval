/****************************************************************************/
/*                  Copyright 1997, Trustees of Boston University.          */
/*                               All Rights Reserved.                       */
/*                                                                          */
/* Permission to use, copy, or modify this software and its documentation   */
/* for educational and research purposes only and without fee is hereby     */
/* granted, provided that this copyright notice appear on all copies and    */
/* supporting documentation.  For any other uses of this software, in       */
/* original or modified form, including but not limited to distribution in  */
/* whole or in part, specific prior permission must be obtained from Boston */
/* University.  These programs shall not be used, rewritten, or adapted as  */
/* the basis of a commercial software or hardware product without first     */
/* obtaining appropriate licenses from Boston University.  Boston University*/
/* and the author(s) make no representations about the suitability of this  */
/* software for any purpose.  It is provided "as is" without express or     */
/* implied warranty.                                                        */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*  Author:    Paul Barford                                                 */
/*  Title:     Surge - Scalable URL Reference GEnerator                     */
/*  Revision:  1.2         5/7/98                                           */
/*                                                                          */
/*  This is the part of the SURGE code which actually makes document        */
/*  requests.  It requires Pthreads be installed in order to operate.       */
/*  See the SURGE-HOW-TO in order to compile and run properly.              */
/*                                                                          */
/*  Surge is set up so that you can multiple Surge client processes can be  */
/*  run on a single system.  It is initiated from the Surgemaster program.  */
/*  Each client process runs multiple threads - each thread makes document  */
/*  requests and then rests for some off time.  Running multiple client     */
/*  processes with each running request threads allows us to overcome the   */
/*  limit on sockets per process in addition to a number of other scaling   */
/*  problems.  Each of the document requests is synchronized internally and */
/*  with other Surgeclient processes via named pipes which are set up and   */
/*  managed by the Surgemaster.                                             */
/*                                                                          */
/*  The Surgeclient is run by the Surgemaster via:                          */
/*                                                                          */
/*    Surgeclient {#threads} {run time in seconds} {client system number}   */
/*                                                                          */
/*  Rev 1.1:  The big change for rev. 1.1 is that SURGE now requests objects*/
/*            An object is a persistent collection of files.  Off times are */
/*            placed between the transfer of objects.  There is no longer   */
/*            any notion of Active off time.  A nice result of the move to  */
/*            to objects is that the client code now has a smaller memory   */
/*            footprint.                                                    */
/*                                                                          */
/*  Rev 1.2:  The big change for rev 1.2 is the capability of running Surge */
/*            using either HTTP/1.0 or HTTP/1.1 (or some other version of   */
/*            HTTP of your choosing).  If you want to run HTTP/1.0 just     */
/*            compile using make client10.  If you want to run HTTP/1.1     */
/*            compile using make client11.  You can add your own protocol   */
/*            by just modifying the Surgeclient.c and client11 routines     */
/*            to make them your own.  All of the #defines have been moved   */
/*            to Surgeclient.h  and clientXX.h since some are used in the   */
/*            new client routines.  There is also a client11p which         */
/*            implements pipelined requests and reads.  This is the         */
/*            default for make all.                                         */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "Surgeclient.h"

/****************************************************************************/
/* Define the routines which are used in this program                       */
/****************************************************************************/

extern void* ClientThread(void *);
extern void* ReaderThread(void *);
int connectsrvr();
void init_arrays();
void results();
void loadoutput (long, long, long, int, long, long, long, long);
int fillreadbuff(int, char *,size_t);
void catcher();
void catcher1();

/****************************************************************************/
/* Define the structures used for results and networking used in the program*/
/****************************************************************************/

struct results {           /* This is the data used to construct the output */
  int threadno;            /* Thread ID                                     */
  long objectno;           /* index of object selected                      */
  long fileno;             /* index of file in object                       */
  long starttimeus;        /* Start time of transaction - microseconds      */
  long starttimesec;       /* Start time of transaction - seconds           */
  long filesize;           /* Request Size of file                          */
  long endtimeus;          /* End time of transaction - microseconds        */
  long endtimesec;         /* End time of transaction - seconds             */
};

struct hostent *srvrptr;   /* points to the server IP name - used in gethost*/
struct sockaddr_in serv_addr; /* used in connectsrvr in connect command     */
struct sigaction act;  /* used in main to catch SIGPIPE signal              */
struct sigaction act1; /* used in main to catch SIGSEGV signal              */

/****************************************************************************/
/* Arrays and variables used by each client to construct requests, results  */
/****************************************************************************/

int seqfifo;              /* ID of object sequence fifo                     */
int offfifo;              /* ID of off time fifo                            */
int outfifo;              /* ID of the output sequence fifo                 */
int cntfifo;              /* ID of the object count fifo                    */
int systemno;             /* ID of this surge client                        */
long size[MATCHSIZE];     /* Sizes of files from mout.txt                   */
long object[MATCHSIZE][OBJLIMIT];    /* File names sequence for each object */
long seq[NAMESIZE];          /* sequence of object names from name.txt      */
char httpcmd[] = HTTPCMD;    /* buffer for the HTTP command used in tests   */
char httpterm[] = HTTPTERM;  /* buffer for HTTP command termination sequnce */
char httpextn[] = HTTPEXTN;  /* buffer for the test files extension         */
char httptail[] = HTTPTAIL;  /* buffer for the tail of the HTTP string      */
char httpstart[URLSIZE];     /* buffer for first half of HTTP rqst string   */
char httpend[URLSIZE];       /* buffer for second half of HTTP rqst string  */
struct results surgeout[OUTSIZE]; /* array for results of tests             */
int readerstatus[MAXTHREAD][MAXCONCTS]; /* status of each reader thread     */
pthread_mutex_t mutex[MAXTHREAD][MAXCONCTS+1]; /* mutexs for reader theads  */
pthread_cond_t cond[MAXTHREAD][MAXCONCTS+1];   /* cond vars for reader thrds*/
//BOKI
int d_clid;
char *httpdir;       /* buffer for root HTTP directory on server - removed from the above */
char *httpsrvr;      /* buffer for the server name                  */


/****************************************************************************/
/* Main simply calls the initialization routines and then spawns the client */
/* threads which actually do the work of getting the documents.             */
/****************************************************************************/

int main(int argc, char *argv[])
{ 
  int i, j[MAXTHREAD], threadnum, runtime;
  pthread_t thread[MAXTHREAD];
  char * servername;

  if (argc < 6)
    {
      printf("SURGEclientUsage:  {#threads} {runtime} {server address} {directory} {surge client id} {client id number}\n");
      exit(1);
    }

  /* Initialize the arrays and variables                                    */

  threadnum = atoi(argv[1]);
  runtime = atoi(argv[2]);
  //BOKI
  servername = argv[3];
  httpsrvr = servername;
  httpdir = argv[4];
  d_clid = atoi(argv[5]);
  systemno = atoi(argv[6]);

  /* Modify the handling of the SIGPIPE signal which can cause SURGE process*/
  /* to abort prematurely.  This signal can pop up when a server is heavily */
  /* loaded.  The current handler simply prints out that the signal has been*/
  /* seen and then does nothing which is like SIG_ING ie. ignore signal     */
  /* Also define a signal handler for the SIGSEGV signal which I had some   */
  /* problems with in the clientXX routines.  This handler again simply     */
  /* prints out that the signal was received and shuts down the Surgeclient */
  /* process which received it.  You must debug and restart if you see this */

  act.sa_handler = catcher;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  
  if(sigaction(SIGPIPE, &act, NULL) == -1) {
    printf("SURGEclient %d: Error: Can't set up signal catcher\n", systemno);
    printf("SURGEclient %d: STOPPED, Restart Simulation\n", systemno);
    exit(0);
  }

  act1.sa_handler = catcher1;
  sigemptyset(&act1.sa_mask);
  act1.sa_flags = SA_RESTART;

  if(sigaction(SIGSEGV, &act1, NULL) == -1) {
    printf("SURGEclient %d: Error: Can't set up SEGSEGV catcher\n", systemno);
    printf("SURGEclient %d: STOPPED, Restart Simulation\n", systemno);
    exit(0);
  }

  printf("SURGEclient %d: running %d threads\n", systemno, threadnum);

  init_arrays();   /* Initialize the data arrays used in this program       */
//Boki
//  pthread_init();  /* Part of Pthreads - does initializations for pthereads */

  /* Put together the head and tail of the URL strings which will be passed */
  /* This makes the generation of HTTP request strings within threads fast  */
  /* For HTTP 1.1 be sure that the request string conforms to the RFC which */
  /* requires that the entire http URI string or Host is part of message    */
  /* NOTE:  This is only set up to deal with HTTP 1.0 or 1.1, if you want to*/
  /* use 0.9 (or something else), you must modify the #defines in           */
  /* Surgeclient.h so that the proper command string is generated in        */
  /* httpstart and httpend buffers                                          */

  strcpy(httpstart, httpcmd);
  // BOKI - strncat(httpstart, &httpsrvr[0], sizeof(httpsrvr));
  strncat(httpstart, &httpsrvr[0], strlen(httpsrvr));
  // BOKI - strncat(httpstart, &httpdir[0], sizeof(httpdir));
  strncat(httpstart, &httpdir[0], strlen(httpdir));
  strcpy(httpend, httpextn);
  strncat(httpend, &httptail[0], sizeof(httptail));
  strncat(httpend, &httpterm[0], sizeof(httpterm));

  /* Resolve specified internet host name using function from netdb.h       */
  /* if this is done every time, it really slows things down and is not     */
  /* realistic for today's browsers which have dns helpers.                 */ 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if ((srvrptr=gethostbyname(servername))==NULL) { 
    printf("SURGEclient:  Error resolving server name\n");
    exit(0);
  }
  
  bcopy((char*)srvrptr->h_addr,(char*)&serv_addr.sin_addr,srvrptr->h_length);

  /* Open named pipes which contain sequence values and off time values     */
  /* which are read by threads.                                             */

  if((seqfifo = open(SEQFIFO, O_RDWR)) < 0)
  {
      perror("SURGEclient:  could not open SEQFIFO ");
      exit(2);
    }
  if((offfifo = open(OFFFIFO, O_RDWR)) < 0)
  {
      perror("SURGEclient:  could not open OFFFIFO ");
      exit(2);
    }
  if((outfifo = open(OUTFIFO, O_RDWR)) < 0)
  {
      perror("SURGEclient:  could not open OUTFIFO ");
      exit(2);
    }
  if((cntfifo = open(CNTFIFO, O_RDWR)) < 0)
  {
      perror("SURGEclient:  could not open CNTFIFO ");
      exit(2);
    }

  /* Start up the threads for the client and then wait until they return    */

  for (i = 0; i < threadnum; i++)
    {

      j[i] = i;
      if(pthread_create(&thread[i], NULL, ClientThread, &j[i]))
	{
	  printf("SURGEclient %d:  Unable to start thread %d\n", systemno, i);
	}
      usleep(50000);  /* Stager thread start to reduce startup transient    */
    }

  /* Now wait until threads are complete or until timeout of parent thread  */

  sleep(runtime);

  /* Call the results routine which writes out the log file                 */

  results();

  exit(0);
  return 0;

}   /* End Main */

/****************************************************************************/
/* This routine is called by sigaction command in main.  It simply notes    */
/* that the SIGPIPE signal has been seen and returns.  This has the effect  */
/* of SIG_IGN ie. ignoring the SIGPIPE signal.                              */
/****************************************************************************/

void catcher()
{
  printf("SURGEclient %d: Received SIGPIPE signal\n", systemno);
}

/****************************************************************************/
/* This routine is called by sigaction command in main.  It simply notes    */
/* that the SIGSEGV signal has been seen.  It shuts down the process which  */
/* got the signal and returns.  You should stop the process and debug the   */
/* clientXX routine that you are using at that point.                       */
/****************************************************************************/

void catcher1()
{
  printf("SURGEclient %d: Received SIGSEGV signal - abort simulation\n", 
	 systemno);
  exit(0);
}

/****************************************************************************/
/* This routine does a single read from the connection pointed to by conct  */
/* into the buffer pointed to by buff and returns either 0, -1 or the number*/
/* of bytes read.  This routine is called by ReaderThread.                  */
/****************************************************************************/

int fillreadbuff(int conct, char * buff, size_t buffsz)
{
  int ret, c;

  c = 0;
  while(c == 0) {
    ret = read(conct,buff,buffsz);
    if(ret < 0 && errno != EINTR) { /* Test for call interrupt signal     */
      printf("SURGEclient %d:  Error on Read: %i\n", systemno, errno);
      c = 1;
      return -1;
    } else if(ret < 0 && errno == EINTR) {
      /*printf("SURGEclient: EINTR error on read...trying again\n"); */
    } else {
      c = 1;
      return ret;
    }
  }

  return(c);

} /* End fillreadbuff */

/****************************************************************************/
/* This routine connects to a remote machine at specified port, and returns */
/* a file descriptor.                                                       */
/****************************************************************************/

int connectsrvr()
{
  int fd;

  /* Open a socket on the system                                            */
  
  if ((fd=socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("SURGEcleint:  Trouble Opening Socket ");
    close(fd);
    return -1;
  }

  /* Now set up connection to the specified host                            */

  if (connect(fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
    perror("SURGEclient:  Trouble Connecting to Socket ");
    close(fd);
    return -1;
  }
 
  return fd;

}  /* End connectsrvr */

/****************************************************************************/
/* This routine opens the input files and writes values to arrays.          */
/* NOTE that the off time values are in milliseconds so change them to      */
/* microseconds.                                                            */
/****************************************************************************/

void init_arrays()
{
  long i, j, filename, sizeval, junk, grpno, objname;
  FILE *fp;

  /* Initialize the object file with -1 so we know where objects end        */
  /* Get object definitions from OBJFILE and load them into array object    */

  for(i=0;i<MATCHSIZE;i++)
    for(j=0;j<OBJLIMIT;j++)
      object[i][j] = -1;
  i = 0;
  j = 0;
  if((fp = fopen(OBJFILE, "r")) == NULL)
    {
      perror("SURGEclient:  Unable to open OBJFILE ");
      exit(1);
    }
  while( fscanf(fp, "%ld", &filename) != EOF)
    {
      if(i > NAMESIZE)
        {
          printf("OBJ: Buffers filled before scan end. Increase MATCHSIZE\n");
          exit(1);
          }
      object[i][j] = filename;
      if(getc(fp) == '\n') {
	i++;
	j = 0;
      } else {
	j++;
      }
    }
  fclose(fp);

/* Get sequence of object names from SEQFILE and load them into array seq  */

  i = 0;
  if((fp = fopen(SEQFILE, "r")) == NULL)
    {
      perror("SURGEclient:  Unable to open SEQFILE ");
      exit(1);
    }
  while( fscanf(fp, "%ld\n", &objname) != EOF)
    {
      if(i > NAMESIZE)
        {
          printf("SEQ: Buffers filled before end of scan.Increase NAMESIZE\n");
          exit(1);
        }
      seq[i] = objname;
      i++;
    }
  fclose(fp);

/* Get request file sizes from MATCHFILE and load them into array sizes      */

  i = 0;
  if((fp = fopen(MATCHFILE, "r")) == NULL)
    {
      perror("SURGEclient:  Unable to open MATCHFILE ");
      exit(1);
    }
  while( fscanf(fp, "%ld %ld %ld\n", &junk, &sizeval, &grpno) != EOF)
    {
      if(i > MATCHSIZE)
        {
          printf("SIZ: Buffers filled before scan end.  Increase MATCHSIZE\n");
          exit(1);
          }
      size[i] = sizeval;
      i++;
    }
  fclose(fp);

  /* Set the starttime in the results array equal to zero                   */

  for(i=0;i<OUTSIZE;i++) surgeout[i].starttimesec = 0;

}  /* End init_arrays  */

/****************************************************************************/
/* Load the output array with values from a transaction                     */
/****************************************************************************/

void loadoutput (long outindx, long obj, long flno, int id, long startus,
		 long starts, long endus, long ends)
{
  /* quick'n dirty fix */
  if( outindx >= OUTSIZE ) {
    printf("surgeout filled before simulation end. Increase OUTSIZE (in Surgeclient.h) and rerun simulation\n");
    return;
  }
  surgeout[outindx].threadno = id;
  surgeout[outindx].objectno = obj;
  surgeout[outindx].fileno = flno;
  surgeout[outindx].starttimeus = startus;
  surgeout[outindx].starttimesec = starts;
  surgeout[outindx].filesize = size[object[obj][flno]];
  surgeout[outindx].endtimeus = endus;
  surgeout[outindx].endtimesec = ends;

} /* End loadoutput  */

/****************************************************************************/
/* Write the results of the session to the log file                         */
/****************************************************************************/

void results()
{
  int clid = CLIENTNO;
  long i;
  FILE *fp1;
  char justurl[URLSIZE+20];
  char srvr[] = SERVER;
  char httpdir[] = HTTPDIR;
  char httpextn[] = HTTPEXTN;
  char logname[20] = LOGFILE;
  char extsnbuff[10];
  char filebuff[10];

  //BOKI
  clid = d_clid;

  printf("SURGEclient %d:  Writing log file\n", systemno);

  /* Open and write to the output file LOGFILE                              */

  sprintf(extsnbuff, "%d", systemno);
  strcat(logname,extsnbuff);
  if((fp1 = fopen(logname, "w")) == NULL)
    {
      perror("SURGEclient:  Unable to open log file ");
      exit(1);
    }
  for(i=0;i<OUTSIZE;i++)
      if(surgeout[i].starttimesec != 0)
	{ 
	  strcpy(justurl, srvr);
	  strncat(justurl, &httpdir[0], sizeof(httpdir));
	  sprintf(filebuff, "%ld", 
		  object[surgeout[i].objectno][surgeout[i].fileno]);
	  strncat(justurl, &filebuff[0], sizeof(filebuff));
	  strncat(justurl, &httpextn[0], sizeof(httpextn));
	  fprintf(fp1,"%d %d %d %ld %ld %s %ld %ld %ld\n",clid, systemno,
		  surgeout[i].threadno,surgeout[i].starttimesec,
		  surgeout[i].starttimeus,justurl,surgeout[i].filesize,
		  surgeout[i].endtimesec,surgeout[i].endtimeus);
	}

  fclose(fp1);

}  /* End results                                                           */


