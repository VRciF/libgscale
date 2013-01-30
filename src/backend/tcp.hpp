/*
 * inode.hpp
 *
 */

#ifndef TCP_HPP_
#define TCP_HPP_

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "ibackend.hpp"
#include "tcp/session.hpp"

namespace GScale{

class Group;
class INode;
class Packet;

namespace Backend{

typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> Acceptor_ptr;

class TCP : public GScale::Backend::IBackend{
    friend class TCP_Session;

    public:
        TCP();
        void initialize(GScale::Group *group, GScale::GroupNodesDAO *gdao);
        ~TCP();

        /* called when a node becomes available */
        void OnLocalNodeAvailable(GScale::LocalNode node);
        /* called when a local node becomes unavailable */
        void OnLocalNodeUnavailable(GScale::LocalNode node);

        /* called when a local node writes data to the group */
        unsigned int OnLocalNodeWritesToGroup(const GScale::Packet &packet);
        void Worker();
    protected:
        void noop(const boost::system::error_code& error);

        void uuid_write_finished(const boost::system::error_code& error, std::size_t bytes_transferred, TCP_Session_ptr session);
        void uuid_read_finished(const boost::system::error_code& error, std::size_t bytes_transferred, TCP_Session_ptr session);

        void start_accept(Acceptor_ptr acceptor_);
        void handle_socket_init(Acceptor_ptr acceptor_, TCP_Session_ptr session,
                                const boost::system::error_code& error);
        void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd, int type);
        void multicastHeartbeat(const boost::system::error_code& /*e*/);

        void on_local_close();
        void bind_or_connect();
    private:
        struct MulticastPacket{
            boost::uuids::uuid hostuuid;
            int ip_port;

            friend class boost::serialization::access;
            // When the class Archive corresponds to an output archive, the
            // & operator is defined similar to <<.  Likewise, when the class Archive
            // is a type of input archive the & operator is defined similar to >>.ost
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & hostuuid;
                ar & ip_port;
            }
        };

        // define a multiply indexed set with indices by id and name
        typedef boost::multi_index_container<
          TCP_Session_ptr,
          boost::multi_index::indexed_by<
              boost::multi_index::ordered_unique<boost::multi_index::member<GScale::Backend::TCP_Session,boost::uuids::uuid,&GScale::Backend::TCP_Session::remotehostuuid> >,
              boost::multi_index::ordered_non_unique<
                boost::multi_index::const_mem_fun<GScale::Backend::TCP_Session,boost::asio::ip::tcp::endpoint,&GScale::Backend::TCP_Session::remote_endpoint>
              >,
              boost::multi_index::ordered_non_unique<
                boost::multi_index::const_mem_fun<GScale::Backend::TCP_Session,boost::asio::ip::tcp::endpoint,&GScale::Backend::TCP_Session::local_endpoint>
              >
            // sort by employee::operator<
            //ordered_unique<identity<employee> >,

            // sort by less<string> on name
            //ordered_non_unique<member<employee,std::string,&employee::name> >

          >
        > TCP_Session_Set;
        TCP_Session_Set sessions;

        boost::scoped_ptr<boost::asio::deadline_timer> mcast_heartbeat_timer;

        boost::asio::ip::udp::endpoint mcast_endpoint_v4, mcast_endpoint_v6;
        boost::scoped_ptr<boost::asio::ip::udp::socket> mcast_sendsocket_v4, mcast_sendsocket_v6;

        boost::asio::ip::udp::endpoint mcast_sender_endpoint_v4, mcast_sender_endpoint_v6;
        boost::scoped_ptr<boost::asio::ip::udp::socket> mcast_readersocket_v4, mcast_readersocket_v6;

        boost::scoped_ptr<boost::asio::ip::tcp::acceptor> acceptor_v4, acceptor_v6;
        TCP_Session_ptr local_v4;

        char udp_rcv_buffer[64*1024];

        GScale::Group *group;
        GScale::GroupNodesDAO *gdao;

        boost::asio::io_service io_service;
        uint16_t listen_port;
        uint16_t multicast_port;

        bool initialized;
};

}

}

#endif /* TCP_HPP_ */
