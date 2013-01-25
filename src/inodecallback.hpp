/*
 * INodeCallback.hpp
 *
 */

#ifndef INODECALLBACK_HPP_
#define INODECALLBACK_HPP_

#include <boost/shared_ptr.hpp>

namespace GScale{

class INode;
class LocalNode;
class Group;
class Packet;

class INodeCallback{
	public:
		virtual void OnRead(GScale::Group *g, const GScale::Packet &packet) = 0;
		virtual void OnNodeAvailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst) = 0;
		virtual void OnNodeUnavailable(GScale::Group *g, const GScale::INode *src, const GScale::INode *dst) = 0;
};

}

#endif /* INODECALLBACK_HPP_ */
