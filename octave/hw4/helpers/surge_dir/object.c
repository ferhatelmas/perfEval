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
/* Utility Name: Object generator                                           */
/* Authors:      Paul Barford                                               */
/* Rev:    v1.0  4/14/98                                                    */
/*                                                                          */
/*                                                                          */
/* Description:                                                             */
/*                                                                          */
/* This program will generate a series names of files which make up each of */
/* the objects which will be accessed by SURGE.  An object consists of a    */
/* file from the base file set and then some number of files from the       */
/* embedded file set.  Each of the base files are assigned to only one      */
/* object while embedded files can be assigned to multiple objects.  The    */
/* number of embedded files in each object is determined by a distribution  */
/* which has been measured to be pareto.  The access of objects by SURGE    */
/* are separated by OFF times.  This program will generate a file which     */
/* contains the numeric names for each file in an object.  The output is    */
/* used by Surgeclient.c to indicate which files to access when an object   */
/* has been selected for transfer.  Based on the algorithm used to assign   */
/* embedded files to objects, this program has the effect of changing some  */
/* of the total number of references for embedded files.  Our experience    */
/* is that the effect is minimal and that observed popularity still         */
/* strongly follows Zipf's law.  The addition of objects to SURGE enables   */
/* the notion of spatial locality in the SURGE model.                       */
/*                                                                          */
/* Inputs:                                                                  */
/*                                                                          */
/* The parameters for the program are listed in the DEFINE area of the      */
/* program header.  The program takes no input from the command line but    */
/* does use the file MOUT.TXT to determine file names.                      */
/*                                                                          */
/* Run this program by typing the following:                                */
/*                                                                          */
/*                                object                                    */
/*                                                                          */
/* Outputs:                                                                 */
/*                                                                          */
/* The program outputs three different files.  The first is objout.txt.     */
/* It consists of rows of numbers where the first value in each row is the  */
/* base file name and the remaining numbers are the embedded file names.    */
/* In this case the name refers to the index into mout.txt.  The second     */
/* is opop.txt.  This is a file which lists rank in the first column and    */
/* number of references in the second column and is used to assess the      */
/* relative popularity of files (to see it still follows Zipf's law).       */
/* The third is ocdf.txt which like mcdf.txt in match.c is used to assess   */
/* the request sizes distribution.                                          */
/*                                                                          */
/* NOTE:  This program must be compiled with the -lm qualifier to link the  */
/* math library.                                                            */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAXTRACE 10000      /*   max number of files used                   */
#define SEED 1              /*   seed value for random number generator     */

/* The following are the parameters used in the distributions which make up */
/* the model for the number of embedded files in each object.  This         */
/* distribution has been shown to be pareto.  Also include the max size of  */
/* an object since some pareto values can be very large.                    */

#define PARETO_K 2          /* min value for on counts                      */
#define PARETO_A 1.245      /* scale factor for pareto dist.                */
#define LIMIT 150           /* limit for max value of on count              */

/* Define routines used in this program */

void initialize();
void getinputs();
void pareto(long, float, long, long);
int oncompare(const void *first, const void *second);
int bcompare(const void *first, const void *second);
int ecompare(const void *first, const void *second);
void results();

/* Define global variables used in this program  */

long num_binfiles;            /* total number of base files                 */
long num_einfiles;            /* total number of embedded files             */
long num_linfiles;            /* total number of loner files                */
int usdn[MAXTRACE];           /* holds used flags                           */
long base_refs[MAXTRACE];     /* base reference numbers from mout.txt       */
long embd_refs[MAXTRACE];     /* embed reference numbers from mout.txt      */
long org_embd_refs[MAXTRACE]; /* embed reference numbers from mout.txt      */
long lone_refs[MAXTRACE];     /* loner reference numbers from mout.txt      */
long base_name[MAXTRACE];     /* number name assigned to base files         */
long embd_name[MAXTRACE];     /* number name assigned to embedded files     */
long lone_name[MAXTRACE];     /* number name assigned to loner files        */
long base_size[MAXTRACE];     /* base file size from mout.txt               */
long embd_size[MAXTRACE];     /* embed file size from mout.txt              */
long lone_size[MAXTRACE];     /* loner file size from mout.txt              */
long base_indx[MAXTRACE];     /* used to index arrays                       */
long embd_indx[MAXTRACE];     /* used to hold ideal value index             */
long oncount[MAXTRACE];       /* frequencies based on Zipfs relation        */
long object[MAXTRACE][LIMIT]; /* contains file names for each object        */

/****************************************************************************/
/* Main calls for initialization of the data arrays, then generates values  */
/* from the model distribution for object size which are between 2 and the  */
/* LIMIT noted above.  It then fills file names into the objects based on   */
/* the simple method for matching.                                          */
/****************************************************************************/

