#include <SFML/Graphics.hpp>
#include <iostream>

#include "NESEmulator.hpp"

sf::VideoMode SCREEN = sf::VideoMode::getDesktopMode();

#define updateRegistersText() registers.setString("A:  #$" + HEX(emu.A) + "\n" + \
												  "X:  #$" + HEX(emu.X) + "\n" + \
												  "Y:  #$" + HEX(emu.Y) + "\n" + \
												  "P:  #$" + HEX(emu.P) + "\n" + \
												  "S:  #$" + HEX(emu.S) + "\n" + \
												  "PC: #$" + HEX(emu.PC));

int main()
{
	NESEmulator emu;

	uint8_t basicAdd[] = { 0xa9, 0x50, 0x69, 0x12, 0x85, 0x00 };
	emu.load(0x200, &basicAdd[0], 6);

	sf::Font font;
	font.loadFromFile("resources/font.ttf");
	sf::Text registers;
	registers.setFont(font);
	registers.setString("");

	updateRegistersText();

	sf::Text memory;
	memory.setFont(font);
	memory.setString("00");

	int32_t memoryScroll = 0;

	uint32_t sDown = 0;

	sf::RenderWindow window(sf::VideoMode(800, 550), "NES Emulator");
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::Resized)
				window.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
			if (event.type == event.MouseWheelMoved)
				memoryScroll -= event.mouseWheel.delta;
		}

		memoryScroll = std::max(0, std::min((int)memoryScroll, 0x10000 - 24));

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			sDown++;
		else
			sDown = 0;

		if (sDown == 1)
		{
			std::cout << emu.cycle();

			updateRegistersText();
		}

		sf::Vector2f ws = (sf::Vector2f)window.getSize();

		window.clear(sf::Color(128, 128, 128));

		registers.setPosition(sf::Vector2f(ws.y + 10, 0));
		window.draw(registers);

		sf::RectangleShape memoryRect(sf::Vector2f(SCREEN.width - (ws.y + 240), (float)SCREEN.height));
		memoryRect.setFillColor(sf::Color(100, 100, 100));
		memoryRect.setPosition(sf::Vector2f(ws.y + 240, 0));
		window.draw(memoryRect);

		for (uint16_t i = 0; i < 24; i++)
		{
			memory.setString("$" + HEX((i + memoryScroll) * 8));
			memory.setPosition(sf::Vector2f(ws.y + 260, i * 40.f));
			window.draw(memory);
			for (uint16_t j = 0; j < 8; j++)
			{
				memory.setString(HEX_1B(emu.readMemory1B(j + 8 * (i + memoryScroll))));
				memory.setPosition(sf::Vector2f(ws.y + 260 + j * 60 + 160, i * 40.f));
				window.draw(memory);
			}
		}

		sf::RectangleShape screen(sf::Vector2f(ws.y, ws.y));
		screen.setFillColor(sf::Color(0, 0, 0));
		window.draw(screen);

		window.display();
	}
	return 0;
}