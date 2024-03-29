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
/*  Title:     Surge Master - Scalable URL Reference GEnerator              */
/*  Revision:  1.1         4/22/98                                          */
/*                                                                          */
/*  This program starts up the multiprocess-multithreaded C version of SURGE*/
/*  This program is necessary since Pthreads doesn't seem to do forking     */
/*  properly.  Surge Master does two things:  it sets up named pipes which  */
/*  synchronizes the document request sequence among Surge clients.  It     */
/*  also forks off the Surge clients, tells them when to quit and then      */
/*  combines their results into one output file.                            */
/*                                                                          */
/*  Run this program via:                                                   */
/*                                                                          */
/*                Surge {#clients} {#threads/client}, {Runtime sec.}        */
/*                                                                          */
/*  This program was written uses Chris Provenzano's version of Pthreads    */
/*  The documentation on this package can be found on the Web at:           */
/*                                                                          */
/*             http://www.mit.edu:8001/people/proven/pthreads.html          */
/*                                                                          */
/*  This implementation is available under a variety of UNIX flavors and    */
/*  can be downloaded from that site.                                       */
/*                                                                          */
/*  See the SURGE-HOW-TO file for more information on this program and the  */
/*  others which make up the C version of SURGE.                            */
/*                                                                          */
/*  Rev 1.1:  Only two named pipes are necessary since objects are what are */
/*            being requested by SURGE clients.  Active OFF times are no    */
/*            longer placed between individual file transfers.              */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#ifdef __linux__
#include <wait.h>
#endif

#define MAXCLIENTS 100
#define PERMS 0666

/* Define the names of the fifos used to pass data to clients  */

#define SEQFIFO "fifo1"
#define OFFFIFO "fifo2"
#define OUTFIFO "fifo3"
#define CNTFIFO "fifo4"

/* Define names of the data files used to fill fifos */

#define SEQFILE "name.txt"
#define OFFFILE "off.txt"
#define CNTFILE "cnt.txt"

/* Define the name of the output file generated by SURGE  */

#define LOGFILE "Surge.log"

/* Define routines used in this program */

void loadseq(long);
void loadoff(long);
void loadout();
void loadcnt();
void results();
void threads(int);
void cleanup();

/* Define global variables used in this program */

long numnames;                          /* Length of object request sequence*/
long numoffs;                           /* Number of OFF time values        */
char clientstring[200] = "./Surgeclient "; /* client start up command string   */
int clid[MAXCLIENTS];                   /* client ID array                  */
int clients;                            /* number of client processes       */
int loader1;                            /* object name loader ID            */
int loader2;                            /* off time loadeer ID              */
int loader3;                            /* output file loader ID            */
int loader4;                            /* object count loader ID           */
//BOKI
char *d_serverIP;
char *http_dir;
int d_clid;

/****************************************************************************/
/* Main begins by setting up fifos and then starting loader routines to keep*/
/* them filled with data from the intput files.  It then forks the SURGE    */
/* client process and waits for them to finish.  It then combines their     */
/* results into a single output file.                                       */
/****************************************************************************/

