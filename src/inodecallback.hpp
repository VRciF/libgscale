/*
 * INodeCallback.hpp
 *
 */

#ifndef INODECALLBACK_HPP_
#define INODECALLBACK_HPP_

#include "group.hpp"
#include "inode.hpp"
#include "localnode.hpp"
#include "types.hpp"
#include "packet.hpp"

namespace GScale{

class Group;

class INodeCallback{
	public:
		virtual void OnRead(GScale::Group *g, const GScale::Packet &packet) = 0;
		virtual void OnNodeAvailable(GScale::Group *g, const GScale::INode *src,
									 const GScale::LocalNode *dst) = 0;
		virtual void OnNodeUnavailable(GScale::Group *g,
									   const GScale::INode *src,
									   const GScale::LocalNode *dst) = 0;
};

}

#endif /* INODECALLBACK_HPP_ */
