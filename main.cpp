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

	std::vector<std::string> palettes = { 
	"Composite Direct (FBX)",
	"Smooth (FBX)",
	"PVM Style D93 (FBX)",
	"NES Classic (FBX)",
	"Sony CXA",
	"PC-10",
	"Wavebeam"
	};

	uint8_t paletteIndex = 6;

	std::string paletteName = palettes[paletteIndex];
	bool paletteLoaded = emu.loadPaletteFromPAL("resources/" + paletteName + ".pal");

	NES_LOG_ADD_LINE(emu, paletteLoaded ? 
	"Palette loaded (" + paletteName + ")" :
	("Couldnt find palette \"" + paletteName + "\""));

	sf::Font font;
	font.loadFromFile("resources/font.ttf");
	sf::Text registers;
	registers.setFont(font);
	registers.setString("");
	sf::Text instructions;
	instructions.setFont(font);

	instructions.setPosition(sf::Vector2f(1020, 0));

	updateRegistersText();

	sf::Text text, line;
	text.setFont(font);
	text.setString("00");
	text.setFont(font);
	text.setString("00");

	int32_t memoryScroll = 0, ppuMemoryScroll = 0;

	uint32_t sDown = 0, fDown = 0, spaceDown = 0, iDown = 0, pDown = 0, 
			 rDown = 0, bDown = 0, lDown = 0, wDown = 0;

	bool running = false;

	uint8_t displayPalette = 0;

	sf::RenderWindow window;
	window.create(sf::VideoMode(800, 550), "NES Emulator");
	window.setFramerateLimit((int)60.1);
	window.setVerticalSyncEnabled(false);

	WindowState windowState = Screen;

	emu.settings.debugBGPalette = false;
	emu.settings.emulateDifferentialPhaseDistortion = true;
	emu.settings.printLog = false;

	while(window.isOpen())
	{
		sf::Event event;

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::Resized)
				window.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
			switch (windowState)
			{
			case CPUDebug:
				if (event.type == event.MouseWheelMoved)
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
						memoryScroll -= 256 / 8 * event.mouseWheel.delta;
					else
						memoryScroll -= event.mouseWheel.delta;
				}
				break;

			case PPUDebug:
				if (event.type == event.MouseWheelMoved)
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
						ppuMemoryScroll -= 256 / 8 * event.mouseWheel.delta;
					else
						ppuMemoryScroll -= event.mouseWheel.delta;
				}
				break;

			default:
				break;
			}
		}

		memoryScroll =    std::max(0, std::min((int)memoryScroll,    0x10000 / 8 - 24));
		ppuMemoryScroll = std::max(0, std::min((int)ppuMemoryScroll, 0x4000 / 8 - 24));

		HANDLE_KEY(sDown, S)	// one cycle
		HANDLE_KEY(fDown, F)	// one frame
		HANDLE_KEY(spaceDown, Space) // pause/unpause
		HANDLE_KEY(iDown, I)	// one instruction
		HANDLE_KEY(pDown, P)	// change palette
		HANDLE_KEY(rDown, R)	// reset
		HANDLE_KEY(bDown, B)	// show bg palette
		HANDLE_KEY(lDown, L)	// change rgb palette
		HANDLE_KEY(wDown, W)	// change window

		windowState = WindowState(int(windowState) + (wDown == 1));
		windowState = WindowState(int(windowState) % 5);

		if (lDown == 1)
		{
			paletteIndex++;
			paletteIndex %= palettes.size();

			std::string paletteName = palettes[paletteIndex];
			bool paletteLoaded = emu.loadPaletteFromPAL("resources/" + paletteName + ".pal");

			NES_LOG_ADD_LINE(emu, paletteLoaded ?
				"Palette loaded (" + paletteName + ")" :
				("Couldnt find palette \"" + paletteName + "\""));
		}

		emu.settings.debugBGPalette ^= (bDown == 1);

		if (spaceDown == 1)
			running ^= true;

		if (fDown == 1 || running)
		{
			while(!emu.frameFinished)
				emu.cycle();

			emu.frameFinished = false;

			updateRegistersText();
		}

		if (sDown == 1)
		{
			emu.cycle();

			emu.frameFinished = false;

			updateRegistersText();
		}

		if (iDown == 1)
		{
			if(!emu.stopCPU)
			for (unsigned int i = 0; i < 3; i++)
			{

				while (emu.CPU_cycles > 0)
					emu.cycle();
				emu.cycle();

				emu.frameFinished = false;

				updateRegistersText();
			}
		}

		if (pDown == 1)
		{
			displayPalette++;
		}

		if (rDown == 1)
		{
			emu.reset();
			emu.loadFromiNES(__argv[0 + 1]);
		}

		while (emu.logLines > 32)
		{
			emu.log.erase(0, emu.log.find("\n") + 1);
			emu.logLines--;
		}

		sf::Vector2f ws = (sf::Vector2f)window.getSize();

		window.clear(sf::Color(128, 128, 128));

