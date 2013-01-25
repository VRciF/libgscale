/*
 * types.hpp
 *
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <string>

#include <boost/uuid/uuid.hpp>

namespace GScale{

class NodeFQIdentifier{
	public:
	    boost::uuids::uuid host;
		std::string group;
		boost::uuids::uuid node;
};

}

#endif /* TYPES_HPP_ */
