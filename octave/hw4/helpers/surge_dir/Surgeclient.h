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
/* insuring that NAMESIZE >= wc objout.txt and OUTSIZE >= total requests    */
/* for files (shown after running zipf.c) and MATCHSIZE >= wc mout.txt and  */
/* OBJLIMIT >= OBJLIMIT from object.c and URLSIZE >= max size of all chars  */
/* which will make up any HTTP command string.                              */
/****************************************************************************/

#define PORT 80                 /* HTTP port on the server                  */
#define CLIENTNO 1              /* Client ID - must be unique on each system*/
#define SERVER "icapeople"      /* test server's name                       */
#define LOGFILE "Surge.log"     /* name of results log file                 */
#define NAMESIZE 110000         /* max size of name.txt ie. sequence length */
#define OUTSIZE  100000         /* max size of output trace file            */
#define MATCHSIZE 15000         /* max size of mout.txt ie. num of files    */
#define OBJLIMIT 150            /* max number of files per object           */
#define HTTPCMD "GET http://"   /* HTTP command which will be passed to srvr*/
#define HTTPDIR "/bradunov/surge"             /* root HTTP directory on server            */
#define HTTPEXTN ".txt"         /* file extension for test files on server  */
#define HTTPTAIL " HTTP/1.1\r\nUser-Agent: SURGE\r\nHost: newbury" /* tail  */
#define HTTPTAIL11 " HTTP/1.1\r\nUser-Agent: SURGE\r\nHost: newbury" /* 1.1 */
#define HTTPTAIL10 " HTTP/1.0\r\nUser-Agent: SURGE"           /*HTTP1.0 tail*/
#define HTTPTERM "\r\n\r\n"     /* termination character sequence for HTTP  */
#define URLSIZE 80              /* max size of request string passed to srvr*/
#define MAXCONCTS 1             /* max number of connections for HTTP 1.1   */

/****************************************************************************/
/* Define the input files - useful if you are doing different tests         */
/****************************************************************************/

#define MATCHFILE "mout.txt"   /* name of file output from match.c          */
#define OBJFILE "objout.txt"   /* name of file output from object.c         */
#define SEQFILE "name.txt"     /* name of output from lru.c                 */
#define MAXTHREAD 250          /* max number threads per process on client  */
#define SEQFIFO "fifo1"        /* FIFO which passes object seq # to client  */
#define OFFFIFO "fifo2"        /* FIFO which passes OFF times to client     */
#define OUTFIFO "fifo3"        /* FIFO which passes output seq # to client  */
#define CNTFIFO "fifo4"        /* FIFO which passes object count to client  */




