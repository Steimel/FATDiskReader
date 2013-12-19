/*
  Tommy Steimel
  directory.cpp
*/

#include <iostream>
#include "directory.h"

using namespace std;

DirectoryEntry::DirectoryEntry(unsigned char entry[])
{
  DIR_Attr = entry[11];
  for(int i = 0; i < 11; i++)
    {
      DIR_Name[i] = entry[i];
    }
  ATTR_LONG_NAME = ((int)DIR_Attr == 15);
  
  // Long Directory Stuff
  LDIR_Ord = entry[0];
  for(int i = 1; i < 11; i++)
    {
      LDIR_Name1[i-1] = entry[i];
    }
  LDIR_Type = entry[12];
  LDIR_Chksum = entry[13];
  for(int x = 14; x < 26; x++)
    {
      LDIR_Name2[x-14] = entry[x];
    }
  LDIR_FstClusLO = (((int)entry[27])<<8)+(int)entry[26];
  for(int x = 28; x < 32; x++)
    {
      LDIR_Name3[x-28] = entry[x];
    }
  // Short Directory Stuff
  DIR_CrtTimeTenth = entry[13];
  DIR_CrtTime[0] = entry[14];
  DIR_CrtTime[1] = entry[15];
  DIR_CrtDate[0] = entry[16];
  DIR_CrtDate[1] = entry[17];
  DIR_LstAccDate[0] = entry[18];
  DIR_LstAccDate[1] = entry[19];
  DIR_WrtTime[0] = entry[22];
  DIR_WrtTime[1] = entry[23];
  DIR_WrtDate[0] = entry[24];
  DIR_WrtDate[1] = entry[25];
  DIR_FstClusLO = (((int)entry[27])<<8)+(int)entry[26];
  int s = (((int)entry[31])<<8)+(int)entry[30];
  s = s << 16;
  s = s + (((int)entry[29])<<8)+(int)entry[28];
  DIR_FileSize = s;


  // Copy into c_str
  for(int x = 0; x < 32; x++)
    {
      c_str[x] = entry[x];
    }
}

bool DirectoryEntry::isValid() const
{
  // blank
  if(((int)DIR_Name[0]) == 229) return false;
  if(((int)DIR_Name[0]) == 0) return false;
  
  // volume id
  if(((int)DIR_Attr) == 8) return false;

  return true;
}

bool DirectoryEntry::isEmpty() const
{
  if(((int)DIR_Name[0]) == 229) return true;
  if(((int)DIR_Name[0]) == 0) return true;
  return false;
}

bool DirectoryEntry::isEnd() const
{
  if(((int)DIR_Name[0]) == 0) return true;
  return false;
}

bool DirectoryEntry::isDirectory() const
{
  return (((((int)DIR_Attr)>>4) % 2) == 1);
}

string* DirectoryEntry::getName() const
{
  if(ATTR_LONG_NAME)
    {
      char name[14];
      for(int x = 0; x < 5; x++)
	{
	  name[x] = LDIR_Name1[x*2];
	}
      for(int x = 0; x < 6; x++)
	{
	  name[x+5] = LDIR_Name2[x*2];
	}
      for(int x = 0; x < 2; x++)
	{
	  name[x+11] = LDIR_Name3[x*2];
	}
      name[13] = '\0';
      return new string(name);
    }


  char name[12];
  bool done = false;
  int i = 0;
  if(isDirectory()) // Directory Name (no ext)
    {
      for(; i < 11 && !done; i++)
	{
	  name[i] = DIR_Name[i];
	  if(name[i] == ' ')
	    {
	      name[i] = '\0';
	      done = true;
	    }
	}
      if(i == 11)
	{
	  name[11] = '\0';
	}
    }
  else // Filename w/ extension
    {
      int i = 0;
      // Get Filename
      for(; i < 8 && !done; i++)
	{
	  name[i] = DIR_Name[i];
	  if(name[i] == ' ')
	    {
	      name[i] = '.';
	      i--;
	      done = true;
	    }
	}
      if(i == 8)
	{
	  name[8] = '.';
	}
      // add extension
      int j = 0;
      for(; j < 3; j++)
	{
	  name[i+j+1] = DIR_Name[8+j];
	  if(name[i+j+1] == ' ')
	    {
	      name[i+j+1] = '\0';
	    }
	}
      name[i+4] = '\0';
      
      // Check if any extension was added
      if(name[i+1] == '\0')
	{
	  // if not, take away the .
	  name[i] = '\0';
	}
    }
  return new string(name);
}

void DirectoryEntry::setFirstCluster(int clus)
{
  DIR_FstClusLO = clus;
  c_str[26] = clus % 256;
  c_str[27] = clus >> 8;
}


void DirectoryEntry::setFileSize(int size)
{
  DIR_FileSize = size;
  c_str[28] = (unsigned char)(size % 256);
  size = size / 256;
  c_str[29] = (unsigned char)(size % 256);
  size = size / 256;
  c_str[30] = (unsigned char)(size % 256);
  size = size / 256;
  c_str[31] = (unsigned char)size;
}
