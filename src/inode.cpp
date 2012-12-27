/*
 * inode.cpp
 *
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/uuid/string_generator.hpp>

#include "inode.hpp"
#include "core.hpp"

namespace GScale{

INode::INode() : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(boost::uuids::random_generator()()){}
INode::INode(std::string alias) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(boost::uuids::random_generator()()), alias(alias){}
INode::INode(boost::uuids::uuid uuid) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(uuid){}
INode::INode(boost::uuids::uuid uuid, std::string alias) : hostuuid(Core::getInstance()->getHostUUID()), nodeuuid(uuid), alias(alias){}

INode::INode(boost::uuids::uuid hostuuid, boost::uuids::uuid uuid) : hostuuid(hostuuid), nodeuuid(uuid){}

INode::~INode(){}

const INode& INode::getNilNode(){
    static const INode n(boost::uuids::nil_uuid(), boost::uuids::nil_uuid());
    return n;
}

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
