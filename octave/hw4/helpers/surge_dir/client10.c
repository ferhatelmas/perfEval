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
/*  Title:     Client10 - HTTP/1.0 compliant Client thread                  */
/*  Revision:  1.0         5/7/98                                           */
/*                                                                          */
/*  This is the part of the SURGE code which actually makes document        */
/*  requests.  It requires Pthreads be installed in order to operate.       */
/*  See the SURGE-HOW-TO in order to compile and run properly.              */
/*                                                                          */
/*  Surge is set up so that you can multiple Surge client processes can be  */
/*  run on a single system.  This code used to be part of Surgeclient.c but */
/*  has been separated so that multiple HTTP flavors can be supported.  This*/
/*  is the code that supports the HTTP/1.0 flavor.  This code is meant to   */
/*  be compiled as a module for Surgeclient.c.  The two routine contained   */
/*  here are what actually make the requests for files from the server.     */
/*  The ClientThread routine is what is referred to as a user-entity.  The  */
/*  ReaderThread routine is what actually makes a single document request.  */
/*                                                                          */
/*  This code is compiled in the Makefile as client10.c and compiles into   */
/*  client.o.  It's HTTP/1.1 counterpart is called client11.c               */
/*                                                                          */
/*             THESE ARE THE HTTP/1.0 COMPLIANT CLIENTTHREAD ROUTINES.      */
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
#include "client10.h"

void* ClientThread(void *);
void* ReaderThread(void *);

/* Each of the following routines and variables are defined in Surgeclient.c*/

extern int seqfifo;
extern int offfifo;
extern int outfifo;
extern int systemno;
extern long object[MATCHSIZE][OBJLIMIT];
extern long seq[NAMESIZE];
extern char httpstart[URLSIZE];
extern char httpend[URLSIZE];

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

  int id, stopobjloop, stopfileloop, numobjs, conct;
  long objindx, fileindx, count;
  struct timeval starttime, timeout;
  struct timezone tz;
  char seqbuff[10];
  char offbuff[10];
  struct readerdata reader;

  /* Begin by initializing all of the variables used in the ClientThread    */

  numobjs = 0;     /* for HTTP1.1 number of objects to get before new concts*/
  count = 0;       /* count is used to store the object index from OBJFIFO  */
  id = *(int *)threadno; /* assign local thread id                          */
  stopobjloop = 0; /* stopobjloop = 0 until the object sequence is complete */
  timeout.tv_sec = 0;  /* timeout used in the select command to test concts */
  timeout.tv_usec = 0;

  /* printf("SURGEclient %d:  Started thread # %d\n", systemno, id); */

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

      /* Get the file names associated with the selected object and request */
      /* them from the server. Use a single connection per file transfer    */

      fileindx = 0;     /* Begin index for each file in an object at 0      */
      stopfileloop = 0; /* stopfileloop = 0 until object has been transferd */

      while (stopfileloop == 0 && stopobjloop == 0)
		{

	  /* Time stamp the start of the transaction                        */
  
	  gettimeofday(&starttime,&tz);

	  /* Open a connection to HTTP server up to maxconcts               */

	  conct = -1;
	  while(conct == -1)
	    conct = connectsrvr();

	  /* Set up reader data for a file and start reader thread          */

	  reader.conctno = conct;
	  reader.readerno = 0;
	  reader.threadno = id;
	  reader.objectno = objindx;
	  reader.fileno = fileindx;
	  reader.startus = starttime.tv_usec;
	  reader.starts = starttime.tv_sec;

	  /* HTTP 1.0:  Read file then close connection                     */

	  ReaderThread(&reader);  /* Start reading specified file        */
	  if((close(conct)) < 0)  /* Close the connection                */
	    printf("SURGEclient %d: Error closing socket %d\n",
		   systemno,conct);
	  /* printf("SURGEclient: Closed connection %d\n", 0); */
	  
	  /* Get the next file name from the current object, if is a -1     */
	  /* then stop the file select loop otherwise get the next file     */
	  /* up to the number of connections that are available             */
	  
	  fileindx++;
	  if(object[objindx][fileindx] == -1 || fileindx == OBJLIMIT)
	    stopfileloop = 1;
	  
	} /* End of file read loop    */

      /* Sleep for OFF time after all files in the object are retrieved     */

      if(read(offfifo, offbuff, sizeof(offbuff)) < 0)
	printf("SURGEclient %d: Error reading from offfifo\n", systemno);
      /*printf("Surgeclient %d:  Sleeping for %d ms\n", 
               systemno, atoi(offbuff)); */
      usleep(1000 * atoi(offbuff));
     
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
  int c, thrdid, readid, conct;
  long objindx, fileindx, stimeus, stimes;
  struct timeval endtime;
  struct timezone tz;
  char rbuff[READBUFFSZ];
  char objbuff[10];
  char outbuff[10];
  char url[URLSIZE];

  conct = ((struct readerdata *)readdat)->conctno;
  readid = ((struct readerdata *)readdat)->readerno;
  thrdid = ((struct readerdata *)readdat)->threadno;
  objindx = ((struct readerdata *)readdat)->objectno;
  fileindx = ((struct readerdata *)readdat)->fileno;
  stimeus = ((struct readerdata *)readdat)->startus;
  stimes = ((struct readerdata *)readdat)->starts;

  /*printf("Reader:  Client %d Thread %d Reader %d Object  %ld Fileno = %ld\n",
       systemno, thrdid, readid, objindx, object[objindx][fileindx]);*/

  /* Generate complete URL string by inserting file number in http string   */

  strcpy(url, httpstart);
  sprintf(objbuff,"%ld", object[objindx][fileindx]);
  strncat(url, &objbuff[0], sizeof(objbuff));
  strncat(url, &httpend[0], sizeof(httpend));

  /* Issue HTTP GET call for the specified file to the specified connection */

  write(conct,url,strlen(url));

  /* Read the file from the connection.  For HTTP1.0 just read until EOF    */
  /* but test for error conditions.                                         */

  c = 1;
  while(c > 0) c = fillreadbuff(conct,rbuff,sizeof(rbuff));

  /* Time stamp the end of the transaction                                  */
	  
  gettimeofday(&endtime,&tz);

  /* Write results to result structure                                      */

  if(c != -1)  /* Don't load results on read error                          */
    {          /* Get the output sequence #, then write results             */
      if(read(outfifo, outbuff, sizeof(outbuff)) < 0)
	printf("SURGEclient %d: Error reading from outfifo\n", 
	       systemno);
      loadoutput(atol(outbuff), objindx, fileindx, thrdid,
		 stimeus, stimes, endtime.tv_usec, endtime.tv_sec);
    }

  return(NULL);  
} /* End ReaderThread */