int main(int argc, char *argv[])
{ 
  long k;
  int i, threadnum,runtime;
  char runbuff[100];
  FILE *fp;

  if (argc < 7)
    {
      printf("Usage: {#clients} {#threads/client} {Runtime sec.} {web server} {HTTP dir} {clientID}\n");
      exit(1);
    }

  // BOKI
  clients = atoi(argv[1]);
  threadnum = atoi(argv[2]);
  runtime = atoi(argv[3]);
  d_serverIP = argv[4];
  http_dir = argv[5];
  d_clid = atoi(argv[6]);

  if(clients > MAXCLIENTS)
    {
      printf("Error:  Need to increase MAXCLIENTS\n");
      exit(0);
    }

  printf("SURGE: Scalable URL Reference Generator\n");
  printf("Running %d clients with %d threads/client for %d seconds\n", 
	 clients,threadnum, runtime);
  printf("Server address: %s\n", d_serverIP);

  /* Get the length of the object request sequence from name.txt file      */

  numnames = 0;
  if((fp = fopen(SEQFILE, "r")) == NULL)
    {
      perror("SURGEmaster:  Unable to open name.txt file ");
      exit(1);
    }
  while( fscanf(fp, "%ld", &k) != EOF) numnames++;
  fclose(fp);
  printf("SURGEmaster:  %ld objects in name sequence\n", numnames);

  /* Get the length of the off.txt file - should be the same as object seq */

  numoffs = 0;
  if((fp = fopen(OFFFILE, "r")) == NULL)
    {
      perror("SURGEmaster:  Unable to open off.txt file ");
      exit(1);
    }
  while( fscanf(fp, "%ld", &k) != EOF) numoffs++;
  fclose(fp);
  
  if(numnames != numoffs)  /* If lengths unequal you could run out of offs  */
    {
      printf("SURGEmaster: Warning: OFF.TXT and NAME.TXT: Unequal lengths\n");
    }
  
  /* Set up the FIFO's which are kept loaded by a child processes           */

  if((mknod(SEQFIFO, S_IFIFO | PERMS,0) < 0) && (errno != EEXIST))
    {
      perror("SURGEmaster:  SEQFIFO create failed ");
      exit(2);
    }
  if((mknod(OFFFIFO, S_IFIFO | PERMS,0) < 0) && (errno != EEXIST))
    {
      perror("SURGEmaster:  OFFFIFO create failed ");
      exit(2);
    }
  if((mknod(OUTFIFO, S_IFIFO | PERMS,0) < 0) && (errno != EEXIST))
    {
      perror("SURGEmaster:  OUTFIFO create failed ");
      exit(2);
    }
  if((mknod(CNTFIFO, S_IFIFO | PERMS,0) < 0) && (errno != EEXIST))
    {
      perror("SURGEmaster:  CNTFIFO create failed ");
      exit(2);
    }
  
  /* Start up the sequence loaders which run until totals are reached       */

  if((loader1 = fork()) < 0)
    {
      perror("SURGEmaster:  Fork of name loader child process failed ");
      exit(2);
    } else if (loader1 == 0)
      {
	loadseq(numnames);  /* loadseq keeps fifo1 filled with object names */
	exit(0);
      }
  if((loader2 = fork()) < 0)
    {
      perror("SURGEmaster:  Fork of off time loader child process failed ");
      exit(2);
    } else if (loader2 == 0)
      {
	loadoff(numoffs);   /* loadoff keeps fifo2 filled with off times    */
	exit(0);
      }
  if((loader3 = fork()) < 0)
    {
      perror("SURGEmaster:  Fork of output seq loader child process failed ");
      exit(2);
    } else if (loader3 == 0)
      {
	loadout();          /*loadout keeps fifo3 filled with output seq #'s*/
	exit(0);
      }
  if((loader4 = fork()) < 0)
    {
      perror("SURGEmaster:  Fork of output cnt loader child process failed ");
      exit(2);
    } else if (loader4 == 0)
      {
	loadcnt();          /*loadcnt keeps fifo4 filled with object counts */
	exit(0);
      }

/****************************************************************************/
/* Start up the threaded clients which will make the document requests.     */
/* Pass them the number of threads they are supposed to start and their     */
/* Surge ID number and the number of seconds that the simulation is to run  */
/* The clients are started by forking processes and then starting the       */
/* Surgeclient code within the child processes.                             */
/****************************************************************************/

  strcat(clientstring,argv[2]);   /* Add number of threads to start string  */
  sprintf(runbuff, " %d",runtime);/* Add simulation runtime to start strng  */
  strcat(clientstring,runbuff);
  //BOKI
  sprintf(runbuff, " %s",d_serverIP); //Add server IP address to start string
  strcat(clientstring,runbuff);
  sprintf(runbuff, " %s",http_dir);   //Add HTTP dir to start string
  strcat(clientstring,runbuff);
  sprintf(runbuff, " %d",d_clid);     //Add client ID
  strcat(clientstring,runbuff);
  
  for(i=0;i<clients;i++)
    {

      /* Start up the child processes in which Surgeclient will run         */

      if((clid[i] = fork()) < 0)
	{
	  perror("SURGEmaster:  Fork of client failed ");
	  exit(2);
	} else if (clid[i] == 0)
	  {
	    threads(i);  /* The threads routine starts Surgeclients         */
	  }
      sleep(1);     /* Stager client start to reduce transient effects      */
    }

  /* Wait until either the time expires or until the clients return         */

  for(i=0;i<clients;i++)
    {
      if (waitpid(clid[i], NULL, 0) != clid[i])
	{
	  perror("SURGEmaster:  waitpid child failed");
	  cleanup();
	  exit(1);
	}
    }

  /* Clean up and write out the results log file                            */

  cleanup();
  exit(0);
  return 0;

} /* End main */

