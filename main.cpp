#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <cstdint>
#include <vector>
#include <cstring>
#pragma comment(lib, "ws2_32.lib")

// ISerialPort interface: allows injection of a real or mock serial port.
class ISerialPort {
public:
    virtual ~ISerialPort() {}
    virtual void write(const char* command) = 0;
};

// ProductionSerialPort implementation.
// Replace the write() body with your actual serial communication logic.
class ProductionSerialPort : public ISerialPort {
public:
    ProductionSerialPort() {
        // Open and configure your serial port here.
    }
    virtual ~ProductionSerialPort() {
        // Close your serial port here.
    }
    virtual void write(const char* command) override {
        // In production, write the command to the serial port.
        // For demonstration, we print it to stdout.
        std::cout << "Serial Output: " << command;
    }
};

// Global pointer for dependency injection.
ISerialPort* g_serialPort_ptr = nullptr;

// Command enumeration
enum Command : uint32_t {
    CMD_CONNECT      = 0x28283CAF,
    CMD_MOUSE_MOVE   = 0xAEDE7345,
    CMD_AUTO_MOVE    = 0xAEDE7346,
    CMD_MOUSE_LEFT   = 0x9823AE8D,
    CMD_MOUSE_MIDDLE = 0x97A3AE8D,
    CMD_MOUSE_RIGHT  = 0x238D8212,
    CMD_MOUSE_WHEEL  = 0xFFEEAD38,
    CMD_BEZIER_MOVE  = 0x5A4538A2,
    CMD_KEYBOARD_ALL = 0xABCD1234, // Example value for keyboard events.
    CMD_REBOOT       = 0xAA8855AA
};

#pragma pack(push, 1)
struct cmd_head_t {
    uint32_t mac;
    uint32_t rand;
    uint32_t indexpts;
    uint32_t cmd;
};

struct mouse_move_t {
    int32_t x;
    int32_t y;
};

struct auto_move_t {
    int32_t x;
    int32_t y;
    uint32_t duration;
};

struct mouse_button_t {
    uint8_t state;
};

struct mouse_wheel_t {
    int32_t value;
};

struct bezier_move_t {
    int32_t targetX;
    int32_t targetY;
    uint32_t duration;
    int32_t ctrlX1;
    int32_t ctrlY1;
    int32_t ctrlX2;
    int32_t ctrlY2;
};

struct keyboard_all_t {
    uint8_t ctrl;
    uint8_t buttons[10];
};
#pragma pack(pop)