////////////////////////////////////////////////////////////////////////////////////////////////

		sf::RectangleShape rect(sf::Vector2f(710, (float)SCREEN.height));
		float screenSize = std::min(ws.x, ws.y);
		sf::RectangleShape screen(sf::Vector2f(screenSize, screenSize));
		sf::Texture screenTex;
		float patternTableSize = std::min(ws.x, ws.y);
		sf::RectangleShape patternTable(sf::Vector2f(patternTableSize, patternTableSize));
		sf::Texture patternTableTex;
		float nametableSize = std::min(ws.x, ws.y) / 2.f;
		sf::RectangleShape nametable(sf::Vector2f(nametableSize, nametableSize));
		sf::Texture nametableTex;
		uint16_t text_y = 10;
		rect.setFillColor(sf::Color(100, 100, 100));

		switch (windowState)
		{
		case CPUDebug:
			registers.setPosition(sf::Vector2f(10, 0));
			window.draw(registers);

			rect.setPosition(sf::Vector2f(280, 0));
			window.draw(rect);

			for (uint16_t i = 0; i < 24; i++)
			{
				text.setString("$" + HEX_2B((i + memoryScroll) * 8));
				text.setPosition(sf::Vector2f(310, i * 40.f));
				window.draw(text);
				for (uint16_t j = 0; j < 8; j++)
				{
					text.setString(HEX_1B(emu.CPU_readMemory1B(j + 8 * (i + memoryScroll))));
					text.setPosition(sf::Vector2f(310 + j * 60 + 160.f, i * 40.f));
					window.draw(text);
				}
			}

			instructions.setString(emu.log);
			window.draw(instructions);

			break;

		case Screen:
			screen.setOrigin(screenSize / 2.f, screenSize / 2.f);
			screen.setPosition(sf::Vector2f(ws.x / 2.f, ws.y / 2.f));
			screenTex.loadFromImage(emu.screen);
			screen.setTexture(&screenTex);
			window.draw(screen);

			break;

		case PPUDebug_Patterntables:
			//patternTable.setOrigin(patternTableSize / 2.f, patternTableSize / 2.f);
			//patternTable.setPosition(sf::Vector2f(ws_ppuDebug_pattern.x / 2.f, ws_ppuDebug_pattern.y / 2.f));

			patternTableTex.loadFromImage(emu.GetPatternTable(0, displayPalette));
			patternTable.setTexture(&patternTableTex);
			window.draw(patternTable);

			patternTable.setPosition(sf::Vector2f(patternTableSize, 0));
			patternTableTex.loadFromImage(emu.GetPatternTable(1, displayPalette));
			window.draw(patternTable);

			break;

		case PPUDebug:
			rect.setPosition(sf::Vector2f(0, 0));
			rect.setSize(sf::Vector2f(665, (float)SCREEN.height));
			window.draw(rect);

			for (uint8_t i = 0; i < 24; i++)
			{
				text.setString("$" + HEX_2B((i + ppuMemoryScroll) * 8));
				text.setPosition(sf::Vector2f(20, 10 + i * 40.f));
				window.draw(text);
				for (uint8_t j = 0; j < 8; j++)
				{
					text.setString(HEX_1B(emu.PPU_readMemory1B(j + 8 * (i + ppuMemoryScroll))));
					text.setPosition(sf::Vector2f(20 + j * 60 + 160.f, 10 + i * 40.f));
					window.draw(text);
				}
			}

			for (uint8_t i = 0; (text_y - 10) / 40 <= 0x17; i++)
			{
				text.setString("Sprite $" + HEX_1B(i) + ": Attributes $" +
					HEX_1B(emu.OAM[i].attributes) + " | Position (" +
					std::to_string(emu.OAM[i].x) + ", " + std::to_string(emu.OAM[i].y) +
					")");

				//text.setPosition(675, 10 + i * 40.f);
				text.setPosition(675, text_y);
				if (!(emu.OAM[i].y >= 240))
				{
					text_y += 40;
					window.draw(text);
				}
			}

			break;

		case PPUDebug_Nametables:
			nametableTex.loadFromImage(emu.GetNametable(0));
			nametable.setTexture(&nametableTex);
			window.draw(nametable);

			nametable.setPosition(nametableSize, 0);
			nametableTex.loadFromImage(emu.GetNametable(1));
			nametable.setTexture(&nametableTex);
			window.draw(nametable);

			nametable.setPosition(0, nametableSize);
			nametableTex.loadFromImage(emu.GetNametable(2));
			nametable.setTexture(&nametableTex);
			window.draw(nametable);

			nametable.setPosition(nametableSize, nametableSize);
			nametableTex.loadFromImage(emu.GetNametable(3));
			nametable.setTexture(&nametableTex);
			window.draw(nametable);

			break;

		default:
			break;
		}

////////////////////////////////////////////////////////////////////////////////////////////////

		window.display();
	}

	return 0;
}