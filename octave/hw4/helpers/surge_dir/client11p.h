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
/*  Title:     Surgeclient header file                                      */
/*  Revision:  1.0         5/7/98                                           */
/*                                                                          */
/*  With the HTTP/1.1 compliant version of SURGE, it is necessary to make   */
/*  a separate header file with variable definitions that may be used by    */
/*  the clientXX.c files.                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* These are the values which you might want to alter for each SURGE test.  */
/* These will go into a config file at some point.                          */
/* NOTE:  You can be sure that you have your SIZE values properly set by    */
/* insuring that NAMESIZE >= wc objout.txt and OUTSIZE >= total requests for*/
/* files (shown after running zipf.c) and MATCHSIZE >= wc mout.txt and      */
/* OBJLIMIT >= OBJLIMIT from object.c and URLSIZE >= max size of all chars  */
/* which will make up any HTTP command string.  READBUFFSZ is the size of   */
/* the buffer that accepts data from the server.                            */
/****************************************************************************/

#define NAMESIZE 110000         /* max size of name.txt ie. sequence length */
#define MATCHSIZE 15000         /* max size of mout.txt ie. num of files    */
#define OBJLIMIT 150            /* max number of files per object           */
#define URLSIZE 80              /* max size of request string passed to srvr*/
#define MAXTHREAD 250          /* max number threads per process on client  */
#define READBUFFSZ 4096         /* size of buffer that data is read into    */
#define WRITEBUFFSZ 16384       /* size of buffer that requests are written */
#define MAXCONCTS 1             /* max number of connections for HTTP 1.1   */

/****************************************************************************/
/* Define the structures used for passing values to the reader routine      */
/****************************************************************************/

struct readerdata {        /* This is the data passed to each reader thread */
  int readerno;            /* reader thread id                              */
  int conctno;             /* Socket connection for this reader             */
  int threadno;            /* calling thread id                             */
  int nofiles;             /* number of files to be retrieved in pipeline   */
  long objectno;           /* index of object selected                      */
  long fileno[OBJLIMIT];   /* index of file selected                        */
  long startus;            /* start time of the transaction -  micro seconds*/
  long starts;             /* start time of the transaction -  seconds      */
};