int main(int argc, char *argv[])
{

  long i, j;

/****************************************************************************/
/* Initialize arrays.                                                       */
/****************************************************************************/

  num_binfiles = 0;
  num_einfiles = 0;
  num_linfiles = 0;
  srand48(SEED);

  initialize();

/****************************************************************************/
/* Read input file.                                                         */
/****************************************************************************/

  getinputs();

/****************************************************************************/
/* Load the oncount array with random numbers from the count distribution.  */
/****************************************************************************/

  printf("Generating objects...\n");
  pareto(PARETO_K, PARETO_A, num_binfiles, LIMIT);

/****************************************************************************/
/* Sort the indexes for the embedded reference (decending), the base        */
/* reference (decending), and the oncount array (ascending).                */
/****************************************************************************/

  qsort(oncount,num_binfiles,sizeof(oncount[0]),oncompare);
  qsort(base_indx,num_binfiles,sizeof(base_indx[0]),bcompare);
  qsort(embd_indx,num_einfiles,sizeof(embd_indx[0]),ecompare);

/****************************************************************************/
/* The algorithm for assigning files to objects is as follows.  Each base   */
/* file is assigned to one and only one object - do this first.  Embedded   */
/* files are then assigned to objects such that those with the largest      */
/* number of references are assigned to the object whose base file has the  */
/* largest number of references.  After an assignment to an object, the     */
/* number of references for embedded files are then adjusted and the process*/
/* begins again for the next object.  This has the possible effects of:     */
/*    (a)  not using all embedded files                                     */
/*    (b)  not using all references of all embedded files                   */
/*    (c)  assigning extra references to embedded files                     */
/* All of these possibilities are monitored and there are informational     */
/* messages printed based on each.  Statistically, the impact is minimal.   */
/****************************************************************************/

  for(i=0;i<num_binfiles;i++)
    {
      object[i][0] = base_name[base_indx[i]];
      for(j=0;j<oncount[i];j++)
	{
	  if(j < num_einfiles) {
	    object[i][j+1] = embd_name[embd_indx[j]];
	    embd_refs[embd_indx[j]] -= base_refs[base_indx[i]];
	    if(usdn[embd_indx[j]] == 0) usdn[embd_indx[j]] = 1;
	  }
	}
      qsort(embd_indx,num_einfiles,sizeof(embd_indx[0]),ecompare);
    }
  printf("Total embedded files from mout.txt = %ld\n", num_einfiles);
  printf("Total total base files = %ld, Total objects = %ld\n", num_binfiles, 
	 num_binfiles + num_linfiles);
  j = 0;
  for(i=0;i<num_einfiles;i++)
    if(usdn[i] == 0) j++;
  printf("Number of unused embedded file = %ld\n", j);
  j = 0;
  for(i=0;i<num_einfiles;i++)
    if(embd_refs[i] > 5) j++;
  printf("Number of embedded files with ref count > 5 %ld\n", j);
  j = 0;
  for(i=0;i<num_einfiles;i++)
    if(embd_refs[i] < -20) j++;
  printf("Number of embedded files with ref count < -20 %ld\n", j);

/****************************************************************************/
/* Write out results.                                                       */
/****************************************************************************/

  results();
  return 0;

} /* End Main */

/****************************************************************************/
/* The getinputs routine fetches the values from mout.txt for group, and    */
/* the number of requests for each file.  The names of the files are simply */
/* their index in mout.txt which is what is used by files.c                 */
/****************************************************************************/

void getinputs()
{
  int grp;
  long i, flsz, refs;
  FILE *fp;

  if((fp = fopen("mout.txt", "r")) == NULL)
    {
      perror("Unable to open mout.txt\n");
      exit(1);
    }
  i = 0;
  while( fscanf(fp, "%ld %ld %d", &refs, &flsz, &grp) != EOF) {
    if(grp == 1) {
      base_refs[num_binfiles] = refs;
      base_size[num_binfiles] = flsz;
      base_name[num_binfiles] = i;
      num_binfiles++;
    }
    if(grp == 2) {
      embd_refs[num_einfiles] = refs;
      org_embd_refs[num_einfiles] = refs;
      embd_size[num_einfiles] = flsz;
      embd_name[num_einfiles] = i;
      num_einfiles++;
    }
    if(grp == 3) {
      lone_refs[num_linfiles] = refs;
      lone_size[num_linfiles] = flsz;
      lone_name[num_linfiles] = i;
      num_linfiles++;
    }
    i++;
  }
  fclose(fp);

} /* End getinputs */

/****************************************************************************/
/* Initialize all data and index arrays.                                    */
/****************************************************************************/

void initialize()
{
  long i, j;

  /* Initialize the output arrays to 0 */

  for(i=0;i<MAXTRACE;i++)
    {
      base_name[i] = 0;
      embd_name[i] = 0;
      lone_name[i] = 0;
      base_refs[i] = 0;
      embd_refs[i] = 0;
      org_embd_refs[i] = 0;
      lone_refs[i] = 0;
      base_size[i] = 0;
      embd_size[i] = 0;
      lone_size[i] = 0;
      base_indx[i] = i;
      embd_indx[i] = i;
      usdn[i] = 0;
      oncount[i] = 0;
      for(j=0;j<LIMIT;j++)
	object[i][j] = 0;
    }

} /* End initialize */

