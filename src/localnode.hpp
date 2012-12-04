/*
 * localnode.hpp
 *
 */

#ifndef LOCALNODE_HPP_
#define LOCALNODE_HPP_

#include <string>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "inode.hpp"
#include "inodecallback.hpp"
#include "types.hpp"
#include "packet.hpp"

namespace GScale{

class INodeCallback;

class LocalNode : public GScale::INode{
	public:
	    LocalNode(GScale::INodeCallback &cbs);
	    LocalNode(std::string alias, GScale::INodeCallback &cbs);
	    ~LocalNode();

	    GScale::INodeCallback& getCallback() const;

	    bool isLocal();
	private:
	    GScale::INodeCallback &cbs;
		boost::posix_time::ptime created;
};

}

#endif /* LOCALNODE_HPP_ */
