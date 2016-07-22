/*
 * BWTree.h
 *
 *  Created on: Jun 15, 2016
 *      Author: tobias
 */

#ifndef BWTREE_H_
#define BWTREE_H_

#include <cstddef>
#include <vector>
#include <iostream>
#include <atomic>
#include <cassert>
#include <algorithm>
#include <math.h>

//debug to make alle private methods public
#define ALL_PUBLIC
#define DEBUG
#define LOG(msg) std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl
#define VNAME(x) #x

// NodeID  is the physical pointer
// encoded type, offset within block and memoryblock
using NodeID = uint64_t;
//used for delta chain links
using PtrOffset = uint32_t;

//PID is the index in mapping table and also the virtual ID
using PID = size_t;

template<typename KEY, typename VALUE>
class BWTree {

public:
	class BaseNode;
	// class KeyType;
	// class WrappedKeyComparator;
	//class BaseLogicalNode;
	// class NodeSnapshot;
	// class MemoryManager;

	BWTree() {
	}
	;
	BWTree(char* p_buffer) :
			m_buffer(p_buffer) {
		initNodeLayout();
		LOG("BW Constructor");
	}
	;

	void initNodeLayout() {

		// need first leafe node
		PID firstPID;
		LeafNode* ptrLeaf = new LeafNode(0);
		placementLeafNode(ptrLeaf, firstPID);
		// install innnerNode
		InnerNode* ptr = new InnerNode(0);
		ptr->m_sepList.push_back(SeparetorItem(firstPID));
		PID secondPID = nextFreePID();
		placementInnerNode(ptr, secondPID);
		installRoot(secondPID, m_root.load());

	}
	/*
	 * private: Basic type definition
	 */
#ifndef ALL_PUBLIC
private:
#else
public:
#endif

	// The maximum number of nodes we could map in this index
	constexpr static NodeID MAPPING_TABLE_SIZE = 10;

	// Bits used for one MEMORY Block
	constexpr static size_t MEMORY_BLOCK_SIZE = 1000;
	constexpr static size_t PID_RESERVED_BIT_TYPE = 10;
	constexpr static size_t PID_RESERVED_SIZE_TYPE = 15;

	// If the length of delta chain exceeds this then we consolidate the node
	constexpr static int DELTA_CHAIN_LENGTH_THRESHOLD = 8;

	// If node size goes above this then we split it
	constexpr static size_t INNER_NODE_SIZE_UPPER_THRESHOLD = 16;
	constexpr static size_t LEAF_NODE_SIZE_UPPER_THRESHOLD = 16;

	constexpr static size_t INNER_NODE_SIZE_LOWER_THRESHOLD = 7;
	constexpr static size_t LEAF_NODE_SIZE_LOWER_THRESHOLD = 7;

	/*
	 * enum class NodeType - Bw-Tree node type
	 */
	enum class NodeType {
		// Data page type
		LeafType = 0,
		InnerType,

		// Only valid for leaf
		LeafInsertType,
		LeafSplitType,
		//   LeafDeleteType,
		//   LeafUpdateType,
		//   LeafRemoveType,
		//   LeafMergeType,
		//   LeafAbortType, // Unconditional abort (this type is not used)

		// Only valid for inner
		InnerInsertType,
		InnerSplitType,
	//   InnerDeleteType,
	//   InnerRemoveType,
	//   InnerMergeType,
	//   InnerAbortType, // Unconditional abort
	};

	/*
	 * enum class OpState - Current state of the state machine
	 *
	 * Init - We need to load root ID and start the traverse
	 *        After loading root ID should switch to Inner state
	 *        since we know there must be an inner node
	 * Inner - We are currently on an inner node, and want to go one
	 *         level down
	 * Abort - We just saw abort flag, and will reset everything back to Init
	 */
	enum class OpState {
		Init, Inner, Leaf, Abort
	};

	struct DataItem {
		KEY key;
		std::vector<VALUE> values;

		DataItem() {
			//std::cout << "def" << std::endl;
		}
		;
		DataItem(const KEY p_key, const std::vector<VALUE> &p_value_list) :
				key { p_key }, values { p_value_list } {

			//std::cout << "Load cons\n";
		}

		DataItem(const DataItem &di) :
				key { di.key }, values { di.values } {

			//std::cout << "Copy cons\n";
		}

		DataItem(DataItem &&di) :
				key { di.key }, values { std::move(di.values) } {
			//std::cout << "Move consd\n";
		}

