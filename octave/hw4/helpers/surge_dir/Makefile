#**************************************************************************
#                  Copyright 1997, Trustees of Boston University.
#                               All Rights Reserved.                      
#                                                                         
# Permission to use, copy, or modify this software and its documentation  
# for educational and research purposes only and without fee is hereby    
# granted, provided that this copyright notice appear on all copies and   
# supporting documentation.  For any other uses of this software, in      
# original or modified form, including but not limited to distribution in 
# whole or in part, specific prior permission must be obtained from Boston
# University.  These programs shall not be used, rewritten, or adapted as 
# the basis of a commercial software or hardware product without first    
# obtaining appropriate licenses from Boston University.  Boston University
# and the author(s) make no representations about the suitability of this  
# software for any purpose.  It is provided "as is" without express or     
# implied warranty.                                                        
#                                                                          
#**************************************************************************
#
# Title:   Makefile for SURGE files
# Author:  Paul Barford
# Date:    Rev. 1.1 4/1/98
#
# NOTE:  Pthreads "compiler" is required to compile Surgeclient - See Howto
#
# NOTE:  This file also has a swithc to enable compilation under kernel
# threads.  Just use the following switch:
#
#**************************************************************************

CC = gcc
CCOPT = -Wall -O3
LIBS = -lm
SRC = files lru match surfoff pbvalclnt pbvalsrvr sizes \
	zipf object client11p Sclient Smaster

SRC10 = client10 Sclient

SRC11 = client11 Sclient

SRC11p = client11p Sclient

all:  $(SRC)

Sclnt10: $(SRC10)

Sclnt11: $(SRC11)

Sclnt11p: $(SRC11p)

files: files.c
	$(CC) $(CCOPT) $(LIBS) -o $@ files.c

object: object.c
	$(CC) $(CCOPT) $(LIBS) -o $@ object.c

lru: lru.c
	$(CC) $(CCOPT) $(LIBS) -o $@ lru.c

match: match.c
	$(CC) $(CCOPT) $(LIBS) -o $@ match.c

surfoff: surfoff.c
	$(CC) $(CCOPT) $(LIBS) -o $@ surfoff.c

pbvalclnt: pbvalclnt.c
	$(CC) $(CCOPT) $(LIBS) -o $@ pbvalclnt.c

pbvalsrvr: pbvalsrvr.c
	$(CC) $(CCOPT) $(LIBS) -o $@ pbvalsrvr.c

sizes: sizes.c
	$(CC) $(CCOPT) $(LIBS) -o $@ sizes.c

zipf: zipf.c
	$(CC) $(CCOPT) $(LIBS) -o $@ zipf.c

Smaster: Surgemaster.c
	$(CC) $(CCOPT) -o Surge Surgemaster.c

Sclient: Surgeclient.c Surgeclient.h
	$(CC) $(CCOPT) -lpthread -o Surgeclient Surgeclient.c client.o

client10: client10.c client10.h
	$(CC) $(CCOPT) -c -o client.o client10.c

client11: client11.c client11.h
	$(CC) $(CCOPT) -c -o client.o client11.c

client11p: client11p.c client11p.h
	$(CC) $(CCOPT) -c -o client.o client11p.c

clean:
	rm -f core lru match surfoff object \
	pbvalsrvr pbvalclnt sizes zipf files *.txt client.o Surge Surgeclient

tar:
	tar -cvf surge_1.00a.tar *

