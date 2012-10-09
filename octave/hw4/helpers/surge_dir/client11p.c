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
/*  Title:     Client11 - HTTP/1.1 compliant Client thread                  */
/*  Revision:  1.0         5/7/98                                           */
/*                                                                          */
/*  This is the part of the SURGE code which actually makes document        */
/*  requests.  It requires Pthreads be installed in order to operate.       */
/*  See the SURGE-HOW-TO in order to compile and run properly.              */
/*                                                                          */
/*  Surge is set up so that you can multiple Surge client processes can be  */
/*  run on a single system.  This code used to be part of Surgeclient.c but */
/*  has been separated so that multiple HTTP flavors can be supported.  This*/
/*  is the code that supports the HTTP/1.1 flavor.  This code is meant to   */
/*  be compiled as a module for Surgeclient.c.  The two routine contained   */
/*  here are what actually make the requests for files from the server.     */
/*  The ClientThread routine is what is referred to as a user-entity.  The  */
/*  ReaderThread routine is what actually makes a single document request.  */
/*                                                                          */
/*  In order to support HTTP/1.1 this code supports multiple persistent     */
/*  connections.  It supports both content length and chunked encoding to   */
/*  determine file end.  It should be used with fewer threads per process   */
/*  since it will use up to MAXCONCTS per user entity for file transfers.   */
/*                                                                          */
/*  This code is compiled in the Makefile as client11.c and compiles into   */
/*  client.o.  It's HTTP/1.0 counterpart is called client10.c               */
/*                                                                          */
/*             THESE ARE THE HTTP/1.1 COMPLIANT CLIENTTHREAD ROUTINES       */
/*             THAT INCLUDE THE ABILITY TO DO PIPELINED REQUESTS AND        */
/*             PIPELINE READING.                                            */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "client11p.h"

void* ClientThread(void *);
void* ReaderThread(void *);
int Checkconnect(void *);
void Writerequest(int, void *);

/* Each of the following routines and variables are defined in Surgeclient.c*/
/* The variables in caps are #defined in Surgeclient.h                      */

extern int seqfifo;
extern int offfifo;
extern int outfifo;
extern int cntfifo;
extern int systemno;
extern long object[MATCHSIZE][OBJLIMIT];
extern long seq[NAMESIZE];
extern char httpstart[URLSIZE];
extern char httpend[URLSIZE];
extern int readerstatus[MAXTHREAD][MAXCONCTS];
extern pthread_mutex_t mutex[MAXTHREAD][MAXCONCTS+1];
extern pthread_cond_t cond[MAXTHREAD][MAXCONCTS+1];

extern int connectsrvr();
extern void loadoutput (long, long, long, int, long, long, long, long);
extern int fillreadbuff(int, char *,size_t);

/****************************************************************************/
/* This routine calls for the connection set up and calls the document      */
/* retriever routine as a separate thread.  It is the first routine which   */
/* is executed by the threads and is called from main.                      */
/****************************************************************************/

