/**********************
*
* Progam Name: MP1. Membership Protocol.
* 
* Code authors: <Harshit Dokania>
*
* Current file: mp2_node.h
* About this file: Header file.
* 
***********************/

#ifndef _NODE_H_
#define _NODE_H_

#include "stdincludes.h"
#include "params.h"
#include "queue.h"
#include "requests.h"
#include "emulnet.h"
#define max(a,b) (((a)>(b))?(a):(b))
#define FAIL 20
#define CLEANUP 40
/* Configuration Parameters */
char JOINADDR[30];                    /* address for introduction into the group. */
extern char *DEF_SERVADDR;            /* server address. */
extern short PORTNUM;                /* standard portnum of server to contact. */

/* Miscellaneous Parameters */
extern char *STDSTRING;

typedef struct membershipTable {
    struct address addr;
    long heartBeat;
    int time;
    int bfailed;
} membershipTable;

typedef struct member{
        struct address addr;            // my address
        int inited;                     // boolean indicating if this member is up
        int ingroup;                    // boolean indiciating if this member is in the group

        queue inmsgq;                   // queue for incoming messages
    
        int bfailed;                    // boolean indicating if this member has failed
        membershipTable *mTable;
        int listSize;
} member;

/* Message types */
/* Meaning of different message types
  JOINREQ - request to join the group
  JOINREP - replyto JOINREQ
*/
enum Msgtypes{
		JOINREQ,			
		JOINREP,
        GOSSIP,
        DELETE,
        DUMMYLASTMSGTYPE
};

/* Generic message template. */
typedef struct messagehdr{ 	
	enum Msgtypes msgtype;
} messagehdr;


/* Functions in mp2_node.c */

/* Message processing routines. */
STDCLLBKRET Process_joinreq STDCLLBKARGS;
STDCLLBKRET Process_joinrep STDCLLBKARGS;
STDCLLBKRET Process_gossip STDCLLBKARGS;


int recv_callback(void *env, char *data, int size);
int init_thisnode(member *thisnode, address *joinaddr);


/*
 Other routines.
 */


int existsInList(member *introducer, char *joiningaddress);
void mergeMembershiptable(member * introducer,int *  introListSize,membershipTable * sendersTable,int * sendersListSize);
int respondtoNewnode(member *introducer, address * joinme);



void nodestart(member *node, char *servaddrstr, short servport);
void nodeloop(member *node);
int recvloop(member *node);
int finishup_thisnode(member *node);

#endif /* _NODE_H_ */