		/*
		 * Move Assignment - DO NOT ALLOW THIS
		 */
		DataItem &operator=(DataItem &&di) {
			key = di.key;
			values = std::move(di.values);
			//std::cout << "move assigned\n";
			return *this;
		}

		//		    friend void swap(DataItem& rhs, DataItem& lhs){
		//		    	DataItem temp(rhs);
		//		    	rhs = std::move(lhs);
		//		    	lhs = std::move(temp);
		//		    }
	};

	static bool dataItemComparator(DataItem const& lhs, DataItem const& rhs) {
		if (lhs.key != rhs.key)
			return lhs.key < rhs.key;
		return false;
	}

	struct SeparetorItem {
		KEY key;
		PID node_pid;

		SeparetorItem(const KEY &p_key, PID p_node) :
				key { p_key }, node_pid { p_node } {
		}

		SeparetorItem(const SeparetorItem &si) :
				key { si.key }, node_pid { si.node_pid } {
		}

		//MAX VALUE
		SeparetorItem(PID p_node) :
				key { std::numeric_limits<KEY>::max() }, node_pid { p_node } {
		}
	};

	static bool separatorItemComparator(SeparetorItem const& lhs,
			SeparetorItem const& rhs) {
		if (lhs.key != rhs.key)
			return lhs.key < rhs.key;
		return false;
	}

	struct DecodedNodeID {
		NodeType type;
		PtrOffset startOffset; // total offset
		PtrOffset endOffset; // total offset
		size_t memoryBlockNumber; // Block in total memory region see MEMORY_BLOCK_SIZE
	};

	class BaseNode {
	public:
		const NodeType m_type;
		const int m_depth;

		BaseNode(NodeType p_type, int p_depth) :
				m_type(p_type), m_depth(p_depth) {
		}
		;
		inline NodeType getType() const {
			return m_type;
		}
		;

		inline int getDepth() {
			return m_depth;
		}
		;

	private:
	};

	class LeafNode: public BaseNode {
	public:
		std::vector<DataItem> data_list;
		LeafNode(int p_depth) :
				BaseNode { NodeType::LeafType, p_depth }, m_rightNode(0), m_hasRightSibling(
						false) {
		}

		LeafNode(int p_depth, std::vector<DataItem> data, PID rightNode,
				bool hasRight) :
				BaseNode { NodeType::LeafType, p_depth }, m_rightNode(
						rightNode), m_hasRightSibling(hasRight) {
			for (auto i : data) {
				data_list.push_back(i);
			}
		}
		LeafNode(LeafNode* ptr) :
				BaseNode { NodeType::LeafType, ptr->m_depth } {
			LOG("Copy Leaf");
			for (auto i : ptr->data_list) {
				data_list.push_back(i);
			}

			m_rightNode = ptr->m_rightNode;
			m_hasRightSibling = ptr->m_hasRightSibling;
		}

		LeafNode *getSplitSibling() {
			size_t nodeSize = data_list.size();
			size_t keyIndex = nodeSize / 2;

			assert(nodeSize >= 2);
			LeafNode *splitNode = new LeafNode(this->m_depth);

			if (m_hasRightSibling) {
				splitNode->m_hasRightSibling = true;
				splitNode->m_rightNode = m_rightNode;
			} else {
				splitNode->m_hasRightSibling = false;
				splitNode->m_rightNode = m_rightNode;
			}

			// Copy data item into the new node
			for (int i = keyIndex; i < (int) nodeSize; i++) {
				splitNode->data_list.push_back(data_list[i]);
			}
			return splitNode;
		}


		KEY getSepKeySplitt(){
			return data_list.front().key;
		}

		void setRightNode(PID rightNode, bool hasRight){
			m_rightNode = rightNode;
			m_hasRightSibling = hasRight;
		}

	private:
		PID m_rightNode;
		bool m_hasRightSibling;
	};

	class LeafInsertNode: public BaseNode {
	public:
		const KEY m_key;
		const VALUE m_value;
		NodeID m_childNode;

		LeafInsertNode(KEY &insertKey, VALUE &insertValue, NodeID childNode,
				int p_depth) :
				BaseNode(NodeType::LeafInsertType, p_depth), m_key(insertKey), m_value(
						insertValue), m_childNode(childNode) {
			LOG("Constructor of LeafeInsert child " << m_childNode);
		}

	};

	class LeafSplitNode: public BaseNode {
	public:
		const KEY m_key;
		NodeID m_childNode;
		PID m_splitSibling;

		LeafSplitNode(PID splitSiblingPID,KEY &splitKey, NodeID childNode, int p_depth) :
				BaseNode(NodeType::LeafSplitType, p_depth),m_splitSibling(splitSiblingPID), m_key(splitKey), m_childNode(
						childNode) {
			LOG("Constructor of LeafSplit child" << m_childNode);
		}

