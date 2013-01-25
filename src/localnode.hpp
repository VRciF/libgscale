/*
 * localnode.hpp
 *
 */

#ifndef LOCALNODE_HPP_
#define LOCALNODE_HPP_

#include <string>
#include <utility>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "inode.hpp"

namespace GScale{

class INodeCallback;

class LocalNode : public GScale::INode{
	public:
	    LocalNode(GScale::INodeCallback &cbs, boost::posix_time::ptime ctime = boost::posix_time::microsec_clock::universal_time());
	    LocalNode(std::string alias, GScale::INodeCallback &cbs, boost::posix_time::ptime ctime = boost::posix_time::microsec_clock::universal_time());
	    ~LocalNode();

	    GScale::INodeCallback& getCallback() const;
	private:
	    GScale::INodeCallback &cbs;
};

typedef boost::shared_ptr<LocalNode> LocalNodePtr;

}

#endif /* LOCALNODE_HPP_ */
