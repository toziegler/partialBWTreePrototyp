/*
 * Page.h
 *
 *  Created on: Jun 13, 2016
 *      Author: tobias
 */

#ifndef ABSTRACTPAGE_H_
#define ABSTRACTPAGE_H_

#include "PageTypes.h"

template <typename KEY, typename VALUE>
class AbstractPage {
public:
	 virtual PageType getType() = 0;

private:
};



#endif /* ABSTRACTPAGE_H_ */
