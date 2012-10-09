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
/* Utility Name: Parse Surge.log file and generate results files.           */
/* Authors:      Paul Barford                                               */
/* Rev:    v1.2  4/27/98                                                    */
/*                                                                          */
/*                                                                          */
/* Description:                                                             */
/*                                                                          */
/* This program enables you to generate a number of "results" files which   */
/* can be used to analyze the performance of a Surge client.  There are     */
/* three analyses which can be done: object size, off times, and on times.  */
/* Files of data for each of these are output by this program.  These files */
/* must then be analyzed in Splus (or some other package) to determine if   */
/* Surge is doing what it is supposed to do which is follow all of the      */
/* appropriate distributions.                                               */
/*                                                                          */
/* Inputs:                                                                  */
/*                                                                          */
/* The parameters for the program are listed in the DEFINE area of the      */
/* program header.  This program reads in all of the data from the log file */
/* specified on the command line.  The log file has the following format:   */
/* <client ID>, <session ID>, <starttime (ms)>, <URL>, <file size>,         */
/* <endtime (ms)>.  Each of the values is read into an array and then each  */
/* of the values needed for analysis is calculated.  This program is run    */
/* via the following:                                                       */
/*                                                                          */
/*                        pbvalclnt {input logfilename}                     */
/*                                                                          */
/*  This program is also set up to work with input from multiple clients    */
/*  where each client is set up with its own distinct client number which   */
/*  is the first column of the input file.  Just merge and sort the logs    */
/*  from the various clients into a single log and then run this program on */
/*  that file.                                                              */
/*                                                                          */
/* Outputs:                                                                 */
/*                                                                          */
/* The program outputs four files each which contain one column of data     */
/* (except for the llcd).  The files are:                                   */
/*                                                                          */
/*               onc.txt - number of files per object based on THRESH value */
/*               oft.txt - off times based on THRESH value (seconds)        */
/*               ont.txt - durations of ON times (seconds)                  */
/*               ontll.txt - LLCD for ON times                              */
/*                                                                          */
/* The program also outputs an average and standard deviation for the       */
/* latency of HTTP operations per second realized during the test.          */
/*                                                                          */
/*  NOTE:   This program must be compiled with the -lm qualifier since it   */
/*  uses log functions to calculate LLCD's                                  */
/*                                                                          */
/* Rev 1.2  This rev now only accepts log file format generated from the    */
/*          the .c versions of SURGE (Java version is no longer supported). */
/*          Also, now that active off times have been removed, there is no  */
/*          longer and analysis of off times less than 1 second.            */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXTRACE 200000      /* MAX number of input files handled           */
#define THRESH 1             /* Threshold number of seconds for off times   */

/* Define the routines used in this program  */

void getinputs();
void initialize();
void results1();
int compare(const void *first, const void *second);

/* Define global variables and data arrays used in this program */

long num_distcnt;           /* number of distinct file sizes                */
long num_counts;            /* number of on counts                          */
long num_offfiles;          /* number of off times                          */
long num_infiles;           /* total number of input trace files            */
long num_lrufiles;          /* total number of elements in lru array        */
long sid[MAXTRACE];         /* session id for each file transfer            */
long strts[MAXTRACE];       /* seconds part of transfer start time          */
long strtms[MAXTRACE];      /* milliseconds part of transfer start time     */
float durt[MAXTRACE];       /* transfer duration                            */
float oft[MAXTRACE];        /* calculated off times                         */
float probont[MAXTRACE];    /* LLCD probs for file sizes                    */
float logont[MAXTRACE];     /* LLCD log file sizes                          */
int clid[MAXTRACE];         /* client id for each file                      */
int sysid[MAXTRACE];        /* system id for each file                      */
int oncount[MAXTRACE];      /* count of number of files transferred in ON   */

// BOKI
double d_thresh;			// Threshold THRESH to be inputed from the command line

/****************************************************************************/
/* Main begins by sorting the input file and reading in the inputs and then */
/* calculates the results and prints them out.                              */
/****************************************************************************/