		KEY getSplitKey(){
			return m_key;
		}
		PID getSplitSibling(){
			return m_splitSibling;
		}

	};

	class InnerNode: public BaseNode {
	public:
		std::vector<SeparetorItem> m_sepList;
		InnerNode(int p_depth) :
				BaseNode { NodeType::InnerType, p_depth }, m_rightNode(0), m_hasRightSibling(
						false) {
		}

		InnerNode(int p_depth, std::vector<SeparetorItem> separetors,
				PID rightNode, bool hasRight) :
				BaseNode { NodeType::InnerType, p_depth }, m_rightNode(
						rightNode), m_hasRightSibling(hasRight) {
			for (auto i : separetors) {
				m_sepList.push_back(i);
			}
		}
		InnerNode(InnerNode* ptr) :
				BaseNode { NodeType::InnerType, ptr->m_depth } {
			LOG("Copy InnerNode");
			for (auto i : ptr->m_sepList) {
				m_sepList.push_back(i);
			}

			m_rightNode = ptr->m_rightNode;
			m_hasRightSibling = ptr->m_hasRightSibling;
		}

		bool findNextNode(KEY searchKey, PID &result) {
			std::sort(m_sepList.begin(), m_sepList.end(),
					separatorItemComparator);
			for (int i = 0; i < m_sepList.size(); i++) {
				if (searchKey < m_sepList[i].key) {
					LOG("found" << m_sepList[i].key);
					result = m_sepList[i].node_pid;
					return true;
				}
			}
			result = m_sepList[m_sepList.size()].node_pid;
			return true;
		}

		InnerNode *getSplitSibling() {
			size_t nodeSize = m_sepList.size();
			size_t keyIndex = nodeSize / 2;

			assert(nodeSize >= 2);
			InnerNode *splitNode = new LeafNode(this->m_depth);

			if (m_hasRightSibling) {
				splitNode->m_hasRightSibling = true;
				splitNode->m_rightNode = m_rightNode;
			} else {
				splitNode->m_hasRightSibling = false;
				splitNode->m_rightNode = m_rightNode;
			}

			// Copy data item into the new node
			for (int i = keyIndex; i < (int) nodeSize; i++) {
				splitNode->m_sepList.push_back(m_sepList[i]);
			}
			return splitNode;
		}
		;

	private:
		PID m_rightNode;
		bool m_hasRightSibling;
	};

	// Node for parent Update when split
	class InnerInsertNode: public BaseNode {
	public:
		const SeparetorItem m_separator;
		NodeID m_childNode;
		const KEY m_high_sepKey;

		InnerInsertNode(KEY &sepKey, PID node, NodeID childNode,
				KEY &highSepKey, int p_depth) :
				BaseNode(NodeType::InnerInsertType, p_depth), m_separator(
						sepKey, node), m_childNode(childNode), m_high_sepKey(
						highSepKey) {
			LOG("Constructor of InnerInsert child " << m_childNode);
		}

	};

	class InnerSplitNode: public BaseNode {
		const KEY m_splitKey;
		PID m_splittedNode;
		NodeID m_childNode;

		InnerSplitNode(KEY &splitKey, PID splittedNode, NodeID childNode,
				int p_depth) :
				BaseNode(NodeType::LeafSplitType, p_depth), m_splitKey(
						splitKey), m_splittedNode(splittedNode), m_childNode(
						childNode) {
			LOG("Constructor of InnerSplit child" << m_childNode);
		}

	};

	class LogicalInnerNode {
	public:
		PID m_rightNode;
		bool m_hasRightSibling;
		std::vector<SeparetorItem> m_sepList;
		NodeID m_startNode;
		PID m_startPID;
		char* m_BWbuffer;

		LogicalInnerNode(NodeID p_startNode, PID p_startPID, char* buffer) :
				m_startNode(p_startNode), m_startPID(p_startPID), m_BWbuffer(
						buffer) {
			mergeDeltaChains();
			std::sort(m_sepList.begin(), m_sepList.end(),
					&separatorItemComparator);
		}

		void printNode() {
			for (auto &item : m_sepList) {
				std::cout << "Item key " << item.key << " Node PID "
						<< item.node_pid << std::endl;
			}
		}

		InnerNode* LogicalToInner(LogicalInnerNode &logicalNode) {
			InnerNode* innerNode = new InnerNode(0, logicalNode.m_sepList,
					logicalNode.m_rightNode, logicalNode.m_hasRightSibling);
			LOG(
					"Logical To Inner type" << static_cast<int>(innerNode->getType()));
			return innerNode;
		}

