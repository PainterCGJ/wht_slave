#pragma once

#include <memory>

#include "../../protocol/messages/Master2Slave.h"
#include "../../protocol/messages/Slave2Master.h"

using namespace WhtsProtocol;

namespace SlaveApp
{

// Forward declarations
class SlaveDevice;

// Message handler interface for Master2Slave messages
class IMaster2SlaveMessageHandler
{
  public:
    virtual ~IMaster2SlaveMessageHandler() = default;
    virtual std::unique_ptr<Message> ProcessMessage(const Message &message, SlaveDevice *device) = 0;
};

// Sync Message Handler
class SyncMessageHandler final : public IMaster2SlaveMessageHandler
{
  public:
    static SyncMessageHandler &GetInstance()
    {
        static SyncMessageHandler instance;
        return instance;
    }
    std::unique_ptr<Message> ProcessMessage(const Message &message, SlaveDevice *device) override;
    SyncMessageHandler &operator=(const SyncMessageHandler &) = delete;
    SyncMessageHandler(const SyncMessageHandler &) = delete;

  private:
    SyncMessageHandler() = default;
};

// Ping Request Message Handler
class PingRequestHandler final : public IMaster2SlaveMessageHandler
{
  public:
    static PingRequestHandler &GetInstance()
    {
        static PingRequestHandler instance;
        return instance;
    }
    std::unique_ptr<Message> ProcessMessage(const Message &message, SlaveDevice *device) override;
    PingRequestHandler(const PingRequestHandler &) = delete;
    PingRequestHandler &operator=(const PingRequestHandler &) = delete;

  private:
    PingRequestHandler() = default;
};

// Short ID Assignment Message Handler
class ShortIdAssignHandler final : public IMaster2SlaveMessageHandler
{
  public:
    static ShortIdAssignHandler &GetInstance()
    {
        static ShortIdAssignHandler instance;
        return instance;
    }
    std::unique_ptr<Message> ProcessMessage(const Message &message, SlaveDevice *device) override;
    ShortIdAssignHandler(const ShortIdAssignHandler &) = delete;
    ShortIdAssignHandler &operator=(const ShortIdAssignHandler &) = delete;

  private:
    ShortIdAssignHandler() = default;
};

// Secondary Control Message Handler

} // namespace SlaveApp