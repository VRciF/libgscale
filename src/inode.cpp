/*
 * inode.cpp
 *
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "inode.hpp"

namespace GScale{

INode::INode(std::string hostuuid){
    this->hostuuid = hostuuid;
    this->nodeuuid = boost::uuids::random_generator()();
}
INode::INode(std::string hostuuid, std::string uuid){
    this->hostuuid = hostuuid;
    this->nodeuuid = boost::uuids::string_generator(uuid);
}
INode::INode(std::string hostuuid, std::string uuid, std::string alias){
    this->hostuuid = hostuuid;
    this->nodeuuid = boost::uuids::string_generator(uuid);
    this->alias    = alias;
}

INode::INode(){
    this->nodeuuid = boost::uuids::random_generator()();
}
INode::INode(std::string uuid){
    this->hostuuid = Core::getInstance()->getHostUUID();
    this->nodeuuid = boost::uuids::string_generator(uuid);
}
INode::INode(std::string uuid, std::string alias){
    this->hostuuid = Core::getInstance()->getHostUUID();
    this->nodeuuid = boost::uuids::string_generator(uuid);
    this->alias    = alias;
}

INode::~INode(){}

bool INode::isLocal() const{
    return this->hostuuid == Core::getInstance()->getHostUUID();
}

const boost::uuids::uuid INode::getNodeUUID() const{
	return this->nodeuuid;
}
const boost::uuids::uuid INode::getHostUUID() const{
    return this->hostuuid;
}

std::string INode::getAlias() const{
	if(this->alias.length()<=0){
		return boost::lexical_cast<std::string>(this->nodeuuid);
	}
	return this->alias;
}


inline bool INode::operator== (const INode &b) const
{
    return b.nodeuuid==this->nodeuuid;
}

}
