/*
 * inode.cpp
 *
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "inode.hpp"

namespace GScale{

INode::INode(){
	this->nodeuuid = boost::uuids::random_generator()();
}
INode::~INode(){}

const boost::uuids::uuid INode::getNodeUUID() const{
	return this->nodeuuid;
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
