#include "enet/ENetServer.h"
#include "Common.h"
#include "time/Time.h"

const std::time_t TIMEOUT_MS = 5000;
const uint8_t RELIABLE_CHANNEL = 0;
const uint8_t UNRELIABLE_CHANNEL = 1;
const uint8_t NUM_CHANNELS = 2;

std::string addressAndPortToString(const ENetAddress* address)
{
    char ip_address_str[256];
    enet_address_get_host_ip(address, ip_address_str, sizeof(ip_address_str));
    return std::string(ip_address_str) + ":" + std::to_string(address->port);
}
std::string addressToString(const ENetAddress* address)
{
    char ip_address_str[256];
    enet_address_get_host_ip(address, ip_address_str, sizeof(ip_address_str));
    std::string ip = ip_address_str;
    if (ip.find("::ffff:") == 0) {
        ip = ip.substr(7);
    }
    return ip;
}

std::string ENetServer::getPeerIP(uint32_t id) const {
    ENetPeer* peer = getClient(id);
    if (peer) {
        return addressToString(&peer->address);
    }
    return "127.0.0.1";
}
uint16_t ENetServer::getPeerPort(uint32_t id) const {
    ENetPeer* peer = getClient(id);
    if (peer) {
        return peer->address.port;
    }
    return 0;
}
ENetServer::Shared ENetServer::alloc()
{
    return std::make_shared<ENetServer>();
}
ENetServer::ENetServer()
    : host_(nullptr)
    , currentMsgId_(0)
{
}

ENetServer::~ENetServer()
{
    stop();
}

bool ENetServer::start(uint32_t port, uint32_t maxClients)
{
    if (host_) return true;

    ENetAddress address = { 0 };
    address.host = ENET_HOST_ANY;
    address.port = (enet_uint16)port;

    host_ = enet_host_create(&address, (size_t)maxClients, NUM_CHANNELS, 0, 0);
	enet_host_compress(host_, nullptr);
    if (host_ == nullptr) {
        LOG_ERROR("Failed to create ENet server host (Port might be busy)");
        return false;
    }
    return true;
}

bool ENetServer::stop()
{
    if (!host_) return false;

    if (!isRunning()) return false;

    for (auto iter : clients_) {
        enet_peer_disconnect(iter.second, 0);
    }

    auto timestamp = Time::timestamp();
    bool success = false;
    ENetEvent event;

    while (true) {
        int32_t res = enet_host_service(host_, &event, 0);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                clients_.erase(event.peer->incomingPeerID);
            }
            else if (event.type == ENET_EVENT_TYPE_CONNECT) {
                enet_peer_disconnect(event.peer, 0);
            }
        }
        else if (res < 0) {
            break;
        }
        else {
            if (clients_.empty()) {
                success = true;
                break;
            }
        }
        if (Time::timestamp() - timestamp > Time::fromMilliseconds(TIMEOUT_MS)) break;
    }

    for (auto iter : clients_) {
        enet_peer_reset(iter.second);
    }
    clients_.clear();

    enet_host_destroy(host_);
    host_ = nullptr;
    return !success;
}

bool ENetServer::isRunning() const
{
    return host_ != nullptr;
}

uint32_t ENetServer::numClients() const
{
    if (!host_) return 0;
    return host_->connectedPeers;
}

void ENetServer::on(uint32_t id, RequestHandler handler)
{
    handlers_[id] = handler;
}

void ENetServer::handleRequest(uint32_t id, uint32_t requestId, StreamBuffer::Shared stream) const
{
    auto iter = handlers_.find(requestId);
    if (iter != handlers_.end()) {
        auto handler = iter->second;
        auto res = handler(id, stream);
        sendResponse(id, requestId, res);
    }
}

ENetPeer* ENetServer::getClient(uint32_t id) const
{
    auto iter = clients_.find(id);
    if (iter == clients_.end()) return nullptr;
    return iter->second;
}

void ENetServer::sendResponse(uint32_t id, uint32_t requestId, StreamBuffer::Shared stream) const
{
    if (!host_) return;
    auto client = getClient(id);
    if (!client) return;

    auto msg = Message::alloc(++currentMsgId_, requestId, MessageType::DATA_RESPONSE, stream);
    sendMessage(id, DeliveryType::RELIABLE, msg);
}

void ENetServer::sendMessage(uint32_t id, DeliveryType type, Message::Shared msg) const
{
    if (!host_) return;
    auto client = getClient(id);
    if (!client) return;

    uint32_t channel = (type == DeliveryType::RELIABLE) ? RELIABLE_CHANNEL : UNRELIABLE_CHANNEL;
    uint32_t flags = (type == DeliveryType::RELIABLE) ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;

    auto data = msg->serialize();
    ENetPacket* p = enet_packet_create(&data[0], data.size(), flags);

    enet_peer_send(client, channel, p);
    //enet_host_flush(host_);
}

void ENetServer::send(uint32_t id, DeliveryType type, StreamBuffer::Shared stream) const
{
    auto msg = Message::alloc(++currentMsgId_, MessageType::DATA, stream);
    sendMessage(id, type, msg);
}

void ENetServer::broadcastMessage(DeliveryType type, Message::Shared msg) const
{
    if (!host_) return;
    if (numClients() == 0) return;

    uint32_t channel = (type == DeliveryType::RELIABLE) ? RELIABLE_CHANNEL : UNRELIABLE_CHANNEL;
    uint32_t flags = (type == DeliveryType::RELIABLE) ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;

    auto data = msg->serialize();
    ENetPacket* p = enet_packet_create(&data[0], data.size(), flags);

    enet_host_broadcast(host_, channel, p);
    //enet_host_flush(host_);
}

void ENetServer::broadcast(DeliveryType type, StreamBuffer::Shared stream) const
{
    auto msg = Message::alloc(++currentMsgId_, MessageType::DATA, stream);
    broadcastMessage(type, msg);
}

std::vector<Message::Shared> ENetServer::poll()
{
    std::vector<Message::Shared> msgs;
    if (!host_) return msgs;

    ENetEvent event;
    while (true) {
        int32_t res = enet_host_service(host_, &event, 0);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                auto stream = StreamBuffer::alloc(event.packet->data, event.packet->dataLength);
                auto msg = Message::alloc(event.peer->incomingPeerID);
                msg->deserialize(stream);
                msgs.push_back(msg);

                if (msg->type() == MessageType::DATA_REQUEST) {
                    handleRequest(event.peer->incomingPeerID, msg->requestId(), msg->stream());
                }
                enet_packet_destroy(event.packet);

            }
            else if (event.type == ENET_EVENT_TYPE_CONNECT) {
                auto msg = Message::alloc(event.peer->incomingPeerID, MessageType::CONNECT);
                msgs.push_back(msg);
                clients_[event.peer->incomingPeerID] = event.peer;

            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                auto msg = Message::alloc(event.peer->incomingPeerID, MessageType::DISCONNECT);
                msgs.push_back(msg);
                clients_.erase(event.peer->incomingPeerID);
            }
        }
        else {
            break;
        }
    }
    return msgs;
}