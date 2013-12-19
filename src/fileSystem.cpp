/*
  Tommy Steimel
  fileSystem.cpp
 */

#include "fileSystem.h"
#include "directory.h"
#include <iostream>
#include <math.h>
#include <vector>
#include <string>
#include <cctype>
#include <fcntl.h>
#include <time.h>
#include <sstream>

using namespace std;

// used to compare paths while ignoring case
string toUpperCase(string in)
{
  char str[in.length()+1];
  for(int x = 0; x < in.length(); x++)
    {
      str[x] = toupper(in.c_str()[x]);
    }
  str[in.length()] = '\0';
  return string(str);
}


// Constructs a file system object from a file descriptor.
// Fills the current directory entries vector with the
// root directory entries.
FileSystem::FileSystem(int fileDes)
{
  // Open and read the file system
  unsigned char buf[10];
  int bytesRead;
  
  fDes = fileDes;

  //BPB_BytsPerSec  11 2
  bytesRead = pread(fileDes, buf, 2, 11);
  if(bytesRead == 2)
    {
      BPB_BytsPerSec = (((int)buf[1]) << 8) + (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_BytsPerSec" << endl;
    }
 


  //BPB_SecPerClus   13 1
  bytesRead = pread(fileDes, buf, 1, 13);
  if(bytesRead == 1)
    {
      BPB_SecPerClus = (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_SecPerClus" << endl;
    }



  //BPB_RsvdSecCnt  14 2
  bytesRead = pread(fileDes, buf, 2, 14);
  if(bytesRead == 2)
    {
      BPB_RsvdSecCnt = (((int)buf[1]) << 8) + (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_RsvdSecCnt" << endl;
    }


  //BPB_NumFATs   16 1
  bytesRead = pread(fileDes, buf, 1, 16);
  if(bytesRead == 1)
    {
      BPB_NumFATs = buf[0];
    }
  else
    {
      cout << "Error reading BPB_NumFATs" << endl;
    }


  //BPB_RootEntCnt  17 2
  bytesRead = pread(fileDes, buf, 2, 17);
  if(bytesRead == 2)
    {
      BPB_RootEntCnt = (((int)buf[1]) << 8) + (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_RootEntCnt" << endl;
    }


  //BPB_TotSec16  19 2
  bytesRead = pread(fileDes, buf, 2, 19);
  if(bytesRead == 2)
    {
      BPB_TotSec16 = (((int)buf[1]) << 8) + (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_TotSec16" << endl;
    }


  //BPB_FatSz16  22 2
  bytesRead = pread(fileDes, buf, 2, 22);
  if(bytesRead == 2)
    {
      BPB_FatSz16 = (((int)buf[1]) << 8) + (int)buf[0];
    }
  else
    {
      cout << "Error reading BPB_FatSz16" << endl;
    }


  //BPB_TotSec32  32 4
  bytesRead = pread(fileDes, buf, 4, 32);
  if(bytesRead == 4)
    {
      int tot = (((int)buf[3]) << 8) + (int)buf[2];
      tot = tot << 16;
      tot = tot + (((int)buf[1]) << 8) + (int)buf[0];
      BPB_TotSec32 = tot;
    }
  else
    {
      cout << "Error reading BPB_TotSec32" << endl;
    }

  //RootDirSectors
  RootDirSectors = ((BPB_RootEntCnt*32.0)+
			       (BPB_BytsPerSec-1.0))/
			      BPB_BytsPerSec;

  
  FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs*BPB_FatSz16) + RootDirSectors;


  getRootDirectoryEntries(curEntries);
}

unsigned int FileSystem::FirstSectorOfCluster(int n)
{
  return ((n-2)*BPB_SecPerClus)+FirstDataSector;
}

unsigned int FileSystem::FATSecNum(int n)
{
  return BPB_RsvdSecCnt+((n*2)/BPB_BytsPerSec);
}

unsigned int FileSystem::FATEntOffset(int n)
{
  return ((n*2)%BPB_BytsPerSec);
}

bool FileSystem::setDirectory(vector<DirectoryEntry*> &eIn, vector<DirectoryEntry*> &pIn, string path)
{
   // parse path
  vector<string> p;
  if(!getPath(p, path))
    {
      // path parsing failed
      return false;
    }

  // Work with a copy of the current path and entries, that way failures
  // don't commit
  vector<DirectoryEntry*> pathCopy = pIn;
  vector<DirectoryEntry*> enCopy = eIn;

  // Follow path
  for(int x = 0; x < p.size(); x++)
    {
      int y = findEntry(enCopy, p[x]);
      if(y < 0)
	{
	  cout << "Error: could not find directory: " << p[x] << endl;
	  return false;
	}
      if(!enCopy[y]->isDirectory())
	{
	  // this entry isn't a directory, so return error
	  cout << "Error: " << p[x] << " is not a directory." << endl;
	  return false;
	}
      
      // Found Directory Entry to Follow
      
      // change path
      if(p[x].compare(".") == 0)
	{
	  // Points to itself, no change to path
	}
      else if(p[x].compare("..") == 0)
	{
	  // Points to its parent, so pop from path
	  pathCopy.pop_back();
	}
      else
	{
	  pathCopy.push_back(enCopy[y]);
	}
      
      // Find the clusters that make up the directory
      // First, check if root
      if(enCopy[y]->DIR_FstClusLO == 0)
	{
	  // is root
	  getRootDirectoryEntries(enCopy);
	}
      else
	{
	  // find clusters using FAT Table
	  vector<int> clusters;
	  if(!getClusters(clusters, enCopy[y]->DIR_FstClusLO))
	    {
	      // Error getting clusters
	      cout << "Could not get clusters for dir: " << p[x] << endl;
	      return false;
	    }
	  
	  // Clear the entries
	  enCopy.clear();
	  
	  // Read entries from clusters
	  int clusSize = BPB_SecPerClus*BPB_BytsPerSec;
	  for(int i = 0; i < clusters.size(); i++)
	    {
	      // For each cluster, read directory entries until done
	      int start = FirstSectorOfCluster(clusters[i]);
	      start = start * BPB_BytsPerSec;
	      getDirEntries(enCopy, start, clusSize);
	    }
	}
    }
  // Path followed successfully, so commit path:
  pIn = pathCopy;
  eIn = enCopy;
  
  return true;
}

void FileSystem::setCurrentDirectory(string path)
{
  setDirectory(curEntries, currentPath, path);
}
  

int FileSystem::findEntry(vector<DirectoryEntry*> &e, string name)
{
  name = toUpperCase(name);

  for(int x = 0; x < e.size(); x++)
    {
      if(e[x]->ATTR_LONG_NAME)
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
		  x-=2;
		}
	    }

	  // put long name together
	  string fullLongName = "";
	  for(int i = longName.size()-1; i >= 0; i--)
	    {
	      fullLongName = fullLongName + *(longName[i]->getName());
	    }
	  fullLongName = toUpperCase(fullLongName);
	  
	  // if match, return x+1 = index of short name directory
	  if(toUpperCase(fullLongName).compare(name) == 0)
	    {
	      return x+1;
	    }
	}
      else
	{
	  // not a long directory name
	  if(toUpperCase(*(e[x]->getName())).compare(name) == 0)
	    {
	      return x;
	    }
	}
    }

  // not found, return -1
  return -1;
}


bool FileSystem::getPath(vector<string> &p, string path)
{
  // method to parse a path
  int x = 0;
  while(x < path.length())
    {
      int hold = x;
      x = path.find('/',x);
      if(x == string::npos)
	{
	  p.push_back(toUpperCase(path.substr(hold)));
	  break;
	}
      if(hold == x)
	{
	  if(x == 0)
	    {
	      // first char is a slash, ignore
	      x++;
	      continue;
	    }
	  else
	    {
	      // two /'s in a row
	      cout << "Error parsing path: " << path << endl;
	      return false;
	    }
	}
      p.push_back(toUpperCase(path.substr(hold,x-hold)));
      x++;
    }
  return true;
}

bool FileSystem::getClusters(vector<int> &c, int start)
{
  c.clear();
  c.push_back(start);
  bool done = false;
  unsigned char buf[2];
  while(!done)
    {
      // Calculate position of entry in FAT table
      int readStart = FATSecNum(c.back())*BPB_BytsPerSec;
      readStart = readStart + FATEntOffset(c.back());
      // Read the FAT entry
      pread(fDes, buf, 2, readStart);
      int entry = (((int)buf[1]) << 8) + (int)buf[0];
    
      if(entry >= 65528)
	{
	  // End of file
	  done = true;
	}
      else if(entry == 65527)
	{
	  // Bad cluster
	  cout << "Error getting clusters: Bad cluster found" << endl;
	  return false;
	}
      else
	{
	  // Use as next entry
	  c.push_back(entry);
	}
    }
  return true;
}

void FileSystem::getDirEntries(vector<DirectoryEntry*> &e, int start, int max)
{
  // Get the directory entries from the region
  // starting at byte start, max being the size
  // of the region
  
  unsigned char dirEntry[32];
  bool done = false;
  
  for(int x = 0; x < max && !done; x=x+32)
    {
      // Read directory entry
      pread(fDes, dirEntry, 32, start+x);
      // create it
      DirectoryEntry* entry = new DirectoryEntry(dirEntry);
      if(entry->isValid())
	{
	  // if the entry is a valid directory entry, add it
	  e.push_back(entry);
	}
      else if(entry->isEnd())
	{
	  done = true;
	}
    }
}

string FileSystem::getCurrentDirectoryPath()
{
  // Build path
  string path = "/";
  for(int x = 0; x < currentPath.size(); x++)
    {
      path = path + *(currentPath[x]->getName()) + "/";
    }
  return path;
}

void FileSystem::getRootDirectoryEntries(vector<DirectoryEntry*> &v)
{ 
  // clear current entries
  v.clear();
  
  // Get start of root directory
  int r = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FatSz16);
  r = r * BPB_BytsPerSec;
  
  // Get size of root directory
  int rSize = RootDirSectors * BPB_BytsPerSec;
  
  // Call handy function
  getDirEntries(v, r, rSize);
}

