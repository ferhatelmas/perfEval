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
/* Utility Name: Parse Server.log file and generate results files.          */
/* Authors:      Paul Barford                                               */
/* Rev:    v1.1  4/23/98                                                    */
/*                                                                          */
/*                                                                          */
/* Description:                                                             */
/*                                                                          */
/* This program enables you to generate a number of "results" files which   */
/* can be used to analyze the performance of Surge clients.  There are      */
/* four analyses that need to be done - popularity, file sizes, files       */
/* transferred, and temporal locality.  Files of data for each of these are */
/* output by this program.  These files must then be analyzed in Splus (or  */
/* some other package) to determine if Surge is doing what it is supposed   */
/* to do which is follow all of the appropriate distributions.              */
/*                                                                          */
/* Inputs:                                                                  */
/*                                                                          */
/* The parameters for the program are listed in the DEFINE area of the      */
/* program header.  This program reads in all of the data from the HTTP     */
/* server log file specified on the command line.  The log file has the     */
/* following format:   <client ID>, <time of access>, <Command>, <name>,    */
/* <status>, and <file size>.  Each of the values is read into an array     */
/* and then each of the values needed for analysis is calculated.  This     */
/* program is run via the following:                                        */
/*                                                                          */
/*              pbvalsrvr {input logfilename} {1=HTTP 0.9,2=HTTP 1.0}       */
/*                                                                          */
/* Outputs:                                                                 */
/*                                                                          */
/* The program outputs six files each which contain one or two columns of   */
/* data.  The files are:                                                    */
/*                                                                          */
/*               pop.txt - number of accesses for each document             */
/*               siz.txt - distinct file sizes accessed                     */
/*               acs.txt - sizes of all files accessed for rqst size analys */
/*               dis.txt - stack distance for temporal locality analysis    */
/*               sizll.txt - LLCD for file size analysis                    */
/*               matll.txt - LLCD for request size analysis                 */
/*                                                                          */
/* The program also outputs an average and standard deviation for the       */
/* throughput in HTTP operations per second realized during the test.       */
/*                                                                          */
/*  NOTE:   This program must be compiled with the -lm qualifier since it   */
/*  uses log functions to calculate LLCD's                                  */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXTRACE 200000      /* MAX number of lines in the server log       */
#define MINTRACE 10000       /* MAX number of distinct files                */

/* Define the routines used in this program  */

void getinputs(char *,int);
void initialize();
void results2();
void results3();
int compare(const void *first, const void *second);
int fcompare(const void *first, const void *second);

/* Define global variables used in this program */

long num_distcnt;              /* number of distinct file sizes             */
long num_infiles;              /* total number of lines in server log       */
long num_lrufiles;             /* total number of elements in lru array     */
long indx[MAXTRACE];           /* index array used to sort trace by file sz */
long flsz[MAXTRACE];           /* sizes of all files appearing in log       */
long distinct[MINTRACE];       /* distinctly named file's sizes             */
long popular[MINTRACE];        /* # of requests of distinctly named file's  */
long positn[MINTRACE];         /* used to calculate LRU stack distance      */
long lru[MAXTRACE];            /* calculated lru stack distances            */
float probsiz[MINTRACE];       /* LLCD probs for file sizes                 */
float logsiz[MINTRACE];        /* LLCD log file sizes                       */
float probmat[MINTRACE];       /* LLCD probs for matching                   */
char names[MAXTRACE][100] ;    /* file names                                */
char dist_name[MINTRACE][100]; /* contains unique names for each file       */
char stack[MINTRACE][100];     /* stack used to calculate LRU stack distance*/
long timestamp[MAXTRACE];      /* timestamp of when a GET took place        */
int getspers[MAXTRACE];        /* number of HTTP ops for each second        */

/****************************************************************************/
/* Main begins by sorting the server log by file sizes and reading values   */
/* into arrays.  First calculate popularity, sizes, and total accesses.     */
/* Then, reread the log by access time and calculate the LRU stack distances*/
/* Finally, calculate the number of HTTP OPS per second.                    */
/****************************************************************************/

