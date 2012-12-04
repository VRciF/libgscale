/*
 * loopback.hpp
 *
 *  Created on: Nov 6, 2012
 *      Author: 1001884
 */

#ifndef LOOPBACK_HPP_
#define LOOPBACK_HPP_

#include "ibackend.hpp"
#include "group.hpp"
#include "inode.hpp"
#include "packet.hpp"

namespace GScale{

namespace Backend{

class Loopback : public GScale::Backend::IBackend{
	public:
		Loopback(GScale::Group *group);
		~Loopback();

		/* called when a node becomes available */
		void OnLocalNodeAvailable(const GScale::INode *node,
				                  const GScale::LocalNodes &localnodes);
		/* called when a local node becomes unavailable */
		void OnLocalNodeUnavailable(const GScale::INode *node,
				                    const GScale::LocalNodes &localnodes);

		/* called when a local node writes data to the group */
		unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet,
				                              const GScale::LocalNodes &localnodes);
	    void Worker(struct timeval *);
	private:
		GScale::Group *group;
};

}

}

#endif /* LOOPBACK_HPP_ */