int main(int argc, char *argv[])
{

  long i;
  char sort[50]= "sort +0 +1 +2 +3 +4 -n -o sorted.log ";
  float intrvl;
  float totdurt, mean, varval;

/****************************************************************************/
/* Initialize variables.                                                    */
/****************************************************************************/

  num_offfiles = 0;
  num_counts = 0;
  totdurt = 0.0;
  mean = 0.0;
  varval = 0.0;

/****************************************************************************/
/* Initialize arrays.                                                       */
/****************************************************************************/

  if (argc < 3)
    {
      printf("Usage: {client log filename} {threshold}\n");
      exit(1);
    }

  d_thresh = (double) atof(argv[2]);
  if (d_thresh == 0) d_thresh = 1;
  printf("Threshold: %lf\n", d_thresh);

  initialize();

/****************************************************************************/
/* Sort the inputs by the client ID and then the session ID then load array */
/* This sorting is necessary to separate out the traces that are generated  */
/* by each thread on each client on each system.  If there was a better way */
/* to do a multikey sort in C then it would use it instead of UNIX sort.    */
/* Suggestions are welcome!  - PB.                                          */
/****************************************************************************/

  strcat(sort,argv[1]);
  if ( (i = system(sort)) < 0)
    {
      printf("Error sorting input file\n");
      exit(1);
    }
  
  getinputs();

/****************************************************************************/
/* Begin by calculating the mean and varience of the transfer delays        */
/****************************************************************************/

  for(i=0;i<num_infiles;i++)
    {
      totdurt += durt[i];
    }
  mean = totdurt/num_infiles;
  for(i=0;i<num_infiles;i++)
    {
      varval += (durt[i] - mean)*(durt[i] - mean);
    }
  varval = varval/(num_infiles - 1);

  printf("Mean transfer delay = %f seconds\n", mean);
  printf("Varience of transfer delay = %f seconds\n", varval);

/****************************************************************************/
/* We now calculate OFF times by only considering the times greater than the*/
/* THRESH value.  We also calculate the size of objects based on this value.*/
/****************************************************************************/

  /* Calculate off times and object sizes for all transactions */

  for(i=0;i<num_infiles;i++)
    if((sid[i]==sid[i+1]) && (clid[i]==clid[i+1]) && (sysid[i]==sysid[i+1]))
      {
	intrvl = (float)(strts[i+1]-strts[i])+
	  ((float)(strtms[i+1]-strtms[i])/1000000)-durt[i];
	if(intrvl  < d_thresh) {
	  oncount[num_counts]++;
	} else {
	  oft[num_offfiles] = intrvl;
	  num_offfiles++;
	  num_counts++;
	}
      } else {
	num_counts++;
      }

  num_offfiles++;    /* Required in order to print out the last value       */

  qsort(durt,num_infiles,sizeof(durt[0]),compare);

  for(i=0;i<num_infiles;i++)   /* Calculate LLCD for on times               */
    {
      logont[i] = log10(durt[i]);
      probont[i] = log10(1.0 - (float)(i+1)/num_infiles);
    }


/****************************************************************************/
/* Write out results for ontimes, offtimes, oncounts and delete sorted.log  */
/****************************************************************************/

 results1();

  if ( (i = system("rm -f sorted.log")) < 0)
    {
      printf("Error sorting input file\n");
      exit(1);
    }
  return 0;

} /* End Main */

/****************************************************************************/
/* This is just a quick comparison routine which is used by qsort to sort   */
/* the measured transfer times in ascending order so that LLCD can be       */
/* calculated.                                                              */
/****************************************************************************/

int compare(const void *first, const void *second)
{
  return(*((long *)first) - *((long *)second));
}

/****************************************************************************/
/* The getinputs routine gets input from the sorted log file.  Start and    */
/* end are divided into seconds and microseconds so delay values must be    */
/* calculated carefully.                                                    */ 
/****************************************************************************/

void getinputs()
{

  int sesid, client, systemid;
  char url[128];
  long insize, ssecs, susecs, esecs, eusecs;
  FILE *fp;

  num_infiles = 0;            /* number of files read into infiles array    */

  /* Get input from specified log file */
  
  if((fp = fopen("sorted.log", "r")) == NULL)
    {
      perror("Unable to input log file ");
      exit(1);
    }

  while( fscanf(fp, "%d %d %d %ld %ld %s %ld %ld %ld", &client, 
		&systemid, &sesid, &ssecs, &susecs, url, &insize, 
		&esecs, &eusecs) != EOF)
    {
      if(num_infiles > MAXTRACE)
	{
	  printf("Buffers filled before end of scan. Increase MAXTRACE\n");
	  exit(1);
	}
      clid[num_infiles] = client;
      sid[num_infiles] = sesid;
      sysid[num_infiles] = systemid;
      strts[num_infiles] = ssecs;
      strtms[num_infiles] = susecs;
      durt[num_infiles] = (float)(esecs-ssecs)+(float)(eusecs-susecs)/1000000;
      num_infiles++;
    }
  
  fclose(fp);
  
} /* End getinputs */

/****************************************************************************/
/* Initializes the data and output arrays                                   */
/****************************************************************************/

void initialize()
{

  int i;

/* Initialize the data and output arrays to 0 */

  for(i=0;i<MAXTRACE;i++)
    {
      sid[i] = 0;
      clid[i] = 0;
      sysid[i] = 0;
      strts[i] = 0;
      strtms[i] = 0;
      durt[i] = 0.0;
      oft[i] = 0.0;
      oncount[i] = 1;
      logont[i] = 0.0;;
      probont[i] = 0.0;
    }

} /* End initialize */

/****************************************************************************/
/* The results routine opens and writes the OFF data, and object size data  */
/****************************************************************************/

void results1()
{
  long i;
  FILE *fp1;

  /* Write file otf.txt  */

  if((fp1 = fopen("oft.txt", "w")) == NULL)
    {
      perror("Unable to open oft.txt\n");
      exit(1);
    }
  for(i=0;i<num_offfiles;i++)
    {
      if(oft[i] > 0) fprintf(fp1,"%7.4f\n", oft[i]);
    }
  fclose(fp1);

  /* Write file onc.txt  */

  if((fp1 = fopen("onc.txt", "w")) == NULL)
    {
      perror("Unable to open onc.txt\n");
      exit(1);
    }
  for(i=0;i<num_counts;i++)
    {
      fprintf(fp1,"%d\n", oncount[i]);
    }
  fclose(fp1);

  /* Write file ont.txt  */

  if((fp1 = fopen("ont.txt", "w")) == NULL)
    {
      perror("Unable to open ont.txt\n");
      exit(1);
    }
  for(i=0;i<num_infiles;i++)
    {
      fprintf(fp1,"%7.4f\n", durt[i]);
    }
  fclose(fp1);

  /* Write file ontll.txt  */

  if((fp1 = fopen("ontll.txt", "w")) == NULL)
    {
      perror("Unable to open ontll.txt\n");
      exit(1);
    }
  for(i=0;i<num_infiles-1;i++)
    {
      fprintf(fp1,"%8.4f %8.4f \n", logont[i], probont[i]);
    }
  fclose(fp1);

} /* End results1 */







