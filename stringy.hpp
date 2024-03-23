#include <vector>
#include <string>
#include <iostream>
using namespace std;

vector<string> make_lines(const string testfile, int& testcnt){
	std::ifstream input;
	input.open(testfile,std::ios::in);

	string tmp;
	std::getline(input, tmp);
	testcnt = std::stoi(tmp);
	tmp = "";

	vector<string> result;
	while(std::getline(input, tmp)){
		result.push_back(tmp);
		tmp = "";
	}

	input.close();
	return result;
}

bool writer(string towrite, string name){
	std::ofstream output;
	output.open(name, std::ios::out);
	if(!output.is_open()){
		return false;
	}
	int sz = size(towrite) + 1;
	output.write(towrite.c_str(), sz);
	return true;
}

void writeToFile(vector<string> answers, string filename){
	int test_cnt = size(answers);
	for(int i = 1; i <= test_cnt; ++i){
		string towrite = "1\n";
		string name = filename;
		name.append(std::to_string(i));
		towrite.append(answers[i-1]);
		towrite.append("\n");
		if(!writer(towrite, name)){

		}
		
	}
}

void copy_solution(string filename){
	string script = "cp ";
	script.append(filename);
	script.append(" sksk_solution");
	std::system(script.c_str());
	return;
}

void cleanup(){
	string script = "rm -f sksk_solution*"; 
	std::system(script.c_str());

	script = "rm -f ppipe*";
	std::system(script.c_str());

	script = "rm -f out*";
	std::system(script.c_str());
	return;
}

