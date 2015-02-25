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
	map.insert(std::make_pair(0, 0));
	map.insert(std::make_pair(1, 1));
	map.insert(std::make_pair(2, 2));
	map.insert(std::make_pair(3, 3));
	std::cout << map.empty() << std::endl;
	std::cout << map.at(0) << std::endl;
	std::cout << map.at(1) << std::endl;
	std::cout << map.at(2) << std::endl;
	std::cout << map.at(3) << std::endl;
	auto map2 = map;
	std::cout << map2.empty() << std::endl;
}
