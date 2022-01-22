#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <thread>
#include <memory>
#include <mutex>
#include <fstream>
#include "SerialPort.h"
#include "TcpClientPort.h"
#include "TeensyPixelBuffer.h"
#include "Commands.h"
#include "Sensei.h"
#include "Timer.h"

class Controller
{
    Port& port; // to the Teensy
    Sensei& sensei;
    TeensyPixelBuffer buffer;
    bool commandRunning = false;
    std::thread commandThread; // so we can do continuous updating of lights while also processing the main console input.
    std::thread watchdog;
    std::string lastCommand;
    Command currentCommand;
    CancelToken token;
    bool terminating = false;
    bool flush = true;
    bool commandCompleted= false;
    bool commandStarted = false;
    Timer commandTimer;
    const int TEENSY_TIMEOUT = 60; // 60 seconds

public:
    Controller(Port& teensyPort, Sensei& sensei, int numStrips, int ledsPerStrip)
        : port(teensyPort), sensei(sensei),
        buffer(port, numStrips, ledsPerStrip, token)
    {
    }

    void StopThread()
    {
        token.Cancel = true;
        if (commandThread.joinable())
        {
            commandThread.join();
        }
        if (watchdog.joinable())
        {
            watchdog.join();
        }
        commandStarted = false;
    }

    void WaitForComplete() {
        if (commandStarted) {
            while (!commandCompleted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    void CommandThread()
    {
        try 
        {
            RunCommand();
        }
        catch (std::exception& e) 
        {
            if (e.what() != nullptr)
            {
                std::cout << "### unhandled exception " << e.what() << "\n";
            }
        }
        commandRunning = false;
    }

    void RunCommand()
    {
        commandRunning = true;
        commandTimer.start();

        if (currentCommand.command == "RunSensei")
        {
            // Sensei is not a command, it is a command loop.
            commandRunning = false;
            RunSensei();
        }
        else if (currentCommand.command == "sensei")
        {
            // this is only used to turn off the lights.
            RunCrossFade();
        }
        else if (currentCommand.command == "ColumnFade")
        {
            RunColumnFade();
        }
        else if (currentCommand.command == "SetPixels")
        {
            RunSetPixels();
        }
        else if (currentCommand.command == "SetColor")
        {
            RunSetColor();
        }
        else if (currentCommand.command == "Breathe")
        {
            RunBreathe();
        }
        else if (currentCommand.command == "SpeedTest")
        {
            RunSpeedTest();
        }
        else if (currentCommand.command == "SerialTest")
        {
            buffer.RunSerialTest(currentCommand.seconds / 1000);
        }
        else if (currentCommand.command == "NeuralDrop")
        {
            buffer.NeuralDrop(currentCommand.iterations);
        }
        else if (currentCommand.command == "WaterDrop")
        {
            buffer.WaterDrop(currentCommand.size, currentCommand.iterations, currentCommand.f1);
        }
		else if (currentCommand.command == "StartRain")
		{
			buffer.StartRain(currentCommand.size, currentCommand.f1);
		}
		else if (currentCommand.command == "StopRain")
		{
			buffer.StopRain();
		}
		else if (currentCommand.command == "CrossFade")
        {
            RunCrossFade();
        }
        else if (currentCommand.command == "Gradient")
        {
            buffer.VerticalGradient(currentCommand.colors, currentCommand.seconds, currentCommand.strip, currentCommand.colorsPerStrip);
        }
        else if (currentCommand.command == "MovingGradient")
        {
            buffer.MovingVerticalGradient(currentCommand.colors, currentCommand.seconds, currentCommand.f1, currentCommand.size);
        }
        else if (currentCommand.command == "Rainbow")
        {
            buffer.Rainbow(currentCommand.size, currentCommand.seconds);
        }
        else if (currentCommand.command == "Fire")
        {
            buffer.Fire(currentCommand.f1, currentCommand.f2, currentCommand.seconds);
        }
		else if (currentCommand.command == "Twinkle")
        {
            if (currentCommand.colors.size() > 1)
            {
                buffer.Twinkle(currentCommand.colors[0], currentCommand.colors[1], currentCommand.seconds, currentCommand.size);
            }
        }
        commandRunning = false;
        commandCompleted = true;
        commandStarted = false;
    }

    void StartCommand()
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        StopThread();
        token.Cancel = false;
        commandStarted = true;
        commandTimer.start();
        commandThread = std::thread(&Controller::CommandThread, this);
        watchdog = std::thread(&Controller::WatchdogThread, this);
    }

    void WatchdogThread()
    {
        // make sure Teensy port is not hanging.
        while (!token.Cancel)
        {
            if (commandRunning && commandTimer.seconds() > TEENSY_TIMEOUT)
            {
                // command is blocked and Teensy is not reponding, so we exit
                // this process which will allow the bash shell script to 
                // reboot the Teensy.
                std::cout << "### Teensy does not seem to be responding!\n";
                token.Cancel = true;
                exit(1);
            }
            else 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    void StartSensei()
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "RunSensei";
        StartCommand();
    }

    void StartBreathe(float seconds, float f1, float f2)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "Breathe";
        currentCommand.seconds = seconds;
        currentCommand.f1 = f1;
        currentCommand.f2 = f2;
        StartCommand();
    }

    void Close()
    {
        port.close();
        terminating = true;
        StopThread();
    }

    bool Terminating() {
        return this->terminating;
    }

    void StartSetColor(Color color, int strip, int index, float seconds)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "SetColor";
        currentCommand.colors.clear();
        currentCommand.colors.push_back(color);
        currentCommand.strip = strip;
        currentCommand.index = index;
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartSetPixels(std::vector<Pixel> pixels)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "SetPixels";
        currentCommand.pixels.clear();
        currentCommand.pixels = pixels;
        StartCommand();
    }

    void StartCrossFade(Color color, float seconds)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "CrossFade";
        currentCommand.colors.clear();
        currentCommand.colors.push_back(color);
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartTwinkle(Color c1, Color c2, float seconds, int density)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "Twinkle";
        currentCommand.colors.clear();
        currentCommand.colors.push_back(c1);
        currentCommand.colors.push_back(c2);
        currentCommand.seconds = seconds;
        currentCommand.size = density;
        StartCommand();
    }

