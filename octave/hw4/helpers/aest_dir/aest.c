#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

/****************************************************************************/
/*                  Copyright 1998, Trustees of Boston University.          */
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

/* alpha estimator: Jan 96 - May 98, Mark Crovella, crovella@bu.edu */

/* assumes input is simply a file of reals, one per line */
/* but ignores lines starting with # */

#ifdef SPLUS
#define exit(a) return
#endif

/* globals input */
char *fname_base;
int  npts_inp;
int aggr, n_aggr;
int want_llcds = 0;
int debug = 0;
int graphics = 0;
int terse = 0;
int zeromean = 1;   /* zeromean behavior now the default */
int subsample = 0;
int want_levels = 0;
double resolution = 0.01;
double datsum;
double threshold = 0.10;

/* set to 1 and recompile to get the average-of-levels estimate in Splus */
int splus_want_level_estimate = 0;

#define STRING_LENGTH 1024

double mean_variance(double *data, int len)
{
     int j;
     double mean = 0.0, retval = 0.0;

     for (j=0; j<len; j++)
	  mean += data[j];
     mean /= (double) len;
     for (j=0; j<len; j++)
	  retval += (data[j]-mean)*(data[j]-mean);
     return retval/(double)(len-1);
}

double variance(double *data, int len, double mean)
{
     int j;
     double retval = 0.0;

     for (j=0; j<len; j++)
	  retval += (data[j]-mean)*(data[j]-mean);
     return retval/(double)(len-1);
}

/* assumes *ndx and *val are arrays both strictly decreasing */
/* (as in a complementary distribution plot) */
int binsearch(double *ndx, double *val, double m, double b, int low, int high)
{
     int mid;
     double line_y;
     
     /* line is defined by y = m*x + b */
     /* invariant: val[low] is above the line, val[high] is below or on line */

     if (high ==  (low + 1))
	  return low;
     mid = low + (high - low)/2;
     line_y = m * ndx[mid] + b;

     if (line_y < val[mid])
	  return binsearch(ndx, val, m, b, mid, high);
     else
	  return binsearch(ndx, val, m, b, low, mid);
}

/* in this case m is ignored and b is the x intercept */
int xsearch (double *ndx, double *val, double m, double b, int low, int high)
{
     int mid;
     
     /* line is defined by x = b */
     /* invariant: ndx[low] is less than b, ndx[high] is greater or equal */
     if (high == (low + 1))
	  return low;
     mid = low + (high - low)/2;

     if (ndx[mid] < b)
	  return xsearch(ndx, val, m, b, mid, high);
     else
	  return xsearch(ndx, val, m, b, low, mid);
}

void readfile(char *fname, int *npts, double **dat, FILE **fp)
{
     char inbuf[STRING_LENGTH];
     int i;
     
     
     *fp = fopen(fname, "r");
     if (!*fp)
     {
	  fprintf(stderr, "Can't open %s\n", fname);
	  exit(1);
     }
     
     *dat = (double *) malloc ((*npts) * sizeof(double));
     if (!*dat)
     {
	  fprintf(stderr, "Couldn't malloc\n");
	  exit(1);
     }

     /* read in data */
     i = 0;
     while (fgets(inbuf, STRING_LENGTH, *fp)) 
	  if (inbuf[0] != '#')
	  {
	       sscanf(inbuf, "%lf", &(*dat)[i]);
	       i++;
	  }

     *npts = i;
}

/* find the coordinates of the intersection of a vertical line with */
/* an arbitrary line */
void xcross(double *ndx, double *val, int low_idx, double xc, double *res_x,
	    double *res_y)
{
     double datam, datab;
     
     datam = (val[low_idx+1] - val[low_idx])/(ndx[low_idx+1] - ndx[low_idx]);
     datab = val[low_idx] - datam * ndx[low_idx];

     *res_x = xc;
     *res_y = datam * (*res_x) + datab;
}

void cross(double *ndx, double *val, int low_idx, double m, double b,
	   double *res_x, double *res_y)
{
     double datam, datab;
     
     datam = (val[low_idx+1] - val[low_idx])/(ndx[low_idx+1] - ndx[low_idx]);
     datab = val[low_idx] - datam * ndx[low_idx];

     *res_x = (datab - b) / (m - datam);
     *res_y = datam * (*res_x) + datab;
}


