/*
 * LeafInsert.h
 *
 *  Created on: Jun 13, 2016
 *      Author: tobias
 */

#ifndef LEAFDELTARECORD_H_
#define LEAFDELTARECORD_H_

#include "AbstractPage.h"

template<typename KEY, typename VALUE>
class LeafDeltaRecord : AbstractPage<KEY,VALUE> {
public:
	//for inserts
	LeafDeltaRecord(PageType type, KEY key, VALUE value):m_type(type),m_key(key), m_value(value),m_PID(-1){};
	//for splits
	LeafDeltaRecord(PageType type, KEY key, int pid):m_type(type),m_key(key), m_value(nullptr),m_PID(pid){};
	PageType getType(){return m_type;};
private:
	PageType m_type;
	KEY m_key; // could be actual key or separator
	VALUE m_value;
	int m_PID; //logical side pointer to the new node while split
	uint64_t ptrNext = 0 ;

};





#endif /* LEAFDELTARECORD_H_ */
