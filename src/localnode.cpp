/*
 * localnode.cpp
 *
 */

#include "localnode.hpp"

#include "inodecallback.hpp"

namespace GScale{

LocalNode::LocalNode(GScale::INodeCallback &cbs) : INode(), cbs(cbs){}
LocalNode::LocalNode(std::string alias, GScale::INodeCallback &cbs) : INode(alias), cbs(cbs){}
LocalNode::LocalNode(const LocalNode &lnode) : INode(lnode), cbs(cbs){

}
LocalNode::~LocalNode(){}

GScale::INodeCallback& LocalNode::getCallback() const{
	return this->cbs;
}

void LocalNode::operator=(const LocalNode &source ){
	this->cbs = source.cbs;
	this->hostuuid = source.hostuuid;
	this->nodeuuid = source.nodeuuid;
	this->alias = source.alias;
}

}

