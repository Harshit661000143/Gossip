/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}



/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;
    
	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();
    
    /*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());
    
    //Bootstrap as ring is still empty.
    //Fetch current membership list from MP1
    //find your neighbors i.e. hasMyReplicas and haveReplicasOf
    if(ring.size() == 0) {
        ring = curMemList;
        findNeighbors();
        return;
    }
    
    
    //If my ring size is different from the currMemList, there has been a change in the ring.
    if(curMemList.size() != ring.size())
        change = true;
    
    if(ht->isEmpty())
        return;

    
    //If the sizes are same curMemList can still have changes i.e two nodes removing and two nodes adding. (size remains same). Therefore address at ith index of both the vectors should be same. If they are different there is a change.
    if(!change) {
        for(int i = 0; i < curMemList.size(); i++) {
            if (!( ring.at(i).nodeAddress == curMemList.at(i).nodeAddress)) {
                change = true;
                break;
            }
        }
    }
    
   //If there is a change fetch the new Membership List from MP1.
    // stabilizationProtocol takes steps for recovery that is ensuring three replicas of very key after the change in the ring.
    if(change) {
        ring=curMemList;
        stabilizationProtocol();
    }
    
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}




/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */
    
    //Initialize my UniqueToTransaction Table for that transactionID.
    initMyTable(key, value, memberNode->addr, CREATE);

    
    //Find the node where this key should go. Use consistent HASHING.
    vector<Node> replicasNode=findNodes(key);
    
    // creates three different message for three different servers.
    
    Message *msgP= new Message(g_transID, memberNode->addr, CREATE,key,value,PRIMARY);
    int sz=sizeof(*msgP);
    //send message to three nodes i.e. three copies of key. QUORUM
    emulNet->ENsend(&memberNode->addr, replicasNode.at(0).getAddress(),(char *)msgP,sz);
    
    Message *msgS= new Message(g_transID, memberNode->addr, CREATE,key,value,SECONDARY);
    emulNet->ENsend(&memberNode->addr,replicasNode.at(1).getAddress(),(char *)msgS,sz);

    Message *msgT= new Message(g_transID, memberNode->addr, CREATE,key,value,TERTIARY);
    emulNet->ENsend(&memberNode->addr, replicasNode.at(2).getAddress(),(char *)msgT,sz);
    
    
    //increment transaction ID so that next transaction gets the new ID.
    g_transID++;
    
    
    
}