/****************************************************************************/
/* This is a comparison routine which is used by qsort to sort oncounts     */
/****************************************************************************/

int oncompare(const void *first, const void *second)
{
  return(*((long *)first) - *((long *)second));
}

/****************************************************************************/
/* This is a comparison routine which is used by qsort to sort by reference */
/* count (actually the base_indx array is what gets sorted).                */
/****************************************************************************/

int bcompare(const void *first, const void *second)
{
  return(base_refs[*((long *)second)] - base_refs[*((long *)first)]);
}

/****************************************************************************/
/* This is a comparison routine which is used by qsort to sort by reference */
/* count (actually the embd_indx array is what gets sorted).                */
/****************************************************************************/

int ecompare(const void *first, const void *second)
{
  return(embd_refs[*((long *)second)] - embd_refs[*((long *)first)]);
}

/****************************************************************************/
/* This routine adds numbers from a pareto distribution to the oncount array*/
/* The values used to parameterize the distribution are passed to this      */
/* function as well as the number of values to add and the maximum value    */
/****************************************************************************/

void pareto(long K, float ALPHA, long numpts, long MAX)
{
  long i, partonum, maxnum;

  i = 0;
  maxnum = 0;
  while(i < numpts)
    {
      partonum = K/(pow(drand48(), 1.0/ALPHA));
      if(partonum <= MAX) {
	oncount[i] = partonum - 1;
	if(partonum > maxnum) maxnum = partonum;
	i += 1;
      }
    }
  if(maxnum > num_einfiles) maxnum = num_einfiles; /* objects cant be larger*/
  printf("Largest object has %ld files\n", maxnum);/* than total embd files */

} /* End routine pareto */

/****************************************************************************/
/* The results routine opens objout.txt and prints out the objects.  It also*/
/* generates opop.txt and ocdf.txt which can be used to assess the impact   */
/* on popularity (opop.txt) and request sizes that the object assignments   */
/* (ocdf.txt) have.                                                         */
/****************************************************************************/

void results()
{
  long i, j;
  FILE *fp1;

  /* Open the output file objout.txt  */

  if((fp1 = fopen("objout.txt", "w")) == NULL)
    {
      perror("Unable to open objout.txt\n");
      exit(1);
    }

  /* Write the loner file objects first  */

  for(i=0;i<num_linfiles;i++)
    {
      fprintf(fp1,"%ld\n", lone_name[i]);
    }

  /* Now write the base+embedded file objects */

  for(i=0;i<num_binfiles;i++)
    {
      fprintf(fp1,"%ld", object[i][0]);
      for(j=1;j<=oncount[i];j++)
	{
	  if(j <= num_einfiles)
	    fprintf(fp1," %ld", object[i][j]);
	}
      fprintf(fp1,"\n");
    }

  /* Close the output file  */

  fclose(fp1);

  /* Open the output file opop.txt  */

  if((fp1 = fopen("opop.txt", "w")) == NULL)
    {
      perror("Unable to open opop.txt\n");
      exit(1);
    }
  /* Write the loner references first  */

  for(i=0;i<num_linfiles;i++)
    {
      fprintf(fp1,"%ld\n", lone_refs[i]);
    }

  /* Now write the base file references */

  for(i=0;i<num_binfiles;i++)
    {
      fprintf(fp1,"%ld\n", base_refs[i]);
    }

  /* Now write the embedded file references */

  for(i=0;i<num_einfiles;i++)
    {
      fprintf(fp1,"%ld\n", org_embd_refs[i] - embd_refs[i]);
    }

  /* Close the output file  */

  fclose(fp1);

  /* Open the output file ocdf.txt  */

  if((fp1 = fopen("ocdf.txt", "w")) == NULL)
    {
      perror("Unable to open ocdf.txt\n");
      exit(1);
    }
  /* Write the loner sizes first  */

  for(i=0;i<num_linfiles;i++)
    {
      for(j=0;j<lone_refs[i];j++)
	fprintf(fp1,"%ld\n", lone_size[i]);
    }

  /* Now write the base file sizes */

  for(i=0;i<num_binfiles;i++)
    {
      for(j=0;j<base_refs[i];j++)
	fprintf(fp1,"%ld\n", base_size[i]);
    }

  /* Now write the embedded file references */

  for(i=0;i<num_einfiles;i++)
    {
      for(j=0;j<org_embd_refs[i] - embd_refs[i];j++)
      fprintf(fp1,"%ld\n", embd_size[i]);
    }

  /* Close the output file  */

  fclose(fp1);

} /* End results */



