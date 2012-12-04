/*
 * packet.cpp
 *
 */

#include "packet.hpp"

namespace GScale{

Packet::Packet(const GScale::INode *sender, const GScale::INode *receiver) : sender(sender), receiver(receiver){
	this->payload = std::string();
}

const GScale::INode* Packet::getSender() const{
	return this->sender;
}
const GScale::INode* Packet::getReceiver() const{
	return this->receiver;
}

void Packet::setPayload(std::string payload){
	this->payload = payload;
}
const std::string& Packet::getPayload() const{
	return this->payload;
}
void Packet::resetPayload(){
	this->setPayload(std::string());
}
unsigned int Packet::size() const{
	return this->payload.size();
}

}
