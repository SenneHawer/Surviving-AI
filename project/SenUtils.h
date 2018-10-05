#pragma once
#include <map>

namespace sen
{
	float Distance(Elite::Vector2 a, Elite::Vector2 b);

	struct Memory
	{
		int AmountGun;
		int AmountMed;
		int AmountFood;

		//map<int, bool> InventoryMap;

		map<int, int> InventoryMap; //first: slot, second: ItemType

	};
}
