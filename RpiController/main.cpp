// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdarg.h>
#include <chrono>
#include <thread>

#include "Utils.h"
#include "Timer.h"
#include "Commands.h"
#include "Controller.h"
#include "Settings.h"
#include "FileSystem.h"
#include "Sensei.h"
#include "SerialPort.h"
#include "TcpClientPort.h"

const int TeensyVid = 0x16C0;
const int TeensyPid = 0x0483;
const int tcpPort = 21567;

const uint32_t LEDSPerStrip = 392;
const uint32_t NUM_LEDStrips = 16;


void PrintUsage()
{
    std::cout << "Usage: RpiController [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --autostart             Do not provide interactive command prompt\n";
    std::cout << "  --tcp                   Connect to Teensy using TCP (works with TeensyUnitTest --client)\n";
    std::cout << "  --serial [name]         Connect to Teensu over serial port (provide optional port name)\n";
    std::cout << "  --noserver              Do not attempt to connect to Ada Server\n";
    std::cout << "  --firmware              Where to store new Firmware.hex file downloaded from Ada Server\n";
}

// Note: you can create a json file in this location "~/Ada/settings.json" containing the following
// information about which network ip addresses to use to connect to the sensei server, default will be "localhost"
// {
//     "localip": 192.168.1.7,
//     "serverip": 192.168.1.12,
// }

void PrintHelp()
{
    std::cout << "Enter one of the following commands:\n";
    std::cout << "  q                       exit this program:\n";
    std::cout << "  ?                       query status of Teensy:\n";
    std::cout << "  s                       get Sensei data over local and server ip addresses\n";
    std::cout << "  b s f1 f2               subtle breath animation with seconds and two f-ratios\n";
    std::cout << "  t                       run the speed test which measures bytes per second\n";
    std::cout << "  r l s                   rainbow animation of given length and seconds\n";
    std::cout << "  n i                     neural drop pattern repeated i times\n";
    std::cout << "  c R G B i j             set a color, with optional strip and led indexes (defaults to all leds).\n";
    std::cout << "  f R G B s               smooth fade to new color over given seconds.\n";
    std::cout << "  g i n s { R G B }*      set a smooth gradient from top to bottom of given strip (-1 for all strips),\n" <<
                 "                          with 'n' colors per strip (0 = all strips the same colors) animated over s seconds.\n";
    std::cout << "  ga s { R G B }*         test setting animated color for each strip one by one in rapid increments.\n";
    std::cout << "  m s d s { R G B }*      moving gradient, speed, direction, size, colors.\n";
    std::cout << "  z R G B R G B s d       setup the twinkle pattern with base color rgb, star color rgb, speed, and density.\n";
    std::cout << "  w s d a                 water droplet over existing color, s=size, d=drops, a=amount of color to add for droplet.\n";
    std::cout << "  p { s l R G B }*        set a series of individual pixels to each color, s=strip, l=led.\n";
    std::cout << "  rain [on|off] s a       start or stop rain animation overlay with given size and color amount\n";
    std::cout << "  fire c s s              fire animation with cooling, sparkle and seconds\n";
    std::cout << "  0                       run serial speed test.\n";
}

bool findServer(Settings& settings, std::string& name, int port)
{
    try
    {
        auto serverip = TcpClientPort::getHostByName(name);
        if (serverip == "")
        {
            std::cout << "server not found: " << name;
            return false;
        }
        TcpClientPort tcp;
        tcp.connect(serverip, port);
        settings.setString("server-ip", tcp.remoteAddress());
        settings.setString("local-ip", tcp.localAddress());
        tcp.close();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "error connecting to server: " << name;
        const char* msg = e.what();
        if (msg != nullptr) {
            std::cout << msg;
        }
        else {
            std::cout << "unknown reason";
        }
    }
    return false;
}