	private:

		bool mergeDeltaChains() {
			DecodedNodeID decodedNID = decodeNodeID(m_startNode);
			BaseNode* basePtr = (BaseNode*) &m_BWbuffer[decodedNID.startOffset];
			int depth = basePtr->getDepth();
			LOG("MERGE DELTA " << depth);
			for (int i = 0; i <= depth; i++) {
				if (decodedNID.type == NodeType::InnerInsertType) {
					InnerInsertNode *ptrInnerInsertNode =
							(InnerInsertNode*) &m_BWbuffer[decodedNID.startOffset];
					m_sepList.push_back(ptrInnerInsertNode->m_separator);
					decodedNID = decodeNodeID(ptrInnerInsertNode->m_childNode);
				} else if (decodedNID.type == NodeType::InnerType) {
					InnerNode *ptrInnerNode =
							(InnerNode*) &m_BWbuffer[decodedNID.startOffset];
					LOG(
							"MERGE DELTA INNER NODE size of Sep LIST " << ptrInnerNode->m_sepList.size());
					m_sepList.insert(m_sepList.end(),
							ptrInnerNode->m_sepList.begin(),
							ptrInnerNode->m_sepList.end());
					return true;
				}
			}
			return false;
		}
	};

	class LogicalLeafeNode {
	public:
		PID m_rightNode;
		bool m_hasRightSibling;
		std::vector<DataItem> m_dataList;
		NodeID m_startNode;
		PID m_startPID;
		int m_depth;
		// Stores all NodeIDs in the chain to replay all operations
		std::vector<NodeID> m_NodeIDChain;
		char* m_BWbuffer;

		LogicalLeafeNode(NodeID p_startNode, PID p_startPID, char* buffer) :
				m_startNode(p_startNode), m_startPID(p_startPID), m_BWbuffer(
						buffer) {
			LOG("LogicalLeafe");
			fillNodeChain();
			replayLog();
			std::sort(m_dataList.begin(), m_dataList.end(),
					&dataItemComparator);
		}

		LeafNode* LogicalToLeaf(LogicalLeafeNode &logicalNode) {
			LeafNode* leaf = new LeafNode(0, logicalNode.m_dataList,
					logicalNode.m_rightNode, logicalNode.m_hasRightSibling);
			LOG("Logical To Leafe type" << static_cast<int>(leaf->getType()));
			return leaf;
		}

		bool findValues(KEY searchKey, std::vector<VALUE> &resultValues) {
			for (int i = 0; i < m_dataList.size(); i++) {
				if (searchKey == m_dataList[i].key) {
					resultValues = m_dataList[i].values;
					LOG("LFN found");
					return true;
				}
			}
			return false;
		}

		void printNode() {
			for (auto &item : m_dataList) {
				std::cout << "Item key " << item.key << " Item Value ";
				for (auto i : item.values) {
					std::cout << i << "  ";
				}
				std::cout << std::endl;
			}
		}

		int getDepth(){
			return m_depth;
		}

		LeafNode* getSplitSibling() {
					size_t nodeSize = m_dataList.size();
					LOG("SPLIT" << nodeSize);
					size_t keyIndex = nodeSize / 2;

					assert(nodeSize >= 2);
					LeafNode *splitNode = new LeafNode(0);

					if (m_hasRightSibling) {
						splitNode->setRightNode(m_rightNode, true);
					} else {
						splitNode->setRightNode(0,false);
					}

					// Copy data item into the new node
					for (int i = keyIndex; i < (int) nodeSize; i++) {
						splitNode->data_list.push_back(m_dataList[i]);
					}
					return splitNode;
				}
				;

	private:

