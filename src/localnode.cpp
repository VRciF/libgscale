/*
 * localnode.cpp
 *
 */

#include "localnode.hpp"

namespace GScale{

LocalNode::LocalNode(GScale::INodeCallback &cbs, boost::posix_time::ptime ctime) : INode(), cbs(cbs){
    this->ctime = ctime;
}
LocalNode::LocalNode(std::string alias, GScale::INodeCallback &cbs, boost::posix_time::ptime ctime) : INode(), cbs(cbs){
	this->alias = alias;
	this->ctime = ctime;
}
LocalNode::~LocalNode(){}

GScale::INodeCallback& LocalNode::getCallback() const{
	return this->cbs;
}

}

