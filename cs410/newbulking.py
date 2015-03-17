import os
import traceback
from elasticsearch import Elasticsearch

ES_HOST = {
    "host" : "localhost", 
    "port" : 9200
}

INDEX_NAME = 'hdokani2_bigindex'
TYPE_NAME = 'doc'
address = "/Users/harshitdokania/Desktop/cs410/hw3part2"
myfile="Docs.txt"
bulk_data = []
body= []
count=0;
with open(os.path.join(address, myfile), 'r') as txt_file:
       for line in txt_file:
           line= line.replace('\n','')
           lhs, rhs = line.split(":",1)
           rhs=rhs.replace('}', '')
           rhs=rhs.replace(',', '')
           rhs=rhs.replace('\u', '')
           rhs=rhs.replace("\\", '')
           count=count+1
           if(count%4==1):
             url=rhs
           elif(count%4==2):
             body=rhs
           elif(count%4==3):
             doc_id=rhs
           elif(count%4==0):
             title=rhs
             op_dict = {
                "index" :{
                   "_index": INDEX_NAME, 
                   "_type": TYPE_NAME, 
                   "doc_id": myfile
                }
             }
             data_dict= {
                   "doc_id": doc_id,
        	   "url": url,
                   "title": title, 
                   "body" : body 
              }
             bulk_data.append(op_dict)
             bulk_data.append(data_dict)
             es = Elasticsearch(hosts = [ES_HOST])
           if(count%1000==0):
             print("bulk indexing...")
             res = es.bulk(index = INDEX_NAME, body = bulk_data, refresh=True)
             print(" response: '%s'" % (res))  
             bulk_data=[]
