/*
 * main.cc
 *
 *  Created on: Jun 13, 2016
 *      Author: tobias
 */

#include "BWTree/BWTree.h"
#include <stdio.h>      /* printf, scanf, NULL */
#include <stdlib.h>     /* malloc, free, rand */
#include <thread>


bool customer_sorter(BWTree<int,int>::DataItem const& lhs, BWTree<int,int>::DataItem const& rhs) {
    if (lhs.key != rhs.key)
        return lhs.key < rhs.key;
    return false;
}



int main() {


	//Construct new BW Tree
	char* buffer =(char*) malloc(4000);
	BWTree<int, int> bw(buffer);

	bw.printMappingTable();

	//Insert items
	for (int var = 0; var < 10; ++var) {
		bw.insert(var,var);
	}


	PID rootNode = bw.m_root;
	NodeID rootNID = bw.getNodeID(rootNode);
	BWTree<int,int>::DecodedNodeID rootDec = bw.decodeNodeID(rootNID);
	// Innerinsert
	NodeID insertNode = bw.createNodeID(
			BWTree<int, int>::NodeType::InnerInsertType, rootDec.endOffset);
	bw.InstallNodeToReplace(rootNode, insertNode, rootNID);
	int key = 25;
	int highkey = 30;
	new (bw.m_buffer + rootDec.endOffset) BWTree<int, int>::InnerInsertNode(key,
			0, rootNID, highkey, 1);

	BWTree<int,int>::LogicalInnerNode lin(insertNode,rootNode,bw.m_buffer);
	lin.printNode();

	NodeID consID;
	bw.printMappingTable();
	bw.consolidate(insertNode,rootNode,consID);
	bw.printMappingTable();
	LOG("Consolidated NID " << consID);
	LOG("Root Node" << rootNode);
	bw.insert(9, 10);
	PID leafNodePID = bw.traverse(0);
	NodeID lfNID = bw.getNodeID(leafNodePID);

	std::cout << "Leaf Node" <<leafNodePID << std::endl;
	BWTree<int, int>::LogicalLeafeNode lf(lfNID, leafNodePID, bw.m_buffer);

	std::cout << "Print logical Node item" << std::endl;
	lf.printNode();


	bw.firstChildSplit(leafNodePID);
//
	PID splitNodePID = bw.traverse(5);
	std::cout << "SPLIT "<<splitNodePID << std::endl;
	bw.printMappingTable();
	NodeID lfNID2 = bw.getNodeID(3);

	std::cout << leafNodePID << std::endl;
	BWTree<int, int>::LogicalLeafeNode lf2(lfNID2, 3, bw.m_buffer);

	lf2.printNode();


//	PID rootPID = bw.m_root.load();
//	std::cout << rootPID << std::endl;
//	NodeID rootID = bw.m_mappingTable[rootPID].load();
//	BWTree<int,int>::DecodedNodeID rootDec= bw.decodeNodeID(rootID);
//	LOG(static_cast<int>(rootDec.type));

//	BWTree<int,int>::InnerNode *ptr = (BWTree<int,int>::InnerNode*) &bw.m_buffer[rootDec.startOffset];
//
//	for(int i = 20; i > 0; i--){
//	ptr->m_sepList.push_back(BWTree<int,int>::SeparetorItem(i,i));
//	}
//
//	for(auto t : ptr->m_sepList){
//	LOG(t.key << "   " << t.node_pid );
//	}
//
//	PID result;
//	if(ptr->findNextNode(18,result)){
//	LOG("Result" << result);
//	}
//
//
//	PID result2;
//	int test = std::numeric_limits<int>::max()-1;
//	std::cout << test << std::endl;
//	if(ptr->findNextNode(test,result2)){
//	LOG("Result" << result2);
//	}
//
//	//(KEY &sepKey , PID node, NodeID childNode, KEY &highSepKey int p_depth) :
//
//	// Innerinsert
//	NodeID insertNode = bw.createNodeID(BWTree<int,int>::NodeType::InnerInsertType,rootDec.endOffset);
//	bw.InstallNodeToReplace(rootPID,insertNode,rootID);
//	int key = 25;
//	int highkey =30;
//	new (bw.m_buffer + rootDec.endOffset) BWTree<int,int>::InnerInsertNode(key,rootPID,rootID, highkey, 0);
//
//
//	PID node = bw.navigateInnerNode(bw.m_mappingTable[rootPID].load(), 50);
//	LOG("Return result of navigate inner Node: " << node );
//
//	BWTree<int,int>::DecodedNodeID lfNode = bw.decodeNodeID(node);
//	LOG(static_cast<int>(rootDec.type));
//	BWTree<int,int>::LeafNode *lfn = (BWTree<int,int>::LeafNode*) &bw.m_buffer[lfNode.startOffset];
//	lfn->data_list.push_back(BWTree<int,int>::DataItem(51,std::vector<int> {12,13,14}));
//
//	bw.traverse(51);
//	std::vector<int> resultDI;
//	if(bw.findKey(51,resultDI)){
//		LOG("Result received");
//		for(int i = 0; i < resultDI.size();i++){
//			std::cout << resultDI[i] << std::endl;
//		}
//	}
//
//

	// check root
//	PID root = bw.m_root.load();
//
//	bw.printMappingTable();
//	//Insert new LeafInsertNode;
//	NodeID rootNodeID = bw.m_mappingTable[root].load();
//	BWTree<int,int>::DecodedNodeID decodedRoot = BWTree<int,int>::decodeNodeID(rootNodeID);
//
//	//FAKE root node entries
//	BWTree<int,int>::LeafNode* rootPtr = (BWTree<int,int>::LeafNode*) &bw.m_buffer[decodedRoot.startOffset];
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (6,std::vector<int> {12,13,14}));
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (5,std::vector<int> {12,13,14}));
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (3,std::vector<int> {12,13,14}));
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (4,std::vector<int> {12,13,14}));
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (1,std::vector<int> {12,13,14}));
//	rootPtr->data_list.push_back(BWTree<int,int>::DataItem (2,std::vector<int> {12,13,14}));
//
//
//	// try to install newInnerNode
//	size_t newBlockInner = bw.newFreeBlock();
//	NodeID innerNode = bw.createNodeID(BWTree<int,int>::NodeType::InnerType,newBlockInner);
//	PID innerPID = bw.nextFreePID();
//	bw.installNewNode(innerPID,innerNode);
//   // std::memcpy(&bw.m_buffer[newBlock], splitNode, sizeof(BWTree<int,int>::LeafNode));
//    new(bw.m_buffer + newBlockInner) BWTree<int,int>::InnerNode (1);
//
//    BWTree<int,int>::InnerNode* innerPTr =(BWTree<int,int>::InnerNode*) &bw.m_buffer[newBlockInner];
//
//
//    innerPTr->m_sepList.push_back(BWTree<int,int>::SeparetorItem (2,2));
//    std::cout << " INNER SEP SIZE " << innerPTr->m_sepList.size() << std::endl;
//
//
//	BWTree<int,int>::LeafNode* splitNode = rootPtr->getSplitSibling();
//
//
//	for(auto &item : splitNode->data_list){
//			std::cout << "Item key " << item.key << std::endl;
//			for(auto i : item.values){
//				std::cout << "Item Value " << i<< std::endl;
//			}
//		}
//
//
//
//	// try to install spitNode
//	size_t newBlock = bw.newFreeBlock();
//	NodeID splitNodeID = bw.createNodeID(BWTree<int,int>::NodeType::LeafType,newBlock);
//	PID splitPID = bw.nextFreePID();
//	bw.installNewNode(splitPID,splitNodeID);
//   // std::memcpy(&bw.m_buffer[newBlock], splitNode, sizeof(BWTree<int,int>::LeafNode));
//    new(bw.m_buffer + newBlock) BWTree<int,int>::LeafNode (splitNode);
//	delete(splitNode);
//    BWTree<int,int>::LeafNode* splitNodeCopied =(BWTree<int,int>::LeafNode*) &bw.m_buffer[newBlock];
//
//	for(auto &item : splitNodeCopied->data_list){
//			std::cout << "Item key " << item.key << std::endl;
//			for(auto i : item.values){
//				std::cout << "Item Value " << i<< std::endl;
//			}
//		}
//
//	bw.printMappingTable();
//
//	//Insertion
//	NodeID insertNode = bw.createNodeID(BWTree<int,int>::NodeType::LeafInsertType,decodedRoot.endOffset);
//	bw.InstallNodeToReplace(root,insertNode,rootNodeID);
//	bw.printMappingTable();
//	bw.installRoot(root,root);
//	assert(bw.m_mappingTable[root] == insertNode);
//	int iKey = 1;
//	int iValue = 10;
//	std::cout << decodedRoot.endOffset << std::endl;
//	new(bw.m_buffer + decodedRoot.endOffset) BWTree<int,int>::LeafInsertNode(iKey, iValue, rootNodeID,0);
//
//	bw.printMappingTable();
//	// second insert
//	BWTree<int,int>::DecodedNodeID decodedInsert = bw.decodeNodeID(insertNode);
//	NodeID insertNode2 = bw.createNodeID(BWTree<int,int>::NodeType::LeafInsertType,decodedInsert.endOffset);
//	bw.InstallNodeToReplace(root,insertNode2,insertNode);
//	assert(bw.m_mappingTable[root] == insertNode2);
//	int iKey2 = 100;
//	int iValue2 = 300;
//	std::cout << decodedInsert.endOffset << std::endl;
//	new(bw.m_buffer + decodedInsert.endOffset) BWTree<int,int>::LeafInsertNode(iKey2, iValue2, insertNode,0);
//
//
//	bw.printMappingTable();
//	//Logical Leafe Node
//
//	root = bw.m_root.load();
//
//	//Insert new LeafInsertNode;
//	NodeID insertNodeID = bw.m_mappingTable[root].load();
//	BWTree<int, int>::LogicalLeafeNode lf(insertNodeID,root,bw.m_buffer);
//
//	assert(lf.m_NodeIDChain[0] == insertNode2);
//	assert(lf.m_NodeIDChain[1] == insertNode);
//
//	std::cout << "Print logical Node item" << std::endl;
//	for(auto &item : lf.m_dataList){
//		std::cout << "Item key " << item.key << std::endl;
//		for(auto i : item.values){
//			std::cout << "Item Value " << i<< std::endl;
//		}
//	}
//
//
//	bw.printMappingTable();
//
//	NodeID consolidatedID;
//	bw.consolidate(insertNodeID, root, consolidatedID);
//	BWTree<int,int>::DecodedNodeID decodedCons = bw.decodeNodeID(consolidatedID);
//	LOG(VNAME(consolidatedID));
//	std::cout << "Cons ID offset" << decodedCons.startOffset << std::endl;
//	BWTree<int,int>::LeafNode* newRoot =(BWTree<int,int>::LeafNode*) &bw.m_buffer[decodedCons.startOffset];
//
//	assert(newRoot->data_list.size() != 0);
//	for(auto &item : newRoot->data_list){
//		std::cout << "Item key " << item.key << std::endl;
//		for(auto i : item.values){
//			std::cout << "Item Value " << i<< std::endl;
//		}
//	}
//
//	bw.printMappingTable();
//
//	bw.traverse(100,consolidatedID,root);
//
//	std::vector<BWTree<int,int>::DataItem> dataItems;
//	dataItems.reserve(20);
//
//
//	std::vector<BWTree<int,int>::SeparetorItem> sepItems;
//
//	for(int i = 20; i > 0; i--){
//		dataItems.emplace_back((BWTree<int,int>::DataItem(i,std::vector<int> {i})));
//		sepItems.emplace_back(BWTree<int,int>::SeparetorItem(i,i));
//	}
//
//	for(int i = 0; i <20 ; i++){
//			std::cout << dataItems[i].key << " ";
//	}
//	std::cout << std::endl;
//	std::sort(dataItems.begin(),dataItems.end(), &bw.dataItemComparator);
//	std::sort(sepItems.begin(),sepItems.end(), &bw.separatorItemComparator);
//
//	for(int i = 0; i <20 ; i++){
//				std::cout << dataItems[i].key << " ";
//				std::cout << sepItems[i].key << " ";
//		}
//	std::cout << std::endl;
//
//	free(buffer);

//	BWTree<int,int>::LeafNode lf;
//	bw.test();
//	BWTree<int,int>::BaseNode *ptr = &lf;
//	BWTree<int,int>::NodeType a= ptr->getType();
//
//	if(a == BWTree<int,int>::NodeType::LeafType){
//		std::cout << "Leaf" << std::endl;
//	}

//	std::cout << bw.nextFreePID() << std::endl;
//	std::cout << bw.nextFreePID() << std::endl;
//	std::cout << bw.nextFreePID() << std::endl;
//	std::cout << bw.nextFreePID() << std::endl;
//	std::cout << bw.nextFreePID() << std::endl;
//	std::cout << bw.nextFreePID() << std::endl;


}



