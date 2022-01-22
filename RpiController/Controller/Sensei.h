#ifndef _SENSEI_H
#define _SENSEI_H

#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <fstream>
#include "SerialPort.h"
#include "TcpClientPort.h"
#include "Commands.h"
#include "Settings.h"

// This class manages the TCP connection to the server.  This connection
// stays open so that the server can "push" new work to the pi with minimal
// latency.  There is also a heart beat to keep the socket alive and auto-reconnect
// if the socket dies for any reason.
class Sensei
{
private:
    std::vector<Command> commandQueue;
    bool terminating = false;
    std::thread tcpThread;
    std::string hostName;
    std::string localIp;
    std::string serverIp;
    std::string response;
    int serverPort;
    std::mutex queueMutex;
    int sequence = 0;
    bool updateFirmware = false;
    bool checkFirmware = false;
public:
    Sensei(const std::string& hostname, std::string localip, std::string serverip, int serverport = 12345) :
        hostName(hostname), localIp(localip), serverIp(serverip), serverPort(serverport)
    {
    }

    ~Sensei()
    {
        Stop();
    }

    Command GetNextCommand()
    {
        Command cmd;
        std::lock_guard<std::mutex> guard(queueMutex);
        if (commandQueue.size() > 0)
        {
            cmd = commandQueue[0];
            commandQueue.erase(commandQueue.begin());
        }
        return cmd;
    }

    int Size()
    {
        std::lock_guard<std::mutex> guard(queueMutex);
        return (int)commandQueue.size();
    }

    // provide optional status response to be sent back to server, could include info about
    // health of the Teensy, etc.
    void SetResponse(const std::string& msg)
    {
        response = msg;
    }

    void Start()
    {
        Stop();
        terminating = false;
        tcpThread = std::thread(&Sensei::TcpThread, this);
    }

    void Stop()
    {
        terminating = true;
        if (tcpThread.joinable())
        {
            tcpThread.join();
        }
    }

    bool UpdateFirmware() 
    {
        return this->updateFirmware;
    }

    void TcpThread()
    {
        const int commandBufferLength = 1000000;
        std::unique_ptr<char[]> commandBuffer = std::unique_ptr<char[]>(new char[commandBufferLength]);
        TcpClientPort tcp;
        bool connected = false;

        while (!terminating)
        {
            int len = 0;
            bool socket_error = false;

            if (!connected)
            {
                try
                {
                    tcp.connect(localIp, 0, serverIp, serverPort);
                    std::cout << "connected to server\n";
                    // send initial hello, which is our hostname.
                    tcp.write((const uint8_t*)hostName.c_str(), (int)hostName.size() + 1);
                    connected = true;
                }
                catch (std::exception & e)
                {
                    socket_error = true;
                    std::cout << "connect to sensei server failed: ";
                    const char* msg = e.what();
                    if (msg != nullptr) {
                        std::cout << msg;
                    }
                    else {
                        std::cout << "unknown reason";
                    }
                    std::cout << "\n";

                    connected = false;
                }
            }

            if (connected)
            {
                try
                {
                    char* buffer = commandBuffer.get();
                    buffer[0] = '\0';
                    int pos = 0;
                    int retries = 5;
                    bool failed = false;
                    while (retries-- > 0)
                    {
                        len = tcp.read((uint8_t*)&buffer[pos], commandBufferLength - pos - 1);
                        if (len < 0)
                        {
                            // was the connection reset?
                            failed = false;
                            connected = false;
                            break;
                        }
                        else
                        {
                            pos += len;
                            buffer[pos] = '\0';
                            try {
                                if (pos > 0)
                                {
                                    ParseCommand(buffer);
                                    failed = false;
                                    break; // we're good, we have the complete message.
                                }
                            }
                            catch (std::exception & e)
                            {
                                // perhaps we need to read another packet or 2 to complete the message.
                                failed = true;
                                if (e.what() != nullptr)
                                {
                                    response = e.what();
                                }
                                else {
                                    response = "parse command failed";
                                }
                            }
                        }
                    }
                    if (failed) {
                        std::cout << "### failed to parse message: \"" << buffer << "\"\n";
                    }
                }
                catch (std::exception & e)
                {
                    socket_error = true;
                    std::cout << "read failure on socket, are we disconnected?";
                    const char* msg = e.what();
                    if (msg != nullptr) {
                        std::cout << msg;
                    }
                    else {
                        std::cout << "unknown reason";
                    }
                    std::cout << "\n";
                }
            }

            if (connected)
            {
                try
                {
                    if (response.size() == 0)
                    {
                        response = "ok";
                    }
                    if (this->checkFirmware) {
                        // trigger the server to send updated firmware!
                        response = "update";
                    }
                    
                    tcp.write((const uint8_t*)response.c_str(), (int)response.size() + 1);

                    if (this->checkFirmware) {
                        this->checkFirmware = false;
                        if (DownloadFirmware(tcp)) {
                            // terminate so the python run.py will reinstall the firmware.
                            this->updateFirmware = true;
                            break;
                        }
                    }

                    response = "";
                }
                catch (std::exception & e)
                {
                    socket_error = true;
                    std::cout << "write failure on socket, is the server gone?";
                    const char* msg = e.what();
                    if (msg != nullptr)
                    {
                        std::cout << msg;
                    }
                    else {
                        std::cout << "unknown reason";
                    }
                    std::cout << "\n";
                }
            }

            if (socket_error)
            {
                // try reconnect the socket then.
                tcp.close();
                connected = false;
                // try again in 1 second.
                std::this_thread::sleep_for(std::chrono::milliseconds((int)1000));
            }
        }
        tcp.close();
    }