void * ClientThread(void * threadno)
{
  int j, numobjs, numreaders, numconcts, statusflag, conctstat;
  int id, stopobjloop, stopfileloop, numfiles;
  long i,objindx, count;
  struct timeval starttime;
  struct timezone tz;
  char seqbuff[10];
  char offbuff[10];
  char cntbuff[10];
  int debugflag = 0;
  pthread_t readerid[MAXCONCTS];
  struct readerdata reader[MAXCONCTS];

  /* Begin by initializing all of the variables used in the ClientThread    */

  numobjs = 0;     /* for HTTP1.1 number of objects to get before new concts*/
  count = 0;       /* count is used to store the object index from OBJFIFO  */
  id = *(int *)threadno; /* assign local thread id                          */
  stopobjloop = 0; /* stopobjloop = 0 until the object sequence is complete */
  numconcts = 0;   /* the number of open TCP connections                    */

  /* The following mutex and cond vars are used by ReaderThreads to signal  */
  /* the ClientThread that they are finished reading                        */

  if(pthread_mutex_init(&mutex[id][MAXCONCTS], NULL))
    printf("pthread_mutex_init() ERROR\n");
  if(pthread_cond_init(&cond[id][MAXCONCTS], NULL))
    printf("pthread_cond_init() ERROR\n");
  pthread_mutex_lock(&mutex[id][MAXCONCTS]);

  /* The following mutex and cond vars are used by the ClientThread to      */
  /* signal the specific ReaderThread to start reading.This loop also starts*/
  /* the pool ReaderThreads that will be used by this ClientThread.         */

  for(i=0;i<MAXCONCTS;i++) {
    readerstatus[id][i] = 0; /* set status of readers to 0 - reader is ready*/
    reader[i].readerno = i;  /* Initialize the reader number                */
    reader[i].threadno = id; /* Initialize the Readers ClientThread number  */
    reader[i].conctno = -1;  /* Initialize socket descriptors to -1         */
    if(pthread_mutex_init(&mutex[id][i], NULL))
      printf("pthread_mutex_init() ERROR\n");
    if(pthread_cond_init(&cond[id][i], NULL))
      printf("pthread_cond_init() ERROR\n");
    pthread_mutex_lock(&mutex[id][i]);
    /* Start up the reader threads which will wait for the signal   */
    if(pthread_create(&readerid[i],NULL,ReaderThread,&reader[i]))
      {
        printf("SURGEclient %d: ClntThread %d cant start Reader %ld\n",
               systemno, id, i);
      }
  }
  if(debugflag)
    printf("SURGEclient %d:  Started ClientThread # %d\n", systemno, id);

  /* Loop until all of the objects in name.txt (OBJFIFO) have been requested*/

  while(stopobjloop == 0)
    {

      /* Get the next object from the object FIFO.  Stop when -1 is read    */
      /* from the object sequence FIFO which means that the entire sequence */
      /* has been read.                                                     */

      if(read(seqfifo, seqbuff, sizeof(seqbuff)) < 0)
        printf("SURGEclient %d:  Error reading from seqfifo\n", systemno);
      count = atoi(seqbuff);
      if(count == -1) {     /* Test to see if FIFO has passed -1, if so end */
        stopobjloop = 1;
	objindx = 0;
      } else {
        objindx = seq[count];
      }
      if(count%100 == 0)
        printf("SURGEclient %d:  Object Count %ld\n",systemno, count);

      /* For HTTP1.1 - Get the number of objects to read before closing the */
      /* connections and startiang a new set of connections to server.      */
      /* I call this the "open connection" period.                          */

      if(numobjs == 0) {
        if(read(cntfifo, cntbuff, sizeof(cntbuff)) < 0)
          printf("SURGEclient %d:  Error reading from cntfifo\n", systemno);
        numobjs = atoi(cntbuff);
        if(debugflag)
          printf("SURGEclient %d: Objs in open connect period = %d\n",
                 systemno, numobjs);
      }

      /* Get the file names associated with the selected object and request */
      /* them from the server. If HTTP1.1 then use multiple connections     */
      
      stopfileloop = 0; /* stopfileloop = 0 until object has been transferd */
      numfiles = 0;     /* Number of files in current object                */
      numreaders = 0;         /* allow up to MAXCONCTS readers to be used   */

      if(stopobjloop == 0)
        {

          /* For pipelined HTTP/1.1 requests, find out how many files are   */
          /* in the current object and then split these between the         */
          /* connections                                                    */

          while(stopfileloop == 0) {
            numfiles++;
            if(object[objindx][numfiles] == -1 || numfiles == OBJLIMIT)
              stopfileloop = 1;
          }

          /* Read the specified file(s) using the specified connection(s)   */
          /* In the case of pipelining in HTTP/1.1, we will request the     */
          /* the first file (HTML) and then divide the remaining files      */
          /* evenly between each pipe and request all of these remaining    */
          /* files at the same time.                                        */

          /* Time stamp the start of the transaction for the first file     */
          /* Open a single connection to the server and request the first   */
          /* file.  Wait until it returns and then request any remaining    */
          /* files opening up additional connects up to MAXCONCTS           */

          gettimeofday(&starttime,&tz);
          if(numconcts == 0) {         /* Get the first connection          */
            while(reader[numconcts].conctno == -1)
              reader[numconcts].conctno = connectsrvr();
            if(debugflag)
              printf("SURGEclient %d: Client %d Opened connect 0\n",
                     systemno, id);
            numconcts++;
          }
          numreaders++;
          readerstatus[id][0] = 1;  /* Set up reader data structures        */ 
          reader[0].objectno = objindx;
          reader[0].nofiles = 1;
          reader[0].startus = starttime.tv_usec;
          reader[0].starts = starttime.tv_sec;
          reader[0].fileno[0] = 0;
          pthread_mutex_lock(&mutex[id][0]); /*Signal reader to get 1st file*/
          pthread_cond_signal(&cond[id][0]);
          pthread_mutex_unlock(&mutex[id][0]);
          if(debugflag)
            printf("Surgeclient %d: Waiting for 1st reader to complete\n",
                   systemno);
          if (pthread_cond_wait(&cond[id][MAXCONCTS], 
                                &mutex[id][MAXCONCTS]) != 0) {
            printf("Surgeclient:  pthread_cond_wait FAILED\n");
            pthread_mutex_unlock(&mutex[id][MAXCONCTS]);
          }
          numreaders--;

          if(numfiles > 1) {  /* Get remaining files on all connects        */

            /*Be sure server didn't close connect #1 after fetching 1st file*/

            conctstat = Checkconnect(&reader[0]);
          
            /* Time stamp the start of the transaction for each pipeline    */
  
            gettimeofday(&starttime,&tz);

            /* Open a connections to HTTP server up to MAXCONCTS            */

            while(numconcts < MAXCONCTS && numfiles-1 > numconcts) { 
              while(reader[numconcts].conctno == -1)    /* Get a connection */
                reader[numconcts].conctno = connectsrvr();
              if(debugflag)
                printf("SURGEclient %d: Client %d Opened connect %d\n",
                       systemno, id, numconcts);
              numconcts++;
            }

            /* Now set the data structures for each reader thread           */

            for(i=0;i<numconcts;i++) {
              readerstatus[id][i] = 1;
              numreaders++;
              reader[i].objectno = objindx;
              reader[i].nofiles = 0;
              reader[i].startus = starttime.tv_usec;
              reader[i].starts = starttime.tv_sec;
            }
            for(j=1;j<numfiles;j++) { /* Split remaining files between readr*/
              i = j%numconcts;
              reader[i].fileno[reader[i].nofiles] = j;
              reader[i].nofiles++;
            }

            /* Signal the appropriate Reader thread that it's time to read  */
            /* the files specified in it's reader structure.                */

            for(i=0;i<numconcts;i++) {
              pthread_mutex_lock(&mutex[id][i]);
              pthread_cond_signal(&cond[id][i]);
              pthread_mutex_unlock(&mutex[id][i]);
            }

            /* ReaderThread routine is run as a thread (HTTP1.1) so wait for*/
            /* the thread(s) to return.  In pipelining, since all files are */
            /* requested at the same time, wait for all threads to return.  */

            if(debugflag)
              printf("Surgeclient %d: Waiting for ALL readers to complete\n",
                     systemno);
            while(numreaders > 0) {
              statusflag = 0;           /* A nasty little hack since        */
              for(i=0;i<MAXCONCTS;i++)  /* pthreads can miss signals!!!     */
                if(readerstatus[id][i] == 1) statusflag = 1;
              if(statusflag == 0) {
                numreaders = 0;
                break;
              }
              if (pthread_cond_wait(&cond[id][MAXCONCTS], 
                                    &mutex[id][MAXCONCTS]) != 0) {
                printf("Surgeclient:  pthread_cond_wait FAILED\n");
                pthread_mutex_unlock(&mutex[id][MAXCONCTS]);
              }
              numreaders--;
            }
          } /* End if numfiles > 1 */
        } /* End of file read sequence */

      /* Sleep for OFF time after all files in the object are retrieved     */

      if(read(offfifo, offbuff, sizeof(offbuff)) < 0)
        printf("SURGEclient %d: Error reading from offfifo\n", systemno);
      if(debugflag)
        printf("Surgeclient %d:  Client %d Sleeping for %d ms\n",
               systemno, id, atoi(offbuff));
      usleep(1000 * atoi(offbuff));

      /* For HTTP 1.1:  If objects for current "open connection" period     */
      /* have been transferred, then close all of open connections.  numobjs*/
      /* tracks the number of objects for "open connection" period.  If all */
      /* objects have not been selected then be sure that connections are   */
      /* still open - if not, reopen them.                                  */

      if(numobjs == 1) {         /* If all objects xfered, close connects */
        for(i=0;i<numconcts;i++)
          if(reader[i].conctno >= 0) {
            if((close(reader[i].conctno)) < 0)
             printf("SURGEclient %d: Client %d Error closing connection %ld\n",
                     systemno, id, i);
            reader[i].conctno = -1;
            if(debugflag)
              printf("SURGEclient %d: Client %d Closed connection %ld\n", 
                   systemno, id, i);
          }
        numconcts = 0;
        numobjs = 0;
      } else {  /*If all objects not xfered,be sure all connects still open*/
        for(i=0;i<numconcts;i++)
          conctstat = Checkconnect(&reader[i]);
        numobjs--;  /* decrement # objects in this "open connection" period*/
      }
 
    } /* End object select while loop */

  pthread_exit(0);
  return(NULL);

}   /* End ClientThread */

