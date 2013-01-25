/*
 * core.hpp
 *
 */

#ifndef CORE_HPP_
#define CORE_HPP_

#include <boost/uuid/uuid_serialize.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace GScale{

class NodeFQIdentifier;
class Packet;

class Core{
	public:
		static Core* getInstance();
		~Core();

		void onNodeAvailable(const GScale::NodeFQIdentifier &fqid, void *tag);
		void onNodeUnavailable(const GScale::NodeFQIdentifier &fqid);
		int32_t sendLocalMessage(const GScale::Packet *packet);

		const boost::uuids::uuid& getHostUUID() const;
	private:
		Core();

		static Core *inst;
		const boost::uuids::uuid hostuuid;
};

}

#endif /* INSTANCE_HPP_ */
