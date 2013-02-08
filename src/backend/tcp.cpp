/*
 * tcp.cpp
 *
 */


#include "core.hpp"
#include "tcp.hpp"
#include "localnode.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/system/system_error.hpp>

namespace GScale{

namespace Backend{

TCP::TCP() {
    this->groupc = NULL;
    this->multicast_port = 8000;
    this->listen_port = 8000;
    this->initialized = false;
}
void TCP::initialize(GScale::GroupCore *groupc, GScale::GroupNodesDAO *gdao){
    this->groupc = groupc;
    this->gdao = gdao;
}
TCP::~TCP(){}

void TCP::OnLocalNodeAvailable(GScale::LocalNode /*node*/)
{
	// a new node is available, so we need to sync pending nodes for every open session
	TCP_Session_Set::nth_index<1>::type& idx=this->sessions.get<1>();
	TCP_Session_Set::nth_index<1>::type::iterator it = idx.begin();
	for(;it!=idx.end();it++){
		(*it)->enqueueSyncNodeList();
	}

    /*
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin();
    while(it != this->gdao->end()){
        if((*it)->getNodeUUID() == node->getNodeUUID()){ continue; }
        (*it)->getCallback().OnNodeAvailable(this->group, node, (*it));
    }
    */
}

void TCP::OnLocalNodeUnavailable(GScale::LocalNode /*node*/)
{
    /*
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->begin();
    while(it != this->gdao->end()){
        if((*it)->getNodeUUID() == node->getNodeUUID()){ continue; }
        (*it)->getCallback().OnNodeUnavailable(this->group, node, (*it));
    }
    */
}

void TCP::OnLocalNodeWritesToGroup(const PacketPtr /*packet*/)
{
    /*
    GScale::Group::LocalNodesSetIdx_uuid::iterator it = this->gdao->findByUUID(packet.getReceiver()->getNodeUUID());
    if(it != this->gdao->end()){
        (*it)->getCallback().OnRead(this->group, packet);
    }

    return packet.size();
    */
}

void TCP::noop(const boost::system::error_code& /*error*/){}

void TCP::uuid_write_finished(const boost::system::error_code& error, std::size_t bytes_transferred, TCP_Session_ptr session){
    if(!error && bytes_transferred == session->remotehostuuid.size()){
        // the full uuid has been transfered
        // wait for read to be finished
    }
    else{
        // write failed, disconnect socket
        session->close();
    }
}
void TCP::uuid_read_finished(const boost::system::error_code& error, std::size_t bytes_transferred, TCP_Session_ptr session){
    if(!error && bytes_transferred == session->remotehostuuid.size()){
        // the condition remote_endpoint < local_endpoint prevents the posibility of having two connections
        // o) one from local to remote, and o) second from remote to local
        // where only one of them may be used
        if(this->sessions.get<0>().find(session->remotehostuuid) == this->sessions.get<0>().end() &&
           session->remote_endpoint() < session->local_endpoint()){
            this->sessions.get<0>().insert(session);
            session->start();
            return;
        }
    }
    // read failed or connection already in list, disconnect socket
    session->close();
}

void TCP::start_accept(Acceptor_ptr acceptor_)
{
    TCP_Session_ptr new_session(new TCP_Session(this, this->io_service));
    acceptor_->async_accept(new_session->socket(),
        boost::bind(&TCP::handle_socket_init, this, acceptor_, new_session,
            boost::asio::placeholders::error));
}

void TCP::handle_socket_init(Acceptor_ptr acceptor_, TCP_Session_ptr session,
                        const boost::system::error_code& error)
{
  if (!error)
  {
      // async write local's hostuuid
      boost::asio::async_write(session->socket(), boost::asio::buffer((char*)&Core::getInstance()->getHostUUID(), Core::getInstance()->getHostUUID().size()),
              boost::bind(&TCP::uuid_write_finished, this, boost::asio::placeholders::error,
                          boost::asio::placeholders::bytes_transferred, session));

      // async read remote hostuuid
      // the trick is that uuid states:
      //     "This library implements a UUID as a POD allowing a UUID to be used in the most efficient ways, including using memcpy, and aggregate initializers."
      boost::asio::async_read(session->socket(), boost::asio::buffer((char*)&session->remotehostuuid, session->remotehostuuid.size()),
              boost::bind(&TCP::uuid_read_finished, this, boost::asio::placeholders::error,
                          boost::asio::placeholders::bytes_transferred, session));
  }

  if(acceptor_.get() != NULL){
      start_accept(acceptor_);
  }
}

void TCP::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd, int type)
{
  if (!error && bytes_recvd>0)
  {
      struct MulticastPacket packet;

      std::stringstream ss;
      ss << std::string(this->udp_rcv_buffer, bytes_recvd);
      boost::archive::text_iarchive ia(ss);
      // read class state from archive
      ia >> packet;

      if(Core::getInstance()->getHostUUID()!=packet.hostuuid &&
         this->sessions.get<0>().find(packet.hostuuid) == this->sessions.get<0>().end()){
          std::cout << "rcvd mcast message" << packet.ip_port << std::endl;

          boost::asio::ip::tcp::socket socket(io_service);
          boost::asio::ip::tcp::endpoint endpoint;
          if(type == boost::asio::ip::udp::v4().type()){
              endpoint = boost::asio::ip::tcp::endpoint(this->mcast_sender_endpoint_v4.address(), packet.ip_port);
          }
          else{
              endpoint = boost::asio::ip::tcp::endpoint(this->mcast_sender_endpoint_v6.address(), packet.ip_port);
          }

          TCP_Session_ptr new_session(new TCP_Session(this, this->io_service));
          new_session->socket().async_connect(endpoint, boost::bind(&TCP::handle_socket_init, this, Acceptor_ptr(), new_session, boost::asio::placeholders::error));
      }
      else{
          std::cout << "packet is from local instance or already connected" << std::endl;
      }
  }

  if(type == boost::asio::ip::udp::v4().type()){
      this->mcast_readersocket_v4->async_receive_from(
          boost::asio::buffer(this->udp_rcv_buffer, sizeof(this->udp_rcv_buffer)), this->mcast_sender_endpoint_v4,
          boost::bind(&TCP::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred, boost::asio::ip::udp::v4().type()));
  }
  else{
      this->mcast_readersocket_v6->async_receive_from(
          boost::asio::buffer(this->udp_rcv_buffer, sizeof(this->udp_rcv_buffer)), this->mcast_sender_endpoint_v6,
          boost::bind(&TCP::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred, boost::asio::ip::udp::v6().type()));
  }
}
void TCP::multicastHeartbeat(const boost::system::error_code& /*e*/)
{
    std::string message_;

    {
        const struct MulticastPacket packet = {Core::getInstance()->getHostUUID(), this->acceptor_v4->local_endpoint().port()};

        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << packet;

        message_ = os.str();

        this->mcast_sendsocket_v4->async_send_to(boost::asio::buffer(message_), this->mcast_endpoint_v4, boost::bind(&TCP::noop, this, boost::asio::placeholders::error));
    }

    {
        const struct MulticastPacket packet = {Core::getInstance()->getHostUUID(), this->acceptor_v6->local_endpoint().port()};

        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << packet;

        message_ = os.str();

        this->mcast_sendsocket_v6->async_send_to(boost::asio::buffer(message_), this->mcast_endpoint_v6, boost::bind(&TCP::noop, this, boost::asio::placeholders::error));
    }

    this->mcast_heartbeat_timer->expires_at(this->mcast_heartbeat_timer->expires_at() + boost::posix_time::seconds(60));
    this->mcast_heartbeat_timer->async_wait(boost::bind(&TCP::multicastHeartbeat, this, boost::asio::placeholders::error));
}

void TCP::on_local_close(){
    this->local_v4.reset();
    this->bind_or_connect();
}

void TCP::bind_or_connect(){
    bool reenqueue = false;

    if(this->acceptor_v4.get()==NULL && this->local_v4.get()==NULL){
        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint localep(boost::asio::ip::address_v4::from_string("127.0.0.1"), this->listen_port);

        try{
            this->acceptor_v4.reset(new boost::asio::ip::tcp::acceptor(io_service, localep));
            this->start_accept(Acceptor_ptr(this->acceptor_v4.get()));
        }
        catch(...){
            this->local_v4.reset(new TCP_Session(this, this->io_service));
            this->local_v4->socket().connect(localep, ec);

            if(!ec){
                this->local_v4->on_close(boost::bind(&TCP::on_local_close, this));

                this->handle_socket_init(Acceptor_ptr(), this->local_v4, ec);
            }
            else{
                this->local_v4.reset();
                reenqueue = true;
            }
        }
    }

    if(this->acceptor_v4.get()!=NULL && this->acceptor_v6.get()==NULL){
        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint localep(boost::asio::ip::address_v6::v4_mapped(boost::asio::ip::address_v4::from_string("127.0.0.1")), this->listen_port);

        try{
            this->acceptor_v6.reset(new boost::asio::ip::tcp::acceptor(io_service, localep));
            this->start_accept(Acceptor_ptr(this->acceptor_v6.get()));
        }
        catch(...){
            reenqueue = true;
        }
    }

    if(reenqueue){
        this->io_service.dispatch(boost::bind(&TCP::bind_or_connect, this));
    }
}

void TCP::Worker(){
    if(!this->initialized){
        const boost::asio::ip::address& multicast_address_v4 = boost::asio::ip::address::from_string("239.255.92.19");
        const boost::asio::ip::address& multicast_address_v6 = boost::asio::ip::address::from_string("ff00:0:0:0:0:0:efff:5c13");

        this->mcast_endpoint_v4 = boost::asio::ip::udp::endpoint(multicast_address_v4, multicast_port);
        this->mcast_sendsocket_v4.reset(new boost::asio::ip::udp::socket(io_service, this->mcast_endpoint_v4.protocol()));

        this->mcast_endpoint_v6 = boost::asio::ip::udp::endpoint(multicast_address_v6, multicast_port);
        this->mcast_sendsocket_v6.reset(new boost::asio::ip::udp::socket(io_service, this->mcast_endpoint_v6.protocol()));

        {
        // Create the socket so that multiple may be bound to the same address.
        boost::asio::ip::udp::endpoint listen_endpoint(boost::asio::ip::address::from_string("0.0.0.0"), multicast_port);
        this->mcast_readersocket_v4.reset(new boost::asio::ip::udp::socket(io_service, listen_endpoint.protocol()));
        this->mcast_readersocket_v4->set_option(boost::asio::ip::udp::socket::reuse_address(true));
        this->mcast_readersocket_v4->bind(listen_endpoint);

        // Join the multicast group.
        this->mcast_readersocket_v4->set_option(boost::asio::ip::multicast::join_group(multicast_address_v4));

        this->handle_receive_from(boost::system::error_code(), 0, boost::asio::ip::udp::v4().type());
        }
        {
        // Create the socket so that multiple may be bound to the same address.
        boost::asio::ip::udp::endpoint listen_endpoint(boost::asio::ip::address::from_string("0::0"), multicast_port);
        this->mcast_readersocket_v6.reset(new boost::asio::ip::udp::socket(io_service, listen_endpoint.protocol()));
        this->mcast_readersocket_v6->set_option(boost::asio::ip::udp::socket::reuse_address(true));
        this->mcast_readersocket_v6->bind(listen_endpoint);

        // Join the multicast group.
        this->mcast_readersocket_v6->set_option(boost::asio::ip::multicast::join_group(multicast_address_v6));

        this->handle_receive_from(boost::system::error_code(), 0, boost::asio::ip::udp::v6().type());
        }

        this->bind_or_connect();

        this->initialized = true;


        this->mcast_heartbeat_timer.reset(new boost::asio::deadline_timer(this->io_service));
        this->mcast_heartbeat_timer->expires_from_now(boost::posix_time::seconds(0)-boost::posix_time::seconds(1));
        this->mcast_heartbeat_timer->async_wait(boost::bind(&TCP::multicastHeartbeat, this, boost::asio::placeholders::error));
    }

    if(this->io_service.stopped()){
        this->io_service.reset();
    }
    this->io_service.poll();
}

}

}