/****************************************************************************/
/* This routine is where the action takes place.  It sets up the HTTP       */
/* command, issues the command, reads the data from the open connection and */
/* takes a time stamp when the transaction finishes and then loads the      */
/* results array.  For pipelined reads and writes:  request the base file   */
/* and then read it.  Then request all of the remaining files and read them */
/****************************************************************************/

void * ReaderThread(void * readdat)
{
  int i, c, thrdid, readid, numfiles, filesread, stopflg, conctstat, conctid;
  long objindx, fileindx[OBJLIMIT], stimeus, stimes, length;
  struct timeval endtime;
  struct timezone tz;
  char rbuff[READBUFFSZ];
  char outbuff[10];
  char contlength[20];
  char headbuff[512];
  char parseline1[] = "chunked";
  char parseline2[] = "Content-Length";
  char parseline3[] = "\r\n\r\n";
  char parseline4[] = "HTTP";
  char *lineptr;
  char *headptr;
  int debugflag = 0;

  readid = ((struct readerdata *)readdat)->readerno;
  thrdid = ((struct readerdata *)readdat)->threadno;

  if(debugflag)
    printf("SURGEclient %d: Started Reader %d\n", thrdid, readid);

  while(1) { /* Readers run until timer expires or all files have been read */
    
    /* Wait until the ClientThread signals that its time to start reading   */

    if (pthread_cond_wait(&cond[thrdid][readid], &mutex[thrdid][readid]) != 0){
      printf("Surgeclient:  pthread_cond_wait FAILED\n");
      pthread_mutex_unlock(&mutex[thrdid][readid]);
    }

    /* Each time this thread gets a signal to start from the ClientThread it*/
    /* is responsible for requesting and reading one object from the        */
    /* server.  Begin by loading the object data from the readdat structure */

    objindx = ((struct readerdata *)readdat)->objectno;
    numfiles = ((struct readerdata *)readdat)->nofiles;
    stimeus = ((struct readerdata *)readdat)->startus;
    stimes = ((struct readerdata *)readdat)->starts;
    conctid = ((struct readerdata *)readdat)->conctno;
    for(i=0;i<numfiles;i++) {
      fileindx[i] = ((struct readerdata *)readdat)->fileno[i];
      if(debugflag)
        printf("SURGEclient %d:  Thread %d Reader %d Object %ld Fileno %ld\n",
               systemno, thrdid, readid, objindx,object[objindx][fileindx[i]]);
    }

    /* Issue HTTP GET call for the specified file to the specified          */
    /* In the case of pipelined HTTP/1.1, request all files at once.        */
    
    filesread = 0;
    Writerequest(filesread, readdat);

    /* Read the file from the connection.  For HTTP1.1 chunked data format  */
    /* is supported.  So, read chunks until the "0 CRLF {footer CRLF} CRLF" */ 
    /* is seen and then consider the transfer to be complete.               */
    /* For HTTP 1.1 must be able to read data from persistent concts        */
    /* Read data from connection into the rbuff and then parse to see if    */
    /* chunked encoding is being used - if yes then read until the "0 CRLF  */
    /* {footer CRLF} CRLF" is seen otherwise use the content length field   */
    /* and read that number of bytes of message and then end.  By rfc2068   */
    /* "Messages MUST NOT include both a Content-Length header field and    */
    /* the "chunked" transfer coding.  If both are received, the            */
    /* Content-Length MUST be ignored."                                     */

    /* Start out by reading the first rbuff of data from the connection     */

    c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
    gettimeofday(&endtime,&tz); /* Get time stamp for end of first read     */
    lineptr = rbuff;
    if(debugflag)
      printf("Read first buffer of data size = %d\n",c);

    /* Find the start of first message and place lineprt where data starts  */

    if((lineptr = strstr(lineptr, parseline3)) == NULL) {
      if(debugflag) {
        printf("Surgeclient %d: Thread %d Reader %d Object %ld Fileno %ld",
               systemno, thrdid, readid, objindx,
               object[objindx][fileindx[filesread]]);
        printf(" Can't find first message start - aborting object transfer\n");
      }
      filesread = numfiles;
      if(close(conctid) < 0)
        printf("SURGEclient %d: Thrd %d Rdr %d Error closing connect %d\n",
               systemno, thrdid, readid, conctid);
      ((struct readerdata *)readdat)->conctno = -1;
    } else {                   /* Store header for pipelinig                */
      size_t sz = lineptr + 4 - rbuff; if (sz>=512) sz=511;
      strncpy(headbuff,rbuff,sz);
      lineptr += 4;  /* Place pointer after the CRLF CRLF in msg header     */
    }

      /* For pipelined HTTP, read all of the files requested. Check after   */
      /* each file read that the connection is still opened. If not, reopen */
      /* and resend requests for the remaining files.                       */

    while(filesread < numfiles) {

      /* Parse the header from first data pkt if chunked encoding is used   */

      if(strstr(headbuff, parseline1) != NULL) {
        if(debugflag) {
          printf("Surgeclient %d: Thread %d Reader %d Object %ld Fileno %ld",
                 systemno, thrdid, readid, objindx, 
                 object[objindx][fileindx[filesread]]);
          printf(" Chunked data transfer\n");
        }

        /* Look for the end string in each packet - begin at body start of  */
        /* first packet and check the length of the next chunk.  Then look  */
        /* for end string in subsequent chunks until the end of the packet  */
        /* Get additional packets of data when necessary until end string   */
        /* is found.                                                        */

        length = 0;
        stopflg = 0;
        while(stopflg == 0 && c != -1) {  /* Cycle through chunks until EOF */
          if(length + lineptr - rbuff < c - 1) {
            lineptr += length;     /* Set lineptr to 1st char of next chnk  */
            i = 0;
            bzero(contlength,20);
            while(rbuff[i + lineptr - rbuff] != '\n') {  /* Read chunk size */
              contlength[i] = rbuff[i + lineptr - rbuff];
              i++;
              /*  If size is on array boundry get next packet of data       */
              if(i + lineptr - rbuff == READBUFFSZ) {
                c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                gettimeofday(&endtime,&tz);   /* Time stamp the end of read */
                i = 0;
                if(debugflag > 1)
                  printf("Read1 chunk buffer of data size = %d\n",c);
              }
            }  /* End while of reading next chunk size */
            length = strtol(contlength, NULL, 16);   /* Chunk size is hex   */
            if(debugflag > 1)
              printf("Next chunk size = %ld hex %s\n",length,contlength);
            lineptr += 3 + i; /* put lineptr at start of data of next chunk */
            if(length == 0) {   /* Look for CRLF CRLF string to end data. If*/
              lineptr -= 3 + i; /* its not in this rbuff the get one more.  */
              if((lineptr = strstr(lineptr, parseline3)) == NULL) {
                c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                gettimeofday(&endtime,&tz);   /* Time stamp the end of read */
                lineptr = rbuff;           /* put lineptr at start of rbuff */
                if(debugflag > 1)
                  printf("Read2 chunk buffer of data size = %d\n",c);
              }
              if(c != -1) {  /* Write out results if reading was successful */
                /*  Get the output sequence # and write results structure   */
                if(read(outfifo, outbuff, sizeof(outbuff)) < 0)
                  printf("SURGEclient %d: Error reading from outfifo\n", 
                         systemno);
                loadoutput(atol(outbuff), objindx, fileindx[filesread], 
                           thrdid, stimeus, stimes, endtime.tv_usec, 
                           endtime.tv_sec);
                if(debugflag) {
                  printf("Surgeclient %d: Thread %d Reader %d Object %ld",
                         systemno, thrdid, readid, objindx);
                  printf(" Finished reading file %ld\n",
                         object[objindx][fileindx[filesread]] );
                }
                
                /* Now get the header for the next file which can lay       */
                /* across a buffer boundry so be careful in constructing    */
                /* headbuff and place the lineptr at the start of the new   */
                /* data.  NOTE:  PIPELINE CHUNKS HAVE NOT BEEN TESTED!!!!   */

                if(filesread + 1 < numfiles) {
                  bzero(headbuff,300);
                  headptr = lineptr; /* put headptr at start of next header */
                  if(lineptr >= rbuff + c) { /*Get new pkt if headptr at end*/
                    
                    /* Before we can get a new packet, we much check to     */
                    /* see if the server has closed the connection.  If     */
                    /* so, reopen it and rerequest the remaining files      */

                    if((conctstat = Checkconnect(readdat)) == 1) {
                      Writerequest(filesread, readdat);
                      conctid = ((struct readerdata *)readdat)->conctno;
                    }
                    c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                    gettimeofday(&endtime,&tz);
                    headptr = rbuff;
                    if(debugflag) {
                      printf("Header is in next packet\n");
                      printf("Read3 chunk buffer of data size = %d\n",c);
                    }
                  } /* End if headptr at end of rbuff */
                  lineptr = headptr;
                  if((lineptr = strstr(lineptr, parseline3)) != NULL) {
			           size_t sz = lineptr + 4 - headptr; if (sz>=512) sz=511;
                    strncpy(headbuff,headptr,sz);
                    lineptr += 4;      /* put lineptr at start of data      */
                    if(debugflag) printf("Header in Page\n");
                  } else {   /* Header has gone over rbuff boundry          */
                    i = c - (headptr - rbuff);
                    if(i > 300) i = 300; /* Be sure to keep the index within*/
                    if(i < 0) i = 0;     /* the length of headbuff          */
                    strncpy(headbuff,headptr,i>511?511:1);   /* Copy start of header  */
                    c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                    gettimeofday(&endtime,&tz);     /* Time stamp the read  */
                    strncat(headbuff,rbuff,300 - i);/* add data to headbuff */
                    if((headptr = strstr(headbuff, parseline4)) != NULL) {
                      if((headptr = strstr(headptr, parseline3)) != NULL) {
                        if((lineptr = strstr(rbuff, parseline3)) != NULL) {
                          if(debugflag) printf("Header over Boundary\n");
                          lineptr += 4;
                        } else {
                          c = -1;
                          if(debugflag) printf("Error finding data start\n");
                        }
                      } else {
                        c = -1;
                        if(debugflag) printf("Error finding next headr end\n");
                      }
                    } else {
                      c = -1;
                      if(debugflag) printf("Error finding next headr start\n");
                    }
                  }  /* End else if header has gone over rbuff boundry      */
                } /* End if filesread < numfiles */
              } /* End if c != -1 to write results */
              stopflg = 1;      /* Chunk data end string found so end read  */
            } /* End if length == 0 */
          } else {   /* If chunk size is beyond current rbuff get next pkt  */
            length -= c - (lineptr - rbuff);
            c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
            gettimeofday(&endtime,&tz);     /* Time stamp the end of read   */
            lineptr = rbuff;
            if(debugflag)
              printf("Read chunk 4 buffer of data size = %d\n",c);
          }
        } /* End while of cycle through chunks */



      } else {  /* If this else is taken then chunks are NOT being used   */

        /********************************************************************/
        /* When chunks not used, use content length to decide when to end   */
        /********************************************************************/

        if((headptr = strstr(headbuff, parseline2)) != NULL) {
          i = 0;          /* A really ugly way to parse out content length  */
          bzero(contlength,20);
          while(headbuff[i + headptr + 16 - headbuff] != '\n') {
            contlength[i] = headbuff[i + headptr + 16 - headbuff];
            i++;
          }
          length = strtol(contlength, NULL, 10); /*Content length is decimal*/
          if(debugflag) {
            printf("Surgeclient %d: Thread %d Readr %d Object %ld Fileno %ld",
                   systemno, thrdid, readid, objindx,
                   object[objindx][fileindx[filesread]]);
            printf(" Content length used length = %ld\n", length);
          }
          
          /*   Test to see if end of message is in current rbuff of data    */
          
          length -= c - (lineptr - rbuff);
          if(debugflag > 1)
            printf("Data remaining after this packet = %ld\n", length);
          
          /*   If data still remains to be read, read until length = 0      */
          
          while(length > 0 && c > 0) {
            c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
            gettimeofday(&endtime,&tz);   /* Time stamp the end of read     */
            if(debugflag > 1)
              printf("Length = %ld Value of next read is %d\n", length,c);
            length -= c;
            if(debugflag > 1)
              printf("Read Cont 1 buffer of data size = %d\n",c);
          }
          if(c != -1) {
            /* Get the output sequence #, then write results structure      */
            if(read(outfifo, outbuff, sizeof(outbuff)) < 0)
              printf("SURGEclient %d: Thread %d Error reading from outfifo\n", 
                     systemno, thrdid);
            loadoutput(atol(outbuff), objindx, fileindx[filesread], 
                       thrdid, stimeus, stimes, endtime.tv_usec, 
                       endtime.tv_sec);
            if(debugflag) {
              printf("Surgeclient %d: Thread %d Reader %d Object %ld",
                     systemno, thrdid, readid, objindx);
              printf(" Finished reading file %ld\n",
                     object[objindx][fileindx[filesread]] );
            }

            /* Now get the header for the next file which can lay           */
            /* across a buffer boundry so be careful in constructing        */
            /* headbuff and place the lineptr at the start of the new       */
            /* data.                                                        */
            
            if(filesread + 1 < numfiles) {
              bzero(headbuff,300);
              headptr = rbuff + c + length; /* Put headptr at start of next */
              if(headptr >= rbuff + c) {/* Get new pkt if ptr at end of buff*/

                /* Before we can get a new packet, we much check to see if  */
                /* the server has closed the connection.  If so, reopen it  */
                /* and rerequest the remaining files.                       */

                if((conctstat = Checkconnect(readdat)) == 1) {
                  Writerequest(filesread, readdat);
                  conctid = ((struct readerdata *)readdat)->conctno;
                }
                c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                gettimeofday(&endtime,&tz);   /* if header of the next file */
                headptr = rbuff;              /* next packet the get it     */
                if(debugflag) printf("Header is in next packet\n");
              } /* End if headptr at end of rbuff */
              lineptr = headptr;
              if((lineptr = strstr(lineptr, parseline3)) != NULL) {
                size_t sz = lineptr + 4 - headptr; if (sz>=512) sz=511;
                strncpy(headbuff,headptr,sz);
                lineptr += 4;      /* put lineptr at start of data          */
                if(debugflag) printf("Header in Page\n");
              } else {   /* Header has gone over rbuff boundry              */
                i = c - (headptr - rbuff);
                if(i > 300) i = 300; /* Be sure to keep the index with in   */
                if(i < 0) i = 0;     /* the length of headbuff              */
                strncpy(headbuff,headptr,i>511?511:i);   /* Copy start of header      */
                c = fillreadbuff(conctid,rbuff,sizeof(rbuff));
                gettimeofday(&endtime,&tz);  /* Time stamp the end of read  */
                strncat(headbuff,rbuff,300 - i);   /* add data to headbuff  */
                if((headptr = strstr(headbuff, parseline3)) != NULL) {
                  if((lineptr = strstr(rbuff, parseline3)) != NULL) {
                    if(debugflag) printf("Header over rbuff Boundary\n");
                    lineptr += 4;
                  } else {
                    c = -1;
                    if(debugflag) printf("Error finding data start\n");
                  }
                } else {
                  c = -1;
                  if(debugflag) printf("Error finding next header\n");
                }
              }
            } /* End if filesread < numfiles */
          } /* End if c != -1 to write results */
        } else {
          if(debugflag) {
            printf("SURGEclient %d: Thread %d Readr %d Objct %ld Fileno %ld",
                   systemno, thrdid, readid, objindx, 
                   object[objindx][fileindx[filesread]]);
            printf("  Can't find Chunk Size or Content Length - abort obj\n");
          }
          c = -1;
        } /* End else when message start can't be found */
      } /* End of else when content length is used */
      
      /* If the last file was successfully read, increment the filesread    */
      /* counter and reset the start times to the last end time.  If c = -1 */
      /* then there was a real problem so close connect and rereques the    */
      /* remaining files.                                                   */
      
      if(c != -1) {   /* If c != -1 then the file was successfully read     */
        filesread++;  /* Increment the # of files read from pipe            */
        stimeus = endtime.tv_usec;
        stimes = endtime.tv_sec;
      } else {
        if(debugflag) {
          printf("SURGEclient %d: Thread %d Readr %d Objct %ld Fileno %ld",
                 systemno, thrdid, readid, objindx, 
                 object[objindx][fileindx[filesread]]);
          printf("  Readbuff problem - abort object read\n");
        }
        filesread = numfiles;
        if(close(conctid) < 0)
          printf("SURGEclient %d: Thrd %d Rdr %d Error closing connect %d\n",
                 systemno, thrdid, readid, conctid);
        ((struct readerdata *)readdat)->conctno = -1;
      }
    } /* End while filesread < numfiles for pipelined reading of an object  */

    /* Send signal to ClientThread that reader is finished reading file set */

    pthread_mutex_lock(&mutex[thrdid][MAXCONCTS]); /* Signal to ClientThread*/
    readerstatus[thrdid][readid] = 0;  /* set status to "not reading"       */
    pthread_cond_signal(&cond[thrdid][MAXCONCTS]);
    pthread_mutex_unlock(&mutex[thrdid][MAXCONCTS]);
    bzero(rbuff,READBUFFSZ);   /* Clear rbuff else we can get bad data      */
    bzero(headbuff,300);       /* Clear headbuff for the same reason        */

  } /* End the continuous ReaderThread while loop */

  return(NULL);

} /* End ReaderThread */

