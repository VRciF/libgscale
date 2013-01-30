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

void Loopback::OnLocalNodeAvailable(GScale::LocalNode node)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin<GScale::Group::idxname_nodeuuid>();
    while(it != this->gdao->end<GScale::Group::idxname_nodeuuid>()){
        if(it->getNodeUUID() == node.getNodeUUID()){ continue; }
        it->getCallback().OnNodeAvailable(this->group, &node, &(*it));
    }
}

void Loopback::OnLocalNodeUnavailable(GScale::LocalNode node)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin<GScale::Group::idxname_nodeuuid>();
    while(it != this->gdao->end<GScale::Group::idxname_nodeuuid>()){
        if(it->getNodeUUID() == node.getNodeUUID()){ continue; }
        it->getCallback().OnNodeUnavailable(this->group, &node, &(*it));
    }
}

unsigned int Loopback::OnLocalNodeWritesToGroup(const GScale::Packet &packet)
{
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->findByUUID(packet.getReceiver().getNodeUUID());
    if(it != this->gdao->end<GScale::Group::idxname_nodeuuid>()){
        it->getCallback().OnRead(this->group, packet);
    }

	return packet.size();
}

void Loopback::Worker(){}

}

}
