/**********************
*
* Progam Name: MP1. Membership Protocol
* 
* Code authors: <Harshit Dokania>
*
* Current file: mp1_node.c
* About this file: Member Node Implementation
* 
***********************/

#include "mp1_node.h"
#include "emulnet.h"
#include "MPtemplate.h"
#include "log.h"


/*
 *
 * Routines for introducer and current time.
 *
 */


char NULLADDR[] = {0,0,0,0,0,0};
int isnulladdr( address *addr){
    return (memcmp(addr, NULLADDR, 6)==0?1:0);
}

/* 
Return the address of the introducer member. 
*/
address getjoinaddr(void){

    address joinaddr;

    memset(&joinaddr, 0, sizeof(address));
    *(int *)(&joinaddr.addr)=1;
    *(short *)(&joinaddr.addr[4])=0;

    return joinaddr;
}

/*
 *
 * Message Processing routines.
 *
 */

/* 
Received a JOINREQ (joinrequest) message.
*/
void Process_joinreq(void *env, char *data, int size)
{
	/* <your code goes in here> */
    //call function to check if this node has been added
    if(existsInList((member *)env, data))
    {
        //do nothing, Already joined
    }
    else {
        member *introducer = (member *) env;
        introducer->mTable[introducer->listSize].heartBeat = 0;
        introducer->mTable[introducer->listSize].time = getcurrtime();
        address joinme;
        memset(&joinme, 0, sizeof(address));
        *(int *)(&joinme.addr)=data[0];
        *(short *)(&joinme.addr[4])=0;
        introducer->mTable[introducer->listSize].addr = joinme;
        introducer->mTable[introducer->listSize].bfailed = 0;
        introducer->listSize++;
        printf("New node has been added in introducer's membership list");
        logNodeAdd(&(introducer->addr), &joinme);
        respondtoNewnode(introducer, &joinme);
    }
    return;
}
int respondtoNewnode(member *introducer, address * joinme){
    
    messagehdr *sentmsg;
#ifdef DEBUGLOG
    static char s[1024];
#endif
    
    size_t msgsize = sizeof(messagehdr) + sizeof(address);
    sentmsg=malloc(msgsize);
    
    /* create JOINREP message: format of data is {struct address myaddr} */
    sentmsg->msgtype=JOINREP;
    memcpy((char *)(sentmsg+1), &introducer->addr, sizeof(address));
    
#ifdef DEBUGLOG
    sprintf(s,"Sending JOINREP");
    LOG(&introducer->addr, s);
#endif
    
    /* send JOINREP message to introducer member. */
    MPp2psend(&introducer->addr, joinme, (char *)sentmsg, msgsize);
    
    free(sentmsg);
    
    
    
    return 1;
}




int existsInList(member *env,char *joinme){
    int iter=0;
    //search for this node in the membershiplist
    for(iter=0;iter<env->listSize;iter++){
        if (joinme[0] == env->mTable[iter].addr.addr[0]){
            LOG(&env->addr,"This node is already added");
            return iter;
        }
        
    }
    //This node has been not added yet, Please add me to membership list
    return 0;
    
}



void Process_joinrep(void *env, char *data, int size)
{
    member *nonintroducer = (member *) env;
    //messagehdr *msghdr = (messagehdr *)data;
    //char *pktdata = (char *)(msghdr+1);
    printf(" JOIN RESPONSE for new node added \n");
    
    //set the group membership
    nonintroducer->ingroup=1;
    return;
}

void Process_gossip(void *env, char *data, int size){
    
    member *node = (member *)env;
    int *newTableSize =data;
    
    membershipTable *newTable=data+sizeof(int);
    
    
    if(!(node->ingroup))
        return;
    else{
        mergeMembershiptable(node,&node->listSize,newTable,newTableSize);
    }
    
    return;
    
}
void mergeMembershiptable(member * introducer,int *  introListSize,membershipTable * sendersTable,int * sendersListSize){
    
    
    membershipTable * introTable = introducer->mTable;
   // int size1 = node->currSize;
    
    for (int i = 0; i < *sendersListSize ; i ++) {
        if (sendersTable[i].bfailed == 1)
            continue;
        int j = 0;
        for (j = 0; j < *introListSize ; j ++) {
            
            if (sendersTable[i].addr.addr[0] == introTable[j].addr.addr[0]) {
                if (introTable[j].bfailed != 1 && introTable[j].heartBeat < sendersTable[i].heartBeat) {
                    introTable[j].heartBeat = sendersTable[i].heartBeat;
                    introTable[j].time = getcurrtime();
                }
                break;
            }
            
        }
        if (j == *introListSize) {
            logNodeAdd(&(introducer->addr),&(sendersTable[i].addr));
            introTable[introducer->listSize].addr = sendersTable[i].addr;
            introTable[introducer->listSize].heartBeat = sendersTable[i].heartBeat;
            introTable[introducer->listSize].time = getcurrtime();
            introducer->listSize++;
        }
    }
    return;
    
}