bool FileSystem::LSPath(vector<DirectoryEntry*> &e, string path)
{
  e = curEntries;
  
  // create a copy of the current path to use
  vector<DirectoryEntry*> pCopy = currentPath;
  
  return setDirectory(e, pCopy, path);
}

bool FileSystem::CopyOut(string source, string dest)
{
  vector<string> sourcePath;
  if(!getPath(sourcePath, source))
    {
      // Error parsing path
      return false;
    }
  
  // Create copies of entries and path
  vector<DirectoryEntry*> e = curEntries;
  vector<DirectoryEntry*> p = currentPath;


  // save the name of the file and remove it from path:
  string fileName = sourcePath.back();
  sourcePath.pop_back();
  
  // get string of new path:
  string newPath = "";
  for(int x = 0; x < sourcePath.size(); x++)
    {
      newPath = newPath + sourcePath[x] + "/";
    }
  
  // follow path
  if(!setDirectory(e,p,newPath))
    {
      cout << "Error: bad path" << endl;
      return false;
    }

  // search entries for file:
  int index = findEntry(e, fileName);

  // Error checking:
  if(index < 0)
    {
      cout << "Error: " << fileName << " not found." << endl;
      return false;
    }
  if(e[index]->isDirectory())
    {
      cout << "Error: cannot copy directories, sorry!" << endl;
      return false;
    }
  if(e[index]->ATTR_LONG_NAME)
    {
      cout << "Error: attempting to copy a long name directory entry" << endl;
      return false;
    }

  // Retrieve clusters
  vector<int> clusters;
  if(!getClusters(clusters, e[index]->DIR_FstClusLO))
    {
      cout << "Error retrieving clusters of file: " << fileName << endl;
      return false;
    }

  // open file to copy to

  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  int destFile = creat(dest.c_str(), mode);
  if(destFile < 0)
    {
      cout << "Error creating file: " << dest << endl;
      return false;
    }

  int bytesWritten = 0;
  int cSize = BPB_SecPerClus*BPB_BytsPerSec;
  int fSize = e[index]->DIR_FileSize;
  // create a buffer to write a cluster at a time
  char buf[cSize];

  for(int x = 0; x < clusters.size(); x++)
    {
      // get start position
      int start = FirstSectorOfCluster(clusters[x]);
      start = start * BPB_BytsPerSec;

      // how much left to copy
      int toCopy = fSize - bytesWritten;

      if(toCopy >= cSize)
	{
	  // If there is enough file left, copy the whole sector
	  pread(fDes, buf, cSize, start);
	  pwrite(destFile, buf, cSize, bytesWritten);
	  bytesWritten += cSize;
	}
      else
	{
	  // Otherwise, only copy to the end of the file
	  pread(fDes, buf, toCopy, start);
	  pwrite(destFile, buf, toCopy, bytesWritten);
	  bytesWritten += toCopy;
	}
    }

  // close file
  close(destFile);

  return true;
}