/****************************************************************************/
/* This routine checks the TCP connection for the specified reader.  If the */
/* connection is closed, it reopens it.  It returns a 0 if it did not have  */
/* to reopen the connection, it returns a 1 if it did have to reopen.       */
/* The inputs to this routine are the client thread number and the reader   */
/* number which specify which connection to check.                          */
/****************************************************************************/

int Checkconnect(void * readdat)
{
  int conctid, thrdid, readid;
  fd_set fdvar;
  struct timeval timeout;
  int debugflag = 0;

  conctid = ((struct readerdata *)readdat)->conctno;
  readid = ((struct readerdata *)readdat)->readerno;
  thrdid = ((struct readerdata *)readdat)->threadno;
  timeout.tv_sec = 0;         /* Return immediately with value from select  */
  timeout.tv_usec = 0;

  if(conctid == -1) {/* conct can be set to -1 when aborted in ReaderThread */
    while(conctid == -1)  /* Reopen connect */
      conctid = connectsrvr();
    ((struct readerdata *)readdat)->conctno = conctid;
    return(1);  /* Did have to reopen connection    */
  } else { /* If connect is supposed to be open, sse select to test status  */
    FD_ZERO(&fdvar);
    FD_SET(conctid, &fdvar);
    if(select(conctid+1, &fdvar, NULL, NULL, &timeout) > 0) {
      if((close(conctid)) < 0)
        printf("SURGEclient %d: Error closing connect %d\n", systemno,conctid);
      conctid = -1;
      while(conctid == -1)  /* Reopen connect */
        conctid = connectsrvr();
      ((struct readerdata *)readdat)->conctno = conctid;
      if(debugflag) {
        printf("SURGEclient %d: Thrd %d Readr %d", systemno, thrdid, readid);
        printf(" Reopened conct %d\n", conctid);
      }
      return(1);  /* Did have to reopen connection    */
    } else {
      return(0);  /* Didn't have to reopen connection */
    }
  }
    
} /* End Checkconnect */

