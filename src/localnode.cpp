/*
 * localnode.cpp
 *
 */

#include "localnode.hpp"

namespace GScale{

LocalNode::LocalNode(GScale::INodeCallback &cbs) : INode(), cbs(cbs){
	this->created = boost::posix_time::microsec_clock::local_time();
}
LocalNode::LocalNode(std::string alias, GScale::INodeCallback &cbs) : INode(), cbs(cbs){
	this->alias = alias;
	this->created = boost::posix_time::microsec_clock::local_time();
}
LocalNode::~LocalNode(){}


GScale::INodeCallback& LocalNode::getCallback() const{
	return this->cbs;
}

bool LocalNode::isLocal(){
	return true;
}

}

