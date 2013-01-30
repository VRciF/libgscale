/*
 * packet.cpp
 *
 */

#include "packet.hpp"

namespace GScale{

Packet::Packet(const GScale::INode sender, const GScale::INode receiver) : sender(sender), receiver(receiver){
    this->type_ = MIN;
}
Packet::~Packet(){}

const GScale::INode& Packet::getSender() const{
	return this->sender;
}
const GScale::INode& Packet::getReceiver() const{
	return this->receiver;
}

const std::string& Packet::payload(const std::string payload){
    this->payload_ = payload;
    return this->payload_;
}
const std::string& Packet::payload() const{
    return this->payload_;
}

unsigned int Packet::size() const{
	return this->payload_.size();
}

Packet::METATYPE Packet::type(Packet::METATYPE t){
    if(t>MIN && t<MAX){
        this->type_ = t;
    }
    return this->type_;
}

}
