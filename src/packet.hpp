/*
 * packet.hpp
 *
 */

#ifndef PACKET_HPP_
#define PACKET_HPP_

#include <string>

#include "inode.hpp"

namespace GScale{

class Packet{
    public:
	    Packet(const GScale::INode *sender, const GScale::INode *receiver);

	    const GScale::INode* getSender() const;
	    const GScale::INode* getReceiver() const;

	    void setPayload(std::string payload);
	    const std::string& getPayload() const;
	    unsigned int size() const;
	    void resetPayload();
	private:
	    const GScale::INode *sender;
	    const GScale::INode *receiver;

		std::string payload;
};

}

#endif /* PACKET_HPP_ */