int main(int argc, char *argv[])
{

  int flag;
  long i, j, k, totpop, pntr, bottom, getscount;
  float totgetpsec, mean, varval;

/****************************************************************************/
/* Initialize variables.                                                    */
/****************************************************************************/

  num_distcnt = 0;
  num_lrufiles = 0;

/****************************************************************************/
/* Initialize arrays.                                                       */
/****************************************************************************/

  if (argc < 2)
    {
      printf("Usage: {client log filename} {1=HTTP 0.9,2=HTTP 1.0}\n");
      exit(1);
    }

  initialize();

/****************************************************************************/
/* Load the data arrays and then sort indx by file sizes                    */
/****************************************************************************/

  getinputs(argv[1],atol(argv[2]));

  qsort(indx,num_infiles,sizeof(indx[0]),fcompare);

/****************************************************************************/
/* Now calculate the distinct files and their total number of accesses      */
/* save the file size of each uniquely named file in the array distinct[]   */
/****************************************************************************/

  strcpy(dist_name[num_distcnt],names[indx[0]]);
  distinct[num_distcnt] = flsz[indx[0]];
  num_distcnt++;
  for(i=1;i<num_infiles;i++)
    {
      flag = -1;
      for(j=0;j<num_distcnt;j++) 
	if(strcmp(names[indx[i]],dist_name[j])==0) flag = 1;
      if(flag == -1) {
	strcpy(dist_name[num_distcnt],names[indx[i]]);
	distinct[num_distcnt] = flsz[indx[i]];
	num_distcnt++;
      }
    }

  for(i=0;i<num_distcnt;i++)
   {
     for(j=0;j<num_infiles;j++) 
       if(strcmp(dist_name[i],names[j])==0) popular[i]++;
   }
  totpop = 0;
  for(i=0;i<num_distcnt;i++) totpop += popular[i];
  printf("Total popularity = %ld Total distinct = %ld\n", totpop,num_distcnt);

  k = num_distcnt-1;
  j = 0;
  for(i=0;i<num_distcnt;i++) /* Calculate LLCD for sizes and matching*/
    {
      j += popular[k];
      logsiz[i] = log10(distinct[k]);
      probsiz[i] = log10(1.0 - (float)(i+1)/num_distcnt);
      probmat[i] = log10(1.0 - (float)j/totpop);
      k--;
    }

/****************************************************************************/
/* Write out results for popularity, sizes, request sizes                   */
/****************************************************************************/

 results2();

/****************************************************************************/
/* Now calculate the stack distances.  The assumption is that entries in the*/
/* server log are made in order in which they were received.                */
/****************************************************************************/

 strcpy(stack[0],names[0]);
 lru[0] = 0;
 bottom = 1;
 for(i=0;i<num_distcnt;i++)  positn[i] = 0;
 for(i=1;i<num_infiles;i++)
   {
     pntr = -1;                         /* Find the location of doc in stack*/
     for(j=0;j<bottom;j++)
       if(strcmp(stack[j],names[i])==0) pntr = j;
     if(pntr == -1) {
       strcpy(stack[bottom],names[i]);
       lru[i] = bottom;
       for(j=0;j<bottom;j++) positn[j]++;
       bottom++;
     } else {
       lru[i] = positn[pntr];            /* Adjust positions in the stack   */
       for(j=0;j<bottom;j++) 
	 {
	   if(positn[j] < lru[i]) {
	     positn[j]++;
	   } else {
	     if(positn[j] == lru[i]) positn[j] = 0;
	   }
	 }
     }
     /*printf("%ld File %s Pointer= %ld Lru= %ld\n",i,names[i],pntr,lru[i]);*/
   }

/****************************************************************************/
/* Calculate mean and varience of number of HTTP ops per second             */
/****************************************************************************/

  qsort(timestamp,num_infiles,sizeof(timestamp[0]),compare);
  getscount = 0;
  for(i=1;i<num_infiles;i++) /* first calculate the HTTP ops per second     */
    {
      if(timestamp[i] == timestamp[i-1])
	{
	  getspers[getscount]++;
	} else {
	  getscount++;
	}
    }
  getscount++;
  totgetpsec = 0.0;           /* Now calculate the mean and varience        */
  mean = 0.0;
  varval = 0.0;
  for(i=0;i<getscount;i++)
    {
      totgetpsec += getspers[i];
    }
  mean = totgetpsec/getscount;
  for(i=0;i<getscount;i++)
    {
      varval += (getspers[i] - mean)*(getspers[i] - mean);
    }
  varval = varval/(getscount - 1);

  printf("Mean number of GETS per second = %f\n", mean);
  printf("Varience of number of GETS per second = %f\n", varval);


/****************************************************************************/
/* Write out results for stack distances                                    */
/****************************************************************************/

 results3();
 return 0;

} /* End Main */

/****************************************************************************/
/* Get inputs from the server log file.  NOTE that the format must be like: */
/* berkeley.bu.edu - - [09/Oct/1997:17:11:59 -0400] "GET /55.txt" 200 1374  */
/* whre the only two fields that are interesting are the file name and the  */
/* file size (which is the last value).                                     */
/* If separate directories are used which may be the case at some point then*/
/* read in the file name as a string and then do string compares to do the  */
/* calculations above.                                                      */
/****************************************************************************/

