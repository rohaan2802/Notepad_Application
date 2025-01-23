#include <iostream>
#include <Windows.h>
#include"Header.h"
#include<fstream>
using namespace std;



linked_list list;



int main(int argc, char* argv[]) 
{
	maximizeConsole();
	show_menu();
	display_fun();
	HANDLE  rhnd = GetStdHandle(STD_INPUT_HANDLE);  
	DWORD Events = 0;     
	DWORD EventsRead = 0; 
	bool Running = true;
	bool isFull = false;  
	bool screen_clear = false;

	int x = left_, y = top_;
	gotoxy(x, y);
	while (Running) 
	{
		GetNumberOfConsoleInputEvents(rhnd, &Events);

		if (Events != 0) 
		{ 
			INPUT_RECORD eventBuffer[200];
			ReadConsoleInput(rhnd, eventBuffer, Events, &EventsRead);
			for (DWORD i = 0; i < EventsRead; ++i) 
			{
				if (eventBuffer[i].EventType == KEY_EVENT && eventBuffer[i].Event.KeyEvent.bKeyDown)
				{
					if (!screen_clear)
					{
						clear_fun();
						screen_clear = true;
					}
					switch (eventBuffer[i].Event.KeyEvent.wVirtualKeyCode)
					{

					case VK_UP: 
						if (y > top_)
						{
							y--;
						}
						gotoxy(x, y);
						break;
					case VK_DOWN: 
						if (y < bottom_)
						{
							y++;
						}
						gotoxy(x, y);
						break;
					case VK_RIGHT: 
						if (x < right_ - 1) 
						{
							insert_character(' ');

							x++;
						}
						gotoxy(x, y);
						break;
					case VK_LEFT: 
						if (x > left_) 
						{
							x--;
						}						
					gotoxy(x, y);
						break;
					
					case VK_RETURN: 
						if (y < bottom_) 
						{	y++;
							x = left_; 
							insert_character('\n');

							gotoxy(x, y); 
						}
						break;
					
					case VK_F1: 
						system("cls");
						show_menu();
						break;




					case VK_BACK: 
						if (x > left_) 
						{
							x--; 
							list.delete_at_end(y, x);
							gotoxy(left_, y); 
							list.fun1(y); 
						}
						break;






					default:
					break;
					}

					if (y == bottom_ && x == right_ - 1)
					{
						isFull = true;
						warning_msg();  
					}
					
					if (!isFull)
					{

						char ch = eventBuffer[i].Event.KeyEvent.uChar.AsciiChar;


						if ((ch >='A' && ch<='Z') || (ch>='a' && ch<='z'))
						{
							

							
							list.insert_at_end(ch, y, x);
							cout << ch;
							insert_character(ch);
							x++;
							if (x >= right_)
							{
								if (y < bottom_)  
								{
									x = left_;
									y++;
								}
								else
								{
									isFull = true;
									warning_msg();
									gotoxy(x, y);  
									continue;
								}
							}
						}
						
					}


					gotoxy(x, y);

				}

			} 

		}

	} 

	return 0;
}