void FileSystem::refreshDirectory()
{
  if(currentPath.size() == 0)
    {
      // root directory
      getRootDirectoryEntries(curEntries);
    }
  else
    {
      // find clusters using FAT Table
      vector<int> clusters;
      int dotPos = findEntry(curEntries, ".");
      if(dotPos < 0)
	{
	  cout << "Error finding '.' directory" << endl;
	  return;
	}
      getClusters(clusters, curEntries[dotPos]->DIR_FstClusLO);
      
      // Clear the entries
      curEntries.clear();
      
      // Read entries from clusters
      int clusSize = BPB_SecPerClus*BPB_BytsPerSec;
      for(int i = 0; i < clusters.size(); i++)
	{
	  // For each cluster, read directory entries until done
	  int start = FirstSectorOfCluster(clusters[i]);
	  start = start * BPB_BytsPerSec;
	  getDirEntries(curEntries, start, clusSize);
	}
    }
}



bool FileSystem::CopyIn(string source, string dest)
{ 
  vector<string> destPath;
  if(!getPath(destPath, dest))
    {
      // Error parsing path
      return false;
    }


  // Check if you can even get the source file
  int sourceFile = open(source.c_str(), O_RDONLY);
  if(sourceFile < 0)
    {
      cout << "Error opening source file: " << source << endl;
      return false;
    }
  // Create copies of entries and path
  vector<DirectoryEntry*> e = curEntries;
  vector<DirectoryEntry*> p = currentPath;

  // save the name of the file and remove it from path:
  
  // get fileName using the original string in order to get
  // correct casing, since getPath() sets to capital letters
  string fileName = dest.substr(dest.size()-destPath.back().size());
  destPath.pop_back();

  // follow path one step at a time
  for(int x = 0; x < destPath.size(); x++)
    {
      if(!setDirectory(e, p, destPath[x]))
	{
	  cout << "Error finding directory" << endl;
	  close(sourceFile);
	  return false;
	}
    }

  // now, we have followed the path to where we're putting the file

  // check to make sure file does not already exist
  int fileExists = findEntry(e, fileName);
  if(fileExists != -1)
    {
      // File already exists in this directory
      cout << "Error: file " << fileName << " already exists." << endl;
      close(sourceFile);
      return false;
    }

  // create file's directory entry
  vector<DirectoryEntry*> fileEntry;
  int dotPos = fileName.find_last_of('.');
  string ext = fileName.substr(dotPos+1);
  string shortName = fileName.substr(0,dotPos);
  if(shortName.size() > 8)
    {
      bool doneShortName = false;
      for(int n = 1; n < 10000 && !doneShortName; n++)
	{
	  stringstream strStream;
	  strStream << shortName.substr(0,6-(n/10)) << "~" << n;
	  shortName = strStream.str();
	  string toAdd = "";
	  if(ext.size() > 0) toAdd = "." + ext;
	  if(findEntry(e, shortName+toAdd) < 0)
	    {
	      doneShortName = true;
	    }
	}
      if(!doneShortName)
	{
	  cout << "Error creating short name" << endl;
	  close(sourceFile);
	  return false;
	}
    }

  createDirEntry(fileEntry, fileName, shortName, ext, false);

  // find empty spot for directory entry
  int emptySpot = 0;

  if(p.size() == 0)
    {
      // in root directory
      
      // Get start of root directory
      int r = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FatSz16);
      r = r * BPB_BytsPerSec;
      
      // Get size of root directory
      int rSize = RootDirSectors * BPB_BytsPerSec;

      // Find empty directory entry
      emptySpot = findEmptyDirEntry(r, rSize, fileEntry.size());
      if(emptySpot < 0)
	{
	  cout << "Error: Root directory can't fit file." << endl;
	  close(sourceFile);
	  return false;
	}	   
    }
  else
    {
      // not in root directory

      // get clusters holding directory's entries
      vector<int> dirClusters;
      if(!getClusters(dirClusters, e[0]->DIR_FstClusLO))
	{
	  cout << "Error getting clusters" << endl;
	  close(sourceFile);
	  return false;
	}

      // look for an empty spot that's big enough
      emptySpot = -1;
      int clusSize = BPB_SecPerClus * BPB_BytsPerSec;
      for(int clusC = 0; clusC < dirClusters.size() && emptySpot < 0; clusC++)
	{
	  int start = FirstSectorOfCluster(dirClusters[clusC]);
	  start = start * BPB_BytsPerSec;
	  emptySpot = findEmptyDirEntry(start, clusSize, fileEntry.size());
	}
      
      if(emptySpot < 0)
	{
	  // allocate a new cluster to the directory
	  int newClus = findEmptyCluster(true, 1);
	  // write that cluster number to end of clusters
	  unsigned char clusBuf[2];
	  clusBuf[1] = newClus >> 8;
	  clusBuf[0] = newClus % 256;
	  int lastClus = dirClusters[dirClusters.size()-1];
	  int cStart = FATSecNum(lastClus)*BPB_BytsPerSec;
	  cStart = cStart + FATEntOffset(lastClus);
	  pwrite(fDes, clusBuf, 2, cStart);
	  
	  // set emptySpot to the start of new cluster
	  emptySpot = FirstSectorOfCluster(newClus) * BPB_BytsPerSec;
	}
    }

  // Now, we have an empty spot for our file directory entry
  // we need to get the file size

  int fileSize = lseek(sourceFile,0,SEEK_END);
  if(fileSize < 0)
    {
      cout << "Error: could not compute size of source file." << endl;
      close(sourceFile);
      return false;
    }

  int neededClusters = ceil((fileSize*1.0)/(BPB_BytsPerSec*1.0));
  neededClusters = ceil((neededClusters*1.0)/(BPB_SecPerClus*1.0));

  int emptyCluster = findEmptyCluster(true, neededClusters);
  if(emptyCluster < 0)
    {
      cout << "Error: could not get necessary clusters" << endl;
      close(sourceFile);
      return false;
    }

  // now, set file size and first cluster number on the directory entry
  fileEntry.back()->setFileSize(fileSize);
  fileEntry.back()->setFirstCluster(emptyCluster);

  // now, write the directory entry
  for(int i = 0; i < fileEntry.size(); i++)
    {
      pwrite(fDes, fileEntry[i]->c_str, 32, emptySpot + i*32);
    }

  

  // finally, get clusters and write the file

  vector<int> fileClusters;
  if(!getClusters(fileClusters, emptyCluster))
    {
      cout << "Error getting empty clusters just made for file" << endl;
      close(sourceFile);
      return false;
    }

  int bytesWritten = 0;
  int cSize = BPB_SecPerClus * BPB_BytsPerSec;
  unsigned char buf[cSize];

  for(int x = 0; x < fileClusters.size(); x++)
    {
      // get start position
      int start = FirstSectorOfCluster(fileClusters[x]);
      start = start * BPB_BytsPerSec;

      // how much left to copy
      int toCopy = fileSize - bytesWritten;

      if(toCopy >= cSize)
	{
	  // If there is enough file left, copy the whole cluster
	  pread(sourceFile, buf, cSize, bytesWritten);
	  pwrite(fDes, buf, cSize, start);
	  bytesWritten += cSize;
	}
      else
	{
	  // Otherwise, only copy to the end of the file
	  pread(sourceFile, buf, toCopy, bytesWritten);
	  pwrite(fDes, buf, toCopy, start);
	  bytesWritten += toCopy;
	}
    }
  
  // close file
  close(sourceFile);

  // refresh directory
  refreshDirectory();

  return true;
}