void RunCommandLine(Controller& controller) {

    std::string command;

    while (true) {

        std::cout << "> ";
        std::getline(std::cin, command);

        auto parts = Utils::split(command, " ", 1);
        int size = (int)parts.size();
        if (size > 0)
        {
            command = parts[0];
        }

        if (command == "q")
        {
            exit(1);
        }
        else if (command == "?")
        {
            controller.QueryStatus();
        }
        else if (command == "t")
        {
            controller.StartSpeedTest();
        }
        else if (command == "s")
        {
            controller.StartSensei();
        }
        else if (command == "0")
        {
            float msdelay = 0;
            if (size > 1) {
                msdelay = (float)atof(parts[1].c_str());
            }
            controller.StartSerialTest(msdelay);
        }
        else if (command == "b")
        {
            float seconds = 0;
            if (size > 1) {
                seconds = (float)atof(parts[1].c_str());
            }
            float f1 = 7;
            if (size > 2) {
                f1 = (float)atof(parts[2].c_str());
            }
            float f2 = 0.4f;
            if (size > 3) {
                f2 = (float)atof(parts[3].c_str());
            }
            controller.StartBreathe(seconds, f1, f2);
        }
        else if (command == "n")
        {
            int drops = 0;
            if (size > 1) {
                drops = atoi(parts[1].c_str());
            }
            controller.StartNeuralDrop(drops);
        }
        else if (command == "r")
        {
            int length = 157;
            if (size > 1) {
                length = atoi(parts[1].c_str());
            }
            float seconds = 0;
            if (size > 2) {
                seconds = (float)atof(parts[2].c_str());
            }
            controller.StartRainbow(length, seconds);
        }
        else if (command == "fire")
        {
            float cooling = 50;
            if (size > 1) {
                cooling = (float)atof(parts[1].c_str());
            }
            float sparkle = 120;
            if (size > 1) {
                sparkle = (float)atof(parts[1].c_str());
            }
            float seconds = 0;
            if (size > 3) {
                seconds = (float)atof(parts[3].c_str());
            }
            controller.StartFire(cooling, sparkle, seconds);
        }
        else if (command == "w")
        {
            int length = 16;
            if (size > 1) {
                length = atoi(parts[1].c_str());
            }
            int drops = 1;
            if (size > 2) {
                drops = atoi(parts[2].c_str());
            }
            float amount = 20;
            if (size > 3) {
                amount = (float)atof(parts[3].c_str());
            }
            controller.StartWaterDrop(length, drops, amount);
        }
        else if (command == "rain")
        {
            bool on = true;
            if (size > 1) {
                if (parts[1] == "off") {
                    on = false;
                }
            }
            int length = 16;
            if (size > 2) {
                length = atoi(parts[2].c_str());
            }
            float amount = 20;
            if (size > 3) {
                amount = (float)atof(parts[3].c_str());
            }
            if (on) {
                controller.StartRain(length, amount);
            }
            else {
                controller.StopRain();
            }
        }
        else if (command == "c")
        {
            uint8_t r = 0, g = 0, b = 0;
            int strip = -1, index = -1;
            if (size > 1) {
                r = (uint8_t)atoi(parts[1].c_str());
            }
            if (size > 2)
            {
                g = (uint8_t)atoi(parts[2].c_str());
            }
            if (size > 3)
            {
                b = (uint8_t)atoi(parts[3].c_str());
            }
            if (size > 4)
            {
                strip = (int)atoi(parts[4].c_str());
            }
            if (size > 5)
            {
                index = (int)atoi(parts[5].c_str());
            }
            controller.StartSetColor(Color{ r, g, b }, strip, index, 0);
        }
        else if (command == "f")
        {
            uint8_t r = 0, g = 0, b = 0;
            float seconds = 3;
            if (size > 1) {
                r = (uint8_t)atoi(parts[1].c_str());
            }
            if (size > 2)
            {
                g = (uint8_t)atoi(parts[2].c_str());
            }
            if (size > 3)
            {
                b = (uint8_t)atoi(parts[3].c_str());
            }
            if (size > 4)
            {
                seconds = (float)atof(parts[4].c_str());
            }
            controller.StartCrossFade(Color{ r, g, b }, seconds);
        }
        else if (command == "z")
        {
            uint8_t r1 = 0, g1 = 0, b1 = 0;
            uint8_t r2 = 0, g2 = 0, b2 = 0;
            float seconds = 0;
            int density = 1;
            if (size > 1) {
                r1 = (uint8_t)atoi(parts[1].c_str());
            }
            if (size > 2)
            {
                g1 = (uint8_t)atoi(parts[2].c_str());
            }
            if (size > 3)
            {
                b1 = (uint8_t)atoi(parts[3].c_str());
            }
            if (size > 4) {
                r2 = (uint8_t)atoi(parts[4].c_str());
            }
            if (size > 5)
            {
                g2 = (uint8_t)atoi(parts[5].c_str());
            }
            if (size > 6)
            {
                b2 = (uint8_t)atoi(parts[6].c_str());
            }
            if (size > 7)
            {
                seconds = (float)atof(parts[7].c_str());
            }
            if (size > 8)
            {
                density = (int)atoi(parts[8].c_str());
            }
            controller.StartTwinkle(Color{ r1, g1, b1 }, Color{ r2, g2, b2 }, seconds, density);
        }
        else if (command == "g")
        {
            int strip = -1;
            int colorsPerStrip = 0;
            float seconds = 0;
            if (size > 1) {
                strip = (int)atoi(parts[1].c_str());
            }
            if (size > 2) {
                colorsPerStrip = (int)atoi(parts[2].c_str());
            }
            if (size > 3) {
                seconds = (float)atof(parts[3].c_str());
            }
            if (strip != -1 && colorsPerStrip > 0) {
                std::cout << "### specify a strip or colorsPerStrip but not both\n";
            }
            std::vector<Color> colors;
            for (size_t i = 4; i + 3 <= size; i += 3)
            {
                uint8_t r = (uint8_t)atoi(parts[i].c_str());
                uint8_t g = (uint8_t)atoi(parts[i + 1].c_str());
                uint8_t b = (uint8_t)atoi(parts[i + 2].c_str());
                colors.push_back(Color{ r, g, b });
            }
            if (colorsPerStrip > 0) {
                if (colors.size() < colorsPerStrip * NUM_LEDStrips) {
                    std::cout << "### not enough colors.  To do " << colorsPerStrip << "colors per strip you need " << NUM_LEDStrips << " colors\n";
                }
            }
            controller.StartGradient(colors, strip, colorsPerStrip, seconds);
        }
        else if (command == "ga")
        {
            float seconds = 0;
            if (size > 1) {
                seconds = (float)atof(parts[1].c_str());
            }
            std::vector<Color> colors;
            for (size_t i = 2; i + 2 <= size; i += 3)
            {
                uint8_t r = (uint8_t)atoi(parts[i].c_str());
                uint8_t g = (uint8_t)atoi(parts[i + 1].c_str());
                uint8_t b = (uint8_t)atoi(parts[i + 2].c_str());
                colors.push_back(Color{ r, g, b });
            }
            for (int strip = 0; strip < NUM_LEDStrips; strip++) {
                controller.StartGradient(colors, strip, 0, seconds);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else if (command == "m")
        {
            float speed = 0;
            float direction = 0;
            uint32_t gradient_size = 0;
            if (size > 1) {
                speed = (float)atof(parts[1].c_str());
            }
            if (size > 2) {
                direction = (float)atof(parts[2].c_str());
            }
            if (size > 3) {
                gradient_size = (uint32_t)atoi(parts[3].c_str());
            }
            std::vector<Color> colors;
            for (size_t i = 4; i + 2 < size; i += 3)
            {
                uint8_t r = (uint8_t)atoi(parts[i].c_str());
                uint8_t g = (uint8_t)atoi(parts[i + 1].c_str());
                uint8_t b = (uint8_t)atoi(parts[i + 2].c_str());
                colors.push_back(Color{ r, g, b });
            }
            controller.StartMovingGradient(colors, speed, direction, gradient_size);
        }
        else if (command == "p")
        {
            std::vector<Pixel> pixels;
            for (size_t i = 1; i + 5 < size; i += 5)
            {
                int x = atoi(parts[i].c_str());
                int y = atoi(parts[i + 1].c_str());
                uint8_t r = (uint8_t)atoi(parts[i + 2].c_str());
                uint8_t g = (uint8_t)atoi(parts[i + 3].c_str());
                uint8_t b = (uint8_t)atoi(parts[i + 4].c_str());
                pixels.push_back(Pixel{ x, y, Color{r, g, b} });
            }
            controller.StartSetPixels(pixels);
        }
        else
        {
            PrintHelp();
        }
    }
}

int run(bool autoStart, bool useTcp, bool useServer, std::string portName, std::string servername, std::string firmware)
{
    std::string home = FileSystem::getUserHomeFolder();
    std::string path = FileSystem::combine(home, FileSystem::combine("Ada", "settings.json"));
    Settings& settings = Settings::loadJSonFile(path);
    std::string localname = settings.getString("local", TcpClientPort::getHostName());
    if (servername.size() == 0) {
        servername = settings.getString("server", "ada-core");
    }
    int serverPort = settings.getInt("server-port", 12345);
    settings.setString("firmware", firmware);
    SerialPort teensySerialPort;
    TcpClientPort teensyTcpPort;
    Port* teensyPort;
    bool interactive = !autoStart;

    while (useServer)
    {
        std::cout << "Looking for server: " << servername << "...\n";
        if (findServer(settings, servername, serverPort))
        {
            break;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    if (portName == "")
    {
        if (useTcp) {
            portName = "localhost";
        }
        else
        {
            auto ports = SerialPort::FindSerialPorts(TeensyVid, TeensyPid);
            if (ports.size() == 0)
            {
                std::cout << "No Teensy found in your serial ports\n";
                return 1;
            }

            for (auto i : ports)
            {
                portName = Utils::utf8(i.portName);
                std::cout << "Found Teensy at: " << portName << ", " << Utils::utf8(i.displayName) << "\n";
            }

            if (ports.size() > 1)
            {
                std::cout << "Found more than one Teensy on your serial ports, please specify which serial port to use\n";
                return 1;
            }
        }
    }

    if (useTcp)
    {
        teensyTcpPort.connect(portName, tcpPort);
        teensyPort = &teensyTcpPort;
    }
    else
    {
        int rc = teensySerialPort.connect(portName.c_str(), 115200);
        if (rc != 0)
        {
            std::cout << "### Error connecting Teensy\n";
            return 1;
        }
        else
        {
            std::cout << "### Teensy connected over serial port\n";
            teensySerialPort.flush();
            // write something bogus to bump teensy parser out of any weird states
            const std::string flushMessage = "##HEADER####HEADER####HEADER####HEADER####HEADER##";
            teensySerialPort.write((uint8_t*)flushMessage.c_str(), (int)flushMessage.size());
        }
        teensyPort = &teensySerialPort;
    }

    std::string localIp = settings.getString("local-ip", "localhost");
    std::string serverIp = settings.getString("server-ip", "localhost");
    Sensei sensei(localname, localIp, serverIp);
    Controller controller(*teensyPort, sensei, NUM_LEDStrips, LEDSPerStrip);
    std::cout << "Using local ip \"" << localIp << "\"\n";
    std::cout << "Using server ip \"" << serverIp << "\"\n";

    try {

        if (autoStart)
        {
            std::cout << "Auto-starting Sensei\n";
            controller.StartSensei();
        }
        else
        {
            PrintHelp();
        }

        if (!interactive)
        {
            std::cout << "RpiController is not interactive\n";
            while (!sensei.UpdateFirmware()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            // return 1 so the run.py script updates the firmware.
            controller.Close();
            sensei.Stop();
            return 1;
        }
        else
        {
            RunCommandLine(controller);
        }
    }
    catch (std::exception& e)
    {
        std::cout << "caught unhandled exception: ";
        const char* msg = e.what();
        if (msg != nullptr) {
            std::cout << msg;
        }
        else {
            std::cout << "unknown reason";
        }
        std::cout << "\n";
        exit(1);
    }

    controller.Close();

    return 0;
}



int main(int argc, char* argv[])
{
    std::string portName;
    bool autoStart = false;
    bool useTcp = false;
    bool useServer = true;
    std::string servername;
    std::string firmware;
    // parse args
    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];
        if (s == "--autostart")
        {
            autoStart = true;
        }
        else if (s == "--tcp")
        {
            useTcp = true;
        }
        else if (s == "--serial")
        {
            useTcp = false;

            if (i + 1 < argc) {
                std::string next = argv[i];
                if (next[0] != '-') {
                    portName = next;
                    i++;
                }
            }
        }
        else if (s == "--noserver")
        {
            useServer = false;
        }
        else if (s == "--server")
        {
            i++;
            if (i < argc) {
                servername = argv[i];
            }
            else {
                std::cout << "### Error: --server arg is missing server name" << std::endl;
                PrintUsage();
                return 0;
            }
        }
        else if (s == "--firmware") {
            i++;
            if (i < argc) {
                firmware = argv[i];
            }
            else {
                std::cout << "### Error:  --firmware arg is missing file name" << std::endl;
                PrintUsage();
                return 0;
            }
        }
        else {

            std::cout << "### Error: Unexpected argument '" << s << "'" << std::endl;
            PrintUsage();
        }
    }

    return run(autoStart, useTcp, useServer, portName, servername, firmware);
}