void Process_timeToDelete(void *env, char *data, int size)
{
    
    member *node = (member *) env;
    int listSize = node->listSize;
    
    
    
    membershipTable * myTable = node->mTable;
    for (int i = 0; i < listSize; i++ ) {
        
        if ((abs(myTable[i].time - getcurrtime()) > FAIL)) {
            // Time out
            myTable[i].bfailed = 1;
        }
        
        if ((abs(myTable[i].time - getcurrtime()) > CLEANUP)) {
            // Remove the node
            logNodeRemove(&(node->addr) , &(myTable[i].addr));
            
            //Now we fill that empty node slot with the last node's history in membershipTable myTable
            myTable[i].addr = myTable[node->listSize-1].addr;
            myTable[i].heartBeat = myTable[node->listSize-1].heartBeat;
            myTable[i].time = myTable[node->listSize-1].time;
            myTable[i].bfailed = 0;
            node->listSize--;
            listSize--;
            i--;
        }
    }
    
    
}

/*
Array of Message handlers.
*/
void ( ( * MsgHandler [20] ) STDCLLBKARGS )={
/* Message processing operations at the P2P layer. */
    Process_joinreq, 
    Process_joinrep,
    Process_gossip,
    Process_timeToDelete
};

/* 
Called from nodeloop() on each received packet dequeue()-ed from node->inmsgq. 
Parse the packet, extract information and process. 
env is member *node, data is 'messagehdr'. 
*/
int recv_callback(void *env, char *data, int size){

    member *node = (member *) env;
    messagehdr *msghdr = (messagehdr *)data;
    char *pktdata = (char *)(msghdr+1);

    if(size < sizeof(messagehdr)){
#ifdef DEBUGLOG
        LOG(&((member *)env)->addr, "wrong MSG");
#endif
        return -1;
    }

#ifdef DEBUGLOG
    LOG(&((member *)env)->addr, "received msg type %d of size %d", msghdr->msgtype, size - sizeof(messagehdr));
#endif

    if((node->ingroup && msghdr->msgtype >= 0 && msghdr->msgtype <= DUMMYLASTMSGTYPE)
        || (!node->ingroup && msghdr->msgtype==JOINREP))            
            /* if not yet in group,JOINREP could be only processed */
        MsgHandler[msghdr->msgtype](env, pktdata, size-sizeof(messagehdr));
    
    free(data);

    return 0;

}

/*
 *
 * Initialization and cleanup routines.
 *
 */

/* 
Find out who I am, and start up. 
*/
int init_thisnode(member *thisnode, address *joinaddr){
    //
    if(MPinit(&thisnode->addr, PORTNUM, (char *)joinaddr)== NULL){ /* Calls ENInit */
#ifdef DEBUGLOG
        LOG(&thisnode->addr, "MPInit failed");
#endif
        exit(1);
    }
#ifdef DEBUGLOG
    else LOG(&thisnode->addr, "MPInit succeeded. Hello.");
#endif

    thisnode->bfailed=0;
    thisnode->inited=1;
    thisnode->ingroup=0;
    /* node is up! */
    //All node getting their membership table initialized.
    thisnode->mTable=(membershipTable *)malloc(MAX_NNB*sizeof(membershipTable));
    thisnode->mTable[0].addr = thisnode->addr;
    thisnode->mTable[0].heartBeat = 0;
    thisnode->mTable[0].time = getcurrtime();
    thisnode->listSize = 1;
    logNodeAdd(&(thisnode->addr),&(thisnode->addr));
    return 0;
}


