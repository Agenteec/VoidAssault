#include "enet/ENetClient.h"

#include "Common.h"
#include "time/Time.h"

const uint8_t SERVER_ID = 0;
const std::time_t TIMEOUT_MS = 5000;
const std::time_t REQUEST_INTERVAL = Time::fromSeconds(1.0 / 60.0);
const uint8_t RELIABLE_CHANNEL = 0;
const uint8_t UNRELIABLE_CHANNEL = 1;
const uint8_t NUM_CHANNELS = 2;

ENetClient::Shared ENetClient::alloc()
{
    return std::make_shared<ENetClient>();
}

ENetClient::ENetClient()
    : host_(nullptr)
    , server_(nullptr)
    , currentMsgId_(0)
{
    /*host_ = enet_host_create(nullptr, 1, NUM_CHANNELS, 0, 0);
    if (!host_) {
        LOG_ERROR("Client host creation failed (ENet might not be initialized yet)");
    }*/
}

ENetClient::~ENetClient()
{
    disconnect();
    if (host_) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }
}

bool ENetClient::connect(const std::string& host, uint32_t port)
{
    if (!host_) {
        host_ = enet_host_create(nullptr, 1, NUM_CHANNELS, 0, 0);
        if (!host_) {
            LOG_ERROR("FATAL: Cannot connect. Failed to create ENet client host.");
            return false;
        }
    }

    if (isConnected()) {
        return true;
    }

    ENetAddress address = { 0 };
    enet_address_set_host(&address, host.c_str());
    address.port = (enet_uint16)port;

    server_ = enet_host_connect(host_, &address, NUM_CHANNELS, 0);
    if (server_ == nullptr) {
        LOG_ERROR("No available peers for initiating an ENet connection");
        return false;
    }

    ENetEvent event;
    // Ждем подключения (блокирующе)
    if (enet_host_service(host_, &event, TIMEOUT_MS) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        LOG_DEBUG("Connection to `" << host << ":" << port << "` succeeded");

        auto msg = Message::alloc(SERVER_ID, MessageType::CONNECT);
        queue_.push_back(msg);

        return true;
    }

    LOG_ERROR("Connection to `" << host << ":" << port << "` failed");
    enet_peer_reset(server_);
    server_ = nullptr;
    return false;
}

bool ENetClient::disconnect()
{
    if (!host_ || !server_) return false;

    if (!isConnected()) return false;

    LOG_DEBUG("Disconnecting from server...");
    enet_peer_disconnect(server_, 0);

    ENetEvent event;
    auto timestamp = Time::timestamp();
    bool success = false;

    while (true) {
        int32_t res = enet_host_service(host_, &event, 0);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                success = true;
                break;
            }
        }
        else if (res < 0) {
            break;
        }

        if (Time::timestamp() - timestamp > Time::fromMilliseconds(TIMEOUT_MS)) break;
    }

    if (!success) {
        enet_peer_reset(server_);
    }
    server_ = nullptr;
    return !success;
}

bool ENetClient::isConnected() const
{
    return host_ != nullptr && server_ != nullptr && server_->state == ENET_PEER_STATE_CONNECTED;
}

void ENetClient::on(uint32_t id, RequestHandler handler)
{
    handlers_[id] = handler;
}

void ENetClient::handleRequest(uint32_t requestId, StreamBuffer::Shared stream) const
{
    auto iter = handlers_.find(requestId);
    if (iter != handlers_.end()) {
        auto handler = iter->second;
        auto res = handler(SERVER_ID, stream);
        sendResponse(requestId, res);
    }
}

void ENetClient::sendResponse(uint32_t requestId, StreamBuffer::Shared stream) const
{
    if (!isConnected()) return;
    auto msg = Message::alloc(++currentMsgId_, requestId, MessageType::DATA_RESPONSE, stream);
    sendMessage(DeliveryType::RELIABLE, msg);
}

void ENetClient::sendMessage(DeliveryType type, Message::Shared msg) const
{
    if (!host_ || !server_) return;

    uint32_t channel = (type == DeliveryType::RELIABLE) ? RELIABLE_CHANNEL : UNRELIABLE_CHANNEL;
    uint32_t flags = (type == DeliveryType::RELIABLE) ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED;

    auto data = msg->serialize();
    ENetPacket* p = enet_packet_create(&data[0], data.size(), flags);

    enet_peer_send(server_, channel, p);
    enet_host_flush(host_);
}

void ENetClient::send(DeliveryType type, StreamBuffer::Shared stream) const
{
    if (!isConnected()) return;
    auto msg = Message::alloc(++currentMsgId_, MessageType::DATA, stream);
    sendMessage(type, msg);
}

void ENetClient::sendRequest(uint32_t requestId, StreamBuffer::Shared stream) const
{
    auto msg = Message::alloc(++currentMsgId_, requestId, MessageType::DATA_REQUEST, stream);
    sendMessage(DeliveryType::RELIABLE, msg);
}

Message::Shared ENetClient::request(uint32_t requestId, StreamBuffer::Shared stream)
{
    if (!isConnected()) return nullptr;
    sendRequest(requestId, stream);

    auto timestamp = Time::timestamp();
    while (true) {
        auto msgs = poll();
        for (auto msg : msgs) {
            if (msg->type() == MessageType::DATA_RESPONSE && msg->requestId() == requestId) {
                return msg;
            }
            queue_.push_back(msg);
        }
        if (Time::timestamp() - timestamp > Time::fromMilliseconds(TIMEOUT_MS)) break;
        Time::sleep(REQUEST_INTERVAL);
    }
    return nullptr;
}

std::vector<Message::Shared> ENetClient::poll()
{
    std::vector<Message::Shared> msgs;

    if (!host_) return msgs;

    ENetEvent event;
    while (true) {
        int32_t res = enet_host_service(host_, &event, 0);
        if (res > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                auto stream = StreamBuffer::alloc(event.packet->data, event.packet->dataLength);
                auto msg = Message::alloc(SERVER_ID);
                msg->deserialize(stream);
                msgs.push_back(msg);

                if (msg->type() == MessageType::DATA_REQUEST) {
                    handleRequest(msg->requestId(), msg->stream());
                }
                enet_packet_destroy(event.packet);

            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                auto msg = Message::alloc(SERVER_ID, MessageType::DISCONNECT);
                msgs.push_back(msg);
                server_ = nullptr;
            }
        }
        else {
            break;
        }
    }
    if (!queue_.empty()) {
        msgs.insert(msgs.end(), queue_.begin(), queue_.end());
        queue_.clear();
    }
    return msgs;
}