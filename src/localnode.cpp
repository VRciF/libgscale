/*
 * localnode.cpp
 *
 */

#include "localnode.hpp"

#include "inodecallback.hpp"

namespace GScale{

LocalNode::LocalNode(boost::asio::io_service io_service, GScale::INodeCallback &cbs) : INode(), io_service(io_service), cbs(cbs){}
LocalNode::LocalNode(boost::asio::io_service io_service, std::string alias, GScale::INodeCallback &cbs) : INode(alias), io_service(io_service), cbs(cbs){}
LocalNode::LocalNode(const LocalNode &lnode) : INode(lnode), io_service(lnode.io_service), cbs(lnode.cbs){}
LocalNode::~LocalNode(){}

void LocalNode::OnRead(GScale::Group *g, const GScale::Packet &packet){
	this->io_service.post(boost::bind(&GScale::INodeCallback::OnRead, this->cbs, g, packet));
}
void LocalNode::OnNodeAvailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst){
	this->io_service.post(boost::bind(&GScale::INodeCallback::OnNodeAvailable, this->cbs, g, dst));
}
void LocalNode::OnNodeUnavailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst){
	this->io_service.post(boost::bind(&GScale::INodeCallback::OnNodeUnavailable, this->cbs, g, src, dst));
}

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

