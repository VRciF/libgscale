/*
 * loopback.hpp
 *
 */

#ifndef LOOPBACK_HPP_
#define LOOPBACK_HPP_

#include "ibackend.hpp"

namespace GScale{

class Group;
class INode;
class Packet;

namespace Backend{

class Loopback : public GScale::Backend::IBackend{
	public:
		Loopback();
		~Loopback();

		void initialize(GScale::GroupCore *groupc, GScale::GroupNodesDAO *gdao);

		/* called when a node becomes available */
		void OnLocalNodeAvailable(GScale::LocalNode node);
		/* called when a local node becomes unavailable */
		void OnLocalNodeUnavailable(GScale::LocalNode node);

		/* called when a local node writes data to the group */
		unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet);
	    void Worker();
	private:
		GScale::GroupCore *groupc;
		GScale::GroupNodesDAO *gdao;
};

}

}

#endif /* LOOPBACK_HPP_ */