		void replayLog() {
			LOG("replayLog Total Runs:" << m_NodeIDChain.size());
			for (unsigned i = m_NodeIDChain.size(); i-- > 0;) {
				LOG("replayLog run:" << i);
				LOG("replayLog Node ID " << m_NodeIDChain[i]);
				DecodedNodeID decID = decodeNodeID(m_NodeIDChain[i]);
				if (decID.type == NodeType::LeafInsertType) {
					LOG("replayLog insertType found" << i);
					bool inserted = false;
					LeafInsertNode* ptrLI =
							(LeafInsertNode*) &m_BWbuffer[decID.startOffset];
					LOG(
							"InsertNode with key " << ptrLI->m_key << " and Value " << ptrLI->m_value);
					//search if KEY already exists
					for (auto& item : m_dataList) {
						if (item.key == ptrLI->m_key) {
							LOG(
									"InsertNode with key " << ptrLI->m_key << " FOUND");
							item.values.push_back(ptrLI->m_value);
							inserted = true;
							break; // exits loop
						}
					}
					if (inserted == false) {
						LOG("InsertNode with key " << ptrLI->m_key << " NEW");
						m_dataList.push_back(
								DataItem(ptrLI->m_key, std::vector<VALUE> {
										ptrLI->m_value }));
					}
				}
				if (decID.type == NodeType::LeafType) {
					LOG("replayLog leafeNode found" << i);
					LeafNode* ptr = (LeafNode*) &m_BWbuffer[decID.startOffset];
					if (ptr->data_list.size() != 0) {
						for (auto& item : ptr->data_list) {
							m_dataList.push_back(DataItem(item));
						}
					}
				}
			}
		}

		void fillNodeChain() {
			LOG("LogicalLeafe fillNodeChain");
			m_NodeIDChain.push_back(m_startNode);
			LOG("Node startNode " << m_startNode);
			DecodedNodeID decodedNID = decodeNodeID(m_startNode);
			BaseNode* ptr = (BaseNode*) &m_BWbuffer[decodedNID.startOffset];
			m_depth = ptr->getDepth();
			LOG("fillNode Chain NodeTyp" << static_cast<int>(decodedNID.type));
			while (decodedNID.type != NodeType::LeafType) {
				switch (decodedNID.type) {
				case NodeType::LeafInsertType:
					LeafInsertNode* ptr =
							(LeafInsertNode*) &m_BWbuffer[decodedNID.startOffset];
					LOG("Node chain ChildNode " << ptr->m_childNode);
					decodedNID = decodeNodeID(ptr->m_childNode);
					m_NodeIDChain.push_back(ptr->m_childNode);
					break;
				}

			}

		}

	};

	bool firstChildSplit(PID splittingNode){
		NodeID splitNID = getNodeID(splittingNode);
		LogicalLeafeNode logicalLeaf(splittingNode,splitNID,m_buffer);
		LeafNode* ptrNewLeaf = logicalLeaf.getSplitSibling();
		PID newNode;
		placementLeafNode(ptrNewLeaf,newNode);
		KEY key = ptrNewLeaf->getSepKeySplitt();

		int depth = (logicalLeaf.getDepth()+1);
		std::cout << "DEPTH "<<depth << std::endl;
		if(placementSplitLeafDelta(splittingNode,newNode,key,splitNID,depth)){
		return true;
		}
		return false;
	}

	bool consolidate(NodeID consolidateNode, PID consolidatePID,
			NodeID& newNodeID) {
		LOG("Consolidate");
		DecodedNodeID decID = decodeNodeID(consolidateNode);

		if (decID.type == NodeType::LeafInsertType) {
			LogicalLeafeNode lf(consolidateNode, consolidatePID, m_buffer);
			LeafNode* ptrToLeafe = lf.LogicalToLeaf(lf);
			if(placementLeafNodeConsolidate(consolidatePID,consolidateNode,ptrToLeafe)){
				LOG("Leaf Consolidate finish");
				return true;
			}
			LOG("Consolidate failed");
			return false;
		} else if (decID.type == NodeType::InnerInsertType) {
			LogicalInnerNode lin(consolidateNode, consolidatePID, m_buffer);
			InnerNode* ptrToInner = lin.LogicalToInner(lin);
			if(placementInnerNodeConsolidate(consolidatePID,consolidateNode,ptrToInner)){
				LOG("Inner Consolidate finish");
				return true;
			}
			LOG("Consolidate failed");
			return false;
		}
		return false;
	}

	PID navigateInnerNode(NodeID startingNode, KEY searchKey) {
		DecodedNodeID decID = decodeNodeID(startingNode);
		for (;;) {
			if (decID.type == NodeType::InnerInsertType) {
				LOG("navigate InnerInsertNode reached");
				InnerInsertNode* ptr =
						(InnerInsertNode*) &m_buffer[decID.startOffset];

				if (ptr->m_separator.key <= searchKey
						&& searchKey < ptr->m_high_sepKey) {
					LOG("navigate key sep found");
					return ptr->m_separator.node_pid;
				}
				decID = decodeNodeID(ptr->m_childNode);

			} else if (decID.type == NodeType::InnerType) {
				LOG("navigate InnerNode reached");
				InnerNode* ptr = (InnerNode*) &m_buffer[decID.startOffset];
				PID result;
				if (ptr->findNextNode(searchKey, result))
					return result;
			}
		}
		LOG("NOT POSSIBLE");

	}