void cleanup(double *ndx, double *val, FILE *fp)
{
     /* clean up */
     free(val);
     free(ndx);
     fclose(fp);
}

void write_llcds(char *fname_base, int aggr, int n_aggr, double **lsize,
		 double **lprob, int *nout, int npts, double final_est)
{
     char file[STRING_LENGTH];
     FILE *fp;
     int i, j, sum;
     double chunk;

     sum = 1;
     
     for (i=0; i<n_aggr; i++)
     {
	  /* write out llcd file */
	  sprintf(file, "%s.sum%d.llcd", fname_base, sum);
#ifndef SPLUS
	  assert(strlen(file) < STRING_LENGTH);
#endif
	  fp = fopen(file, "w");
	  if (!fp)
	  {
	       fprintf(stderr, "can't open %s\n", file);
	       exit(1);
	  }

	  if (subsample)
	  {
	       chunk = lsize[i][0];
	       for (j = 0; j < nout[i]; j++)
		    if (lsize[i][j] >= chunk)
		    {
			 fprintf(fp, "%lf %lf\n", lsize[i][j], lprob[i][j]);
			 chunk += resolution;
		    }
	  }
	  else
	  {
	       for (j = 0; j < nout[i]; j++)
		    fprintf(fp, "%lf %lf\n", lsize[i][j], lprob[i][j]);
	  }	  
	  fclose(fp);
	  
	  sum *= aggr;
     }
     
     /* write a corresponding gnuplot file */
     sprintf(file, "%s.aest.gp", fname_base);
#ifndef SPLUS
     assert(strlen(file) < STRING_LENGTH);
#endif
     fp = fopen(file, "w");
     if (!fp)
       {
	   fprintf(stderr, "can't open %s\n", file);
	   exit(1);
       }
     if (zeromean)
	  fprintf(fp, "set xlabel \"Log10(size - %0.3f)\"\n", datsum);
     else
	  fprintf(fp, "set xlabel \"Log10(size)\"\n");
     fprintf(fp, "set ylabel \"Log10(P[X > x])\"\n");
     if (final_est == -1.0)
	  fprintf(fp, "set title \"File: %s  No. points: %d  *No Alpha Estimate*\"\n", fname_base, npts);
     else
	  fprintf(fp, "set title \"File: %s  No. points: %d  Alpha Estimate: %0.3f\"\n", fname_base, npts, final_est);

     fprintf(fp, "plot \"%s.sum1.llcd\" title \"Raw Data\" with lines,\\\n",
	     fname_base); 
     sum = aggr;
     for (i=1; i<n_aggr-1; i++)
     {
	 fprintf(fp,"\"%s.sum%d.llcd\" title \"%d-Aggregated\" with lines,\\\n",
	     fname_base, sum, sum); 
	 sum *= aggr;
     }     
     if (graphics)
     {
       fprintf(fp, "\"%s.sum%d.llcd\" title \"%d-Aggregated\" with lines,\\\n",
		  fname_base, sum, sum); 
       fprintf(fp, "\"%s.pts\" with points\n",fname_base);
     }
     else
     {
       fprintf(fp, "\"%s.sum%d.llcd\" title \"%d-Aggregated\" with lines\n",
		  fname_base, sum, sum); 
     }
     fclose(fp);
}


/* needed for qsort */
int dblcmp(const void *a, const void *b)
{
     double la = * (double *)a;
     double lb = * (double *)b;
     if (la < lb)
	  return -1;
     else if (la > lb)
	  return 1;
     else return 0;
}

typedef struct result
{
     double agg_err;
     double alf_est;
     double x1, x2, y1, y2;
     int aggr;
     double epsilon;
} result_t;

int resultcmp(const void *a, const void *b)
{
     result_t la = * (result_t *)a;
     result_t lb = * (result_t *)b;
     if (la.agg_err < lb.agg_err)
	  return -1;
     else if (la.agg_err > lb.agg_err)
	  return 1;
     else return 0;
}

