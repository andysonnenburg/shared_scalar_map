#include "shared_scalar_map.hpp"

#include <iostream>

int main()
{
	EML::shared_scalar_map<int, int> map;
	std::cout << map.empty() << std::endl;
	map.insert(std::make_pair(0, 0));
	std::cout << map.at(0) << std::endl;
	std::cout << map.empty() << std::endl;
	map.clear();
	std::cout << map.empty() << std::endl;
}