    void StartGradient(std::vector<Color> colors, int strip = -1, int colorsPerStrip = 0, float seconds = 0)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "Gradient";
        currentCommand.seconds = seconds;
        currentCommand.strip = strip;
        currentCommand.colorsPerStrip = colorsPerStrip;
        currentCommand.colors.clear();
        for (auto c : colors) 
        {
            currentCommand.colors.push_back(c);
        }
        StartCommand();
    }

    void StartMovingGradient(std::vector<Color> colors, float speed, float direction, uint32_t size)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "MovingGradient";
        currentCommand.seconds = speed;
        currentCommand.size = size;
        currentCommand.f1 = direction;
        currentCommand.colors.clear();
        for (auto c : colors)
        {
            currentCommand.colors.push_back(c);
        }
        StartCommand();
    }

    void StartRainbow(int length, float seconds)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "Rainbow";
        currentCommand.size = length;
        currentCommand.seconds = seconds;
        StartCommand();
    }

    void StartFire(float cooling, float sparkle, float seconds)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "Fire";
        currentCommand.f1 = cooling;
        currentCommand.f2 = sparkle;
        currentCommand.seconds = seconds;
        StartCommand();
    }

	void StartRain(int length, float f1)
	{
        WaitForComplete(); // wait for previous command to be sent to Teensy.
		currentCommand.command = "StartRain";
		currentCommand.size = length;
		currentCommand.f1 = f1;
		StartCommand();
	}

	void StopRain()
	{
        WaitForComplete(); // wait for previous command to be sent to Teensy.
		currentCommand.command = "StopRain";
		StartCommand();
	}

    void StartWaterDrop(int length, int drops, float amount)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "WaterDrop";
        currentCommand.size = length;
        currentCommand.iterations = drops;
        currentCommand.f1 = amount;
        StartCommand();
    }

    void StartNeuralDrop(int drops)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "NeuralDrop";
        currentCommand.iterations = drops;
        StartCommand();
    }

    void QueryStatus()
    {
        buffer.QueryStatus();
    }

    void StartSpeedTest()
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "SpeedTest";
        StartCommand();
    }

    void StartSerialTest(float msdelay)
    {
        WaitForComplete(); // wait for previous command to be sent to Teensy.
        currentCommand.command = "SerialTest";
        currentCommand.seconds = msdelay * 1000;
        StartCommand();
    }