	PID checkLeafSplit(PID leafPID, KEY searchKey) {
		LOG("CHECK LEAF SPLIT");
		NodeID startingNode = getNodeID(leafPID);
		DecodedNodeID decID = decodeNodeID(startingNode);
		while (decID.type != NodeType::LeafType) {
			if (decID.type == NodeType::LeafInsertType) {
				LOG("navigate LeafeInsert");
				LeafInsertNode* ptr =
						(LeafInsertNode*) &m_buffer[decID.startOffset];
				decID = decodeNodeID(ptr->m_childNode);

			}
			if (decID.type == NodeType::LeafSplitType) {
				LOG("Split found");
				LeafSplitNode* ptr = (LeafSplitNode*) &m_buffer[decID.startOffset];
				if(searchKey < ptr->getSplitKey()){
					return leafPID;
				}
				return ptr->getSplitSibling();
			}
		}

		return leafPID;


	}


	PID traverse(KEY searchKey) {
		LOG("Traverse from root");
		// Starting node
		NodeID startingNode = m_mappingTable[m_root.load()].load();
		DecodedNodeID decID = decodeNodeID(startingNode);
		bool traversing = true;
		// if it is a fresh node
		PID resultPID;
		resultPID = navigateInnerNode(startingNode, searchKey);
		LOG("NavigateInner finished");
		resultPID = checkLeafSplit(resultPID,searchKey);
		NodeID resultNodeID = m_mappingTable[resultPID].load();
		decID = decodeNodeID(resultNodeID);
		if (decID.type == NodeType::LeafType
				|| decID.type == NodeType::LeafSplitType
				|| decID.type == NodeType::LeafInsertType) {
//			LogicalLeafeNode lf(resultNodeID, resultPID, m_buffer);
//			std::vector<VALUE> result;
//			if(lf.findValues(searchKey, result)){
//				LOG("Found Node");
//
//			}

			return resultPID;
		}

		//
		//
		//		while(1){
		//		if (decID.type  == NodeType::LeafInsertType || decID.type == NodeType::LeafType) {
		//			LogicalLeafeNode lf(node,pid,m_buffer);
		//			auto lower = std::lower_bound(lf.m_dataList.begin(), lf.m_dataList.end(), DataItem (searchKey, std::vector<VALUE> {}), &dataItemComparator);
		//			if(lower != lf.m_dataList.end())
		//			LOG("Found key in item" << lower->key);
		//		}
		//		if(decID.type  == NodeType::InnerInsertType){
		//			LOG("InnerInsertType");
		//			PID correctNode = navigateInnerNode(node,searchKey);
		//		}
		//		}

	}

	bool findKey(KEY searchKey, std::vector<VALUE> &result) {
		PID leafNode = traverse(searchKey);
		NodeID leafID = getNodeID(leafNode);
		LOG("Node ID " << leafNode);
		DecodedNodeID decNode = decodeNodeID(leafID);
		if (decNode.type == NodeType::LeafType
				|| decNode.type == NodeType::LeafSplitType
				|| decNode.type == NodeType::LeafInsertType) {
			LogicalLeafeNode lf(leafID, leafNode, m_buffer);
			if (lf.findValues(searchKey, result)) {
				LOG("Found Node");

			}
			std::cout << result.size() << std::endl;
			return true;

		}
		return false;

	}

	bool placementInsertInnerDelta(PID pidNode, KEY &sepKey, PID node,
			NodeID childNode, KEY &highSepKey, int p_depth) {
		DecodedNodeID childID = decodeNodeID(childNode);
		NodeID newNode = createNodeID(NodeType::InnerInsertType,
				childID.endOffset);
		new (m_buffer + childID.endOffset) InnerInsertNode(sepKey, node,
				childNode, highSepKey, p_depth);
		if (InstallNodeToReplace(pidNode, newNode, childNode)) {
			return true;
		}
		return false;
	}

	bool placementInsertLeafDelta(PID pidNode, KEY &insertKey,
			VALUE &insertValue, NodeID childNode, int p_depth, NodeID& newNode) {
		DecodedNodeID childID = decodeNodeID(childNode);
		newNode = createNodeID(NodeType::LeafInsertType,
				childID.endOffset);
		new (m_buffer + childID.endOffset) LeafInsertNode(insertKey,
				insertValue, childNode, p_depth);
		if (InstallNodeToReplace(pidNode, newNode, childNode)) {
			return true;
		}
		return false;
	}