/****************************************************************************/
/* Threads is the only routine which is run by the children.  It simply     */
/* generates the correct startup string which then starts up threads        */
/* program which makes document requests. If that program returns, the      */
/* child just dies.                                                         */
/****************************************************************************/

void threads(int i)
{
  int k;
  char numbuff[5];
  char startclnt[50];

  strcpy(startclnt,clientstring);
  sprintf(numbuff, " %d", i);  /* Finish the startup string by adding clid  */
  strcat(startclnt,numbuff);
  if ( (k = system(startclnt)) < 0)  /* Start the Surgeclient code          */
    {
      printf("SURGEmaster:  Error starting client %d\n", i);
      cleanup();
      exit(1);
    }
  exit(0);  /* Child process ends when Surgeclient completes  */

} /* End routine threads */

/****************************************************************************/
/* Cleanup collects together the client logs and cats them into one big     */
/* file.  It also closes the named pipe and kills the clients if they are   */
/* still alive.                                                             */
/****************************************************************************/

void cleanup()
{
  char cmd[50];
  int i,in1,in2,in3;
  long in4,in5,in7,in8,in9;
  char in6[100];
  FILE *fp1, *fp2;
  char logname[20];
  char extsnbuff[10];

  /* Kill loader processes and then get rid of the fifos                    */

  sprintf(cmd,"kill -9 %d",loader1);
  system(cmd);
  sprintf(cmd,"kill -9 %d",loader2);
  system(cmd);
  sprintf(cmd,"kill -9 %d",loader3);
  system(cmd);
  sprintf(cmd,"kill -9 %d",loader4);
  system(cmd);
  if(unlink(SEQFIFO) < 0) perror("SURGEmaster:  cannot unlink from SEQFIFO ");
  if(unlink(OFFFIFO) < 0) perror("SURGEmaster:  cannot unlink from OFFFIFO ");
  if(unlink(OUTFIFO) < 0) perror("SURGEmaster:  cannot unlink from OUTFIFO ");
  if(unlink(CNTFIFO) < 0) perror("SURGEmaster:  cannot unlink from CNTFIFO ");

  /* Cat all of the log files together into one output file                 */

  printf("SURGEmaster:  Writing Results\n");

  if((fp1 = fopen(LOGFILE, "w")) == NULL)
    {
      perror("SURGEmaster: Unable to open output log file ");
      exit(0);
    }  
  for(i=0;i<clients;i++)
    {
      sprintf(logname,LOGFILE);
      sprintf(extsnbuff, "%d", i);
      strcat(logname,extsnbuff);      /* construct log file name for clients*/
      if((fp2 = fopen(logname, "r")) == NULL)
	{
	  perror("Surgemaster:  Unable to open SURGEclient log ");
	} else {                      /* read standard client logfile format*/
	  while( fscanf(fp2, "%d %d %d %ld %ld %s %ld %ld %ld",&in1,&in2,&in3,
			&in4,&in5,in6,&in7,&in8,&in9) != EOF)
	    {
	      fprintf(fp1,"%d %d %d %ld %ld %s %ld %ld %ld\n",in1,in2,in3,in4,
		      in5,in6,in7,in8,in9);
	    }
	}
      fclose(fp2);
      sprintf(cmd, "rm -f ");   /* delete client log files  */
      strcat(cmd,logname);
      system(cmd);
    }
  fclose(fp1);

}  /* End routine cleanup */