private:

    void RunBreathe()
    {
        buffer.Breathe(currentCommand.seconds, currentCommand.f1, currentCommand.f2);
    }

    void RunSetColor()
    {
        if (currentCommand.colors.size() > 0)
        {
            auto c = currentCommand.colors[0];
            buffer.SendSetColor(c, currentCommand.strip, currentCommand.index);
        }
    }

    void RunSensei()
    {
        Timer senseiTimer;
        senseiTimer.start();
        Command cmd;
        sensei.Start();
        while (!token.Cancel)
        {
            // throttle our calls to the server so it is not more than 1 second.
            int size = sensei.Size();
            if (size > 0)
            {
                for (int i = 0; i < size; i++)
                {
                    cmd = sensei.GetNextCommand();
                    std::cout << "received command: " << cmd.command << "\n";
                    senseiTimer.start();
                    if (cmd.command.size() > 0)
                    {
                        currentCommand = cmd;
                        // flush the buffer on the last command.  This makes it possible for the
                        // server to send some compound commands like gradient + rain.
                        flush = (i == size - 1);
                        RunCommand();
                    }
                }
            }
            else
            {
                // check token.Cancel every 50 ms.
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        sensei.Stop();
    }

    void RunColumnFade()
    {
        // Note: the way ParseColumns works, we should always have the exact same number
        // of colors as columns, in other words, each column has it's own color.
        for (int index = 0; index < currentCommand.columns.size(); index++)
        {
            int col = currentCommand.columns[index];
            auto c = currentCommand.colors[index];
            buffer.SetColumn(c, col);
        }
        if (flush) {
            buffer.SendEncodedBuffer(currentCommand.seconds);
        }
    }

    void RunSetPixels()
    {
        for (auto c : currentCommand.pixels)
        {
            buffer.SetPixel(c.color, c.strip, c.led);
        }
        if (flush) {
            buffer.SendEncodedBuffer(0);
        }
    }

    void RunCrossFade()
    {
        if (currentCommand.colors.size() > 0)
        {
            buffer.CrossFadeTo(currentCommand.colors[0], currentCommand.seconds);
        }
    }

    void RunSpeedTest()
    {
        double sum = 0.0;

        int green = 0;
        int direction = 1;

        Timer timer;
        int errors = 0;
        int white = 0;
        double totalTime = 0;
        int ledsPerStrip = buffer.NumLedsPerStrip();
        int iterations = ledsPerStrip;
        int size = ledsPerStrip * buffer.NumStrips();

        for (int count = 0; count < iterations && !token.Cancel; count++) {
            timer.start();

            // smooth fade of green to full and back.
            buffer.SetColor(Color{ 0, (uint8_t)green, 0 });
            green += direction;
            if (green == 256) {
                direction = -1;
                green = 254;
            }
            else if (green == 0 && direction == -1) {
                green = 1;
                direction = 1;
            }

            // and chase a white light down the string.
            buffer.SetRow(Color{ 255, 255, 255 }, white);
            white++;
            if (white == ledsPerStrip)
            {
                white = 0;
            }

            buffer.SendFullBuffer(0);

            timer.stop();
            auto ms = timer.microseconds();
            double seconds = (double)ms / 1000000.0;
            totalTime += seconds;
            double bytesPerSecond = (double)size / seconds;

            //std::cout << "Bytes per second = " << (int)bytesPerSecond << "\n";
            sum += bytesPerSecond;
        }

        double avg = sum / (double)iterations;
        std::cout << "Average bytes per second = " << (int)avg << " and found " << errors << " errors\n";
        double fps = (double)iterations / totalTime;
        std::cout << "Frames per second = " << std::setprecision(3) << fps << "\n";
    }
};

#endif