/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */
    
    //Initialize my UniqueToTransaction Table for that transactionID.
    initMyTable(key,"", memberNode->addr, READ);
    
    //Find the node where this key should go. Use consistent HASHING.
    vector<Node> replicasNode=findNodes(key);
    
    
    
   // creates one READ message for three different servers.
    Message *msg= new Message(g_transID,memberNode->addr,READ,key);
  
    int sz=sizeof(*msg);

    //send message to three nodes i.e. to receive consistent copy from QUORUM.
    emulNet->ENsend(&memberNode->addr, replicasNode.at(0).getAddress(),(char *)msg,sz);
    emulNet->ENsend(&memberNode->addr,replicasNode.at(1).getAddress(),(char *)msg,sz);
    emulNet->ENsend(&memberNode->addr, replicasNode.at(2).getAddress(),(char *)msg,sz);

    
     //increment transaction ID so that next transaction gets the new ID.
    g_transID++;
    
    
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
    
    //Initialize my UniqueToTransaction Table for that transactionID.
    initMyTable(key, value, memberNode->addr, UPDATE);

    
    //Find the node where this key should go. Use consistent HASHING.
    vector<Node> replicasNode=findNodes(key);
    
    // creates three different message for three different servers.
    Message *msgP= new Message(g_transID, memberNode->addr, UPDATE,key,value,PRIMARY);
    int sz=sizeof(*msgP);
    
    //send message to three nodes i.e. to update three replicas. QUORUM.
    emulNet->ENsend(&memberNode->addr, replicasNode.at(0).getAddress(),(char *)msgP,sz);
    
    Message *msgS= new Message(g_transID, memberNode->addr, UPDATE,key,value,SECONDARY);
    emulNet->ENsend(&memberNode->addr,replicasNode.at(1).getAddress(),(char *)msgS,sz);
    
    Message *msgT= new Message(g_transID, memberNode->addr, UPDATE,key,value,TERTIARY);
    emulNet->ENsend(&memberNode->addr, replicasNode.at(2).getAddress(),(char *)msgT,sz);

    //increment transaction ID so that next transaction gets the new ID.
    g_transID++;
    
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */
    
    // Initialize my UniqueToTransaction Table for that transactionID.
    initMyTable(key,"", memberNode->addr, DELETE);
    
    //Find the node where this key should go. Use consistent HASHING.
    vector<Node> replicasNode=findNodes(key);
    
    

    // creates one DELETE message for three different servers.
    Message *msg= new Message(g_transID,memberNode->addr,DELETE,key);
    int sz=sizeof(*msg);

    
    //send message to delete copies from three nodes i.e. to achieve QUORUM.
    emulNet->ENsend(&memberNode->addr, replicasNode.at(0).getAddress(),(char *)msg,sz);
    emulNet->ENsend(&memberNode->addr,replicasNode.at(1).getAddress(),(char *)msg,sz);
    emulNet->ENsend(&memberNode->addr, replicasNode.at(2).getAddress(),(char *)msg,sz);
    

   
    
    
    
     //increment transaction ID so that next transaction gets the new ID.
    g_transID++;
    
    
    
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica, int transID, Address coordinator) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
    
    
    bool result= ht->create(key, value);
    //if result is FALSE key has been not CREATED at server.
    //LOG FAIL
    
    if(!result)
        log->logCreateFail(&memberNode->addr,false,transID,key,value);
    //LOG FAIL
    //Insert key in your respective set of Replicas.
    else
    {
        log->logCreateSuccess(&memberNode->addr,false,transID,key,value);
        insertKeysinMySet(replica,key);
    }
    
    
    // creates one REPLY message for coordinator.
    //sends transID for coordintaor to identify which transaction is this reply for.
    
    Message *msg= new Message(transID, memberNode->addr,REPLY,result);
    int sz=sizeof(*msg);
    emulNet->ENsend(&memberNode->addr,&coordinator,(char *)msg,sz);
    return result;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key,int transID,Address coordinator) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
    string value=ht->read(key);
    
    //IF value is empty LOG READ FAIL
    if(value.empty())
        log->logReadFail(&memberNode->addr,false,transID,key);
    
    //else LOG SUCCESS
    else
        log->logReadSuccess(&memberNode->addr, false,transID,key,value);
    
   
    // creates one READREPLY message for coordinator.
    //sends transID for coordintaor to identify which transaction is this reply for.

    Message *msg= new Message(transID, memberNode->addr,value);
    int sz=sizeof(*msg);
    emulNet->ENsend(&memberNode->addr,&coordinator,(char *)msg,sz);
    return value;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica, int transID,Address coordinator) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
    bool result= ht->update(key, value);
    //if result is FALSE key has been not UPDATED at server.
    //LOG FAIL
        if(!result)
        log->logUpdateFail(&memberNode->addr,false, transID,key,value);
    //LOG SUCCESS
    //Insert key in your respective set of Replicas.
    else{
        log->logUpdateSuccess(&memberNode->addr,false, transID,key,value);
    insertKeysinMySet(replica,key);
    }

    
    
    // creates one REPLY message for coordinator.
    //sends transID for coordintaor to identify which transaction is this reply for.
    
    Message *msg= new Message(transID, memberNode->addr,REPLY,result);
    int sz=sizeof(*msg);
    emulNet->ENsend(&memberNode->addr,&coordinator,(char *)msg,sz);
    return result;
    
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key,int transID,Address coordinator) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
    bool result=ht->deleteKey(key);
    
    //if result is true key has been DELETED at server.
    //LOG Success
    //DELETE key from your respective set of Replicas.
    if(!result)
        log->logDeleteFail(&memberNode->addr,false,transID,key);
    //LOG FAIL
    else
    {
        log->logDeleteSuccess(&memberNode->addr,false,transID,key);
        
        bool existsinPrimary = iamPrimary.find(key) != iamPrimary.end();
        bool existsinSecondary = iamSecondary.find(key) != iamSecondary.end();
        bool existsinTertiary = iamTertiary.find(key) != iamTertiary.end();
        if(existsinPrimary)
            iamPrimary.erase(key);
        if(existsinSecondary)
            iamSecondary.erase(key);
        if (existsinTertiary)
            iamTertiary.erase(key);
    }
        
    
    
    // creates one REPLY message for coordinator.
    //sends transID for coordintaor to identify which transaction is this reply for.

    Message *msg= new Message(transID, memberNode->addr,REPLY,result);
    int sz=sizeof(*msg);
    emulNet->ENsend(&memberNode->addr,&coordinator,(char *)msg,sz);
    return result;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;
    
	/*
	 * Declare your local variables here
	 */
    
	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();
        
		/*string message(data, data + size);
         * Handle the message types here
         * This function should also ensure all READ and UPDATE operation
         * get QUORUM replies
         */
        
        //mp2 node process its MessageType i.e. CREATE, READ, UPDATE, DELETE, REPLY, READREPLY
        recvCallBack((void *)memberNode, (char *)data, size);
        
	}
    
	
}
/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */

bool MP2Node::recvCallBack(void *env, char *data, int size ) {
    
    //Typecast data into Message
	Message *msg = (Message *)data;
    /*CREATE, READ, UPDATE, DELETE, REPLY, READREPLY*/
    switch(msg->type) {
        case CREATE:
            createKeyValue(msg->key, msg->value, msg->replica,msg->transID,msg->fromAddr);
            break;
        case READ:
            readKey(msg->key,msg->transID,msg->fromAddr);
            break;
        case UPDATE:
            updateKeyValue(msg->key,msg->value, msg->replica,msg->transID,msg->fromAddr);
            break;
        case DELETE:
            deletekey(msg->key,msg->transID,msg->fromAddr);
            break;
        case REPLY:
            processReply(msg->transID, msg->success);
            break;
        case READREPLY:
            processReadReply(msg->transID,msg->value);
            break;
            
        default:cout<<"INVALID OPERATION";
            break;
    }
}
/**
 * FUNCTION NAME: processReply
 *
 * DESCRIPTION: Increase successCount/failCount based on the result returned by replicas
 */

void MP2Node::processReply(int transID, bool result)
{
    if(result){
        MyTable[transID].successCount++;
        if(MyTable[transID].successCount==2)
        {
            switch(MyTable[transID].coordinatorMsg){
                case CREATE:
                    log->logCreateSuccess(&memberNode->addr,true,transID,MyTable[transID].key,MyTable[transID].value);
                    break;
                case UPDATE:
                    log->logUpdateSuccess(&memberNode->addr,true,transID,MyTable[transID].key,MyTable[transID].value);
                    break;
                case DELETE:
                    log->logDeleteSuccess(&memberNode->addr,true,transID,MyTable[transID].key);
                    break;
                default: cout<<"REPLY MSG ERROR DURING SUCCESS";
            }
            MyTable[transID].alreadyLogged=true;

        }
    }
    else
    {
        MyTable[transID].failCount++;
        if(MyTable[transID].failCount==2 ||par->getcurrtime()-MyTable[transID].transTime>=TIMEOUT)
        {
            
            switch(MyTable[transID].coordinatorMsg){
                case CREATE:
                    log->logCreateFail(&memberNode->addr, true,transID,
                                       MyTable[transID].key,MyTable[transID].value);
                    
                    break;
                case UPDATE:
                    log->logUpdateFail(&memberNode->addr,true, transID,MyTable[transID].key,MyTable[transID].value);
                    
                    break;
                case DELETE:
                    log->logDeleteFail(&memberNode->addr,true,transID,MyTable[transID].key);
                    break;
                default: cout<<"REPLY MSG ERROR FAILURE";
            }
            MyTable[transID].alreadyLogged=true;
        }
    }
}

void MP2Node::processReadReply(int transID, string value)
{
    if(value.empty())
    {
        MyTable[transID].failCount++;
        if(MyTable[transID].failCount==2)
        {
            log->logReadFail(&memberNode->addr,true,transID,MyTable[transID].key);
            MyTable[transID].alreadyLogged=true;
        }
    }
    else
    {
        MyTable[transID].readValues[MyTable[transID].successCount]=value;
        MyTable[transID].successCount++;
        if(MyTable[transID].successCount==2)
        {
            if(MyTable[transID].readValues[0].compare(MyTable[transID].readValues[1])==0){
                log->logReadSuccess(&memberNode->addr,true,transID,MyTable[transID].key,value);
                MyTable[transID].alreadyLogged=true;
            }
            
        }
        else if(MyTable[transID].successCount==3)
        {
            if((MyTable[transID].readValues[1].compare(MyTable[transID].readValues[2])==0) ||(MyTable[transID].readValues[0].compare(MyTable[transID].readValues[2])==0)||(MyTable[transID].readValues[0].compare(MyTable[transID].readValues[1])==0))
            {
                if(!MyTable[transID].alreadyLogged)
                {
                    log->logReadSuccess(&memberNode->addr,true,transID,MyTable[transID].key,value);
                    MyTable[transID].alreadyLogged=true;
                }
            }
            else
            {
                log->logReadFail(&memberNode->addr,true,transID,MyTable[transID].key);
                MyTable[transID].alreadyLogged=true;
                
            }
            
        }
    }
}


/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */
    for (map<int,UniqueToTransaction>::iterator it=MyTable.begin(); it!=MyTable.end(); ++it)
    {
        if((!it->second.alreadyLogged)&&((par->getcurrtime() - it->second.transTime)>=TIMEOUT))
        {
            switch(it->second.coordinatorMsg){
                case CREATE:
                    log->logCreateFail(&memberNode->addr, true,it->first,
                                       it->second.key,it->second.value);
                    it->second.alreadyLogged=true;
                    break;
                case UPDATE:
                    log->logUpdateFail(&memberNode->addr,true, it->first,it->second.key,it->second.value);
                    it->second.alreadyLogged=true;
                    break;
                case DELETE:
                    log->logDeleteFail(&memberNode->addr,true,it->first,it->second.key);
                    it->second.alreadyLogged=true;
                    break;
                case READ:
                    log->logReadFail(&memberNode->addr,true,it->first,it->second.key);
                    it->second.alreadyLogged=true;
                    break;
                    
                    
                default: cout<<"STABALIZATION ERROR FAILURE";
            }
            
            
        }
        
    }
    findNeighbors();
}