void usage(char *pgmname)
{
     fprintf(stderr, "Usage: %s -f datafile -n npts -a aggr-factor -l number-levels [-wdgtsZ] [-r resolution] [-h threshold]\n", pgmname);
     fprintf(stderr, "-w : write the .llcd files for plotting\n");
     fprintf(stderr, "-g : write the .pts file showing points used (for plotting)\n");
     fprintf(stderr, "-d : (debug) write the .alphas file showing alphas used\n");
     fprintf(stderr, "-t : (terse) only emit estimate (if available)\n");
     fprintf(stderr, "-i : output alpha estimate for each level\n");
     fprintf(stderr, "-s : subsample plot datasets to reduce size\n");
     fprintf(stderr, "-r num : x-axis resolution of subsampled plots (default %lf);\n         increase to decrease plotfile size\n",resolution);
     fprintf(stderr, "-h num : set the relative error threshold (theta) for accepting a trial alpha value; default %lf)\n",threshold);
     fprintf(stderr, "-Z : DO NOT re-locate the dataset to have zero mean\n");
     exit(1);
}


#ifndef SPLUS

void parse_args(int argc, char *argv[])
{
     int c;
     extern char *optarg;
     extern int optind;
     int okflg = 0;
     
     while ((c = getopt(argc, argv, "f:n:a:l:r:h:wdgtZsi")) != EOF)
	  switch(c)
	  {
	  case 'f':
	       fname_base = optarg;
	       okflg |= 0x1;
	       break;
	       
	  case 'n':
	       npts_inp = atoi(optarg);
	       okflg |= 0x2;
	       break;

	  case 'a':
	       aggr = atoi(optarg);
	       okflg |= 0x4;
	       break;
	       
	  case 'l':
	       n_aggr = atoi(optarg);
	       okflg |= 0x8;
	       break;

	  case 'w':
	       want_llcds++;
	       break;
	       
	  case 'd':
	       debug++;
	       break;
	       
	  case 'g':
	       graphics++;
	       break;
	       
	  case 'h':
	       threshold = atof(optarg);
	       break;
	       
	  case 't':
	       terse++;
	       break;

	  case 's':
	       subsample++;
	       break;

	  case 'r':
	       resolution = atof(optarg);
	       break;

	  case 'i':
	       want_levels++;
	       break;

	  case 'Z':
	       zeromean=0;
	       break;
	       
	  default:
	       okflg |= 0x10;
	       break;
	  }
     if (okflg != 0xf)
	  usage(argv[0]);
}

#endif