/****************************************************************************/
/* Loadseq is a very simple routine which runs as a child process and keeps */
/* the named pipe filled with object name sequence numbers which are read   */
/* by the clients and insures temporal locality among them.                 */
/****************************************************************************/

void loadseq(long max)
{
  int fd;
  long i;
  char buff[10];

  if((fd = open(SEQFIFO, 1)) < 0)   /* Open the sequence # FIFO             */
    {
      perror("SURGEmaster:  Seq loader could not open SEQFIFO ");
      exit(2);
    }
  for (i = 0; i < max; i++)
    {
      sprintf(buff,"%ld",i);
      write(fd, buff, sizeof(buff));   /* Write object sequence # to FIFO   */ 
    }
  sprintf(buff,"%d",-1);   /* When sequence is finished write -1 to FIFO    */
  while(1) write(fd, buff, sizeof(buff));
  close(fd);
  exit(0);

} /* End routine loadseq */

/****************************************************************************/
/* Loadoff is just like loadseq - it just keeps the pipe filled with OFF    */
/* times (BUT NOT sequence numbers) which insure that all of the entries in */
/* the off.txt file are read and used only once during a Surge run.         */
/****************************************************************************/

void loadoff(long max)
{
  int fd;
  long offval;
  char buff[10];
  FILE *fp;

  if((fd = open(OFFFIFO, 1)) < 0)  /* Open the OFF time FIFO                */
    {
      perror("SURGEmaster:  Off time Loader could not open OFFFIFO ");
      exit(2);
    }
  if((fp = fopen(OFFFILE, "r")) == NULL) /* Open the OFF time data file     */
    {
      perror("SURGEmaster:  Unable to open off.txt file ");
      exit(1);
    }
  while( fscanf(fp, "%ld", &offval) != EOF)      /* read val from off.txt   */
    {
      sprintf(buff,"%ld",offval);                /* write off time to FIFO  */
      write(fd, buff, sizeof(buff));
    }
  sprintf(buff,"%d",0);  /* When sequence is finished just write 0 to FIFO  */
  while(1) write(fd, buff, sizeof(buff));
  close(fd);
  fclose(fp);
  exit(0);

} /* End routine loadoff */

/****************************************************************************/
/* Loadout is just like loadseq - it just keeps the pipe filled with results*/
/* file sequence numbers to insure that each file transfer gets its own     */
/* entry in the results array in Surgeclient.                               */
/****************************************************************************/

void loadout()
{
  long i, fd;
  char buff[10];

  if((fd = open(OUTFIFO, 1)) < 0)
    {
      perror("SURGEmaster:  Output seq Loader could not open OUTFIFO ");
      exit(2);
    }
  i = 0;
  while(1)   /* Just keep writing increasing seq numbers to output fifo     */
    {
      sprintf(buff,"%ld",i);
      write(fd, buff, sizeof(buff));
      i++;
    }
  close(fd);
  exit(0);

} /* End routine loadoff */

/****************************************************************************/
/* Loadcnt is just like loadoff - it just keeps the pipe filled with counts */
/* of objects.  These counts are not necessary for HTTP 0.9 or 1.0 but for  */
/* HTTP 1.1 they dictate the number of objects which are transfered before  */
/* connections are closed.                                                  */
/****************************************************************************/

void loadcnt()
{
  long fd, cntval;
  char buff[10];
  FILE *fp;

  if((fd = open(CNTFIFO, 1)) < 0)
    {
      perror("SURGEmaster:  Object count Loader could not open CNTFIFO ");
      exit(2);
    }
  if((fp = fopen(CNTFILE, "r")) == NULL) /* Open the CNT count data file    */
    {
      perror("SURGEmaster:  Unable to open cnt.txt file ");
      exit(1);
    }
  while( fscanf(fp, "%ld", &cntval) != EOF)
    {
      sprintf(buff, "%ld", cntval);
      write(fd, buff, sizeof(buff));
    }
  close(fd);
  fclose(fp);
  exit(0);

} /* End routine loadcnt */


