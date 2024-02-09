#include <SFML/Graphics.hpp>
#include <iostream>

#include "NESEmulator.hpp"

sf::VideoMode SCREEN = sf::VideoMode::getDesktopMode();

//int main(int argc, char** argv)
int WinMain()
{
	if (__argc != 1 + 1)
		return 1;

	NESEmulator emu;
	//emu.powerUp();
	emu.loadFromiNES(__argv[0 + 1]);
	//emu.PC = 0xC000; // nestest.nes in automation

	sf::Font font;
	font.loadFromFile("resources/font.ttf");
	sf::Text registers;
	registers.setFont(font);
	registers.setString("");
	sf::Text instructions;
	instructions.setFont(font);

	instructions.setPosition(sf::Vector2f(1020, 0));

	updateRegistersText();

	sf::Text memory;
	memory.setFont(font);
	memory.setString("00");

	int32_t memoryScroll = 0;

	uint32_t sDown = 0, fDown = 0, spaceDown = 0, iDown;

	bool running = false;

	sf::RenderWindow window, window_cpuDebug, window_ppuDebug;
	window_cpuDebug.create(sf::VideoMode(800, 550), "NES Emulator | CPU Debug");
	window_ppuDebug.create(sf::VideoMode(800, 550), "NES Emulator | PPU Debug");
	window.create(sf::VideoMode(800, 550), "NES Emulator");
	window.setFramerateLimit(60);
	window.setVerticalSyncEnabled(false);

	while (window.isOpen() && window_cpuDebug.isOpen() && window_ppuDebug.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::Resized)
				window.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
		}
		while (window_cpuDebug.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window_cpuDebug.close();
			if (event.type == sf::Event::Resized)
				window_cpuDebug.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
			if (event.type == event.MouseWheelMoved)
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
					memoryScroll -= 256 / 8 * event.mouseWheel.delta;
				else
					memoryScroll -= event.mouseWheel.delta;
			}
		}
		while (window_ppuDebug.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window_ppuDebug.close();
			if (event.type == sf::Event::Resized)
				window_ppuDebug.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
		}

		memoryScroll = std::max(0, std::min((int)memoryScroll, 0x2000 - 24));

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			sDown++;
		else
			sDown = 0;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F))
			fDown++;
		else
			fDown = 0;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
			spaceDown++;
		else
			spaceDown = 0;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::I))
			iDown++;
		else
			iDown = 0;

		if (spaceDown == 1)
			running ^= true;

		if (fDown == 1 || running)
		{
			//for(uint16_t i = 0; i < 1.789773 * 1000000 * 3; i++)
			//for (uint16_t i = 0; i < 21.477272 * 1000000 / 4; i++)
			while(!emu.frameFinished)
				emu.cycle(true);

			emu.frameFinished = false;

			updateRegistersText();
		}

		if (sDown == 1)
		{
			emu.cycle(true);

			emu.frameFinished = false;

			updateRegistersText();
		}

		if (iDown == 1)
		{
			for (unsigned int i = 0; i < 3; i++)
			{

				while (emu.CPU_cycles > 0)
					emu.cycle(true);
				emu.cycle(true);

				emu.frameFinished = false;

				updateRegistersText();
			}
		}

		while (emu.logLines > 32)
		{
			emu.log.erase(0, emu.log.find("\n") + 1);
			emu.logLines--;
		}

		sf::Vector2f ws = (sf::Vector2f)window.getSize(),
					 ws_cpuDebug = (sf::Vector2f)window_cpuDebug.getSize();

		window.clear(sf::Color(128, 128, 128));
		window_cpuDebug.clear(sf::Color(128, 128, 128));
		window_ppuDebug.clear(sf::Color(128, 128, 128));

////////////////////////////////////////////////////////////////////////////////////////////////

		registers.setPosition(sf::Vector2f(10, 0));
		window_cpuDebug.draw(registers);

		sf::RectangleShape memoryRect(sf::Vector2f(710, (float)SCREEN.height));
		memoryRect.setFillColor(sf::Color(100, 100, 100));
		memoryRect.setPosition(sf::Vector2f(280, 0));
		window_cpuDebug.draw(memoryRect);

		for (uint16_t i = 0; i < 24; i++)
		{
			memory.setString("$" + HEX((i + memoryScroll) * 8));
			memory.setPosition(sf::Vector2f(310, i * 40.f));
			window_cpuDebug.draw(memory);
			for (uint16_t j = 0; j < 8; j++)
			{
				memory.setString(HEX_1B(emu.CPU_readMemory1B(j + 8 * (i + memoryScroll))));
				memory.setPosition(sf::Vector2f(310 + j * 60 + 160.f, i * 40.f));
				window_cpuDebug.draw(memory);
			}
		}

		instructions.setString(emu.log);
		window_cpuDebug.draw(instructions);

////////////////////////////////////////////////////////////////////////////////////////////////

		float screenSize = std::min(ws.x, ws.y);

		sf::RectangleShape screen(sf::Vector2f(screenSize, screenSize));
		screen.setOrigin(screenSize / 2.f, screenSize / 2.f);
		screen.setPosition(sf::Vector2f(ws.x / 2.f, ws.y / 2.f));
		sf::Texture screenTex;
		screenTex.loadFromImage(emu.screen);
		screen.setTexture(&screenTex);
		window.draw(screen);

		window.display();
		window_cpuDebug.display();
		window_ppuDebug.display();
	}

	return 0;
}