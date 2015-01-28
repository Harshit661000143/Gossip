/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_
#define TIMEOUT 10
/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"
#include <set>
/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
struct UniqueToTransaction{
    int successCount;
    int failCount;
    string key;
    string value;
    string readValues[3];
    Address coordinator;
    MessageType coordinatorMsg;
    bool alreadyLogged;
    int transTime;
};

class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
    map<int,UniqueToTransaction> MyTable;
    set<string> iamPrimary;
    set<string> iamSecondary;
    set<string> iamTertiary;

   
public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
    void processReply(int transID, bool result);
    void processReadReply(int transID, string value);
    void deletedMypredecessors(bool haveArray[2]);
    void deletedMysuccessors(bool hasArray[2]);

	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
    bool recvCallBack(void *env, char *data, int size);
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();
    
   // void initMyTable(string key, string value, Address coordinator, MessageType coordinatorMsg);

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica, int transID,Address coordinator);
	string readKey(string key, int transID, Address coordinator);
	bool updateKeyValue(string key, string value, ReplicaType replica, int transID,Address coordinator);
	bool deletekey(string key, int transID,Address coordinator);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();
    
    void initMyTable(string key, string value, Address coordinator, MessageType coordinatorMsg){
        MyTable[g_transID].successCount=0;
        MyTable[g_transID].failCount=0;
        MyTable[g_transID].key=key;
        MyTable[g_transID].value=value;
        MyTable[g_transID].coordinator=coordinator;
        MyTable[g_transID].coordinatorMsg=coordinatorMsg;
        MyTable[g_transID].alreadyLogged=false;
        MyTable[g_transID].transTime=par->getcurrtime();
        
    }
    void insertKeysinMySet(ReplicaType replica,string key)
    {
        switch(replica)
        {
            case PRIMARY:
                iamPrimary.insert(key);
                break;
            case SECONDARY:
                iamSecondary.insert(key);
                break;
            case TERTIARY:
                iamTertiary.insert(key);
                break;
        }
        
    }


	~MP2Node();
};

#endif /* MP2NODE_H_ */
