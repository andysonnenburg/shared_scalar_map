#include "SharedRadixTree.hpp"

#include <iostream>

int main()
{
	EML::SharedRadixTree<int, int> map;
	std::cout << map.empty() << std::endl;
	map.insert(std::make_pair(0, 0));
	std::cout << map.find(0)->second << std::endl;
	std::cout << map.empty() << std::endl;
	map.clear();
	std::cout << map.empty() << std::endl;
	map.insert(std::make_pair(0, 0));
	map.insert(std::make_pair(1, 1));
	map.insert(std::make_pair(2, 2));
	map.insert(std::make_pair(3, 3));
	std::cout << map.empty() << std::endl;
	std::cout << map.find(0)->second << std::endl;
	std::cout << map.find(1)->second << std::endl;
	std::cout << map.find(2)->second << std::endl;
	std::cout << map.find(3)->second << std::endl;
	auto map2 = map;
	std::cout << map2.empty() << std::endl;
	EML::shared_scalar_map<int*, int> map3;
	int i = 0;
	map3.insert(std::make_pair(&i, 0));
	std::cout << map3.empty() << std::endl;
}
