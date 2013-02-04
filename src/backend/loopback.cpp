/*
 * loopback.cpp
 *
 */

#include "loopback.hpp"

#include "groupcore.hpp"
#include "inodecallback.hpp"
#include "localnode.hpp"
#include "packet.hpp"

namespace GScale{

namespace Backend{

Loopback::Loopback(){
	this->groupc = NULL;
	this->gdao = NULL;
}
Loopback::~Loopback(){}

void Loopback::initialize(GScale::GroupCore *groupc, GScale::GroupNodesDAO *gdao){
    this->groupc = groupc;
    this->gdao = gdao;
}

void Loopback::OnLocalNodeAvailable(GScale::LocalNode node)
{
    GScale::GroupCore::LocalNodeSetIdx_uuid::iterator it = this->gdao->begin<GScale::GroupCore::idxnode_nodeuuid>();
    while(it != this->gdao->end<GScale::GroupCore::idxnode_nodeuuid>()){
        if(it->node.getNodeUUID() == node.getNodeUUID()){ continue; }
        it->node.getCallback().OnNodeAvailable(this->groupc->getGroupAPI(), &node, &it->node);
    }
}

void Loopback::OnLocalNodeUnavailable(GScale::LocalNode node)
{
    GScale::GroupCore::LocalNodeSetIdx_uuid::iterator it = this->gdao->begin<GScale::GroupCore::idxnode_nodeuuid>();
    while(it != this->gdao->end<GScale::GroupCore::idxnode_nodeuuid>()){
        if(it->node.getNodeUUID() == node.getNodeUUID()){ continue; }
        it->node.getCallback().OnNodeUnavailable(this->groupc->getGroupAPI(), &node, &it->node);
    }
}

unsigned int Loopback::OnLocalNodeWritesToGroup(const GScale::Packet &packet)
{
    GScale::GroupCore::LocalNodeSetIdx_uuid::iterator it = this->gdao->findByUUID(packet.getReceiver().getNodeUUID());
    if(it != this->gdao->end<GScale::GroupCore::idxnode_nodeuuid>()){
        it->node.getCallback().OnRead(this->groupc->getGroupAPI(), packet);
    }

	return packet.size();
}

void Loopback::Worker(){}

}

}
