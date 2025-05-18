#include "../../include/network/NetworkManager.hpp"
#include "../../include/threading/ThreadPool.hpp"
#include "../../include/core/DataPacket.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define SOCKET_ERROR_VAL INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <netdb.h>
    typedef int socket_t;
    #define SOCKET_ERROR_VAL (-1)
    #define CLOSE_SOCKET close
#endif

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <cstring>
#include <algorithm>

namespace dataflow {
namespace network {

// Implementation class for platform-specific socket details
class NetworkManager::SocketImpl {
public:
    SocketImpl() : serverSocket(SOCKET_ERROR_VAL), initialized(false) {
        // Initialize socket library if necessary
        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
        initialized = true;
        #endif
    }

    ~SocketImpl() {
        // Clean up any open sockets
        for (const auto& [id, socket] : clientSockets) {
            CLOSE_SOCKET(socket);
        }
        
        if (serverSocket != SOCKET_ERROR_VAL) {
            CLOSE_SOCKET(serverSocket);
        }
        
        // Clean up socket library if necessary
        #ifdef _WIN32
        if (initialized) {
            WSACleanup();
        }
        #endif
    }

    socket_t serverSocket;
    std::map<std::string, socket_t> clientSockets;
    std::mutex clientsMutex;
    bool initialized;
};

NetworkManager::NetworkManager(const std::string& host, int port, 
                             std::shared_ptr<threading::ThreadPool> threadPool)
    : host_(host), port_(port), threadPool_(threadPool), impl_(new SocketImpl()) {
    
    if (!threadPool_) {
        throw std::invalid_argument("ThreadPool cannot be null");
    }
}

NetworkManager::~NetworkManager() {
    stopServer();
}

bool NetworkManager::startServer() {
    if (running_) {
        return false; // Already running
    }
    
    try {
        // Create socket
        impl_->serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (impl_->serverSocket == SOCKET_ERROR_VAL) {
            throw std::runtime_error("Failed to create socket");
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(impl_->serverSocket, SOL_SOCKET, SO_REUSEADDR, 
                     reinterpret_cast<char*>(&opt), sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }
        
        // Bind socket
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        
        if (host_ == "localhost" || host_ == "127.0.0.1") {
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else if (host_ == "0.0.0.0") {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            // Convert string IP to binary
            if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
                throw std::runtime_error("Invalid address: " + host_);
            }
        }
        
        if (bind(impl_->serverSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }
        
        // Listen for connections
        if (listen(impl_->serverSocket, 10) < 0) {
            throw std::runtime_error("Listen failed");
        }
        
        // Start the acceptor thread
        running_ = true;
        acceptorThread_ = std::thread(&NetworkManager::acceptorLoop, this);
        
        std::cout << "Server started on " << host_ << ":" << port_ << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error starting server: " << e.what() << std::endl;
        
        if (impl_->serverSocket != SOCKET_ERROR_VAL) {
            CLOSE_SOCKET(impl_->serverSocket);
            impl_->serverSocket = SOCKET_ERROR_VAL;
        }
        
        running_ = false;
        return false;
    }
}

void NetworkManager::stopServer() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Close the server socket to wake up the acceptor thread
    if (impl_->serverSocket != SOCKET_ERROR_VAL) {
        CLOSE_SOCKET(impl_->serverSocket);
        impl_->serverSocket = SOCKET_ERROR_VAL;
    }
    
    // Wait for the acceptor thread to finish
    if (acceptorThread_.joinable()) {
        acceptorThread_.join();
    }
    
    // Close all client sockets
    std::lock_guard<std::mutex> lock(impl_->clientsMutex);
    for (const auto& [id, socket] : impl_->clientSockets) {
        CLOSE_SOCKET(socket);
    }
    impl_->clientSockets.clear();
    
    std::cout << "Server stopped" << std::endl;
}

void NetworkManager::setDataReceivedCallback(
    std::function<void(std::shared_ptr<core::DataPacket>)> callback) {
    dataCallback_ = std::move(callback);
}

bool NetworkManager::sendData(const std::string& clientId, 
                           std::shared_ptr<core::DataPacket> packet) {
    if (!packet) {
        return false;
    }
    
    socket_t clientSocket = SOCKET_ERROR_VAL;
    
    {
        std::lock_guard<std::mutex> lock(impl_->clientsMutex);
        auto it = impl_->clientSockets.find(clientId);
        if (it == impl_->clientSockets.end()) {
            return false; // Client not found
        }
        clientSocket = it->second;
    }
    
    try {
        // Serialize the packet
        auto data = serializePacket(packet);
        
        // Send data size first (as a 4-byte integer)
        uint32_t size = static_cast<uint32_t>(data.size());
        size = htonl(size); // Convert to network byte order
        
        if (send(clientSocket, reinterpret_cast<char*>(&size), sizeof(size), 0) < 0) {
            throw std::runtime_error("Failed to send packet size");
        }
        
        // Send the actual data
        if (send(clientSocket, reinterpret_cast<char*>(data.data()), data.size(), 0) < 0) {
            throw std::runtime_error("Failed to send packet data");
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending data to client " << clientId << ": " << e.what() << std::endl;
        return false;
    }
}

size_t NetworkManager::broadcastData(std::shared_ptr<core::DataPacket> packet) {
    if (!packet) {
        return 0;
    }
    
    std::vector<std::string> clientIds;
    
    {
        std::lock_guard<std::mutex> lock(impl_->clientsMutex);
        clientIds.reserve(impl_->clientSockets.size());
        for (const auto& [id, _] : impl_->clientSockets) {
            clientIds.push_back(id);
        }
    }
    
    size_t successCount = 0;
    for (const auto& id : clientIds) {
        if (sendData(id, packet)) {
            ++successCount;
        }
    }
    
    return successCount;
}

size_t NetworkManager::getClientCount() const {
    std::lock_guard<std::mutex> lock(impl_->clientsMutex);
    return impl_->clientSockets.size();
}

void NetworkManager::acceptorLoop() {
    while (running_) {
        try {
            // Accept a new connection
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            
            socket_t clientSocket = accept(impl_->serverSocket, 
                                        reinterpret_cast<struct sockaddr*>(&clientAddr), 
                                        &clientAddrLen);
            
            if (clientSocket == SOCKET_ERROR_VAL) {
                if (running_) {
                    std::cerr << "Error accepting connection" << std::endl;
                }
                break;
            }
            
            // Get client info
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            
            std::string clientId = std::string(clientIP) + ":" + std::to_string(clientPort);
            
            std::cout << "New connection from " << clientId << std::endl;
            
            // Store the client socket
            {
                std::lock_guard<std::mutex> lock(impl_->clientsMutex);
                impl_->clientSockets[clientId] = clientSocket;
            }
            
            // Handle the client in a separate thread
            threadPool_->enqueue([this, clientSocket, clientId]() {
                handleClient(clientSocket, clientId);
            });
        }
        catch (const std::exception& e) {
            std::cerr << "Error in acceptor loop: " << e.what() << std::endl;
            if (!running_) break;
            
            // Sleep briefly to avoid hammering the CPU if there's a persistent error
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NetworkManager::handleClient(int clientSocket, const std::string& clientId) {
    try {
        const size_t bufferSize = 4096;
        std::vector<uint8_t> buffer(bufferSize);
        std::vector<uint8_t> messageBuffer;
        
        uint32_t expectedSize = 0;
        bool readingSize = true;
        size_t bytesRead = 0;
        
        while (running_) {
            // Receive data
            int result = recv(clientSocket, reinterpret_cast<char*>(buffer.data()), bufferSize, 0);
            
            if (result <= 0) {
                // Connection closed or error
                break;
            }
            
            bytesRead = static_cast<size_t>(result);
            
            // Process received data
            size_t processedBytes = 0;
            while (processedBytes < bytesRead) {
                if (readingSize) {
                    // Reading the message size (4 bytes)
                    size_t remainingForSize = sizeof(uint32_t) - messageBuffer.size();
                    size_t bytesToCopy = std::min(remainingForSize, bytesRead - processedBytes);
                    
                    messageBuffer.insert(messageBuffer.end(), 
                                      buffer.begin() + processedBytes,
                                      buffer.begin() + processedBytes + bytesToCopy);
                    
                    processedBytes += bytesToCopy;
                    
                    if (messageBuffer.size() == sizeof(uint32_t)) {
                        // We have the complete size, convert from network byte order
                        std::memcpy(&expectedSize, messageBuffer.data(), sizeof(uint32_t));
                        expectedSize = ntohl(expectedSize);
                        
                        // Reset buffer for message data
                        messageBuffer.clear();
                        readingSize = false;
                    }
                } else {
                    // Reading the message data
                    size_t remainingForMessage = expectedSize - messageBuffer.size();
                    size_t bytesToCopy = std::min(remainingForMessage, bytesRead - processedBytes);
                    
                    messageBuffer.insert(messageBuffer.end(), 
                                      buffer.begin() + processedBytes,
                                      buffer.begin() + processedBytes + bytesToCopy);
                    
                    processedBytes += bytesToCopy;
                    
                    if (messageBuffer.size() == expectedSize) {
                        // We have the complete message
                        auto packet = parseIncomingData(messageBuffer);
                        
                        // Call the callback if set
                        if (packet && dataCallback_) {
                            dataCallback_(packet);
                        }
                        
                        // Reset for next message
                        messageBuffer.clear();
                        readingSize = true;
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client " << clientId << ": " << e.what() << std::endl;
    }
    
    // Clean up
    {
        std::lock_guard<std::mutex> lock(impl_->clientsMutex);
        
        // Close the socket
        CLOSE_SOCKET(clientSocket);
        
        // Remove from client list
        impl_->clientSockets.erase(clientId);
    }
    
    std::cout << "Client " << clientId << " disconnected" << std::endl;
}

std::shared_ptr<core::DataPacket> NetworkManager::parseIncomingData(const std::vector<uint8_t>& data) {
    // In a real implementation, this would deserialize the binary data into a DataPacket
    // using a proper serialization format (e.g., Protocol Buffers, FlatBuffers, etc.)
    
    // This is a simplified placeholder
    auto packet = std::make_shared<core::DataPacket>();
    
    // For demonstration purposes, assume the data is a simple string message
    if (data.size() > 0) {
        std::string message(reinterpret_cast<const char*>(data.data()), data.size());
        packet->setData("message", message);
        packet->setData("received_at", std::chrono::system_clock::now());
    }
    
    return packet;
}

std::vector<uint8_t> NetworkManager::serializePacket(std::shared_ptr<core::DataPacket> packet) {
    // In a real implementation, this would serialize the DataPacket into a binary format
    // using a proper serialization library
    
    // This is a simplified placeholder
    std::vector<uint8_t> result;
    
    // For demonstration purposes, just convert to a simple string representation
    auto message = packet->getData<std::string>("message");
    if (message) {
        const std::string& str = *message;
        result.resize(str.size());
        std::memcpy(result.data(), str.data(), str.size());
    } else {
        // Default message if none provided
        std::string defaultMsg = "DataPacket " + packet->getId();
        result.resize(defaultMsg.size());
        std::memcpy(result.data(), defaultMsg.data(), defaultMsg.size());
    }
    
    return result;
}

} // namespace network
} // namespace dataflow
