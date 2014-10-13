#include <iostream>
#include <string>
#include <vector>
#include <fstream>

std::vector<std::string> args2vector(int argc, char **argv){
	std::vector<std::string> ret;
	for(int i=0; i<argc; ++i){
		ret.push_back(argv[i]);
	}
	return ret;
}

int main(int argc, char **argv){
	std::cout << "Hello world!" << std::endl;
	std::vector<std::string> args=args2vector(argc, argv);
	for(auto i=args.begin(); i!=args.end(); ++i){
		std::cout << *i << std::endl;
	}
	std::ifstream file("info:/procs");
	while(!file.eof()){
		std::string line;
		getline(file, line);
		std::cout << line << std::endl;
	}
    std::cout << "std::cout" << std::endl;
    std::cerr << "std::cerr" << std::endl;
    std::string input;
    getline(std::cin, input, 128);
    std::cout << input << std::endl;
	return 0;
}