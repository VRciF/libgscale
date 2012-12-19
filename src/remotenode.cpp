/*
 * remotenode.cpp
 *
 */


#include "remotenode.hpp"

namespace GScale{

RemoteNode::RemoteNode(std::string nodeuuid) : INode(nodeuuid){}
RemoteNode::RemoteNode(std::string nodeuuid, std::string alias) : INode(nodeuuid, alias){}
RemoteNode::~RemoteNode(){}

bool RemoteNode::isLocal(){
	return false;
}

}