	bool placementSplitLeafDelta(PID pidNode, PID splitSibling,KEY &splitKey, NodeID childNode,
			int p_depth) {
		DecodedNodeID childID = decodeNodeID(childNode);
		NodeID newNode = createNodeID(NodeType::LeafSplitType,
				childID.endOffset);
		new (m_buffer + childID.endOffset) LeafSplitNode(splitSibling,splitKey, childNode,p_depth);
		if (InstallNodeToReplace(pidNode, newNode, childNode)) {
			return true;
		}
		return false;
	}

	bool placementSplitInnersDelta(PID pidNode, KEY &splitKey, PID splittedNode,
			NodeID childNode, int p_depth) {
		DecodedNodeID childID = decodeNodeID(childNode);
		NodeID newNode = createNodeID(NodeType::InnerSplitType,
				childID.endOffset);
		new (m_buffer + childID.endOffset) InnerSplitNode(splitKey,
				splittedNode, childNode, p_depth);
		if (InstallNodeToReplace(pidNode, newNode, childNode)) {
			return true;
		}
		return false;
	}

	bool placementInnerNodeConsolidate(PID pidNode, NodeID oldNode,
			InnerNode* ptr) {
		size_t newBlock = newFreeBlock();
		NodeID newNode = createNodeID(NodeType::InnerType, newBlock);
		new (m_buffer + newBlock) InnerNode(ptr);
		if (InstallNodeToReplace(pidNode, newNode, oldNode)) {
			return true;
		}
		return false;
	}

	bool placementInnerNode(InnerNode* ptr, PID& returnPID) {
		size_t newBlock = newFreeBlock();
		returnPID = nextFreePID();
		NodeID newNode = createNodeID(NodeType::InnerType, newBlock);
		new (m_buffer + newBlock) InnerNode(ptr);
		installNewNode(returnPID, newNode);
		return true;
	}

	bool placementLeafNodeConsolidate(PID pidNode, NodeID oldNode,
			LeafNode* ptr) {
		size_t newBlock = newFreeBlock();
		NodeID newNode = createNodeID(NodeType::LeafType, newBlock);
		new (m_buffer + newBlock) LeafNode(ptr);
		if (InstallNodeToReplace(pidNode, newNode, oldNode)) {
			return true;
		}
		return false;
	}

	bool placementLeafNode(LeafNode* ptr, PID& returnPID) {
		size_t newBlock = newFreeBlock();
		returnPID = nextFreePID();
		NodeID newNode = createNodeID(NodeType::LeafType, newBlock);
		new (m_buffer + newBlock) LeafNode(ptr);
		installNewNode(returnPID, newNode);
		return true;
	}

	bool insert(KEY key, VALUE value) {
		PID leafNode = traverse(key);
		NodeID leafID = getNodeID(leafNode);
		DecodedNodeID decNode = decodeNodeID(leafID);
		if (decNode.type == NodeType::LeafType
				|| decNode.type == NodeType::LeafSplitType
				|| decNode.type == NodeType::LeafInsertType) {
			NodeID insertNode;
			BaseNode* ptr = (BaseNode*) &m_buffer[decNode.startOffset];
			int prevDepth = ptr->getDepth();
			int newDepth = prevDepth + 1;
			if (placementInsertLeafDelta(leafNode,key, value, leafID, newDepth,insertNode)) {
				if (newDepth > LEAF_NODE_SIZE_LOWER_THRESHOLD) {
					NodeID consolidatedID;
					consolidate(insertNode, leafNode, consolidatedID);
				}
				return true;
			}
		}
		LOG("Insert Failed");
		return false;
	}

	size_t nextFreePID() {
		if (m_nextFreePid.load() >= MAPPING_TABLE_SIZE) {
			std::cerr << "Exceeded Maximum number of PIDs" << std::endl;
		}
		LOG("NextFreePID called: " << m_nextFreePid.load());
		return m_nextFreePid.fetch_add(1, std::memory_order_relaxed);
	}

	size_t newFreeBlock() {
		// TODO Atomic update return old value
		LOG("NextFreeBlock called: " << m_nextFreeBlock.load());
		return m_nextFreeBlock.fetch_add(MEMORY_BLOCK_SIZE,
				std::memory_order_relaxed);
	}

	/*
	 * Create Node ID
	 * input startOffset total
	 */

