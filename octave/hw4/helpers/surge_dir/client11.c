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
/*             THESE ARE THE HTTP/1.1 COMPLIANT CLIENTTHREAD ROUTINES.      */
/*             THIS VERSION OF READERTHREADS DOES NOT IMPLEMENT PIPELINED   */
/*             REQUESTS AND READS.                                          */
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
#include "client11.h"

void* ClientThread(void *);
void* ReaderThread(void *);

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
  int numobjs, numconcts, numreaders, conct[MAXCONCTS], statusflag;
  int id, stopobjloop, stopfileloop;
  long i,objindx, fileindx, count;
  struct timeval starttime, timeout;
  struct timezone tz;
  char seqbuff[10];
  char offbuff[10];
  char cntbuff[10];
  fd_set fdvar;
  int debugflag = 0;
  pthread_t readerid[MAXCONCTS];
  struct readerdata reader[MAXCONCTS];

  /* Begin by initializing all of the variables used in the ClientThread    */

  numobjs = 0;     /* for HTTP1.1 number of objects to get before new concts*/
  count = 0;       /* count is used to store the object index from OBJFIFO  */
  id = *(int *)threadno; /* assign local thread id                          */
  stopobjloop = 0; /* stopobjloop = 0 until the object sequence is complete */
  numconcts = 0;   /* the number of open TCP connections                    */
  timeout.tv_sec = 0;  /* timeout used in the select command to test concts */
  timeout.tv_usec = 0;

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
    conct[i] = -1;   /* initialize socket connection file descriptors to -1 */
    readerstatus[id][i] = 0; /* set status of readers to 0 - reader is ready*/
    reader[i].readerno = i;  /* Initialize the reader number                */
    reader[i].threadno = id; /*Initialize the Readers ClientThread number   */
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
	  printf("SURGEclient %d: Objs in open connect period=%d\n",
		 systemno, numobjs);
      }

      /* Get the file names associated with the selected object and request */
      /* them from the server. If HTTP1.1 then use multiple connections     */
      
      fileindx = 0;     /* Begin index for each file in an object at 0      */
      stopfileloop = 0; /* stopfileloop = 0 until object has been transferd */
      numreaders = 0;         /* allow up to MAXCONCTS readers to be used   */

      while (stopfileloop == 0 && stopobjloop == 0)
	{

	  /* Read the specified file(s) using the specified connection(s)   */

	  while(numreaders < MAXCONCTS && stopfileloop == 0)
	    {

	      /* Time stamp the start of the transaction                    */
  
	      gettimeofday(&starttime,&tz);

	      /* Open a connection to HTTP server up to MAXCONCTS           */
	      if(numconcts < MAXCONCTS && fileindx >= numconcts) { 
		while(conct[numconcts] == -1) /* Keep trying to get conct   */
		  conct[numconcts] = connectsrvr();
		if(debugflag)
		  printf("SURGEclient %d: Client %d Opened connect %d\n",
			 systemno, id, numconcts);
		numconcts++;
	      }

	      i = 0;/* Set up reader data for a file and start reader thread*/
	      /* For HTTP1.1 we can have more than 1 reader - find next one */
	      while(readerstatus[id][i]== 1) {  /* find an available reader */
		i++;
		if(i == MAXCONCTS)   /* Don't test past end of status array */
		  i = 0;
	      }
	      readerstatus[id][i] = 1;          /* set status to "reading"  */
	      numreaders++;                     /* pass all required data   */
	      reader[i].conctno = conct[i];
	      reader[i].objectno = objindx;
	      reader[i].fileno = fileindx;
	      reader[i].startus = starttime.tv_usec;
	      reader[i].starts = starttime.tv_sec;

              /* Signal the appropriate Reader thread that it's time to     */
              /* read the file specified in it's reader structure.          */

              pthread_mutex_lock(&mutex[id][i]);
              pthread_cond_signal(&cond[id][i]);
              pthread_mutex_unlock(&mutex[id][i]);


	      /* Get the next file name from the current object, if is a -1 */
	      /* then stop the file select loop otherwise get the next file */
	      /* up to the number of connections that are available         */

	      fileindx++;
              if(object[objindx][fileindx] == -1 || fileindx == OBJLIMIT)
                stopfileloop = 1;

	    } /* End reader start loop */

	  /* ReaderThread routine is run as a thread (HTTP1.1) so wait      */
	  /* for the thread(s) to return.  If some files for current object */
	  /* still have to be retrieved, then wait for the next reader to   */
	  /* return.  If all files in current object have been read, wait   */
	  /* for all readers to return.                                     */

	  if(stopfileloop == 0) {
	    if(debugflag)
	      printf("Surgeclient %d: Waiting for ANY reader to complete\n", 
		     systemno);
              if (pthread_cond_wait(&cond[id][MAXCONCTS], 
				    &mutex[id][MAXCONCTS]) != 0) {
                printf("Surgeclient:  pthread_cond_wait FAILED\n");
                pthread_mutex_unlock(&mutex[id][MAXCONCTS]);
              }
	    numreaders--;
	  } else {
	    if(debugflag)
	      printf("Surgeclient %d: Waiting for ALL readers to complete\n",
		     systemno);
	    while(numreaders > 0) {
	      statusflag = 0;           /* A nasty little hack since      */
	      for(i=0;i<MAXCONCTS;i++)  /* pthreads can miss signals!!!   */
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
	  }
	  
	} /* End of file read loop    */

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
	  if(conct[i] >= 0) {
	    if((close(conct[i])) < 0)
	      printf("SURGEclient %d: Error closing connection %d\n",
		     systemno,conct[i]);
	    conct[i] = -1;
	    if(debugflag)
	      printf("SURGEclient %d: Client %d Closed connection %ld\n", 
		   systemno, id, i);
	  }
	numconcts = 0;
	numobjs = 0;
      } else {  /*If all objects not xfered,be sure all connects still open*/
	for(i=0;i<numconcts;i++) {
	  FD_ZERO(&fdvar);            /* Use select to test if connection  */
	  FD_SET(conct[i], &fdvar);   /* is still available for read/write */
	  if(select(conct[i] + 1, &fdvar, NULL, NULL, &timeout) > 0) {
	    if((close(conct[i])) < 0) /* Close SURGE side of connect       */
	      printf("SURGEclient %d: Error closing connection %d\n",
		     systemno,conct[i]);
	    conct[i] = -1;
	    while(conct[i] == -1)
	      conct[i] = connectsrvr(); /* Reopen the connection to server */
	    if(debugflag)
	      printf("SURGEclient %d: Client %d Reopened connect %ld\n",
		     systemno, id, i);
	  }
	}
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
/* results array.                                                           */
/****************************************************************************/

void * ReaderThread(void * readdat)
{
  int i, c, thrdid, readid, conct;
  long objindx, fileindx, stimeus, stimes, length;
  struct timeval endtime;
  struct timezone tz;
  char rbuff[READBUFFSZ];
  char objbuff[10];
  char outbuff[10];
  char url[URLSIZE];
  char contlength[20];
  char parseline1[] = "chunked";
  char parseline2[] = "Content-Length";
  char parseline3[] = "\r\n\r\n";
  char *lineptr;
  int debugflag = 0;

  readid = ((struct readerdata *)readdat)->readerno;
  thrdid = ((struct readerdata *)readdat)->threadno;
  if(debugflag)
    printf("Client %d Started Reader %d\n", thrdid, readid);

  while(1) { /* Readers run until timer expires or all files have been read */
    
    /* Wait until the ClientThread signals that its time to start reading   */

    if (pthread_cond_wait(&cond[thrdid][readid], &mutex[thrdid][readid]) != 0){
      printf("Surgeclient:  pthread_cond_wait FAILED\n");
      pthread_mutex_unlock(&mutex[thrdid][readid]);
    }

    conct = ((struct readerdata *)readdat)->conctno;
    objindx = ((struct readerdata *)readdat)->objectno;
    fileindx = ((struct readerdata *)readdat)->fileno;
    stimeus = ((struct readerdata *)readdat)->startus;
    stimes = ((struct readerdata *)readdat)->starts;

    if(debugflag)
      printf("Reader: Client %d Thread %d Reader %d Object %ld Fileno %ld\n",
	     systemno, thrdid, readid, objindx, object[objindx][fileindx]);

    /* Generate complete URL string by inserting file number in http string */

    strcpy(url, httpstart);
    sprintf(objbuff,"%ld", object[objindx][fileindx]);
    strncat(url, &objbuff[0], sizeof(objbuff));
    strncat(url, &httpend[0], sizeof(httpend));

    /* Issue HTTP GET call for the specified file to the specified connectin*/

    write(conct,url,strlen(url));

    /* Read the file from the connection.  For HTTP1.1 chunked data format  */
    /* is supported.  So, read chunks until the "0 CRLF {footer CRLF} CRLF" */ 
    /* is seen and then consider the transfer to be complete.               */

    c = 1;
    
    /* For HTTP 1.1 must be able to read data from persistent concts        */
    /* Read data from connection into the rbuff and then parse to see if    */
    /* chunked encoding is being used - if yes then read until the "0 CRLF  */
    /* {footer CRLF} CRLF" is seen otherwise use the content length field   */
    /* and read that number of bytes of message and then end.  By rfc2068   */
    /* "Messages MUST NOT include both a Content-Length header field and    */
    /* the "chunked" transfer coding.  If both are received, the            */
    /* Content-Length MUST be ignored."                                     */

    /* Start out by reading the first rbuff of data from the connection     */

    c = fillreadbuff(conct,rbuff,sizeof(rbuff));

    if(c != -1) {    /* Be sure that the first rbuff of data has been read  */

      /* Parse the HTTP header from first data if 'chunked' encoding is used*/

      if(strstr(rbuff, parseline1) != NULL) {
	if(debugflag) {
	  printf("Surgeclient %d: Thread %d Reader %d Object %ld Fileno %ld",
		 systemno, thrdid, readid, objindx, object[objindx][fileindx]);
	  printf(" Chunked data transfer\n");
	}

	/* Look for the end string in each packet - begin at body start of  */
	/* first packet and check the length of the next chunk.  Then look  */
	/* for end string in subsequent chunks until the end of the packet. */
	/* Get additional packets of data when necessary until end string   */
	/* is found.                                                        */

	if((lineptr = strstr(rbuff, parseline3)) != NULL) { /*Find body strt*/
	  lineptr += 4;  /* Move pointer beyond the first CRLF CRLF in the  */
	  length = 0;    /* body of the first buffer/packet of data.        */
	  while(c > 0) { /* Cycle through chunks until end sequence is found*/
	    if(length + lineptr - rbuff < c - 1) {
	      lineptr += length;   /* Set lineptr to 1st char of next chunk */
	      i = 0;               /* Get the next chunk length             */
	      bzero(contlength,20);
	      while(rbuff[i + lineptr - rbuff] != '\n') {
		contlength[i] = rbuff[i + lineptr - rbuff];
		i++;
		/*If size is on array boundry get next packet of data       */
		if(i + lineptr - rbuff == READBUFFSZ) {
		  c = fillreadbuff(conct,rbuff,sizeof(rbuff));
		  lineptr = rbuff;
		  i = 0;
		}
	      } /* End while of reading next chunk size */
	      length = strtol(contlength, NULL, 16);
	      /* printf("Next chunk size = %ld hex %s\n",length,contlength);*/
	      lineptr += 3 + i;
	      if(length == 0) {   /* Look for CRLF CRLF string to end data  */
		lineptr -= 3 + i; /* If not in this rbuff the get one more  */
		if((lineptr = strstr(lineptr, parseline3)) == NULL)
		  c = fillreadbuff(conct,rbuff,sizeof(rbuff));
		c = 0;        /* Chunk data end string found so end reading */
	      }
	    } else { /* If chunk size is beyond current rbuff get new rbuff */
	      length -= c - (lineptr - rbuff);
	      c = fillreadbuff(conct,rbuff,sizeof(rbuff));
	      lineptr = rbuff;
	    }
	  } /* End while of cycle through chunks */
	  
	} else {
	  if(debugflag)
	   printf("Surgeclient %d: Thread %d Rdr %d Cant Find Chnk Msg strt\n",
		   systemno, thrdid, readid);
	  c = -1;
	}
      } else {  /* If this else is taken then chunks are NOT being used     */
	
	/* When chunks not used, use content length to decide to end reading*/
	
	if((lineptr = strstr(rbuff, parseline2)) != NULL) {
	  i = 0;        /* A really ugly way to parse out content length    */
	  while(rbuff[i + lineptr + 16 - rbuff] != '\n') {/*+16 skips prsln2*/
	    contlength[i] = rbuff[i + lineptr + 16 - rbuff];
	    i++;
	  }
	  length = strtol(contlength, NULL, 10); /*Content length is decimal*/
	  if(debugflag) {
	    printf("Surgeclient %d Thread %d Reader %d Object %ld Fileno %ld",
		   systemno, thrdid, readid,objindx,object[objindx][fileindx]);
	    printf(" Content length used length =%ld\n", length);
	  }

	  /* Test to see if end of message is in first packet               */
	  
	  if((lineptr = strstr(rbuff, parseline3)) != NULL) {
	    length -= strlen(rbuff) - (lineptr + 4 - &rbuff[0]);
	    /*printf("Data remaining after 1st packet = %ld\n", length);*/
	  } else {
	    if(debugflag)
	      printf("SURGEclient: ReadThrd can't find message start\n");
	    c = -1;
	  }

	  /* If data still remains to be read, read until length = 0        */
	
	  while(length > 0 && c > 0) {
	    c = fillreadbuff(conct,rbuff,sizeof(rbuff));
	    /* printf("Length = %ld Value of next read is %d\n", length, c);*/
	    if(c > 0)
	      length -= c;
	  }
	} else {
	  if(debugflag)	{
            printf("SURGEclient %d Thread %d Reader %d Object %ld Fileno %ld",
		   systemno, thrdid, readid,objindx,object[objindx][fileindx]);
	    printf("  Can't find Message Size\n");
          }
	  c = -1;
	}
      } /* End of else when content length is used */
    } /* End if for first rbuff being read */

    /* Time stamp the end of the transaction                                */
  
    gettimeofday(&endtime,&tz);

    /* Write results to result structure                                    */

    if(c != -1)  /* Don't load results on read error                        */
      {          /* Get the output sequence #, then write results           */
	if(read(outfifo, outbuff, sizeof(outbuff)) < 0)
	  printf("SURGEclient %d: Error reading from outfifo\n", 
		 systemno);
	loadoutput(atol(outbuff), objindx, fileindx, thrdid,
		   stimeus, stimes, endtime.tv_usec, endtime.tv_sec);
      }

    /* Send signal to ClientThread that reader is finished reading this file*/

    pthread_mutex_lock(&mutex[thrdid][MAXCONCTS]); /* Signal to ClientThread*/
    readerstatus[thrdid][readid] = 0;  /* set status to "not reading"       */
    pthread_cond_signal(&cond[thrdid][MAXCONCTS]);
    pthread_mutex_unlock(&mutex[thrdid][MAXCONCTS]);
    bzero(rbuff,READBUFFSZ);   /* Clear rbuff else we can get bad data      */
    
  } /* End the continuous ReaderThread while loop */

  return(NULL);

} /* End ReaderThread */



