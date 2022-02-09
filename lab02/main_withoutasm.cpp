#include<iostream>
#include<string>
#include<fstream>
using namespace std;

// extern "C" {
//     void print_data(char *str, int len);
//     void print_red(char *str, int len);
// }

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define FAT_START  0x0200                // FAT12中，FAT区从第1扇区开始，1 * 512 = 512 = 0x0200
#define ROOT_START 0x2600                // FAT12中，根目录区从第19扇区开始，19 * 512 = 9728 = 0x2600
#define DATA_START 0x4200                // FAT12中，数据区从第33扇区开始，33 * 512 = 16896 = 0x4200
#define LAST_CLUSTER 0xFF8
#define BROKEN_CLUSTER 0xFF7

#define DIR_TYPE 0
#define FILE_TYPE 1

#define LS_L 1
#define LS 0
#define EXIT -1
#define CAT 2
#define ERROR -2
#define COMMAND_ERROR -3
#define NOT_FOUND -4

#define MAX_FILE_NUM 100
#define MAX_FILE_SIZE 10000

struct FileAndDir {
	string name;	// 文件名
	int type;	// 文件or文件夹or结束
	string path;	// 全路径名
	int fileSize;	// 文件大小
	int startPos;	// 起始读取位置
	int depth;
};

FileAndDir filesArray[MAX_FILE_NUM];
int fileNum = 0;
ifstream fat12;
string imgName = "./a.img";
int params = 0;

//根目录条目
struct RootEntry {
	char DIR_Name[8]; // 文件名8字节 
	char Ext_Name[3]; // 扩展名3字节
	u8 DIR_Attr; // 文件属性
	u8 reserved[10]; // 保留位
	u16 DIR_WrtTime; // 最后一次写入时间
	u16 DIR_WrtDate; // 最后一次写入日期
	u16 DIR_FstClus; // 文件开始的簇号
	u32 DIR_FileSize; // 文件大小
};
//根目录条目结束，32字节

struct Command {
	int type;
	string path;
};

void initFilesArray();
void load(FileAndDir &curDir);
string parseName(RootEntry &curDir);
string strip(string s);
int countDirectSub(int pos, int target_type);
void exec_ls(int pos, int option);
int findFile(string path);
string getName(string s);
void exec_cat(Command command);
string* split(string x, char c);
Command parseCommand();
void ExecuteCommand();

void initFilesArray() {
	filesArray[fileNum].name = "/";
	filesArray[fileNum].path = "/";
	filesArray[fileNum].type = DIR_TYPE;
	filesArray[fileNum].fileSize = 0;
	filesArray[fileNum].startPos = ROOT_START;
	filesArray[fileNum].depth = 0;
	fileNum++;
}

void load(FileAndDir &curDir) {
	RootEntry rootEntry;
	RootEntry *rootPtr = &rootEntry;
	for (int i = 0; i < 16; i++) {
		fat12.seekg(curDir.startPos + i * 32);
		fat12.read((char *)rootPtr, 32);
		if (rootEntry.DIR_Name[0] == '\0') {
			break;
		}
		if (rootEntry.DIR_Name[0] > 'Z' || rootEntry.DIR_Name[0] < 'A')
			continue;
		if (rootEntry.DIR_Attr != 0x10 && rootEntry.DIR_Attr != 0x20 && rootEntry.DIR_Attr != 0x00) {
			continue;
		}
		FileAndDir f;
		f.name = parseName(rootEntry);
		f.path = curDir.name == "/" ? "/" + f.name : curDir.name + "/" + f.name;
		if (rootEntry.DIR_Attr == 0x10) {
			f.type = DIR_TYPE;
		}
		else {
			f.type = FILE_TYPE;
		}
		f.startPos = (rootEntry.DIR_FstClus + 31) * 512;
		f.fileSize = rootEntry.DIR_FileSize;
		f.depth = curDir.depth + 1;
		filesArray[fileNum++] = f;
		if (f.type == DIR_TYPE) {
			load(f);
		}
	}
}

string parseName(RootEntry &curDir) {
	string fileName;
	for (int i = 0; i < 8; i++) {
		fileName += curDir.DIR_Name[i];
	}
	fileName = strip(fileName);
	if (curDir.DIR_Attr != 0x10) {
		fileName = fileName + "." + strip(curDir.Ext_Name);
	}
	return fileName;
}

string strip(string s) {
	char res[10];
	int cnt = 0;
	for (int i = 0; i < s.size() && s[i] != ' '; i++) {
		res[cnt++] = s[i];
	}
	return string(res);
}

string getName(string s) {
	string res = "";
	for(int i=s.size()-1; i >= 0; i--){
		if(s[i] == '/'){
			break;
		}
		else{
			res = s[i] + res;
		}
	}
	return res;
}

int countDirectSub(int pos, int target_type){
	FileAndDir cur = filesArray[pos];
	int num = 0;
	for(int i = pos + 1;i < fileNum;i++){
		if(cur.depth == filesArray[i].depth){
			return num;
		}
		if((cur.depth == filesArray[i].depth - 1) && filesArray[i].type == target_type){
			num += 1;
		}
	}
	return num;
}

