/*
  Tommy Steimel
  fat.cpp
*/

#include <iostream>
#include <string>
#include <fcntl.h>
#include <math.h>
#include "fileSystem.h"
#include <vector>


using namespace std;

void printDirEnts(vector<DirectoryEntry*> &e);
string toLowerCase(string in);
string trim(string in);

int main(int argc, const char* argv[])
{
  // Check number of arguments
  if(argc != 2)
    {
      cout << "Error: wrong number of arguments" << endl;
      return 1;
    }

  // Get the file system's descriptor
  int fileDes = open(argv[1], O_RDWR);
  if(fileDes < 0)
    {
      cout << "Error: file not found" << endl;
      return 1;
    }

  // Open and read file system
  FileSystem f(fileDes);
 
  // Start taking commands
  string input = "";
  do
    {
      // Prompt and get input
      cout << ": " << toLowerCase(f.getCurrentDirectoryPath()) << " > ";
      getline(cin, input);
      
      // Parse input
      if(input.substr(0,3) == "cd ")
	{
	  // Change working directory
	  f.setCurrentDirectory(input.substr(3));
	}
      else if(input == "ls")
	{
	  // List files of current working directory
	  vector<DirectoryEntry*> e = f.curEntries;
	  printDirEnts(e);
	}
      else if(input.substr(0,3) == "ls ")
	{
	  // List files of given directory
	  
	  vector<DirectoryEntry*> e;
	  
	  // if path-follow is successful, print
	  bool success = f.LSPath(e, input.substr(3));
	  if(success)
	    {
	      printDirEnts(e);
	    }
	}
      else if(input.substr(0,5) == "cpin ")
	{
	  // Copy into the file system
	  string rest = input.substr(5);
	  int spacePos = rest.find(" ");
	  if(spacePos == string::npos || spacePos == 0)
	    {
	      cout << "Command not recognized." << endl;
	    }
	  else
	    {
	      string source = trim(rest.substr(0,spacePos));
	      string dest = trim(rest.substr(spacePos+1));
	     
	      if(f.CopyIn(source, dest))
		{
		  cout << "File copied successfully!" << endl;
		}
	      else
		{
		  cout << "File was unable to be copied." << endl;
		}
	    }
	}
      else if(input.substr(0,6) == "cpout ")
	{
	  // Copy out of the file system
	  string rest = input.substr(6);
	  int spacePos = rest.find(" ");
	  if(spacePos == string::npos || spacePos == 0)
	    {
	      cout << "Command not recognized." << endl;
	    }
	  else
	    {
	      string source = trim(rest.substr(0,spacePos));
	      string dest = trim(rest.substr(spacePos+1));
	      if(f.CopyOut(source, dest))
		{
		  cout << "File copied successfully!" << endl;
		}
	      else
		{
		  cout << "File was unable to be copied." << endl;
		}
	    }
	}
      else if(input == "exit")
	{
	  // Exit the program
	}
      else if(input == "")
	{
	  // Ignore simple enter key hits
	}
      else
	{
	  // Command not recognized
	  cout << "Command not recognized." << endl;
	}
    }while(input != "exit");
  
  // close files
  close(fileDes);
}

/*
void printDirEnts(vector<DirectoryEntry*> &e)
{
  for(int x = 0; x < e.size(); x++)
    {
      cout << *(e[x]->getName()) << " : " << e[x]->DIR_FileSize  << endl;
    }
}
*/


// Print directory entries (used for ls)
void printDirEnts(vector<DirectoryEntry*> &e)
{
  vector<string> toPrint;

  // put together a list of things to be printed
  for(int x = 0; x < e.size(); x++)
    {
      string cur = "";
      if(!e[x]->ATTR_LONG_NAME)
	{
	  // not a long name
	  if(e[x]->isDirectory())
	    {
	      cur = "<DIR> ";
	    }
	  toPrint.push_back(cur + *(e[x]->getName()));
	}
      else
	{
	  // retrieve entire long name
	  vector<DirectoryEntry*> longName;

	  bool done = false;
	  for(; x < e.size() && !done; x++)
	    {
	      if(e[x]->ATTR_LONG_NAME)
		{
		  longName.push_back(e[x]);
		}
	      else
		{
		  done = true;
		  x--;
		}
	    }

	  // put long name together
	  string fullLongName = "";
	  for(int i = longName.size()-1; i >= 0; i--)
	    {
	      fullLongName = fullLongName + *(longName[i]->getName());
	    }
	  
	  // x now has the index of the short entry
	  // named by the long name, so check if directory
	  if(e[x]->isDirectory())
	    {
	      cur = "<DIR> ";
	    }
	  toPrint.push_back(cur + fullLongName);
	}
    }

  // finally, print
  for(int x = 0; x < toPrint.size(); x++)
    {
      cout << toPrint[x] << endl;
    }
}


// used to compare paths while ignoring case
string toLowerCase(string in)
{
  char str[in.length()+1];
  for(int x = 0; x < in.length(); x++)
    {
      str[x] = tolower(in.c_str()[x]);
    }
  str[in.length()] = '\0';
  return string(str);
}

// remove spaces to avoid bad input
string trim(string in)
{
  int spaceCount = 0;
  for(int x = 0; x < in.size(); x++)
    {
      if(in.c_str()[x] == ' ')
	{
	  spaceCount++;
	}
    }

  char newStr[in.size()-spaceCount+1];
  int curPos = 0;
  for(int x = 0; x < in.size(); x++)
    {
      if(in.c_str()[x] != ' ')
	{
	  newStr[curPos] = in.c_str()[x];
	  curPos++;
	}
    }
  newStr[in.size()-spaceCount] = '\0';
  return string(newStr);
}
