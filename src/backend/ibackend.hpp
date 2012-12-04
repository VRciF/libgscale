/*
 * ibackend.hpp
 *
 */

#ifndef IBACKEND_HPP_
#define IBACKEND_HPP_

#include "inode.hpp"
#include "packet.hpp"
#include "group.hpp"

namespace GScale{

namespace Backend{

class IBackend{
	public:
		virtual ~IBackend(){}

		/* called when a node becomes available */
		virtual void OnLocalNodeAvailable(const GScale::INode *node,
				                          const GScale::LocalNodes &localnodes) = 0;
		/* called when a local node becomes unavailable */
		virtual void OnLocalNodeUnavailable(const GScale::INode *node,
				                            const GScale::LocalNodes &localnodes) = 0;

		/* called when a local node writes data to the group */
		virtual unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet,
				                                      const GScale::LocalNodes &localnodes)=0;

		virtual void Worker(struct timeval *timeout) = 0;
};

}

}

#endif /* IBACKEND_HPP_ */
