/*
 * Page.h
 *
 *  Created on: Jun 13, 2016
 *      Author: tobias
 */

#ifndef LEAFPAGE_H_
#define LEAFPAGE_H_

#include "AbstractPage.h"

template<typename KEY, typename VALUE>
class LeafPage : public AbstractPage<KEY,VALUE> {
public:
	PageType getType(){return m_type;};
private:
	PageType m_type = LeafNode;
};



#endif /* LEAFPAGE_H_ */