int FileSystem::findEmptyDirEntry(int start, int max, int cons)
{
  unsigned char dirEntry[32];
  bool done = false;

  int curStreak = 0;
  
  for(int x = 0; x < max && !done; x=x+32)
    {
      // Read directory entry
      pread(fDes, dirEntry, 32, start+x);
      // create it
      DirectoryEntry entry(dirEntry);
      if(entry.isEmpty())
	{
	  curStreak++;
	  if(curStreak >= cons) return ((start+x)-32*(curStreak-1));
	}
      else
	{
	  curStreak = 0;
	}
    }
  
  return -1;
}


int FileSystem::findEmptyCluster(bool set, int num)
{
  unsigned char buf[2];
  int firstClus = 0;
  int prevClus = -1;
  int found = 0;

  // don't let x be above the number of FAT entries
  int xBound = BPB_FatSz16*BPB_BytsPerSec/2;

  for(int x = 2; x < xBound; x++)
    {
      // get pos of FAT entry and read it
      int start = FATSecNum(x)*BPB_BytsPerSec + FATEntOffset(x);
      pread(fDes, buf, 2, start);
      if(buf[0] == 0 && buf[1] == 0)
	{
	  if(set)
	    {
	      found++;
	      if(found > 1)
		{
		  buf[1] = x >> 8;
		  buf[0] = x % 256;
		  int pStart = FATSecNum(prevClus)*BPB_BytsPerSec + FATEntOffset(prevClus);
		  pwrite(fDes, buf, 2, pStart);
		  prevClus = x;
		}
	      else if(found == 1)
		{
		  firstClus = x;
		  prevClus = x;
		}

	      if(found == num)
		{
		  buf[0] = 255;
		  buf[1] = 255;
		  pwrite(fDes, buf, 2, start);
		  return firstClus;
		}
	    }
	  else
	    {
	      return x;
	    }
	}
    }
  cout << "out of memory error!" << endl;
  return -1;
}