    void ParseCommand(char* message)
    {
        nlohmann::json doc;
        std::stringstream ss;
        ss << message;
        ss >> doc;

        if (doc.is_array())
        {
            std::cout << message << "\n";

            // more than one command
            for (auto it = doc.begin(); it != doc.end(); ++it)
            {
                auto v = it.value();
                if (v.is_object())
                {
                    Command cmd;
                    if (cmd.ParseCommand(v))
                    {
                        std::lock_guard<std::mutex> guard(queueMutex);
                        if (cmd.sequence > 0)
                        {
                            this->sequence = cmd.sequence;
                        }
                        if (IsFirmwareCommand(cmd)) {
                            CheckFirmwareUpdate(cmd.hash);
                        } else {
                            commandQueue.push_back(cmd);
                        }
                    }
                }
            }
        }
        else if (doc.is_object())
        {
            // just one command
            Command cmd;
            if (cmd.ParseCommand(doc))
            {
                std::lock_guard<std::mutex> guard(queueMutex);
                if (cmd.sequence > 0)
                {
                    // not all commands have a sequence number
                    this->sequence = cmd.sequence;
                    std::cout << "### sequence number: " << sequence << "\n";
                }

                if (cmd.command == "ping")
                {
                    // Return our current sequence number so server can decide if we are out of date or not.
                    // This makes it possible to keep pi's in sync even after reboots.
                    response = Utils::stringf("%d", this->sequence);
                }
                else
                {
                    std::cout << message << "\n";
                    if (IsFirmwareCommand(cmd)) {
                        CheckFirmwareUpdate(cmd.hash);
                    }
                    else {
                        commandQueue.push_back(cmd);
                    }
                }
            }
        }
    }

    bool IsFirmwareCommand(Command& cmd) {
        return cmd.command == "FirmwareHash";
    }

    void CheckFirmwareUpdate(const std::string& hash) {
        Settings& settings = Settings::singleton();
        auto existing = settings.getString("FirmwareHash", "");
        if (existing != hash) {
            // need to do a firmware update!
            settings.setString("FirmwareHash", hash);
            settings.saveJSonFile();
            this->checkFirmware = true;
        }
    }

    bool DownloadFirmware(TcpClientPort& tcp) {
        const int buffer_size = 50000;
        uint8_t* buffer = new uint8_t[50000];
        if (!buffer) {
            throw std::runtime_error("out of memory");
        }

        // Read the length header
        int len = 0;
        uint8_t header[4];
        if (tcp.read(header, 4) == 4) {
            // number is in little big format so we can shift in the bytes
            for (size_t i = 0; i < 4; i++)
            {
                len = (len << 8) + header[i];
            }
        }
        else {
            return false;
        }


        Settings& settings = Settings::singleton();
        std::string hash = settings.getString("FirmwareHash", "");
        std::string filename = settings.getString("firmware","");
        if (filename.size() == 0)
        {
            std::cout << "Skipping firmware update because no --firmware filename was provided" << std::endl;

            // consume the bits though...
            int count = 0;
            while (count < len) {
                int i = tcp.read(buffer, buffer_size);
                if (i < 0) {
                    break;
                }
                count += i;
            }

            return false;
        }

        std::ofstream f(filename, std::ios::binary);

        int count = 0;
        while (count < len) {
            int i = tcp.read(buffer, buffer_size);
            if (i < 0) {
                break;
            }
            else {
                f.write((char*)buffer, i);
            }
            count += i;
        }


        // todo: check the sha256 hash.

        std::cout << "Firmware updated." << std::endl;
        delete[] buffer;
        return true;
    }
};

#endif