void getinputs(char *infile, int surge)
{

  int junk, gethrs, getmin, getsec;
  char junk1[20],junk3[20],junk4[20],junk5[10];
  char whenget[25];
  char filename[100];
  long insize;
  FILE *fp;

  num_infiles = 0;            /* number of files read into infiles array    */

  /* Get input from specified log file */
  
  if((fp = fopen(infile, "r")) == NULL)
    {
      perror("Unable to input log file\n");
      exit(1);
    }
  if(surge == 1) 
    {
      while( fscanf(fp, "%s - - %s %s %s %s %d %ld", junk1, whenget, 
		    junk3, junk4, filename, &junk, &insize) != EOF)
	{
	  if(num_infiles > MAXTRACE)
	    {
	      printf("Buffers filled before end of scan. Increase MAXTRACE\n");
	      exit(1);
	    }
	  flsz[num_infiles] = insize;
	  strcpy(names[num_infiles],filename);
	  memmove(junk1, &whenget[13],2);
	  gethrs = atol(junk1);
	  memmove(junk1, &whenget[16],2);
	  getmin = atol(junk1);
	  memmove(junk1, &whenget[19],2);
	  getsec = atol(junk1);
	  timestamp[num_infiles] = gethrs*10000 + getmin*100 + getsec;
	  num_infiles++;
	}
    } else {
      while( fscanf(fp, "%s - - %s %s %s %s %s %d %ld", junk1, whenget, 
		    junk3, junk4, filename, junk5, &junk, &insize) != EOF)
	{
	  if(num_infiles > MAXTRACE)
	    {
	      printf("Buffers filled before end of scan. Increase MAXTRACE\n");
	      exit(1);
	    }
	  flsz[num_infiles] = insize;
	  strcpy(names[num_infiles],filename);
	  memmove(junk1, &whenget[13],2);
	  gethrs = atol(junk1);
	  memmove(junk1, &whenget[16],2);
	  getmin = atol(junk1);
	  memmove(junk1, &whenget[19],2);
	  getsec = atol(junk1);
	  timestamp[num_infiles] = gethrs*10000 + getmin*100 + getsec;
	  num_infiles++;
	}
    }
  fclose(fp);

} /* End getinputs */

/****************************************************************************/
/* This comparison routine is used by qsort to sort the timestamp array in  */
/* ascending order.                                                         */
/****************************************************************************/

int compare(const void *first, const void *second)
{
  return(*((long *)first) - *((long *)second));
}

/****************************************************************************/
/* This comparison routine is used by qsort to sort the trace by file size  */
/* (actually the indx array is what gets sorted).  Sort in descending order */
/****************************************************************************/

int fcompare(const void *first, const void *second)
{
  return(flsz[*((long *)second)] - flsz[*((long *)first)]);
}

/****************************************************************************/
/* Initialize the data arrays                                               */
/****************************************************************************/

void initialize()
{

  long i;

/* Initialize the data arrays and index arrays */

  for(i=0;i<MAXTRACE;i++)
    {
      lru[i] = 0;
      flsz[i] = 0;
      indx[i] = i;
      timestamp[i] = 0;
      getspers[i] = 1;
    }
  for(i=0;i<MINTRACE;i++)
    {
      distinct[i] = 0;
      positn[i] = 0;
      popular[i] = 0;
      logsiz[i] = 0.0;
      probsiz[i] = 0.0;
      probmat[i] = 0.0;
    }

} /* End initialize */

/****************************************************************************/
/* This results routine opens and writes the size, popularity, and LLCD's   */
/****************************************************************************/

void results2()
{
  long i, j;
  FILE *fp1;

  /* Write file acs.txt  */

  if((fp1 = fopen("acs.txt", "w")) == NULL)
    {
      perror("Unable to open acs.txt\n");
      exit(1);
    }
  for(i=0;i<num_distcnt;i++)
    {
      for(j=0;j<popular[i];j++)
	fprintf(fp1,"%ld\n", distinct[i]);
    }
  fclose(fp1);

  /* Write file pop.txt  */

  if((fp1 = fopen("pop.txt", "w")) == NULL)
    {
      perror("Unable to open pop.txt\n");
      exit(1);
    }
  for(i=0;i<num_distcnt;i++)
    {
      fprintf(fp1,"%ld\n", popular[i]);
    }
  fclose(fp1);

  /* Write file siz.txt  */

  if((fp1 = fopen("siz.txt", "w")) == NULL)
    {
      perror("Unable to open siz.txt\n");
      exit(1);
    }
  for(i=0;i<num_distcnt;i++)
    {
      fprintf(fp1,"%ld\n", distinct[i]);
    }
  fclose(fp1);

  /* Write file sizll.txt  */

  if((fp1 = fopen("sizll.txt", "w")) == NULL)
    {
      perror("Unable to open sizll.txt\n");
      exit(1);
    }
  for(i=0;i<num_distcnt-1;i++)
    {
      fprintf(fp1,"%8.4f %8.4f \n", logsiz[i], probsiz[i]);
    }
  fclose(fp1);

  /* Write file matll.txt  */

  if((fp1 = fopen("matll.txt", "w")) == NULL)
    {
      perror("Unable to open matll.txt\n");
      exit(1);
    }
  for(i=0;i<num_distcnt-1;i++)
    {
      fprintf(fp1,"%8.4f %8.4f \n", logsiz[i], probmat[i]);
    }
  fclose(fp1);

} /* End results2 */

/****************************************************************************/
/* This results routine opens and write stack distance results              */
/****************************************************************************/

void results3()
{
  int i;
  FILE *fp1;

  /* Write file dis.txt  */

  if((fp1 = fopen("dis.txt", "w")) == NULL)
    {
      perror("Unable to open dis.txt\n");
      exit(1);
    }
  for(i=0;i<num_infiles;i++)
    {
      fprintf(fp1,"%ld\n", lru[i]);
    }
  fclose(fp1);

} /* End results3 */






