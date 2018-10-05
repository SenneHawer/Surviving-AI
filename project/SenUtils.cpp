#include "stdafx.h"
#include "SenUtils.h"


float sen::Distance(Elite::Vector2 a, Elite::Vector2 b)
{
	auto term1 = std::pow((a.x - b.x), 2);
	auto term2 = std::pow((a.y - b.y), 2);

	auto result = sqrt(term1 + term2);
	return result;
}