void MP2Node::findNeighbors()
{
    
    bool haveArray[2]={false,false};
    bool hasArray[2]={false,false};
    vector<Node> mySuccessors;
    vector<Node> myPredecessors;
    //haveReplicasOf.clear();
    //hasMyReplicas.clear();
    int neighbour;
    Address myAddr =memberNode->addr;
    int have;
    int has;
    //Find my neignbour nodes, and update my hasMyReplicas & haveReplicasOf
    for(int i = 0; i < ring.size(); i++) {
        
        if ( ring.at(i).nodeAddress == myAddr)
        {
            // This is me on the ring
            have = i;
            has= i;
            
            neighbour=0;
            //Find two neighbours
            while(neighbour!=2){
                //If first node at ring, assign haveReplicas to the end of the ring
                if(have==0)
                    have = ring.size()-1;
                else
                    --have;
                myPredecessors.push_back(ring.at(have));
                ++has;
                if(has==ring.size())
                    has=0;
                mySuccessors.push_back(ring.at(has));
                neighbour++;
            }
            break;
        }
    }
    if(haveReplicasOf.empty() && hasMyReplicas.empty())
    {
        hasMyReplicas=mySuccessors;
        haveReplicasOf=myPredecessors;
    }
    else
    {
        
        for(int i = 0; i < myPredecessors.size(); i++)
        {
            for(int j=0;j<myPredecessors.size(); j++)
            {
                if(haveReplicasOf.at(i).nodeHashCode==myPredecessors.at(j).nodeHashCode)
                    haveArray[i]=true;
            }
        }
        
        for(int i = 0; i < mySuccessors.size(); i++)
        {
            for(int j=0;j<mySuccessors.size(); j++)
            {
                if(hasMyReplicas.at(i).nodeHashCode==mySuccessors.at(j).nodeHashCode)
                    hasArray[i]=true;
            }
        }
        //calls with boolean array
        
        if(haveArray[0]!=true||haveArray[1]!=true)
        {
            haveReplicasOf=myPredecessors;
            deletedMypredecessors(haveArray);
        }
        if(hasArray[0]!=true||hasArray[1]!=true)
        {
            hasMyReplicas=mySuccessors;
            deletedMysuccessors(hasArray);
        }
    }
    
}
void MP2Node:: deletedMysuccessors(bool hasArray[2])
{
    //    cout<<"reached";
    string value;
    if(hasArray[0]==false && hasArray[1]==false)
    {
        for (std::set<string>::iterator it=iamPrimary.begin(); it!=iamPrimary.end(); ++it){
            value=ht->read(*it);
            
            Message *msgS= new Message(g_transID, memberNode->addr, CREATE,*it,value,SECONDARY);
            int sz=sizeof(*msgS);
            emulNet->ENsend(&memberNode->addr,hasMyReplicas.at(0).getAddress(),(char *)msgS,sz);
            Message *msgT= new Message(g_transID, memberNode->addr, CREATE,*it,value,TERTIARY);
            emulNet->ENsend(&memberNode->addr, hasMyReplicas.at(1).getAddress(),(char *)msgT,sz);
            g_transID++;
            
            
            
            
        }
    }
    else if((hasArray[0]==false &&hasArray[1]==true) || (hasArray[0]==true && hasArray[1]==false)){
        
        for (std::set<string>::iterator it=iamPrimary.begin(); it!=iamPrimary.end(); ++it){
            value=ht->read(*it);
            
            Message *msgT= new Message(g_transID, memberNode->addr, CREATE,*it,value,TERTIARY);
            g_transID++;
            int sz=sizeof(*msgT);
            emulNet->ENsend(&memberNode->addr, hasMyReplicas.at(1).getAddress(),(char *)msgT,sz);
            
        }
    }
    
    
    
}
void MP2Node:: deletedMypredecessors(bool haveArray[2])
{
    if(haveArray[0]==false && haveArray[1]==false)
    {
        iamPrimary.insert(iamSecondary.begin(),iamSecondary.end());
        iamPrimary.insert(iamTertiary.begin(),iamTertiary.end());
    }
    else if(haveArray[0]==false)
    {
        iamPrimary.insert(iamSecondary.begin(),iamSecondary.end());
        iamSecondary=iamTertiary;
    }
    
    else if(haveArray[1]==false)
    {
        iamSecondary.insert(iamTertiary.begin(),iamTertiary.end());
    }
}


