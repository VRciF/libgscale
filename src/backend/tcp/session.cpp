/*
 * session.cpp
 *
 */

#include "session.hpp"

#include <boost/bind.hpp>

#include "group.hpp"
#include "tcp.hpp"

namespace GScale{

namespace Backend{

TCP_Session::TCP_Session(TCP *backend, boost::asio::io_service& io_service) : io_service(io_service),socket_(io_service)
{
    this->backend = backend;
    this->proto.reset(new TCP_Protocol<TCP_Session>(this->socket(), *this));
}

boost::asio::ip::tcp::socket& TCP_Session::socket()
{
    return this->socket_;
}

void TCP_Session::start(){
    this->io_service.dispatch(boost::bind(&TCP_Session::syncNodeList, this));
}

void TCP_Session::onReadError(const boost::system::error_code& error){
    this->close();
}
void TCP_Session::onSendError(GScale::Packet &p, const boost::system::error_code& error){
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
void TCP_Session::sendFinished(Packet &p){
    this->io_service.dispatch(boost::bind(&TCP_Session::syncNodeList, this));
}

void TCP_Session::close(){
    this->socket_.close();
    if(this->onclose){
        this->io_service.dispatch(this->onclose);
    }
}



void TCP_Session::syncNodeList(){
    std::pair<GScale::Group::LocalNodesSetIdx_creationtime::iterator, GScale::Group::LocalNodesSetIdx_creationtime::iterator> range;
    if(this->nodesyncctime.is_not_a_date_time()){
        range = this->backend->gdao->rangeByCreated();
    }
    else{
        range = this->backend->gdao->rangeByCreated(this->nodesyncctime);
        if(range.first!=range.second && range.first->created()==this->nodesyncctime){
            range.first++;
        }
    }

    if(range.first==range.second){ return; }

    this->syncpacket = GScale::Packet(*range.first, INode::getNilNode());
    this->syncpacket.type(Packet::NODEAVAIL);
    this->proto->send<Packet>(this->syncpacket);
    this->nodesyncctime = range.first->created();

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
