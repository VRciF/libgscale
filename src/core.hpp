/*
 * core.hpp
 *
 */

#ifndef CORE_HPP_
#define CORE_HPP_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "types.hpp"
#include "packet.hpp"

namespace GScale{

class Core{
	public:
		static Core* getInstance();
		~Core();

		void onNodeAvailable(const GScale::NodeFQIdentifier &fqid, void *tag);
		void onNodeUnavailable(const GScale::NodeFQIdentifier &fqid);
		int32_t sendLocalMessage(const GScale::Packet *packet);

		boost::uuids::uuid getHostUUID();
	private:
		Core();

		static Core *inst;
		const boost::uuids::uuid hostuuid;
};

}

#endif /* INSTANCE_HPP_ */