void FileSystem::createDirEntry(vector<DirectoryEntry*> &e, string name, string shortName, string ext, bool isDir)
{
  e.clear();
  int nameLeft = name.size();
  const char* nameCStr = name.c_str();

  for(int x = 0; nameLeft > 0; x++)
    {
      // Create long directory names
      unsigned char ld[32];
      
      //LDIR_Ord
      ld[0] = x+1;
      //LDIR_Name1
      for(int i = 0; i < 5; i++)
	{
	  if(nameLeft > 0)
	    {
	      ld[i*2+1] = (unsigned char)nameCStr[name.size()-nameLeft];
	      nameLeft--;
	    }
	  else if(nameLeft == 0)
	    {
	      ld[i*2+1] = '\0';
	      nameLeft--;
	    }
	  else
	    {
	      ld[i*2+1] = 255;
	    }
	}
      //LDIR_Attr
      ld[11] = 15;
      //LDIR_Type
      ld[12] = 0;
      //LDIR_Name2
      for(int i = 0; i < 6; i++)
	{
	  if(nameLeft > 0)
	    {
	      ld[i*2+14] = (unsigned char)nameCStr[name.size()-nameLeft];
	      nameLeft--;
	    }
	  else if(nameLeft == 0)
	    {
	      ld[i*2+14] = '\0';
	      nameLeft--;
	    }
	  else
	    {
	      ld[i*2+14] = 255;
	    }
	}
      //FstClusLO
      ld[26] = 0;
      ld[27] = 0;
      //LDIR_Name3
      for(int i = 0; i < 2; i++)
	{
	  if(nameLeft > 0)
	    {
	      ld[i*2+28] = (unsigned char)nameCStr[name.size()-nameLeft];
	      nameLeft--;
	    }
	  else if(nameLeft == 0)
	    {
	      ld[i*2+28] = '\0';
	      nameLeft--;
	    }
	  else
	    {
	      ld[i*2+28] = 255;
	    }
	}
      e.push_back(new DirectoryEntry(ld));
    }
  
  // now long names are created, but in reverse order
  // flip order and set LDIR_Ord on first one
  vector<DirectoryEntry*> hold = e;
  for(int x = 0; x < e.size(); x++)
    {
      e[x] = hold[hold.size()-x-1];
    }
  e[0]->c_str[0] = 64;

  // now create the short directory

  // we want the capital casing of the name
  // with no extension:

  shortName = toUpperCase(shortName);

  // check if there is an extension:
  if(ext.size() > 0)
    {
      ext = toUpperCase(ext);
    }

  unsigned char dir[32];
  // DIR_Name name
  for(int x = 0; x < 8; x++)
    {
      if(x < shortName.size())
	{
	  dir[x] = shortName.c_str()[x];
	}
      else
	{
	  dir[x] = ' ';
	}
    }
  // DIR_Name ext
  for(int x = 0; x < 3; x++)
    {
      if(x < ext.size())
	{
	  dir[x+8] = ext.c_str()[x];
	}
      else
	{
	  dir[x+8] = ' ';
	}
    }
  // DIR_Attr
  if(isDir)
    {
      dir[11] = 16;
    }
  else
    {
      dir[11] = 0;
    }

  //wrttime/date
  time_t theTime;
  struct tm *t;
  time(&theTime);
  t = localtime(&theTime);

  unsigned int date;
  date = (t->tm_year - 80);
  date = date << 7;
  date = date + t->tm_mon+1;
  date = date << 4;
  date = date + t->tm_mday;

  dir[22] = (unsigned char)(date >> 8);
  dir[23] = (unsigned char)(date % 256);

  unsigned int time;
  time = t->tm_hour;
  time = time << 5;
  time = time + t->tm_min;
  time = time << 6;
  time = time + (t->tm_sec/2);
  
  dir[24] = (unsigned char)(time >> 8);
  dir[25] = (unsigned char)(time % 256);

  //filesize
  dir[28] = 0;
  dir[29] = 0;
  dir[30] = 0;
  dir[31] = 0;

  e.push_back(new DirectoryEntry(dir));  
}