/****************************************************************************/
/* This routine is used by the pipelined version of client11.c in order to  */
/* wirte the multiple HTTP requests in a single buffer.  It takes as input  */
/* the reader structure which contains the file names, and where to start   */
/* makeing requests of the files.                                           */
/****************************************************************************/

void Writerequest(int start, void * readdat)
{
  int i, thrdid, readid, conctid, numfiles;
  long objindx;
  char wbuff[WRITEBUFFSZ];
  char objbuff[10];
  char url[URLSIZE];
  int debugflag = 0;


  readid = ((struct readerdata *)readdat)->readerno;
  thrdid = ((struct readerdata *)readdat)->threadno;
  objindx = ((struct readerdata *)readdat)->objectno;
  numfiles = ((struct readerdata *)readdat)->nofiles;
  conctid = ((struct readerdata *)readdat)->conctno;
  bzero(wbuff, WRITEBUFFSZ);

  /* Generate complete URL string by inserting file number in http string   */
  /* For pipeline requests, place all requests in the write buffer          */

  for(i=start;i<numfiles;i++) {
    bzero(url, URLSIZE);
    strcpy(url, httpstart);
    sprintf(objbuff,"%ld", 
            object[objindx][((struct readerdata *)readdat)->fileno[i]]);
    strncat(url, &objbuff[0], sizeof(objbuff));
    strncat(url, &httpend[0], sizeof(httpend));
    if(i == 0) {
      strcpy(wbuff, url);
    } else {
      strncat(wbuff, &url[0], sizeof(url));
    }
  }
  if(debugflag) {
    printf("SURGEclient %d: Thrd %d Readr %d Object %ld",
           systemno, thrdid, readid, objindx);
    printf(" Requested fileidxs %d to %d\n", start, numfiles);
  }

  /* Issue HTTP GET call for the specified file to the specified connectin  */

  write(conctid,wbuff,strlen(wbuff));

} /* End Writerequest */

