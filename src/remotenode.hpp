/*
 * remotenode.hpp
 *
 */

#ifndef REMOTENODE_HPP_
#define REMOTENODE_HPP_

#include "inode.hpp"

namespace GScale{

class RemoteNode : public GScale::INode{
	public:
        RemoteNode(std::string nodeuuid);
        RemoteNode(std::string nodeuuid, std::string alias);
		virtual ~RemoteNode();
		bool isLocal();
};

}

#endif /* REMOTENODE_HPP_ */