	NodeID createNodeID(NodeType nodeType, PtrOffset startOffset) {
		size_t memoryBlockNumber = (int) startOffset / MEMORY_BLOCK_SIZE;
		startOffset = startOffset - (memoryBlockNumber * MEMORY_BLOCK_SIZE);
		LOG(
				"createNodeID called: startOffset " << startOffset << "Type " << static_cast<int>(nodeType) << " memBlock" << memoryBlockNumber);
		uint64_t mask = 0;

		mask = mask | static_cast<int>(nodeType);
		mask = mask << PID_RESERVED_SIZE_TYPE;
		mask = mask | startOffset;
		mask = mask
				<< (sizeof(uint64_t) * 8 - PID_RESERVED_BIT_TYPE
						- PID_RESERVED_SIZE_TYPE);
		mask = mask | memoryBlockNumber; // starting block of new element;
		std::bitset<64> bitset(mask);
		LOG("BITSET " << bitset);

#ifdef DEBUG
		LOG("DEBUG");
		DecodedNodeID dec = decodeNodeID(mask);
		assert(dec.type == nodeType);
		assert(
				dec.startOffset
						== startOffset
								+ (memoryBlockNumber * MEMORY_BLOCK_SIZE));
		assert(dec.memoryBlockNumber == memoryBlockNumber);
#endif

		return mask;
	}

	static DecodedNodeID decodeNodeID(NodeID pid) {
		DecodedNodeID ret;
		uint64_t mask = 0;
		mask = ~(mask & 0);
		ret.type = static_cast<NodeType>((pid & mask)
				>> (sizeof(uint64_t) * 8 - PID_RESERVED_BIT_TYPE));
		size_t startOffsetinBlock = (pid & (mask >> PID_RESERVED_BIT_TYPE))
				>> (sizeof(uint64_t) * 8 - PID_RESERVED_BIT_TYPE
						- PID_RESERVED_SIZE_TYPE);

		ret.memoryBlockNumber = (pid
				& (mask >> (PID_RESERVED_SIZE_TYPE + PID_RESERVED_BIT_TYPE)));

		ret.startOffset = startOffsetinBlock
				+ (ret.memoryBlockNumber * MEMORY_BLOCK_SIZE);

		ret.endOffset = ret.startOffset + getSizeOfType(ret.type);
		LOG(
				"Decoded ID type" << static_cast<int>(ret.type) << "startOffset " << ret.startOffset << "endOffset " << ret.endOffset << "MemBlok" << ret.memoryBlockNumber);
		return ret;
	}

	static size_t getSizeOfType(NodeType type_p) {
		size_t ret = 0;
		switch (type_p) {
		case NodeType::LeafType:
			ret = sizeof(LeafNode);
			break;
		case NodeType::LeafInsertType:
			ret = sizeof(LeafInsertNode);
			break;
		case NodeType::LeafSplitType:
			ret = sizeof(LeafSplitNode);
			break;
		case NodeType::InnerType:
			ret = sizeof(InnerNode);
			break;
		case NodeType::InnerInsertType:
			ret = sizeof(InnerInsertNode);
			break;
		case NodeType::InnerSplitType:
			ret = sizeof(InnerSplitNode);
			break;
		}
		return ret;

	}

	NodeID getNodeID(PID pid) {
		return m_mappingTable[pid].load();
	}
	void installNewNode(PID pid, NodeID &node_p) {
		NodeID initValue = 0;
		bool ret = InstallNodeToReplace(pid, node_p, initValue);
		(void) ret;
	}

	bool installRoot(PID newRoot_p, PID oldRoot_p) {
		LOG("installRoot");
		bool ret = m_root.compare_exchange_strong(oldRoot_p, newRoot_p);
		return ret;
	}

	bool InstallNodeToReplace(PID node_pid, NodeID &node_p, NodeID &prev_p) {
		LOG("installNodeToReplace");

		assert(node_pid < MAPPING_TABLE_SIZE);
		bool ret = m_mappingTable[node_pid].compare_exchange_strong(prev_p,
				node_p);

		return ret;
	}

	std::atomic<PID> m_root { 0 };
	int m_depth;

	std::atomic<NodeID> m_mappingTable[MAPPING_TABLE_SIZE] = { };
	//	//indexCounter
	std::atomic<size_t> m_nextFreePid { 0 };

	// Buffer Management
	char* m_buffer;

	//
	//ToDo Atomic Operation
	std::atomic<size_t> m_nextFreeBlock { 0 };

#ifndef ALL_PUBLIC
private:

#endif

#ifdef DEBUG
	void printMappingTable() {
		for (size_t i = 0; i < MAPPING_TABLE_SIZE; i++) {
			std::cout << "Index: " << i << "  PID:" << m_mappingTable[i]
					<< std::endl;
		}
	}
#endif
};

#endif /* BWTREE_H_ */
