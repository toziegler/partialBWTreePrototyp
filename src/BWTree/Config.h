/*
 * Config.h
 *
 *  Created on: Jun 13, 2016
 *      Author: tobias
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstddef>

using namespace std;

static const size_t BWTreeSize = 1000;
static const size_t BWBlockSize = 250;
static const size_t BWMappingTableSize = 1000;


//Constants for PID creation first X bits Type, then Size and remaining offset in memory block
static const size_t BWPIDTypeBits = 3;
//BWBlockSize = 100 => 7 bits;
static const size_t BWPIDSizeBits = 7;



#endif /* CONFIG_H_ */
