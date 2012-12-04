/*
 * loopback.cpp
 *
 */

#include "loopback.hpp"
#include "localnode.hpp"

namespace GScale{

namespace Backend{

Loopback::Loopback(GScale::Group *group){
	this->group = group;
}
Loopback::~Loopback(){}


void Loopback::OnLocalNodeAvailable(const GScale::INode *node,
		                            const GScale::LocalNodes &localnodes)
{
	GScale::LocalNodes::const_iterator cbit;
	for(cbit = localnodes.begin(); cbit!=localnodes.end(); cbit++){
		if(node == *cbit){ continue; }
		(*cbit)->getCallback().OnNodeAvailable(this->group, node, (*cbit));
	}
}

void Loopback::OnLocalNodeUnavailable(const GScale::INode *node,
		                              const GScale::LocalNodes &localnodes)
{
	GScale::LocalNodes::const_iterator cbit;
	for(cbit = localnodes.begin(); cbit!=localnodes.end(); cbit++){
		if(node == *cbit){ continue; }

		(*cbit)->getCallback().OnNodeUnavailable(this->group, node, (*cbit));
	}
}

unsigned int Loopback::OnLocalNodeWritesToGroup(const GScale::Packet &packet,
		                                        const GScale::LocalNodes &localnodes)
{
	GScale::LocalNodes::const_iterator cbit = localnodes.find((const GScale::LocalNode*)packet.getReceiver());
	if(cbit != localnodes.end()){
		(*cbit)->getCallback().OnRead(this->group, packet);
	}

	return packet.size();
}

void Worker(struct timeval *){}

}

}
