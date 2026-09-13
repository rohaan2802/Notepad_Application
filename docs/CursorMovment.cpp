#include <iostream>
#include <Windows.h>
using namespace std;

int main(int argc, char* argv[])
{

	system("cls");
	HANDLE  rhnd = GetStdHandle(STD_INPUT_HANDLE);  // handle to read console

	DWORD Events = 0;     // Event count
	DWORD EventsRead = 0; // Events read from console

	bool Running = true;
	
	int x = 1, y = 1;
	gotoxy(x, y);
	//programs main loop
	while (Running) {

		// gets the systems current "event" count
		GetNumberOfConsoleInputEvents(rhnd, &Events);

		if (Events != 0) { // if something happened we will handle the events we want

			// create event buffer the size of how many Events
			INPUT_RECORD eventBuffer[200];

			// fills the event buffer with the events and saves count in EventsRead
			ReadConsoleInput(rhnd, eventBuffer, Events, &EventsRead);
			
			// loop through the event buffer using the saved count
			for (DWORD i = 0; i < EventsRead; ++i) {

				// check if event[i] is a key event && if so is a press not a release
				if (eventBuffer[i].EventType == KEY_EVENT && eventBuffer[i].Event.KeyEvent.bKeyDown) {

										
					// check if the key press was an arrow key
					switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode) {
					
					case VK_UP: //up
						y--;
						gotoxy(x, y);
						break;
					case VK_DOWN: //down
						y++;
						gotoxy(x, y);
						break;
					case VK_RIGHT: //right
						x++;
						gotoxy(x, y);
						break;
					case VK_LEFT: //left
						x--;
						gotoxy(x, y);
						break;
					default:        
						cout << eventBuffer->Event.KeyEvent.uChar.AsciiChar;
						x++;
						gotoxy(x, y);
						break;
					}
				}

			} // end EventsRead loop

		}

	} // end program loop

	return 0;
}