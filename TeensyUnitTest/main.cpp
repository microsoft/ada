// UnitTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Timer.h"
#include "crc32.h"
#include <thread>

#define UNITTEST
#include "ArduinoMock.h"
#include "Utils.h"
#include "StreamWriter.h"
#include "Vector.h"
#include "MultiWS2812Mock.h"
#include "PixelBuffer.h"
#include "Animations.h"
#include "Controller.h"
#include "Commands.h"
#include "Bitmap.h"
#include "HlsColor.h"
#include "TestWindow.h"

TestWindow window;
const int animation_delay = 16;
const int numStrips = 16;
const int numLeds = 392;
Controller controller(numStrips, numLeds);
const int tcpPort = 21567;
TeensyStatus gTeensyStatus = { 0,0,0 };

void TestCRC()
{
    uint8_t buffer[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint32_t crc = crc32(buffer, 8);
    std::cout << "crc of zero buffer=" << crc << "\n";

    uint8_t buffer2[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    crc = crc32(buffer2, 8);
    std::cout << "crc of range buffer=" << crc << "\n";
}

void TestCommands(std::string name, bool biglength, bool nocolors, bool badCrc)
{
    StreamWriter writer;
    Command command;
    writer.WriteString("xxxx##HEADER##");
    writer.WriteString(name.c_str()); // 10 bytes
    writer.WriteByte(0); // null terminate the command string
    auto lenOffset = writer.Size();
    writer.WriteInt(0); // place holder for length
    int offset = writer.Size();
    writer.WriteFloat(3); // seconds
    if (!nocolors) {
        writer.WriteInt(Color{ 255,0,0 }.pack()); // number of colors
    }
    if (biglength)
    {
        writer.WriteLength(lenOffset, 100000000);
    }
    else
    {
        writer.WriteLength(lenOffset, writer.Size() - offset);
    }
    if (badCrc) {
        writer.WriteInt(1234);
    }
    else {
        writer.WriteCRC(offset);
    }

    Serial.setBuffer((char*)writer.GetBuffer(), writer.Size());

    bool rc = command.readNextCommand();

    std::cout << "readNextCommand returned " << rc << "\n";
    if (rc) {
        if (command.error.size() > 0) {
            std::cout << "error: " << command.error.c_str() << "\n";
        }
        else {
            std::cout << "read command " << command.command.c_str() << "\n";
        }
    }    
}

void TestStrings()
{
    SimpleString s = "12345";

    s += '6';

    std::cout << s.c_str() << "\n";
    s = "apples";
    std::cout << s.c_str() << "\n";

    SimpleString s2 = "123 ";
    SimpleString s3 = s2 + s;
    std::cout << s3.c_str() << "\n";

    s3 += " more";
    std::cout << s3.c_str() << "\n";

    s3 = stringf("This %d and that", 123);
    std::cout << s3.c_str() << "\n";
}

void TestVectors()
{
    Vector<int> v;
    v.push_back(10);
    v.push_back(20);

    Vector<int> w(v);

    std::cout << w[0] << "," << w[1] << "\n";

    v.clear();
}

void TestPixelBuffer()
{
    PixelBuffer& buffer = controller.GetBuffer();
    buffer.SetColor(Color{ 80,0,0 }); // set base color to red
    buffer.SetColumn(Color{ 255,255 }, 3); // set a yellow column.
    buffer.SetRow(Color{ 0,0,255 }, 5); // set a blue row
    buffer.SetPixel(Color{ 0,0,0 }, 0, 0); // make first pixel black
	buffer.Write();

    // give UI time to update
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void TestGradientFade()
{
    PixelBuffer& buffer = controller.GetBuffer();
    buffer.SetColor(Color{ 0,0,0 });

    Vector<Color> colors;
    colors.push_back(Color{ 0, 0, 255}); // blue
    colors.push_back(Color{ 0, 255, 255 }); // cyan
    colors.push_back(Color{ 255, 255, 0 }); // yellow
    GradientAnimation gradient(buffer, false);
    gradient.AddStrip(-1, colors, 2.0);    
    std::cout << "fading to blue/red gradient from black...";
    while (!gradient.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }

    // test adding individual strip override
    colors.clear();
    colors.push_back(Color{ 255, 0, 0 }); // red
    colors.push_back(Color{ 255, 255, 0 }); // yellow
    colors.push_back(Color{ 0, 255, 0 }); // green    
    gradient.AddStrip(5, colors, 2.0);
    std::cout << std::endl << "fading strip 5 to red/yellow/green gradient ...";
    while (!gradient.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }

    std::cout << "done\n";
}

void TestFireAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    buffer.SetColor(Color{ 0,0,0 });

    FireAnimation fire(buffer, 50, 100, 0);
    std::cout << "running fire animation...";
    Timer timer;
    timer.start();
    while (!fire.Run() && timer.seconds() < 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }

    // now test it via the command interface
    buffer.SetColor(Color{ 0,0,0 });
    Command& cmd = controller.GetCommand();
    cmd.type = CommandType::Fire;
    cmd.command = "Fire";
    cmd.f1 = 50;
    cmd.f2 = 60;
    cmd.seconds = 10;
    controller.StartCommand();

    while (!controller.RunAnimation())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }

    std::cout << "done\n";
}

void TestCrossFade()
{
    PixelBuffer& buffer = controller.GetBuffer();
    Vector<Color> colors;
    colors.push_back(Color{ 255,0,0 });
    CrossFadeToAnimation crossFade(buffer, colors, 2.0);
    std::cout << "fading to red...";
    while (!crossFade.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";

    // test cross fade to another buffer.
    PixelBuffer buffer2(numStrips, numLeds);
    buffer2.Initialize();
    buffer2.SetColor(Color{ 0,0,255 });

    uint32_t size = buffer2.GetBufferSize();
    CrossFadeToAnimation crossFadeTarget(buffer, buffer2.GetPixelBuffer(), size, 2.0);
    std::cout << "fading to blue...";
    while (!crossFadeTarget.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestOverlayAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    Vector<Color> colors;
    colors.push_back(Color{ 0, 180, 120 });
    CrossFadeToAnimation crossFade(buffer, colors, 2.0);
    crossFade.AddOverlay(new RainOverlayAnimation(buffer, 16, 30));
    std::cout << "fading to green with raindrops...";
    while (!crossFade.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";

    // overlay on an animating gradient
    buffer.SetColor(Color{ 0,0,0 });

    colors.push_back(Color{ 0, 0, 180 });
    GradientAnimation gradient(buffer, false);
    gradient.AddStrip(-1, colors, 2.0);
    gradient.AddOverlay(new RainOverlayAnimation(buffer, 16, 30));
    std::cout << "fading to green/blue gradient with raindrops...";
    while (!gradient.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestWaterDrop()
{
    PixelBuffer& buffer = controller.GetBuffer();
    buffer.VerticalGradient(Color{ 0,0,0 }, Color{ 30, 80, 200 });
	buffer.Write();

    WaterDropAnimation waterDrop(buffer, 2, 16, 20);
    std::cout << "water drop ...";
    while (!waterDrop.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestRainbowAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    RainbowAnimation rainbow(buffer, 150, 2);
    std::cout << "rainbow...";
    while (!rainbow.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestNeuralDropAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    NeuralDropAnimation neural(buffer, 1);

    std::cout << "neural...";
    while (!neural.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestTwinkleAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    TwinkleAnimation twinkle(buffer, Color{ 0,0,40 }, Color{ 0,0,255 }, 10, 5);

    std::cout << "twinkle...";
    int timeout = 60;
    while (!twinkle.Run() && timeout-- > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestBreatheAnimation()
{
    PixelBuffer& buffer = controller.GetBuffer();
    buffer.VerticalGradient(Color{ 0,0,0 }, Color{ 30, 80, 200 });

    BreatheAnimation breathe(buffer, 5, 7, 0.4f);

    std::cout << "breathe...";
    while (!breathe.Run())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestGradient()
{
	std::cout << "gradient...";

    PixelBuffer& buffer = controller.GetBuffer();
	buffer.VerticalGradient(Color{ 0,0,0 }, Color{ 30, 80, 200 });

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "done\n";
}

void TestMovingGradient()
{
    std::cout << "MovingGradient Animation...";
    Command& cmd = controller.GetCommand();
    cmd.type = CommandType::MovingGradient;
    cmd.command = "MovingGradient";
    cmd.f1 = 1;
    cmd.seconds = 1;
    cmd.size = 100;
    // animate red on leading edge leaving yellow behind.
    cmd.addColor(Color{ 80, 0, 0 });
    cmd.addColor(Color{ 80, 80, 0 });
    cmd.iterations = 1;
    controller.StartCommand();

    while (!controller.RunAnimation())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }

    // add some rain overlay
    std::cout << "Adding rain...";
    cmd.type = CommandType::StartRain;
    cmd.command = "StartRain";
    controller.StartCommand();

    // start another moving gradient
    cmd.type = CommandType::MovingGradient;
    cmd.command = "MovingGradient";
    cmd.f1 = -1;
    cmd.seconds = 5;
    cmd.size = 100;
    // fading the yellow into a blue background
    cmd.clearColors();
    cmd.addColor(Color{ 80, 80, 0 });
    cmd.addColor(Color{ 0, 0, 40 });
    cmd.iterations = 1;
    controller.StartCommand();
    while (!controller.RunAnimation())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}


void TestController()
{
    std::cout << "Controller NeuralDrop...";
    Command& cmd = controller.GetCommand();
    cmd.type = CommandType::NeuralDrop;
    cmd.command = "NeuralDrop";
    cmd.iterations = 1;
    controller.StartCommand();
    while (!controller.RunAnimation())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestRainAnimation()
{
    std::cout << "Test StartRain...";
    Command& cmd = controller.GetCommand();
    cmd.type = CommandType::StartRain;
    cmd.command = "StartRain";
    cmd.size = 16;
    cmd.f1 = 40;
    Timer timer;
    timer.start();
    controller.StartCommand();
    while (!controller.RunAnimation() && timer.seconds() < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}

void TestControlledCrossFade()
{
    std::cout << "Test Controller CrossFade...";
    Timer timer;
    timer.start();
    int count = 0;
    while (!controller.RunAnimation() && timer.seconds() < 5)
    {
        if (count % 60 == 0) {
            // make sure rain animation survices resetting the fade animation with no crashes!
            controller.GetBuffer().SetColor(Color{ 0,80,0 });
            controller.StartCrossFade(Color{ 70,0,10 }, 2.0f);
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
    }
    std::cout << "done\n";
}


void TestHlsColors()
{
    int w = 100;
    int h = 100;
    Bitmap bmp(w, h);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            HlsColor hls((float)x * 360.0f / (float)w, (float)y / (float)h, 1);
            Color c = hls.GetRGB();
            bmp.SetPixel(x, y, c.r, c.g, c.b);
        }
    }
    bmp.SaveBitmap("test.bmp");
    window.DrawBitmap(bmp);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void TestThread()
{
    // Controller tests
    TestFireAnimation();
    TestCrossFade();
    TestMovingGradient();
    TestController();
    TestRainAnimation();
    TestControlledCrossFade();

    // Low level buffer tests
    TestGradientFade();
    TestHlsColors();
    TestCRC();
    TestStrings();
    TestVectors();
    TestCommands("CrossFade", false, false, false);
    TestCommands("CrossxFade", false, false, false);
    TestCommands("CrossFade", true, false, false);
    TestCommands("CrossFade", false, true, false);
    TestCommands("CrossFade", false, false, true);
    TestCommands("CrossFade", false, false, false); // make sure it recovers after an error.
    TestPixelBuffer();
    TestWaterDrop();
    TestTwinkleAnimation();
    TestNeuralDropAnimation();
    TestRainbowAnimation();
    TestGradient();
    TestBreatheAnimation();
    TestOverlayAnimation();

    window.Close();
}

void FirmwareTest()
{
	Command cmd;
	while (true) {
		if (Serial.available())
		{
			bool read_something = cmd.readNextCommand();
			if (read_something)
			{
				if (cmd.error.size() > 0)
				{
					DebugPrint("%s\n", cmd.error.c_str());
					cmd.error = "";
				}
				else
				{
					// new command received!
					controller.StartCommand(cmd);
				}
				DebugPrint("##COMPLETE##\n");
			}
		}

		if (controller.HasAnimation())
		{
			if (controller.RunAnimation()) {
				// done!
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay));
	}
}

int main(int argc, char* argv[])
{
	bool runClient = false;
    std::string clientAddress = "localhost";
	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if (arg == "--client") {
			runClient = true;
            if (i + 1 < argc) {
                clientAddress = argv[++i];
            }
		}
	}

	// Initialize the buffer and the controller.
	controller.Initialize();
	controller.GetBuffer().GetDriver().setWindow(&window);

	if (runClient)
	{
        DebugPrint("Waiting for RpiController to connect...\n");
		Serial.connect(clientAddress.c_str(), tcpPort);
		std::thread testThread = std::thread(&FirmwareTest);
		window.SetSize(1000, 400);
		window.Run();
		testThread.join();
	}
	else
	{
		std::thread testThread = std::thread(&TestThread);
		window.SetSize(1000, 400);
		window.Run();
		testThread.join();
	}
    return 0;
}
