#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>

using namespace std;

class DirectoryEntry{
public:
	char DIR_Name[11];
	char DIR_Attr;
	char DIR_CrtTimeTenth;
	char DIR_CrtTime[2];
	char DIR_CrtDate[2];
	char DIR_LstAccDate[2];
	char DIR_WrtTime[2];
	char DIR_WrtDate[2];
	int DIR_FstClusLO;
	int DIR_FileSize;
	
	bool ATTR_LONG_NAME;
	char LDIR_Ord;
	char LDIR_Name1[10];
	char LDIR_Type;
	char LDIR_Chksum;
	char LDIR_Name2[12];
	int LDIR_FstClusLO;
	char LDIR_Name3[4];

	bool isValid() const;
	bool isEnd() const;
	bool isEmpty() const;
	bool isDirectory() const;
	string* getName() const;

	void setFirstCluster(int clus);
	void setFileSize(int size);

	unsigned char c_str[32];

	DirectoryEntry(unsigned char entry[]);
};

#endif