void exec_ls(int pos, int option){
	if(pos == NOT_FOUND){
		cout << "ERROR: File Or Dir Not Found" << endl;
		return;
	}
	FileAndDir cur = filesArray[pos];
	if (cur.depth == 0) {
        cout << cur.name;
    }
	else {
        cout << "/" + cur.name + "/";
    }
	if(option == LS_L){
		cout << " " << countDirectSub(pos, DIR_TYPE) << " " << countDirectSub(pos, FILE_TYPE);
	}
	cout << ":" << endl;
	if (cur.depth > 0) {
        if(option == LS_L){
			cout << "." << endl;
			cout << ".." << endl;
		}
		else{
			cout << ". .. " << endl;
		}
    }
	for(int i = pos + 1; i < fileNum; i++){
		if(cur.depth == filesArray[i].depth){
			break;
		}
		if(cur.depth == filesArray[i].depth - 1){
			if(filesArray[i].type == DIR_TYPE){
				cout << filesArray[i].name << " ";
				if(option == LS_L){
					cout << countDirectSub(i, DIR_TYPE) << " " << countDirectSub(i, FILE_TYPE) << endl;
				}
			}
			else{
				cout << filesArray[i].name << " ";
				if(option == LS_L){
					cout << filesArray[i].fileSize << endl;
				}
			}
		}
	}
	cout << "\n";
	for(int i = pos + 1; i < fileNum; i++){
		if(cur.depth == filesArray[i].depth){
			break;
		}
		if(cur.depth == filesArray[i].depth - 1 && filesArray[i].type == DIR_TYPE){
			exec_ls(i, option);
		}
	}
}

int findFile(string path){
	if(path == "/"){
		return 0;
	}
	if(path[0] == '/'){
		path = path.substr(1, path.length());
	}
	path = getName(path);
	for(int i = 0; i < fileNum; i++){
		if(filesArray[i].name == path){
			return i;
		}
	}
	return NOT_FOUND;
}

int getNextCluster(int curCluster){
	int start = curCluster * 3 / 2 + FAT_START;
	u16 tmp;
	u16* tmp_ptr = &tmp;
	int res = 0;
	fat12.seekg(start);
	fat12.read((char *)tmp_ptr, 16);
	if(curCluster % 2 == 0){
		res = tmp & 0x0FFF;
	}
	else{
		res = ((tmp & 0xFFF0) >> 4);
	}
	return res;
}

void exec_cat(Command command){
	int pos = findFile(command.path);
	if(pos == NOT_FOUND){
		cout << "ERROR: No Such File" << endl;
		return;
	}
	FileAndDir target = filesArray[pos];
	if(target.type == DIR_TYPE){
		cout << "ERROR: This Is A Directory" << endl;
		return;
	}
	else if(target.name.substr(target.name.length()-3, target.name.length()) != "TXT"){
		cout << "ERROR: Can Only Accept TXT Files" << endl;
		return;
	}
	char txt[MAX_FILE_SIZE];
	int curCluster = target.startPos / 512 - 31;
	int cnt = 0;
	while(curCluster <= LAST_CLUSTER){
		long start = (curCluster + 31) * 512;
		fat12.seekg(start);
		fat12.read(txt + cnt * 512, 512);
		curCluster = getNextCluster(curCluster);
		cnt++;
		if(curCluster == BROKEN_CLUSTER){
			cout << "ERROR: Broken Cluster" << endl;
			return;
		}
	}
	cout << string(txt) << endl;
	return;
}

string* split(string x, char c) {
	x = x + " ";
	string s[10];
	int cnt = 0;
	int last = 0;
	for (unsigned int i = 0; i < x.length(); i++) {
		if (x[i] == c) {
			for (unsigned int j = last; j < i; j++) {
				s[cnt] += x[j];
			}
			last = (i + 1);
			cnt += 1;
		}
	}
	string* ss;
	ss = new string[cnt];
	for (int i = 0; i < cnt; i++) {
		ss[i] = s[i];
	}
	params = cnt;
	return ss;
}

Command parseCommand(){
	Command curCommand;
	string input;
	getline(cin, input);
	string* commands = split(input, ' ');
	if(commands[0] == "cat"){
		if(params > 2){
			curCommand.type = ERROR;
		}
		else{
			curCommand.type = CAT;
			curCommand.path = commands[1];
			if((curCommand.path.size() > 3) && (curCommand.path.substr(curCommand.path.length() - 4, curCommand.path.length()) == ".txt")){
				curCommand.path = curCommand.path.substr(0, curCommand.path.length() - 4) + ".TXT";
			}
		}
	}
	else if(commands[0] == "exit"){
		curCommand.type = EXIT;
	}
	else if(commands[0] == "ls"){
		if(params == 1){
			curCommand.type = LS;
			curCommand.path = "/";
			return curCommand;
		}
		bool hasPath = false;
		curCommand.path = "/";
		curCommand.type = LS;
		for(int i = 1; i < params; i++){
			string::size_type idx = commands[i].find("/");
			if(idx != string::npos){
				if(hasPath){
					curCommand.path = ERROR;
					return curCommand;
				}
				else{
					hasPath = true;
					curCommand.path = commands[i];
				}
			}
			else{
				if(commands[i][0] == '-' && commands[i][1] == 'l'){
					curCommand.type = LS_L;
				}
			}
		}
	}
	else{
		curCommand.type = COMMAND_ERROR;
	}
	return curCommand;
}

void ExecuteCommand(){
	while(true){
		Command command = parseCommand();
		if(command.type == EXIT){
			break;
		}
		else if(command.type == COMMAND_ERROR){
			cout << "ERROR: Command Error" << endl;
		}
		else if(command.type == ERROR){
			cout << "ERROR: Too Many Parameters" << endl;
		}
		else if(command.type == LS_L){
			exec_ls(findFile(command.path), LS_L);
		}
		else if(command.type == LS){
			exec_ls(findFile(command.path), LS);
		}
		else if(command.type == CAT){
			exec_cat(command);
		}
		else{
			cout << "ERROR: Unknown command" << endl;
		}
	}
}

int main(){
	fat12.open(imgName.c_str());
	initFilesArray();
	load(filesArray[0]);
	cout << "Successfully Loaded!" << endl;
	ExecuteCommand();
	fat12.close();
	return 0;
}