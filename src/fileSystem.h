#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "directory.h"
#include <vector>

using namespace std;

class FileSystem{
public:
	int fDes;	
	
	// holds the directory entries of the working directory
	vector<DirectoryEntry*> curEntries;

	// holds the path of the working directory
	vector<DirectoryEntry*> currentPath;

	unsigned int BPB_BytsPerSec;
	unsigned int BPB_SecPerClus;
	unsigned int BPB_RsvdSecCnt;
	unsigned int BPB_NumFATs;
	unsigned int BPB_RootEntCnt;
	unsigned int BPB_TotSec16;
	unsigned int BPB_FatSz16;
	unsigned int BPB_TotSec32;
	unsigned int RootDirSectors;
	unsigned int FirstDataSector;

	unsigned int FirstSectorOfCluster(int n);
	unsigned int FATSecNum(int n);
	unsigned int FATEntOffset(int n);

	// puts the root directory entries into v after clearing v
	void getRootDirectoryEntries(vector<DirectoryEntry*> &v);

	// returns a string containing the path of the working directory
	string getCurrentDirectoryPath();
	
	// using the path relative to the working directory,
	// puts the directories entries into eIn and the
	// path of the directory into pIn
	bool setDirectory(vector<DirectoryEntry*> &eIn, vector<DirectoryEntry*> &pIn, string path);

	// calls setDirectory on the working directory
	void setCurrentDirectory(string path);

	// starting from cluster start, reads the FAT table
	// to find all linked clusters and puts them into c
	bool getClusters(vector<int> &c, int start);

	// parses the path and puts into p
	bool getPath(vector<string> &p, string path);

	// reads 32-byte directory entries from the disk and
	// puts them into e
	void getDirEntries(vector<DirectoryEntry*> &e, int start, int max);
	
	// finds the index of the directory entry in e that has
	// name as EITHER its long or short filename
	// returns -1 if not found
	int findEntry(vector<DirectoryEntry*> &e, string name);

	// follows path and puts directory entries into e
	bool LSPath(vector<DirectoryEntry*> &e, string path);

	// takes data from source path on disk and copies it into
	// dest path on "real" file system
	bool CopyOut(string source, string dest);

	// takes data from source path on "real" file system and
	// copies it into dest path on disk
	bool CopyIn(string source, string dest);

	// refreshes directory entries
	void refreshDirectory();

	// starting at byte start, read directory entries
	// until you find cons consecutive empty ones
	// return the position of the empty directory entry
	// return -1 if none found
	int findEmptyDirEntry(int start, int max, int cons);

	// finds position of next empty cluster
	// if set==true, then it sets that empty cluster
	// to be filled
	int findEmptyCluster(bool set, int num);

	// creates a directory entry, including long names, from
	// the given name and sets the times/dates to now. Also
	// puts the entries (long name with short name) into e
	void createDirEntry(vector<DirectoryEntry*> &e, string name, string shortName, string ext, bool isDir);

	// constructs a FileSystem from a file descriptor
	FileSystem(int fileDes);
};


#endif
