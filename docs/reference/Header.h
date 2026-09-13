#pragma once

#ifndef Header_H
#define Header_H

#include <iostream>
#include <Windows.h>
#include<conio.h>
#include<fstream>
using namespace std;

const int left_ = 1;
const int top_ = 0;
const int right_ = 143;
const int bottom_ = 31;
int x = left_, y = top_;
ofstream obj1;
ifstream obj_2;
bool flag = false;
bool safe_flag = false;
bool isFull = false;
char file_[20];
void load_file(int& x,int& y,bool& isFull);
void create_file();
void insert_character(char ch);
void save_file();

int confrim_msg() 
{
    return MessageBox(NULL, L"Do you want to save the file before exiting?", L"Confirm Exit", MB_YESNO | MB_ICONQUESTION);
}

void saved_msg()
{
    MessageBox(NULL, L"File saved successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
}


void  show_menu() 
{
    label:
    int choice = 0,val;
    cout << "\n\n\n\n\n\n\n\n";
    cout << "\t\t\t\t\t\t\t\t************************ MAIN MENU:   ************************\n";
    cout << "\t\t\t\t\t\t\t\t************************ 1. New File  ************************\n";
    cout << "\t\t\t\t\t\t\t\t************************ 2. Load File ************************\n";
    cout << "\t\t\t\t\t\t\t\t************************ 3. Save File ************************\n";
    cout << "\t\t\t\t\t\t\t\t************************ 4.   Exit    ************************\n\n";
    cout << "\t\t\t\t\t\t\t\t\t\t\tEnter your choice: ";
    cin >> choice;

    switch (choice) 
    {
    case 1:
        system("cls");
        create_file();
        system("pause");
        system("cls");
        return ;
        break;
    case 2:
        system("cls");
        load_file(x,y,isFull);
        cout << "\n\n\n\n\n\n\t\t\t\t\t\t";
        system("pause");
        system("cls");
        return ;
        break;
    case 3:
        if (safe_flag==true)
        {
            saved_msg();
        }
        else
        {
            system("cls");
            cout << "\n\n\n\n\n\n\t\t\t\t\t\t";
            cout << "\n\n\n\t\t\t\t\t\t\t\t\t************** CREATE FILE FIRST OR LOAD FILE  **************\n\n\t\t\t\t\t\t\t\t\t";
            system("pause");
            system("cls");
        }
             system("cls");
            goto label;
        
        return ;
        break;
    case 4:
        if (safe_flag==true)
        {
            val = confrim_msg();
            if (val == IDYES)
            {
                saved_msg();
            }
        }
        exit(0);
        break;
    default:
        cout << "\n\n\n\t\t\t\t\t\t\t\t\t************** INVALID CHOICE  **************\n\n\t\t\t\t\t\t\t\t\t";
        system("pause");
        system("cls");
        show_menu();
        break;
    }
}



void gotoxy(int x, int y)
{
	COORD c = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void clear_fun()
{
	for (int i = 0; i < 30; i++)
	{
		gotoxy(1, i);
		cout << "                                                                                                                  ";
	}
	gotoxy(1, 0);
}

void warning_msg()
{
	MessageBox(NULL, L"Writing space is full.", L"Space Full", MB_ICONWARNING | MB_OK);
}

void maximizeConsole()
{
	HWND consoleWindow = GetConsoleWindow();
	ShowWindow(consoleWindow, SW_MAXIMIZE);
}

void display_fun() 
{
	cout << "| Welcome to the Notepad.                                                                                                                      | SEARCH" << endl;
	cout << "| This is the area where you are supposed to write the content.                                                                                |" << endl;
	for (int i = 0; i < 30; i++)
		cout << "|                                                                                                                                              |" << endl;
	cout << "|______________________________________________________________________________________________________________________________________________|" << endl << endl;
	cout << "  WORD SUGGESTIONS\n\n";
}

class Node
{
public:
    char data;
    Node* left;
    Node* right;
    Node* up;
    Node* down;

    Node(char value)
    {
        data = value;
        left = right = up = down = nullptr;
    }
};

class linked_list
{
private:
    Node* head;

public:
    linked_list()
    {
        head = nullptr;
    }

    void insert_at_end(char value, int &x, int &y)
    {
        Node* newNode = new Node(value);

        if (head == nullptr)
        {
            head = newNode;
            return;
        }

        Node* current = head;
        Node* prev = nullptr;

        for (int i = 0; i < x; i++)
        {
            if (current->down != nullptr)
            {
                prev = current;
                current = current->down;
            }
            else
            {
                Node* new_row = new Node('\0');
                current->down = new_row;
                new_row->up = current;
                prev = current;
                current = new_row;
            }
        }

        Node* new_current = current;
        for (int j = 0; j < y; j++)
        {
            if (new_current->right != nullptr)
            {
                new_current = new_current->right;
            }
            else
            {

                Node* newColNode = new Node('\0');
                new_current->right = newColNode;
                newColNode->left = new_current;
                new_current = newColNode;
            }
        }

        newNode->left = new_current->left;
        newNode->right = new_current;
        if (new_current->left != nullptr)
        {
            new_current->left->right = newNode;
        }
        new_current->left = newNode;
        if (prev != nullptr)
        {
            Node* upper_ch = prev;
            for (int j = 0; j < y; j++)
            {
                if (upper_ch->right != nullptr)
                {
                    upper_ch = upper_ch->right;
                }
            }
            newNode->up = upper_ch;
            upper_ch->down = newNode;
        }
      
    }



   

    void delete_at_end(int x, int y)
    {
        Node* current = head;
        for (int i = 0; i < x; i++)
        {
            if (current->down != nullptr)
            {
                current = current->down;
            }
            else
            {
                return; 
            }
        }

        Node* new_current = current;
        for (int j = 0; j < y; j++)
        {
            if (new_current->right != nullptr)
            {
                new_current = new_current->right;
            }
            else
            {
                return; 
            }
        }

        if (new_current->right != nullptr)
        {
            Node* nextNode = new_current->right;
            new_current->data = nextNode->data; 
            new_current->right = nextNode->right;

            if (nextNode->right != nullptr)
            {
                nextNode->right->left = new_current;
            }

            delete nextNode; 
        }
        else
        {
            new_current->data = '\0';
        }

        delete_from_file();
    }


    void fun1(int x)
    {
        Node* current = head;
        for (int i = 0; i < x; i++)
        {
            if (current->down != nullptr)
            {
                current = current->down;
            }
            else
            {
                return; 
            }
        }

        gotoxy(1, x); 
        for (Node* new_current = current; new_current != nullptr; new_current = new_current->right)
        {
            cout << new_current->data; 
        }
        while (current != nullptr)
        {
            cout << " ";
            current = current->right;
        }
    }


    void delete_from_file()
    {
        obj1.open(file_, ios::trunc);
        if (obj1.is_open())
        {
            Node* currentRow = head;
            while (currentRow != nullptr)
            {
                Node* currentChar = currentRow;
                while (currentChar != nullptr)
                {
                    if (currentChar->data != '\0') 
                    {
                        obj1 << currentChar->data; 
                    }
                    currentChar = currentChar->right; 
                }
                obj1 << '\n'; 
                currentRow = currentRow->down; 
            }
            obj1.close();   
        }
       
    }


};


linked_list list1;

void create_file()
{
    cout << "\n\n\n\n\n\n\n\n";
    cout << "\t\t\t\t\t\t\t\tEnter the new file name :         ";
    char ch;
    int i = 0;
    cin.ignore();
    while (i < 12 && (ch = cin.get()) != '\n')
    {

        file_[i++] = ch;
    }
    file_[i++] = '.';
    file_[i++] = 't';
    file_[i++] = 'x';
    file_[i++] = 't';
    file_[i++] = '\0';
    obj1.open(file_);
    if (obj1.is_open())
    {

        cout << "\n\n\n\n";
        cout << "\t\t\t\t\t\t\t\tFile " << file_ << " created successfully.\n\n\t\t\t\t\t\t\t\t\t";
        for (int j = 0; j < 19; j++)
        {
            file_[j] = file_[j];
        }
        obj1.close();
        safe_flag = true;

        return;
    }
    else
    {
        cout << "\n\n\n\n";
        cout << "\t\t\t\t\t\t\t\tError creating file.\n\n\t\t\t\t\t\t\t\t";
        system("pause");
        system("cls");
        create_file();
    }
}

void load_file(int &x,int &y,bool &isFull)
{
    cout << "\n\n\n\n\n\n\n\n";
    cout << "\t\t\t\t\t\t\t\tEnter the file name to load:       ";
    char ch;
    int i = 0;
    cin.ignore();
    while (i < 12 && (ch = cin.get()) != '\n')
    {
        file_[i++] = ch;
    }
    file_[i++] = '.';
    file_[i++] = 't';
    file_[i++] = 'x';
    file_[i++] = 't';
    file_[i++] = '\0';

    obj_2.open(file_);

    if (obj_2.is_open())
    {
        cout << "\n\n\n\n";
        cout << "\t\t\t\t\t\t\t\tFILE " << file_ << " EXISTS. LOADING DATA.......\n\n\t\t\t\t\t\t\t\t";
        system("pause");
        system("cls");
        display_fun();
     
        gotoxy(x, y);

        char file_char;
        while (obj_2.get(file_char) && !isFull)  
        {
            if ((file_char >= 'A' && file_char <= 'Z') || (file_char >= 'a' && file_char <= 'z') || file_char == ' ' || file_char == '\n')
            {
                if (file_char == '\n')
                {
                    y++;
                    x = left_;  
                }
                else
                {
                    list1.insert_at_end(file_char, y, x);
                    cout << file_char;
                    x++;
                }

                if (x >= right_)
                {
                    x = left_;  
                    y++;  
                }

                if (y == bottom_ && x == right_ - 1)
                {
                    isFull = true;
                    warning_msg();  
                }

                gotoxy(x, y);
            }


        }


        obj_2.close();



        if (!isFull)
        {
            char ch;
            while ((ch = _getch()) != 27)
            {
                if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
                {
                    list1.insert_at_end(ch, y, x);
                    cout << ch;
                    insert_character(ch);
                    x++;

                    if (x >= right_)
                    {
                        x = left_;
                        y++;
                    }

                    if (y == bottom_ && x == right_ - 1)
                    {
                        isFull = true;
                        warning_msg();
                        break;
                    }

                    gotoxy(x, y);
                }
                    else if (ch == '\b') 
                    {
                        if (x > left_)
                        {
                            x--;
                            list1.delete_at_end(y, x); 
                            gotoxy(x, y);
                            cout << ' '; 
                            gotoxy(x, y); 
                        }
                    }
                    else if (ch == '\r') 
                    {
                        y++; 
                        x = left_; 
                        gotoxy(x, y); 
                    }
                    else if (ch == ' ') 
                {
                    list1.insert_at_end(' ', y, x);
                    cout << ' '; 
                    x++;

                    if (x >= right_)
                    {
                        x = left_;
                        y++;
                    }

                    if (y == bottom_ && x == right_ - 1)
                    {
                        isFull = true;
                        warning_msg();
                        break;
                    }

                    gotoxy(x, y);
                }
            }
            system("cls");
            show_menu();
        }
    }
    else
    {
        cout << "\n\n\n\n";
        cout << "\t\t\t\t\t\t\t\tFILE " << file_ << " DOESN'T EXIST. CREATE A NEW FILE......\n\n\t\t\t\t\t\t\t\t";
        system("pause");
        system("cls");

        obj1.open(file_, ios::out);
        if (obj1.is_open())
        {
            cout << "\n\n\n\n\n\n\n\n\n";

            cout << "\t\t\t\t\t\t\t\tFile " << file_ << " created successfully.\n\n\t\t\t\t\t\t\t\t\t";
            safe_flag = true;
            system("pause");
            system("cls");

            for (int j = 0; j < i; j++)
            {
                file_[j] = file_[j];
            }
            file_[i] = '\0';
            obj1.close();

        }

       
        obj1.close();

    }
}


void insert_character(char ch) 
{
    obj1.open(file_, ios::app);
    if (obj1.is_open()) 
    {
        obj1 << ch; 
        obj1.close(); 
    }
    
}

void save_file()
{
    list1.delete_from_file();
}



#endif