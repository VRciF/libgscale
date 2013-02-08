/*
 * session.cpp
 *
 */

#include "session.hpp"

#include <boost/bind.hpp>

#include "groupcore.hpp"
#include "tcp.hpp"

namespace GScale{

namespace Backend{

TCP_Session::TCP_Session(TCP *backend, boost::asio::io_service& io_service) : io_service(io_service),socket_(io_service)
{
	this->is_syncnode_enqueued = false;
    this->backend = backend;
    this->proto.reset(new TCP_Protocol<TCP_Session>(this->socket(), *this));
}

boost::asio::ip::tcp::socket& TCP_Session::socket()
{
    return this->socket_;
}

void TCP_Session::start(){
	this->enqueueSyncNodeList();

}

void TCP_Session::onReadError(const boost::system::error_code& /*error*/){
    this->close();
}
void TCP_Session::onSendError(GScale::Packet &/*p*/, const boost::system::error_code& /*error*/){
    this->close();
}

void TCP_Session::onRead(Packet &p){
    // dispatch packet!
    switch(p.type()){
        case Packet::MIN: case Packet::MAX:
            break;

        case Packet::NODEAVAIL:
            break;
        case Packet::NODEUNAVAIL:
            break;
        case Packet::DATA:
            break;
    }
}
void TCP_Session::sendFinished(Packet &/*p*/){
	//this->enqueueSyncNodeList();
}

void TCP_Session::close(){
	if(!this->socket_.is_open()){
		return;
	}

	boost::system::error_code error;
	this->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    this->socket_.close(error);

    if(this->onclose){
        this->io_service.dispatch(this->onclose);
        // clear out boost function
        this->onclose = 0;
	}
}

void TCP_Session::enqueueSyncNodeList(){
	if(!this->is_syncnode_enqueued){
		this->io_service.dispatch(boost::bind(&TCP_Session::syncNodeList, this));
		this->is_syncnode_enqueued = true;
	}
}

void TCP_Session::syncNodeList(){
	this->is_syncnode_enqueued = false;

	bool cont = false;
	do{
        std::pair<GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator, GScale::GroupCore::LocalNodeSetIdx_time_connect::iterator> range;
        if(this->nodesyncctime.is_not_a_date_time()){
            range = this->backend->gdao->rangeByCreated();
        }
        else{
            range = this->backend->gdao->rangeByCreated(this->nodesyncctime);
            // if the first range entry, is our last synced node time
            // increase the range, so the next node will get synced
            if(range.first!=range.second && range.first->time_connect==this->nodesyncctime){
                range.first++;
            }
        }

        if(range.first==range.second){
    	    // there is nothing to sync
    	    return;
        }

        cont = true;
        // if node has no disconnect time
        if(range.first->node.time_disconnect.is_not_a_date_time()){
    	    cont = false;
            // sync it
            this->syncpacket = GScale::Packet(range.first->node, INode::getNilNode());
            this->syncpacket.type(Packet::NODEAVAIL);
            this->proto->send<Packet>(this->syncpacket);
        }

        this->nodesyncctime = range.first->time_connect;
	}while(cont);

    /*


    if(this->nodesyncctime.is_not_a_date_time()){ // start sync
        this->state = STATES.SYNC;
        range = this->backend->gdao->rangeByCreated();
    }
    else{
        range = this->backend->gdao->rangeByCreated(this->nodesyncctime);
    }

    if(range.first==range.second){
        this->state = STATES.AVAIL;
        return;
    }

    for(;range.first != range.second; range.first++){
        // send node avail packet
    }
    */
}

boost::asio::ip::tcp::endpoint TCP_Session::remote_endpoint() const{
    return this->socket_.remote_endpoint();
}
boost::asio::ip::tcp::endpoint TCP_Session::local_endpoint() const{
    return this->socket_.local_endpoint();
}


}

}