// processPacket parses the UDP packet and sends the corresponding B+ command
// through the injected serial port.
void processPacket(const uint8_t* data, int size, const sockaddr_in& remoteAddr) {
    if (remoteAddr.sin_family != AF_INET) {
        std::cerr << "ERROR: Invalid remote address family" << std::endl;
        return;
    }
    if (size < static_cast<int>(sizeof(cmd_head_t))) {
        std::cerr << "ERROR: Packet too small (" << size << " bytes)" << std::endl;
        return;
    }
    
    const cmd_head_t* header = reinterpret_cast<const cmd_head_t*>(data);
    uint32_t cmd = ntohl(header->cmd);
    uint32_t clientMac = ntohl(header->mac);
    
    std::cout << "Received CMD: 0x" << std::hex << cmd 
              << " from MAC: 0x" << clientMac << std::dec << std::endl;
    
    std::string serialCommand;
    switch (cmd) {
        case CMD_MOUSE_MOVE: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(mouse_move_t))) {
                std::cerr << "ERROR: Packet too small for MOUSE_MOVE" << std::endl;
                break;
            }
            const mouse_move_t* move = reinterpret_cast<const mouse_move_t*>(data + sizeof(cmd_head_t));
            int32_t x = ntohl(move->x);
            int32_t y = ntohl(move->y);
            serialCommand = "MOUSE_MOVE " + std::to_string(x) + " " + std::to_string(y) + "\r\n";
            break;
        }
        case CMD_AUTO_MOVE: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(auto_move_t))) {
                std::cerr << "ERROR: Packet too small for AUTO_MOVE" << std::endl;
                break;
            }
            const auto_move_t* autoMoveCmd = reinterpret_cast<const auto_move_t*>(data + sizeof(cmd_head_t));
            int32_t x = ntohl(autoMoveCmd->x);
            int32_t y = ntohl(autoMoveCmd->y);
            uint32_t duration = ntohl(autoMoveCmd->duration);
            serialCommand = "AUTO_MOVE " + std::to_string(x) + " " +
                            std::to_string(y) + " " + std::to_string(duration) + "\r\n";
            break;
        }
        case CMD_MOUSE_LEFT: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(mouse_button_t))) {
                std::cerr << "ERROR: Packet too small for MOUSE_LEFT" << std::endl;
                break;
            }
            const mouse_button_t* btn = reinterpret_cast<const mouse_button_t*>(data + sizeof(cmd_head_t));
            serialCommand = "MOUSE_LEFT " + std::to_string(btn->state) + "\r\n";
            break;
        }
        case CMD_MOUSE_MIDDLE: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(mouse_button_t))) {
                std::cerr << "ERROR: Packet too small for MOUSE_MIDDLE" << std::endl;
                break;
            }
            const mouse_button_t* btn = reinterpret_cast<const mouse_button_t*>(data + sizeof(cmd_head_t));
            serialCommand = "MOUSE_MIDDLE " + std::to_string(btn->state) + "\r\n";
            break;
        }
        case CMD_MOUSE_RIGHT: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(mouse_button_t))) {
                std::cerr << "ERROR: Packet too small for MOUSE_RIGHT" << std::endl;
                break;
            }
            const mouse_button_t* btn = reinterpret_cast<const mouse_button_t*>(data + sizeof(cmd_head_t));
            serialCommand = "MOUSE_RIGHT " + std::to_string(btn->state) + "\r\n";
            break;
        }
        case CMD_MOUSE_WHEEL: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(mouse_wheel_t))) {
                std::cerr << "ERROR: Packet too small for MOUSE_WHEEL" << std::endl;
                break;
            }
            const mouse_wheel_t* wheelCmd = reinterpret_cast<const mouse_wheel_t*>(data + sizeof(cmd_head_t));
            int32_t value = ntohl(wheelCmd->value);
            serialCommand = "MOUSE_WHEEL " + std::to_string(value) + "\r\n";
            break;
        }
        case CMD_BEZIER_MOVE: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(bezier_move_t))) {
                std::cerr << "ERROR: Packet too small for BEZIER_MOVE" << std::endl;
                break;
            }
            const bezier_move_t* bezier = reinterpret_cast<const bezier_move_t*>(data + sizeof(cmd_head_t));
            int32_t targetX = ntohl(bezier->targetX);
            int32_t targetY = ntohl(bezier->targetY);
            uint32_t duration = ntohl(bezier->duration);
            serialCommand = "BEZIER_MOVE " + std::to_string(targetX) + " " +
                            std::to_string(targetY) + " " + std::to_string(duration) + "\r\n";
            break;
        }
        case CMD_KEYBOARD_ALL: {
            if (size < static_cast<int>(sizeof(cmd_head_t) + sizeof(keyboard_all_t))) {
                std::cerr << "ERROR: Packet too small for KEYBOARD_ALL" << std::endl;
                break;
            }
            const keyboard_all_t* kb = reinterpret_cast<const keyboard_all_t*>(data + sizeof(cmd_head_t));
            serialCommand = "KEYBOARD_ALL CTRL:0x" + std::to_string(kb->ctrl) + " BUTTONS:";
            for (int i = 0; i < 10; i++) {
                serialCommand += "0x" + std::to_string(kb->buttons[i]) + " ";
            }
            serialCommand += "\r\n";
            break;
        }
        case CMD_REBOOT: {
            serialCommand = "REBOOT\r\n";
            break;
        }
        default:
            std::cerr << "Unknown command: 0x" << std::hex << cmd << std::dec << std::endl;
            return;
    }
    
    if (g_serialPort_ptr) {
        g_serialPort_ptr->write(serialCommand.c_str());
    }
}

int main() {
    // Instantiate and inject the production serial port.
    ProductionSerialPort prodSerial;
    g_serialPort_ptr = &prodSerial;
    
    // Initialize Winsock.
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }
    
    // Create a UDP socket.
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Error creating UDP socket!" << std::endl;
        WSACleanup();
        return 1;
    }
    
    // Bind the socket to localhost on port 12345.
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(12345);
    if (bind(udpSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Listening for UDP packets on 127.0.0.1:12345..." << std::endl;
    
    // Main loop: Receive UDP packets and process them.
    char buffer[1024];
    sockaddr_in remoteAddr = {};
    int remoteAddrSize = sizeof(remoteAddr);
    bool running = true;
    while (running) {
        int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                      reinterpret_cast<sockaddr*>(&remoteAddr), &remoteAddrSize);
        if (bytesReceived > 0) {
            processPacket(reinterpret_cast<const uint8_t*>(buffer), bytesReceived, remoteAddr);
        }
        // You can add termination logic here.
    }
    
    closesocket(udpSocket);
    WSACleanup();
    return 0;
}
