/*
 * loopback.cpp
 *
 */

#include "loopback.hpp"

#include "group.hpp"
#include "inodecallback.hpp"
#include "localnode.hpp"
#include "packet.hpp"

namespace GScale{

namespace Backend{

Loopback::Loopback(){
	this->group = NULL;
	this->gdao = NULL;
}
Loopback::~Loopback(){}

void Loopback::initialize(GScale::Group *group, GScale::GroupNodesDAO *gdao){
    this->group = group;
    this->gdao = gdao;
}

void Loopback::OnLocalNodeAvailable(const GScale::INode *node)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin<GScale::Group::idxname_nodeuuid>();
    while(it != this->gdao->end()){
        if((*it)->getNodeUUID() == node->getNodeUUID()){ continue; }
        (*it)->getCallback().OnNodeAvailable(this->group, node, (*it).get());
    }
}

void Loopback::OnLocalNodeUnavailable(const GScale::INode *node)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin<GScale::Group::idxname_nodeuuid>();
    while(it != this->gdao->end()){
        if((*it)->getNodeUUID() == node->getNodeUUID()){ continue; }
        (*it)->getCallback().OnNodeUnavailable(this->group, node, (*it).get());
    }
}

unsigned int Loopback::OnLocalNodeWritesToGroup(const GScale::Packet &packet)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->findByUUID(packet.getReceiver()->getNodeUUID());
    if(it != this->gdao->end()){
        (*it)->getCallback().OnRead(this->group, packet);
    }

	return packet.size();
}

void Loopback::Worker(){}

}

}