#ifndef SPLUS
void main(int argc, char *argv[])
#else
void aest(double *dat_inp, double *factorp, double *dpts_inp, double *result)
#endif
{
     FILE *inp;
#ifndef SPLUS
     double *dat_inp;
#endif
     double sum, oldsum;
     int i, j, ctr;
     double **dat_aggr;
     double **lsize, **lprob;
     int *npts, *nout;
     double *varnce;
     char ptsfile[STRING_LENGTH], alfile[STRING_LENGTH];
     FILE *pts, *alf;
     double *index1, *value1, *index2, *value2;
     double minpt, maxpt;
     int npts1, npts2;
     int lower;
     double y1, x1, y2, x2, alpha_lower;
     double estimate, relerr;
     int stat_arr_size;
     double totalmax, totalmin;
     int totalpoints;
     double factor, lfactor, lefactor;
     double accum;
     int n_accum = 0;
     double *level_accum;
     int *n_level_accum;
     int considered;
     result_t *results;

     double final_estimate, var, sdev, final_level_estimate;
     double *res_arr;
     double zvar;
     double true_x1, true_x2, true_y1;
     double factor_i, factor_i2;

#ifndef SPLUS
     parse_args(argc, argv);
     factor = (double) aggr;
     lfactor = log10(factor);
     lefactor = log(factor);

     readfile(fname_base, &npts_inp, &dat_inp, &inp);
#else
     double *zmean;
     
     npts_inp = (int) *dpts_inp;
     factor = *factorp;
     aggr = (int) (factor+0.5);
     lfactor = log10(factor);
     lefactor = log(factor);
     n_aggr = 2;
     if (*zmean == 0.0)
	  zeromean=0;
#endif

     /* do zero-mean conversion if requested */
     if (zeromean)
     {
	  datsum = 0.0;
	  for (i = 0; i < npts_inp; i++)
	       datsum += dat_inp[i];
	  datsum /= (double) npts_inp;
	  for (i = 0; i < npts_inp; i++)
	       dat_inp[i] -= datsum;
	  zvar = variance(dat_inp, npts_inp, 0.0);
     }

     /* set up pointers to each dataset */
     npts = (int *) malloc (n_aggr * sizeof(int));
     nout = (int *) malloc (n_aggr * sizeof(int));
     varnce = (double *) malloc (n_aggr * sizeof(double));
     if (!npts || !nout || !varnce)
     {
	  fprintf(stderr, "Can't malloc npts/nout/varnce\n");
	  exit(1);
     }

     lsize = (double **) malloc (n_aggr * sizeof(double *));
     lprob = (double **) malloc (n_aggr * sizeof(double *));
     dat_aggr = (double **) malloc (n_aggr * sizeof(double *));
     if (!lprob || !lsize || !dat_aggr)
     {
	  fprintf(stderr, "Can't malloc lsize/lprob/dat_aggr\n");
	  exit(1);
     }
     
     level_accum = (double *) malloc (n_aggr * sizeof(double));
     n_level_accum = (int *) malloc (n_aggr * sizeof(int));
     if (!level_accum || !n_level_accum)
     {
	  fprintf(stderr, "Can't malloc level_accum/n_level_accum\n");
	  exit(1);
     }

     /* form n_aggr aggregations of dataset */
     dat_aggr[0] = dat_inp;
     npts[0] = npts_inp;
     varnce[0] = mean_variance(dat_aggr[0], npts[0]);
     for(i = 1; i < n_aggr; i++)
     {
	  int j, k = 0;
	  npts[i] = (int) ceil(npts[i-1] / (double) aggr);
	  dat_aggr[i] = (double *) malloc (npts[i] * sizeof(double));
	  if (!dat_aggr[i])
	  {
	       fprintf(stderr, "Can't malloc %d bytes for dat_aggr[%d]\n",
		       npts[i] * sizeof(double), i);
	       exit(1);
	  } 
	  for (j = 0; j < npts[i-1]; j++)
	  {
	       if ((j % aggr) == 0)
	       {
		    if (j > 0)
		    {
			 dat_aggr[i][k] = sum;
			 k++;
		    }
		    sum = 0.0;
	       }
	       oldsum = sum;
	       sum += dat_aggr[i-1][j];

	       /* check for overflow rather than using long long int's */
#ifndef SPLUS	       
	       if (dat_aggr[i-1][j] > 0)
		    assert(sum > oldsum);
#endif 
	  }
#ifndef SPLUS	       
	  assert (k == (npts[i] - 1));
#endif 
	  dat_aggr[i][k] = sum;
	  
	  varnce[i] = mean_variance(dat_aggr[i], npts[i]);
     }
     
     /* start to form the llcd's of each dataset; first, sort each one */
     for (i = 0; i < n_aggr; i++)
     {
	  qsort((void *) dat_aggr[i], npts[i], sizeof(double), dblcmp);
     }

     /* now form llcd of each dataset */
     for (i = 0; i < n_aggr; i++)
     {
	  double current;
	  int j, k = 0;
	  int counts = 0;
	  
	  lsize[i] = (double *) malloc (npts[i] * sizeof(double));
	  lprob[i] = (double *) malloc (npts[i] * sizeof(double));

	  /* form the log-log complementary distribution for this dataset */
	  current = dat_aggr[i][0];
	  /* don't do the last point; has complementary cum prob of zero */
	  for (j = 1; j < npts[i]; j++)
	  {
	       counts++;
	       if (dat_aggr[i][j] != current)
	       {
		    /* only deal with the positive upper tail */
		    if (current > 0.0)
		    {
			 lsize[i][k] = log10(current);
			 lprob[i][k] = log10(1.0 -
					(double) counts / (double) npts[i]);
			 k++;
		    }
		    current = dat_aggr[i][j];
	       }
	  }
	  nout[i] = k;
	  
	  /* find overall min and max of all datasets */
	  if (i == 0)
	  {
	       totalmin = lsize[i][0];
	       totalmax = lsize[i][k-1];
	       totalpoints = k;
	  }
	  else
	  {	       
	       totalmin = lsize[i][0] < totalmin ? lsize[i][0] : totalmin;
	       totalmax = lsize[i][k-1] > totalmax ? lsize[i][k-1] : totalmax;
	       totalpoints += k;
	  }	  
     }
     
     /* open log files */
     sprintf(ptsfile, "%s.pts", fname_base);
#ifndef SPLUS
     assert(strlen(ptsfile) < STRING_LENGTH);
#endif
     sprintf(alfile, "%s.alphas", fname_base);
#ifndef SPLUS
     assert(strlen(alfile) < STRING_LENGTH);
#endif

     if (debug)
     {
	  alf = fopen(alfile, "w");
	  if (!alf)
	  {
	       fprintf(stderr, "can't open %s\n", alfile);
	       exit(1);
	  }
     }
     
     if (graphics)
     {
	  pts = fopen(ptsfile, "w");
	  if (!pts)
	  {
	       fprintf(stderr, "can't open %s\n", ptsfile);
	       exit(1);
	  }
     }

     stat_arr_size = totalpoints;
     results = (result_t *) malloc(sizeof(result_t) * stat_arr_size);
     if (!results)
     {
	  fprintf(stderr, "Can't malloc results array\n");
	  exit(1);
     }
     res_arr = (double *) malloc(sizeof(double) * stat_arr_size);
     if (!res_arr)
     {
	  fprintf(stderr, "Can't malloc res_arr array\n");
	  exit(1);
     }

     /* for each pair of datasets, run the algorithm */
     considered = 0;
     ctr=0;
     factor_i = 1.0;
     factor_i2 = factor;
     for (i = 0; i < n_aggr-1; i++)
     {
	  
	  /* rename arrays */
	  index1 = lsize[i];
	  index2 = lsize[i+1];
	  value1 = lprob[i];
	  value2 = lprob[i+1];
	  npts1  = nout[i];
	  npts2  = nout[i+1];

	  /* find the bounds of our search */
	  /* no longer used */
	  minpt = (index1[0] > index2[0]) ? index1[0] : index2[0];
	  maxpt = (index1[npts1-1] < index2[npts2-1]) ?
	       index1[npts1-1] : index2[npts2-1];

	  /* first build the error array, measuring where the tail law holds */
	  /* stepping across the plot horixontally, using vertical lines */
	  for (j = 0; j < npts1; j++)
	  {

	       /* find the bracketing points, lower and lower+1 */
	       x1 = index1[j];
	       y1 = value1[j];

	       /* do some filtering of data points to be considered */
	       if ((x1 < index2[0]) || (x1 > index2[npts2-1]))
		    continue;
	       
	       /* only consider the 10% tails */
	       if (y1 >= -1)
		    continue; 

	       /* can't estimate alpha if upper line doesn't reach this lvl */
	       if (y1 < value2[npts2-1])
		    continue;
	       
	       considered++;
	       lower = xsearch(index2, value2, 0, x1, 0, npts2-1);
	       xcross(index2, value2, lower, x1, &x2, &y2);
	  
	       /* the estimate of log10(factor) */
	       estimate = y2-y1;
	       relerr = fabs(estimate-lfactor)/lfactor;

	       /* measure the horizontal alpha at the lower point  */
	       lower = binsearch(index2, value2, 0.0, y1, 0, npts2-1);
	       cross(index2, value2, lower, 0.0, y1, &x2, &y2);
	       alpha_lower = lfactor/(x2 - x1);

	       /* store away the results */
#ifndef SPLUS
	       assert(ctr < stat_arr_size);
#endif
	       true_x1 = pow(10, x1);
	       true_x2 = pow(10, x2);

	       results[ctr].alf_est = alpha_lower;
	       results[ctr].agg_err = relerr;
	       results[ctr].x1 = x1;
	       results[ctr].x2 = x2;
	       results[ctr].y1 = y1;
	       results[ctr].y2 = y2;
	       results[ctr].aggr = i;
	       ctr++;
	  }

	  factor_i = factor_i2;
	  factor_i2 *= factor;
     }

     /* sort the alpha estimates by increasing relative error */
     qsort((void *) results, ctr, sizeof(result_t), resultcmp);

     /* verbose debug output */
     if (debug)
     {
	  fprintf(alf, "# i relerr estimate log-x log-p x p log-x2 x2 aggr-level normal-error-term\n");
	  for (i=0; i<ctr; i++)
	  {
	       true_x1 = pow(10, results[i].x1);
	       true_y1 = pow(10, results[i].y1);
	       true_x2 = pow(10, results[i].x2);
	       fprintf(alf, "%d %lf %lf %lf %lf %lf %lf %lf %lf %d %lf\n",
		       i,
		       results[i].agg_err,
		       results[i].alf_est,
		       results[i].x1,
		       results[i].y1,
		       true_x1,
		       true_y1,
		       results[i].x2,
		       true_x2,
		       results[i].aggr,
		       results[i].epsilon);
	  }
     }
     
     /* form the final estimate */
     accum = 0.0;
     n_accum = 0;
     for (i=0; i<n_aggr-1; i++)
     {
	  level_accum[i] = 0.0;
	  n_level_accum[i] = 0;
     }
     i = 0;
     while ((i < stat_arr_size) && (results[i].agg_err < threshold))
     {
	  /* filter out outliers */
	  if (results[i].alf_est < 3.0)
	  {
	       if (graphics)
		    fprintf(pts, "%lf %lf\n",
			    results[i].x1,
			    results[i].y1); 
	       res_arr[n_accum] = results[i].alf_est;
	       level_accum[results[i].aggr] += results[i].alf_est;
	       n_level_accum[results[i].aggr]++;
	       accum += results[i].alf_est;
	       n_accum++;
	  }
	  i++;
     }

     if (n_accum != 0)
     {
	  double l_accum = 0.0;
	  int l_n_accum = 0;

	  final_estimate = accum / (double) n_accum;
	  var = variance(res_arr, n_accum, final_estimate);
	  sdev = sqrt(var);
	  
	  for(i=0; i<n_aggr-1; i++)
	  {
	       if (n_level_accum[i])
	       {
		    l_accum += level_accum[i] / (double) n_level_accum[i];
		    l_n_accum++;
	       }
	  }
	  final_level_estimate = l_accum / (double) l_n_accum;
     }
     else
     {
	  final_estimate = -1.0;
	  final_level_estimate = -1.0;
     }
     
     
#ifndef SPLUS
     if (terse)
     {
	  if (n_accum != 0) 
	       printf("%lf\n", final_estimate);
     }
     else if (n_accum == 0)
     {
	  /* flag for no-estimate case */
     	  printf("** No Estimate **\n");
     }
     else
     {
	  printf("Estimate: %lf\n", final_estimate);
	  if (want_levels)
	  {
	       printf("Estimate at each level: ");
	       for(i=0; i<n_aggr-1; i++)
	       {
		    if (n_level_accum[i])
			 printf("%lf ", level_accum[i] /
				(double) n_level_accum[i]);
		    else
			 printf("n/a ");
	       }
	       printf("\n");
	       printf("Average of Levels Estimate: %lf\n",
		      final_level_estimate);
	  }	  
	  if (zeromean)
	       printf("Subtracted mean: %lf\n", datsum);
     }
     
#else
     if (splus_want_level_estimate)
	  *result = final_level_estimate;
     else
	  *result = final_estimate;
#endif

     /* if requested, emit llcd files */
     if (want_llcds)
	  write_llcds(fname_base, aggr, n_aggr, lsize, lprob,
		      nout, npts_inp, final_estimate);

     if (debug)
	  fclose(alf);
     if (graphics)
	  fclose(pts);

     /* clean up */
     free(npts);
     free(nout);
     for (i=0; i<n_aggr; i++)
     {
	  free(lsize[i]);
	  free(lprob[i]);
	  if (i != 0)
	       free(dat_aggr[i]);
     }
     free(varnce);
     free(res_arr);
     free(results);
     free(n_level_accum);
     free(level_accum);
     free(lsize);
     free(lprob);
     free(dat_aggr);
     
}

