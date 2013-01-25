/*
 * ibackend.hpp
 *
 */

#ifndef IBACKEND_HPP_
#define IBACKEND_HPP_

namespace GScale{

class INode;
class Group;
class GroupNodesDAO;
class Packet;

namespace Backend{

class IBackend{
	public:
		virtual ~IBackend(){}

		virtual void initialize(GScale::Group *group, GScale::GroupNodesDAO *gdao) = 0;

		/* called when a node becomes available */
		virtual void OnLocalNodeAvailable(const GScale::INode *node) = 0;
		/* called when a local node becomes unavailable */
		virtual void OnLocalNodeUnavailable(const GScale::INode *node) = 0;

		/* called when a local node writes data to the group */
		virtual unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet)=0;

		virtual void Worker() = 0;
};

}

}

#endif /* IBACKEND_HPP_ */