/* 
Clean up this node. 
*/
int finishup_thisnode(member *node){

	/* <your code goes in here> */
     free(node->mTable);
    return 0;
}


/* 
 *
 * Main code for a node 
 *
 */

/* 
Introduce self to group. 
*/
int introduceselftogroup(member *node, address *joinaddr){
    
    messagehdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if(memcmp(&node->addr, joinaddr, 4*sizeof(char)) == 0){
        /* I am the group booter (first process to join the group). Boot up the group. */
#ifdef DEBUGLOG
        LOG(&node->addr, "Starting up group...");
#endif

        node->ingroup = 1;
    }
    else{
        size_t msgsize = sizeof(messagehdr) + sizeof(address);
        msg=malloc(msgsize);

    /* create JOINREQ message: format of data is {struct address myaddr} */
        msg->msgtype=JOINREQ;
        //piggy back address after the msgtype
        memcpy((char *)(msg+1), &node->addr, sizeof(address));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        LOG(&node->addr, s);
#endif

    /* send JOINREQ message to introducer member. */
        MPp2psend(&node->addr, joinaddr, (char *)msg, msgsize);
        
        free(msg);
    }

    return 1;

}

/* 
Called from nodeloop(). 
*/
void checkmsgs(member *node){
    void *data;
    int size;

    /* Dequeue waiting messages from node->inmsgq and process them. */
	
    while((data = dequeue(&node->inmsgq, &size)) != NULL) {
        recv_callback((void *)node, data, size); 
    }
    return;
}


/* 
Executed periodically for each member. 
Performs necessary periodic operations. 
Called by nodeloop(). 
*/
void nodeloopops(member *node){

	/* <your code goes in here> */
    if (node->listSize == 1)
        return;
    
   
    int index=-1;
    while((index = rand() % node->listSize) == 0);
    
    /* create GOSSIP message: format of data is {struct address myaddr} */
    messagehdr *msg;
    size_t msgsize = sizeof(messagehdr) + sizeof(int) +(sizeof(membershipTable)*(node->listSize)) ;
    msg=malloc(msgsize);
    
    msg->msgtype=GOSSIP;
    memcpy((char *)(msg+1), &node->listSize, sizeof(int));
    memcpy(((char *)(msg+1)+ sizeof(int)), node->mTable, sizeof(membershipTable)* (node->listSize));
    
        /* send Gossip message to selected index member. */
        MPp2psend(&node->addr, &node->mTable[index].addr, (char *)msg, msgsize);
        
        free(msg);
    // Periodic DELETE check
   size_t newmsgsize = sizeof(messagehdr);
    messagehdr *newmsg;
    newmsg = malloc(newmsgsize);
    
    newmsg->msgtype = DELETE;
    
    MPp2psend(&node->addr, &node->addr, (char *)newmsg, newmsgsize);
    
    free(newmsg);
    
    // increasing my own heartbeat
    node->mTable[0].heartBeat++;
    // Updating its local time at which heartbeat was increased
    node->mTable[0].time = getcurrtime();
    
    
    return;
}

    

/* 
Executed periodically at each member. Called from app.c.
*/
void nodeloop(member *node){
    if (node->bfailed) return;

    checkmsgs(node);

    /* Wait until you're in the group... */
    if(!node->ingroup) return ;

    /* ...then jump in and share your responsibilites! */
    nodeloopops(node);
    
    return;
}

/* 
All initialization routines for a member. Called by app.c. 
*/
void nodestart(member *node, char *servaddrstr, short servport){
    
    //introducer's address
    address joinaddr=getjoinaddr();

    /* Self booting routines */
    if(init_thisnode(node, &joinaddr) == -1){

#ifdef DEBUGLOG
        LOG(&node->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if(!introduceselftogroup(node, &joinaddr)){
        finishup_thisnode(node);
#ifdef DEBUGLOG
        LOG(&node->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/* 
Enqueue a message (buff) onto the queue env. 
*/
int enqueue_wrppr(void *env, char *buff, int size){    return enqueue((queue *)env, buff, size);}

/* 
Called by a member to receive messages currently waiting for it. 
*/
int recvloop(member *node){
    if (node->bfailed) return -1;
    else return MPrecv(&(node->addr), enqueue_wrppr, NULL, 1, &node->inmsgq); 
    /* Fourth parameter specifies number of times to 'loop'. */
}


