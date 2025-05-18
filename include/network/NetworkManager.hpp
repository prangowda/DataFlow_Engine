#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <functional>

// Forward declarations
namespace dataflow {
namespace threading {
    class ThreadPool;
}
namespace core {
    class DataPacket;
}
}

namespace dataflow {
namespace network {

/**
 * @class NetworkManager
 * @brief Manages network communications for the dataflow system
 * 
 * Handles asynchronous socket communications to receive and send data
 * over a network. Implements a non-blocking I/O model and supports
 * multiple simultaneous connections.
 */
class NetworkManager {
public:
    /**
     * @brief Constructor
     * @param host The hostname or IP address to bind to
     * @param port The port number to use
     * @param threadPool Thread pool for asynchronous operations
     */
    NetworkManager(const std::string& host, int port, 
                  std::shared_ptr<threading::ThreadPool> threadPool);
    
    /**
     * @brief Destructor
     */
    ~NetworkManager();
    
    /**
     * @brief Start the network server
     * @return true if started successfully, false otherwise
     */
    bool startServer();
    
    /**
     * @brief Stop the network server
     */
    void stopServer();
    
    /**
     * @brief Set the callback for when data is received
     * @param callback Function to call when data is received
     */
    void setDataReceivedCallback(std::function<void(std::shared_ptr<core::DataPacket>)> callback);
    
    /**
     * @brief Send data packet to a remote client
     * @param clientId ID of the client to send to
     * @param packet Data packet to send
     * @return true if sent successfully, false otherwise
     */
    bool sendData(const std::string& clientId, std::shared_ptr<core::DataPacket> packet);
    
    /**
     * @brief Broadcast data packet to all connected clients
     * @param packet Data packet to broadcast
     * @return Number of clients the packet was sent to
     */
    size_t broadcastData(std::shared_ptr<core::DataPacket> packet);
    
    /**
     * @brief Check if the server is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Get the number of currently connected clients
     * @return The client count
     */
    size_t getClientCount() const;

private:
    std::string host_;
    int port_;
    std::shared_ptr<threading::ThreadPool> threadPool_;
    std::function<void(std::shared_ptr<core::DataPacket>)> dataCallback_;
    
    // Implementation details would depend on the socket library used
    // We'll use platform-specific code or a cross-platform library
    class SocketImpl;
    std::unique_ptr<SocketImpl> impl_;
    
    std::atomic<bool> running_{false};
    std::thread acceptorThread_;
    
    // Methods for internal implementation
    void acceptorLoop();
    void handleClient(int clientSocket, const std::string& clientId);
    std::shared_ptr<core::DataPacket> parseIncomingData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serializePacket(std::shared_ptr<core::DataPacket> packet);
};

} // namespace network
} // namespace dataflow
