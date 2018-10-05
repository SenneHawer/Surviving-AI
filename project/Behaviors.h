#pragma once
#include "stdafx.h"
#include "Blackboard.h"
#include "BehaviorTree.h"
#include <Exam_HelperStructs.h>
#include <limits>
#include "SenUtils.h"

//*** GENERAL BEHAVIORS ***
bool HasTarget(Blackboard* pBlackboard)
{
	//Get data
	auto hasTarget = false;
	auto dataAvailable = pBlackboard->GetData("TargetSet", hasTarget);
	if (!dataAvailable)
		return false;

	return hasTarget;
}

bool B_Seek(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	Elite::Vector2 targetPosition;
	SteeringBehaviour* pSeekBehaviour = nullptr;
	SteeringPlugin_Output steering;

		bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", targetPosition) &&
		pBlackboard->GetData("SeekBehaviour", pSeekBehaviour);
	if (!isDataAvailable)
		return Failure;

	steering = pSeekBehaviour->CalculateSteering(agentInfo);
	pBlackboard->ChangeData("SteeringOutput", steering);

	return Success;
}

bool B_SeekNew(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	Elite::Vector2 targetPosition;
	SteeringPlugin_Output steering;
	IExamInterface* pInterface = nullptr;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", targetPosition) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	steering.LinearVelocity = targetPosition - agentInfo.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed;

	pBlackboard->ChangeData("SteeringOutput", steering);

	pInterface->Draw_Direction(agentInfo.Position, steering.LinearVelocity, steering.LinearVelocity.Magnitude(), { 1.0f, 0.0f, 0.0f });

	return Success;
}

bool B_Pipeline(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	Elite::Vector2 targetPosition;
	SteeringBehaviour* pPipelineBehaviour = nullptr;
	SteeringPlugin_Output steering;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", targetPosition) &&
		pBlackboard->GetData("PipelineBehaviour", pPipelineBehaviour);
	if (!isDataAvailable)
		return Failure;

	steering = pPipelineBehaviour->CalculateSteering(agentInfo);
	pBlackboard->ChangeData("SteeringOutput", steering);

	return Success;
	
}

bool B_Wait(Blackboard* pBlackboard)
{
	return Success;
}

//ENEMY AVOIDING
bool B_CheckForEnemies(Blackboard* pBlackboard)
{
	//Get Data
	bool canSeeEnemy;
	AgentInfo agentInfo;
	bool isDataAvailable = pBlackboard->GetData("CanSeeEnemy", canSeeEnemy) &&
		pBlackboard->GetData("AgentInfo", agentInfo);
	if (!isDataAvailable)
		return Failure;

	if (canSeeEnemy)
		return Success;
	else
		return Failure;
}
bool B_GetClosestEnemy(Blackboard* pBlackboard)
{
	//Get Data
	IExamInterface* pInterface = nullptr;
	AgentInfo agentInfo;

	bool isDataAvailable =
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("AgentInfo", agentInfo);
	if (!isDataAvailable) return Failure;

	//Get Enemies
	vector<EnemyInfo> vEnemiesInFOV = {};

	EnemyInfo enemyInfo = {};
	EntityInfo entityInfo = {};

	for (int i = 0;; ++i)
	{
		if (pInterface->Fov_GetEntityByIndex(i, entityInfo))
		{
			if (entityInfo.Type == eEntityType::ENEMY)
			{
				if (pInterface->Enemy_GetInfo(entityInfo, enemyInfo))
					vEnemiesInFOV.push_back(enemyInfo);
			}
			continue;
		}
		break;
	}

	float closestDist = 1000.0f;
	EnemyInfo closestEnemy;

	//Get Closest
	for ( auto e : vEnemiesInFOV)
	{
		float dist = sen::Distance(agentInfo.Position, e.Location);
		if (dist < closestDist)
		{
			closestDist = dist;
			closestEnemy = e;
		}
	}

	pBlackboard->ChangeData("ClosestEnemy", closestEnemy);

	return Success;
}
bool B_FindPointAround(Blackboard* pBlackboard)
{
	//Get Data
	Elite::Vector2 currTarget;
	AgentInfo agentInfo;
	EnemyInfo closestEnemy;
	IExamInterface* pInterface = nullptr;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", currTarget) &&
		pBlackboard->GetData("ClosestEnemy", closestEnemy) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable) return Failure;

	auto agent2Enemy = closestEnemy.Location - agentInfo.Position;
	auto dir2Target = (currTarget - agentInfo.Position).GetNormalized();
	auto dist2ClosestPoint = Elite::Dot(agent2Enemy, dir2Target);

	//if (dist2ClosestPoint < 0.0f || dist2ClosestPoint >= 50.0f)
	//{
	//	return Failure;
	//}

	auto closestPointToEnemy = agentInfo.Position + (dist2ClosestPoint * dir2Target);

	auto dirOffset = (closestPointToEnemy - closestEnemy.Location);
	auto distFromObstacleCenter = dirOffset.Normalize();
	float avoidMargin = 4.0f;


	//if (distFromObstacleCenter > closestEnemy.Size + avoidMargin)
	//{
	//	return Failure;
	//}

	Elite::Vector2 newTarget = closestEnemy.Location + (closestEnemy.Size + avoidMargin)*dirOffset;

	auto dist2NewTarget = Elite::Distance(agentInfo.Position, newTarget);
	//if (dist2NewTarget >= 50.0f)
	//{
	//	return Failure;
	//}

	pInterface->Draw_Circle(newTarget, 1.0f, Elite::Vector3(0.0f, 1.0f, 1.0f));

	pBlackboard->ChangeData("TargetPosition", newTarget);
	return Success;
}
bool B_StartRunning(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	SteeringPlugin_Output steering;
	bool canRun;

	bool isDataAvailable =
		pBlackboard->GetData("CanRun", canRun);
	if (!isDataAvailable)
		return Failure;


	float runCountDown = 3.0f;
	canRun = true;

	pBlackboard->ChangeData("RunCountDown", runCountDown);
	pBlackboard->ChangeData("CanRun", canRun);

	return Success;
}
bool B_StopRunning(Blackboard* pBlackboard)
{
	//Get Data
	bool canRun;
	float elapsedSec;
	float runCountDown;
	bool isDataAvailable =
		pBlackboard->GetData("CanRun", canRun) &&
		pBlackboard->GetData("ElapsedSec", elapsedSec) &&
		pBlackboard->GetData("RunCountDown", runCountDown);
	if (!isDataAvailable)
		return Failure;


	if (runCountDown <= 0)
	{
		runCountDown = -1.0f;
		canRun = false;
	}
	else
	{
		runCountDown -= elapsedSec;
	}

	pBlackboard->ChangeData("RunCountDown", runCountDown);
	pBlackboard->ChangeData("CanRun", canRun);

	return Failure;
}
bool B_OnBitten(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	float runCountDown;
	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("RunCountDown", runCountDown);
	if (!isDataAvailable)
		return Failure;

	if (agentInfo.Bitten)
	{
		runCountDown = 3.0f;
		return Success;
	}

	return Failure;
}

//ENEMY SHOOTING
bool B_CheckGun(Blackboard* pBlackboard)
{
	//Get Data
	sen::Memory memory;
	bool isDataAvailable = pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	if (memory.AmountGun != 0)
	{
		return Success;
	}
	
	return Failure;
}
bool B_Aim(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	Elite::Vector2 target;
	EnemyInfo closestEnemy;
	SteeringPlugin_Output steering;
	IExamInterface* pInterface = nullptr;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", target) &&
		pBlackboard->GetData("ClosestEnemy", closestEnemy) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	//steering.AutoOrientate = false;

	steering.LinearVelocity = closestEnemy.Location - agentInfo.Position;
	steering.LinearVelocity.Normalize();
	////steering.LinearVelocity *= 0.5f;
	steering.AngularVelocity = agentInfo.MaxAngularSpeed;

	//pBlackboard->ChangeData("SteeringOutput", steering);

	target = closestEnemy.Location;

	//Get angle between agent dir and enemy
	auto agentDir = agentInfo.LinearVelocity.GetNormalized();
	auto agentToEnemy = (closestEnemy.Location - agentInfo.Position).GetNormalized();

	
	pInterface->Draw_Direction(agentInfo.Position, agentToEnemy, 100.0f, Elite::Vector3(0, 1, 0));
	pInterface->Draw_Circle(closestEnemy.Location, 2.0f, Elite::Vector3(1, 1, 0));

	//Get Boundary Angle
	float distance = Elite::DistanceSqrt(agentInfo.Position, closestEnemy.Location);
	float radius = closestEnemy.Size / 2;
	float boundaryAngle = atan(radius / distance);

	float dirAngle = atan(agentDir.y / agentDir.x);

	//if (!(angle < boundaryAngle / 2.0f && angle > -boundaryAngle / 2.0f))
	//boundaryAngle += dirAngle;
	//pInterface->Draw_Direction(agentInfo.Position, Elite::Vector2(cos(boundaryAngle), sin(boundaryAngle)), 100.0f, Elite::Vector3(1, 0, 1));
	
	agentToEnemy.x += cos(boundaryAngle/2);
	agentToEnemy.y += sin(boundaryAngle/2);
	pInterface->Draw_Direction(agentInfo.Position, agentToEnemy, 100.0f, Elite::Vector3(1, 0, 1));

	auto enemyAngle = Elite::GetOrientationFromVelocity(agentToEnemy);
	std::cout << enemyAngle * 180 / M_PI << std::endl;

	agentToEnemy.x -= 2*cos(boundaryAngle);
	agentToEnemy.y -= 2*sin(boundaryAngle);
	pInterface->Draw_Direction(agentInfo.Position, agentToEnemy, 100.0f, Elite::Vector3(1, 0, 1));


	pBlackboard->ChangeData("TargetPosition", target);
	pBlackboard->ChangeData("SteeringOutput", steering);

	return Success;
}
bool B_AimNew(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	Elite::Vector2 target;
	EnemyInfo closestEnemy;
	SteeringPlugin_Output steering;
	IExamInterface* pInterface = nullptr;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("TargetPosition", target) &&
		pBlackboard->GetData("ClosestEnemy", closestEnemy) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	//Get Variables
	Elite::Vector2 agent2Enemy = (closestEnemy.Location - agentInfo.Position).GetNormalized();
	float angleToTurn = Elite::GetOrientationFromVelocity(agent2Enemy);
	if (angleToTurn < 0) //Correct angle
		angleToTurn += 2 * M_PI;
	float width = closestEnemy.Size / 2;
	float dist = Elite::Distance(agentInfo.Position, closestEnemy.Location);
	float boundAngle = atan(width / dist);

	//Draw
	float playerAngle = agentInfo.Orientation;
	if (playerAngle < 0)
		playerAngle += 2 * M_PI;
	pInterface->Draw_Direction(agentInfo.Position, Elite::OrientationToVector(playerAngle), 100.0f, Elite::Vector3(1, 1, 1));
	pInterface->Draw_Direction(agentInfo.Position, Elite::OrientationToVector(angleToTurn - boundAngle), 100.0f, Elite::Vector3(1, 0, 1));
	pInterface->Draw_Direction(agentInfo.Position, Elite::OrientationToVector(angleToTurn + boundAngle), 100.0f, Elite::Vector3(1, 0, 1));

	//Rotate to enemy
	steering.LinearVelocity = agent2Enemy;
	steering.LinearVelocity.Normalize();
	////steering.LinearVelocity *= 0.5f;
	steering.AngularVelocity = agentInfo.MaxAngularSpeed;

	//Return
	pBlackboard->ChangeData("SteeringOutput", steering);

	return Success;
}
bool B_Shoot(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	EnemyInfo closestEnemy;
	sen::Memory memory;
	IExamInterface* pInterface = nullptr;

	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("ClosestEnemy", closestEnemy) &&
		pBlackboard->GetData("Memory", memory) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	//Get Variables
	Elite::Vector2 agent2Enemy = (closestEnemy.Location - agentInfo.Position).GetNormalized();
	float angleToTurn = Elite::GetOrientationFromVelocity(agent2Enemy);
	if (angleToTurn < 0) //Correct angle
		angleToTurn += 2 * M_PI;
	float width = closestEnemy.Size / 2;
	float dist = Elite::Distance(agentInfo.Position, closestEnemy.Location);
	float boundAngle = atan(width / dist);
	float playerAngle = agentInfo.Orientation;
	if (playerAngle < 0)
		playerAngle += 2 * M_PI;

	if (!(playerAngle < angleToTurn + boundAngle && playerAngle > angleToTurn - boundAngle)) //If this is NOT true, He CAN'T shoot
	{
		return Success;
	}

	if (memory.InventoryMap[0] == int(eItemType::PISTOL))
	{
		ItemInfo gun;
		pInterface->Inventory_GetItem(0, gun);
		int ammo = pInterface->Item_GetMetadata(gun, "ammo"); //INT
		if (ammo > 0)
		{
			pInterface->Inventory_UseItem(0);
		}
		else
		{
			//pInterface->Inventory_DropItem(0);
			pInterface->Inventory_RemoveItem(0);
			memory.InventoryMap[0] = -1;
			--memory.AmountGun;
		}
	}

	pBlackboard->ChangeData("Memory", memory);


	return Success;
}

//PICK UP ITEMS
bool B_CheckForItems(Blackboard* pBlackboard)
{
	//Get Data
	bool canSeeItems;
	bool isDataAvailable = pBlackboard->GetData("CanSeeItem", canSeeItems);
	if (!isDataAvailable)
		return Failure;

	if (canSeeItems)
		return Success;
	else
		return Failure;
}
bool B_GetClosestItem(Blackboard* pBlackboard)
{
	//Get Data
	IExamInterface* pInterface = nullptr;
	AgentInfo agentInfo;

	bool isDataAvailable =
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("AgentInfo", agentInfo);
	if (!isDataAvailable) return Failure;

	//Get Items
	vector<EntityInfo> vItemsInFOV = {};

	EntityInfo entityInfo = {};

	for (int i = 0;; ++i)
	{
		if (pInterface->Fov_GetEntityByIndex(i, entityInfo))
		{
			if (entityInfo.Type == eEntityType::ITEM)
			{
				vItemsInFOV.push_back(entityInfo);
			}
			continue;
		}
		break;
	}

	float closestDist = 1000.0f;
	EntityInfo closestItem;

	//Get Closest
	for (auto item : vItemsInFOV)
	{
		float dist = sen::Distance(agentInfo.Position, item.Location);
		if (dist < closestDist)
		{
			closestDist = dist;
			closestItem = item;
		}
	}

	
	pBlackboard->ChangeData("TargetPosition", closestItem.Location);
	pBlackboard->ChangeData("ClosestItem", closestItem);

	pInterface->Draw_Circle(closestItem.Location, 2.0f, Elite::Vector3(0, 1, 1));

	return Success;
}
bool B_IsItemInRange(Blackboard* pBlackboard)
{
	//Get Data
	IExamInterface* pInterface = nullptr;
	AgentInfo agentInfo;
	EntityInfo item;
	sen::Memory memory;

	bool isDataAvailable =
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("ClosestItem", item) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable) return Failure;

	float distance = Elite::Distance(agentInfo.Position, item.Location);

	if (distance < 2.0f)
	{
		return Success;
	}

	return Failure;
}
bool B_PickUpItem(Blackboard* pBlackboard)
{
	//Get Data
	IExamInterface* pInterface = nullptr;
	AgentInfo agentInfo;
	EntityInfo item;
	sen::Memory memory;

	bool isDataAvailable =
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("ClosestItem", item) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable) return Failure;

	ItemInfo itemInfo;
	if (pInterface->Item_Grab(item, itemInfo))
	{
		if (itemInfo.Type == eItemType::GARBAGE)
		{
			bool done = false;
			for (auto slot : memory.InventoryMap)
			{
				if (slot.second == -1)
				{
					pInterface->Inventory_AddItem(slot.first, itemInfo);
					pInterface->Inventory_RemoveItem(slot.first);
					done = true;
					break;
				}
			}

			if (!done)
			{
				ItemInfo droppedItem;
				pInterface->Inventory_GetItem(4, droppedItem);
				pInterface->Inventory_DropItem(4);
				pInterface->Inventory_AddItem(4, itemInfo);
				pInterface->Inventory_RemoveItem(4);
				memory.InventoryMap[4] = -1;
				//pInterface->Item_Grab(droppedItem)
			}

			pInterface->Inventory_AddItem(4, itemInfo);
			pInterface->Inventory_RemoveItem(4);
		}
		else if (itemInfo.Type == eItemType::PISTOL)
		{
			//Gun always go in slot 0 and 1

			//No gun
			if (memory.AmountGun == -1)
			{
				//Put primary gun in first slot
				++memory.AmountGun;
				memory.InventoryMap[0] = int(eItemType::PISTOL);
				pInterface->Inventory_AddItem(0, itemInfo);
			}
			else
			{
				//One Gun
				if (memory.InventoryMap.at(0) ==  -1) // AKA empty
				{
					pInterface->Inventory_AddItem(0, itemInfo);
					++memory.AmountGun;
					memory.InventoryMap[0] = int(eItemType::PISTOL);

				}
				else if (memory.InventoryMap.at(1) == 0)
				{
					pInterface->Inventory_AddItem(0, itemInfo);
					++memory.AmountGun;
					memory.InventoryMap[1] = int(eItemType::PISTOL);

				}
			}
		}
		else if (itemInfo.Type == eItemType::MEDKIT)
		{
			for (int i{2}; i < 5; ++i)
			{
				if (memory.InventoryMap.at(i) == -1)
				{
					pInterface->Inventory_AddItem(i, itemInfo);
					++memory.AmountMed;
					memory.InventoryMap[i] = int(eItemType::MEDKIT);

					break;
				}
			}
		}
		else if (itemInfo.Type == eItemType::FOOD)
		{
			for (int i{ 2 }; i < 5; ++i)
			{
				if (memory.InventoryMap.at(i) == -1)
				{
					pInterface->Inventory_AddItem(i, itemInfo);
					++memory.AmountFood;
					memory.InventoryMap[i] = int(eItemType::FOOD);

					break;
				}
			}

			//if not
			pInterface->Inventory_UseItem(3);
			pInterface->Inventory_AddItem(3, itemInfo);
		}

		pBlackboard->ChangeData("Memory", memory);
		return Success;
	}
	else
		return Failure;
}
////GUN
bool B_IsGun(Blackboard* pBlackboard)
{
	//Get Data
	EntityInfo closestItem;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("ClosestItem", closestItem) &&
		pBlackboard->GetData("Interface", pInterface);

	if (!isDataAvailable)
		return Failure;

	ItemInfo itemInfo;
	pInterface->Item_Grab(closestItem, itemInfo); //ONLY GRAB ONCE!

	pBlackboard->ChangeData("SelectedItem", itemInfo);

	if (itemInfo.Type == eItemType::PISTOL)
	{
		
		return Success;
	}

	////TEMP
	//pInterface->Inventory_AddItem(4, itemInfo);
	//pInterface->Inventory_RemoveItem(4);
	return Failure;
}
bool B_HasGun(Blackboard* pBlackboard)
{
	//Get Data
	EntityInfo closestItem;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("ClosestItem", closestItem) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	if (memory.AmountGun == 0)
		return Failure;

	return Success;
}
bool B_CheckAmmo(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo gun;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", gun) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	//Search gun
	auto found = std::find_if(memory.InventoryMap.begin(), memory.InventoryMap.end(), [](std::pair<int, int> slot)
	{
		return slot.second == int(eItemType::PISTOL);
	});
	if (found == memory.InventoryMap.end()) return Failure;

	//get Gun
	ItemInfo currGun;
	pInterface->Inventory_GetItem(found->first, currGun);

	int currAmmo = pInterface->Item_GetMetadata(currGun, "ammo");
	int newAmmo = pInterface->Item_GetMetadata(gun, "ammo");

	if (newAmmo > currAmmo)
	{
		//Swap
		pInterface->Inventory_DropItem(found->first);
		pInterface->Inventory_AddItem(found->first, gun);

		return Success;
	}

	//Gun has less bullets
	pInterface->Inventory_AddItem(4, gun);
	pInterface->Inventory_RemoveItem(4);

	return Success;
}
bool B_AddGun(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo gun;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", gun) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	pInterface->Inventory_AddItem(0, gun);
	++memory.AmountGun;
	memory.InventoryMap[0] = int(eItemType::PISTOL);

	pBlackboard->ChangeData("Memory", memory);

	return Success;
}
////MedKit
bool B_IsMedkit(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo selectedItem;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", selectedItem) &&
		pBlackboard->GetData("Interface", pInterface);

	if (!isDataAvailable)
		return Failure;

	//ItemInfo itemInfo;
	//pInterface->Item_Grab(selectedItem, itemInfo);

	if (selectedItem.Type == eItemType::MEDKIT)
	{
		pBlackboard->ChangeData("SelectedItem", selectedItem);
		return Success;
	}

	////TEMP
	//pInterface->Inventory_AddItem(4, selectedItem);
	//pInterface->Inventory_RemoveItem(4);

	return Failure;
}
bool B_EmptySlot(Blackboard* pBlackboard)
{
	//Get Data
	IExamInterface* pInterface = nullptr;
	ItemInfo item;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("SelectedItem", item) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	auto found = std::find_if(memory.InventoryMap.begin(), memory.InventoryMap.end(), [&](std::pair<int, int> slot)
	{
		return slot.first != 0 && slot.first != (memory.InventoryMap.size() - 1) && slot.second == -1; //Not gunslot && empty
	});
	if (found == memory.InventoryMap.end())
	{
		if (item.Type != eItemType::GARBAGE)
		{
			pInterface->Inventory_AddItem(4, item);
			pInterface->Inventory_UseItem(4);
			pInterface->Inventory_RemoveItem(4);

			return Success;
		}

	}

	//Put in empty
	pInterface->Inventory_AddItem(int(found->first), item);
	if (item.Type == eItemType::MEDKIT)
	{
		++memory.AmountMed;
		memory.InventoryMap[found->first] = int(eItemType::MEDKIT);
	}
	else if (item.Type == eItemType::FOOD)
	{
		++memory.AmountFood;
		memory.InventoryMap[found->first] = int(eItemType::FOOD);
	}

	pBlackboard->ChangeData("Memory", memory);

	return Success;
}
bool B_HasMedkit(Blackboard* pBlackboard)
{
	//Get Data
	EntityInfo closestItem;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("ClosestItem", closestItem) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	if (memory.AmountMed == 0)
		return Failure;

	return Success;
}
bool B_GetLowestMedKit(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo medkit;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", medkit) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	int lowestHealth = int((std::numeric_limits<int>::max));
	int lowestIdx = -1;

	for (unsigned int i{1}; i < memory.InventoryMap.size(); ++i)
	{
		if (memory.InventoryMap.at(i) == int(eItemType::MEDKIT))
		{
			ItemInfo med;
			pInterface->Inventory_GetItem(i, med);
			int health = pInterface->Item_GetMetadata(med, "health");

			if (health < lowestHealth)
			{
				lowestHealth = health;
				lowestIdx = i;
			}
		}
	}

	if (lowestIdx == -1)
	{
		//TEMP
		pInterface->Inventory_AddItem(4, medkit);
		pInterface->Inventory_RemoveItem(4);
		return Failure; //Safety check
	}

	if (int(pInterface->Item_GetMetadata(medkit, "energy")) < lowestHealth)
	{
		pInterface->Inventory_AddItem(4, medkit);
		pInterface->Inventory_UseItem(4);
		pInterface->Inventory_RemoveItem(4);
	}
	else
	{
		pInterface->Inventory_UseItem(lowestIdx);
		pInterface->Inventory_RemoveItem(lowestIdx);
		pInterface->Inventory_AddItem(lowestIdx, medkit);
	}

	return Success;
}

////Food
bool B_IsFood(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo selectedItem;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", selectedItem) &&
		pBlackboard->GetData("Interface", pInterface);

	if (!isDataAvailable)
		return Failure;

	if (selectedItem.Type == eItemType::FOOD)
	{
		pBlackboard->ChangeData("SelectedItem", selectedItem);
		return Success;
	}

	////TEMP
	//pInterface->Inventory_AddItem(4, selectedItem);
	//pInterface->Inventory_RemoveItem(4);

	return Failure;
}
bool B_HasFood(Blackboard* pBlackboard)
{
	//Get Data
	EntityInfo closestItem;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("ClosestItem", closestItem) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	if (memory.AmountFood == 0)
		return Failure;

	return Success;
}
bool B_GetLowestFood(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo food;
	IExamInterface* pInterface = nullptr;
	sen::Memory memory;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", food) &&
		pBlackboard->GetData("Interface", pInterface) &&
		pBlackboard->GetData("Memory", memory);
	if (!isDataAvailable)
		return Failure;

	int lowestEnergy = int((std::numeric_limits<int>::max));
	int lowestIdx = -1;

	for (unsigned int i{ 1 }; i < memory.InventoryMap.size(); ++i)
	{
		if (memory.InventoryMap.at(i) == int(eItemType::FOOD))
		{
			ItemInfo f;
			pInterface->Inventory_GetItem(i, f);
			int energy = pInterface->Item_GetMetadata(f, "energy");

			if (energy < lowestEnergy)
			{
				lowestEnergy = energy;
				lowestIdx = i;
			}
		}
	}

	if (lowestIdx == -1)
	{
		//TEMP
		pInterface->Inventory_AddItem(4, food);
		pInterface->Inventory_RemoveItem(4);
		return Failure; //Safety check
	}

	int newEnergy = pInterface->Item_GetMetadata(food, "energy");
	if (newEnergy < lowestEnergy)
	{
		pInterface->Inventory_AddItem(4, food);
		pInterface->Inventory_UseItem(4);
		pInterface->Inventory_RemoveItem(4);
	}
	else
	{
		pInterface->Inventory_UseItem(lowestIdx);
		pInterface->Inventory_RemoveItem(lowestIdx);
		pInterface->Inventory_AddItem(lowestIdx, food);
	}

	return Success;
}

bool B_IsGarbage(Blackboard* pBlackboard)
{
	//Get Data
	ItemInfo selectedItem;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedItem", selectedItem) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	if (selectedItem.Type == eItemType::GARBAGE)
	{
		//TEMP
		pInterface->Inventory_AddItem(4, selectedItem);
		pInterface->Inventory_RemoveItem(4);
	}

	return Success;

}

//USE ITEMS
//Health
bool B_CheckHealth(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo);
	if (!isDataAvailable)
		return Failure;

	int maxHealth = 10;
	int currHealth = agentInfo.Health;

	int healthNeeded = maxHealth - currHealth;

	pBlackboard->ChangeData("HealthNeeded", healthNeeded);

	return Success;
}
bool B_TakeRightMedKit(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	int healthNeeded;
	sen::Memory memory;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("HealthNeeded", healthNeeded) &&
		pBlackboard->GetData("Memory", memory) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	int bestFitIdx = -1;
	int bestFitHealth = 0;

	for (int i{ 1 }; i < memory.InventoryMap.size() - 1; ++i)
	{
		if (memory.InventoryMap.at(i) == int(eItemType::MEDKIT))
		{
			ItemInfo medkit;
			pInterface->Inventory_GetItem(i, medkit);

			int health = pInterface->Item_GetMetadata(medkit, "health");

			if (health > bestFitHealth && health <= healthNeeded)
			{
				bestFitHealth = health;
				bestFitIdx = i;
			}
		}
	}

	if (bestFitIdx != -1)
	{
		pInterface->Inventory_UseItem(bestFitIdx);
		pInterface->Inventory_RemoveItem(bestFitIdx);
		memory.InventoryMap[bestFitIdx] = -1;
		--memory.AmountMed;

		pBlackboard->ChangeData("Memory", memory);

		return Success;
	}

	return Failure;
}
//Energy
bool B_CheckEnergy(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo);
	if (!isDataAvailable)
		return Failure;

	int maxEnergy = 10;
	int currEnergy = agentInfo.Energy;

	int energyNeeded = maxEnergy - currEnergy;

	pBlackboard->ChangeData("EnergyNeeded", energyNeeded);

	return Success;
}
bool B_TakeRightFood(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	int energyNeeded;
	sen::Memory memory;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("EnergyNeeded", energyNeeded) &&
		pBlackboard->GetData("Memory", memory) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	int bestFitIdx = -1;
	int bestFitEnergy = 0;

	for (int i{ 1 }; i < memory.InventoryMap.size() - 1; ++i)
	{
		if (memory.InventoryMap.at(i) == int(eItemType::FOOD))
		{
			ItemInfo food;
			pInterface->Inventory_GetItem(i, food);

			int energy = pInterface->Item_GetMetadata(food, "energy");

			if (energy > bestFitEnergy && energy <= energyNeeded)
			{
				bestFitEnergy = energy;
				bestFitIdx = i;
			}
		}
	}

	if (bestFitIdx != -1)
	{
		pInterface->Inventory_UseItem(bestFitIdx);
		pInterface->Inventory_RemoveItem(bestFitIdx);
		memory.InventoryMap[bestFitIdx] = -1;
		--memory.AmountFood;

		pBlackboard->ChangeData("Memory", memory);

		return Success;
	}

	return Failure;
}

//HOUSES
bool B_CanSeeHouse(Blackboard* pBlackboard)
{
	//Get Data
	bool canSeeHouse;
	Elite::Vector2 prevHousePos;
	Elite::Vector2 prevPrevHousePos;
	AgentInfo agentInfo;
	HouseInfo selectedHouse;
	IExamInterface* pInterface = nullptr;
	bool isDataAvailable =
		pBlackboard->GetData("CanSeeHouse", canSeeHouse) &&
		pBlackboard->GetData("PrevHousePos", prevHousePos) &&
		pBlackboard->GetData("PrevPrevHousePos", prevPrevHousePos) &&
		pBlackboard->GetData("AgentInfo", agentInfo);
		pBlackboard->GetData("SelectedHouse", selectedHouse) &&
		pBlackboard->GetData("Interface", pInterface);
	if (!isDataAvailable)
		return Failure;

	//HouseInfo emptyinfo = {};
	//if (selectedHouse.Center != emptyinfo.Center)
	//{
	//	return Failure;
	//}

	//if (agentInfo.IsInHouse)
	//{
	//	return Success;
	//}

	vector<HouseInfo> vHousesInFOV;

	HouseInfo hi;
	for (int i = 0;; ++i)
	{
		if (pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}
		break;
	}

	bool targetFound = false;
	for (auto house : vHousesInFOV)
	{
		float offset = 0.01f;
		if (house.Center.x <= prevHousePos.x + offset && house.Center.x >= prevHousePos.x - offset &&
			house.Center.y <= prevHousePos.y + offset && house.Center.y >= prevHousePos.y -  offset)
		{
			//AKA prevHouse
			//Don't want to go there
			continue;
		}
		else if (house.Center.x <= prevPrevHousePos.x + offset && house.Center.x >= prevPrevHousePos.x - offset &&
				 house.Center.y <= prevPrevHousePos.y + offset && house.Center.y >= prevPrevHousePos.y - offset)
		{
			//AKA prevprevHouse
			//Don't want to go there
			continue;
		}
		else
		{
			targetFound = true;
			auto navPoint = pInterface->NavMesh_GetClosestPathPoint(house.Center);
			pBlackboard->ChangeData("TargetPosition", navPoint);
			pBlackboard->ChangeData("SelectedHouse", house);
			pBlackboard->ChangeData("HaveHouseSelected", true);
		}

	}

	HouseInfo emptyhouse = {};
	if (targetFound || selectedHouse.Center != emptyhouse.Center)
		return Success;
	else
		return Failure;
}
bool B_Arrived(Blackboard* pBlackboard)
{
	//Get Data
	AgentInfo agentInfo;
	HouseInfo selectedHouse;
	Elite::Vector2 prevHousePos;
	bool isDataAvailable =
		pBlackboard->GetData("SelectedHouse", selectedHouse) &&
		pBlackboard->GetData("AgentInfo", agentInfo) &&
		pBlackboard->GetData("PrevHousePos", prevHousePos);
	if (!isDataAvailable)
		return Failure;

	float distance = Elite::Distance(agentInfo.Position, selectedHouse.Center);

	if (distance < 2.0f)
	{
		pBlackboard->ChangeData("PrevHousePos", selectedHouse.Center); //The selectedHouse becomes the previous house
		pBlackboard->ChangeData("PrevPrevHousePos", prevHousePos); //The previous house becomes the house before that. Tis prevents the bot going back and forth between houses.
		pBlackboard->ChangeData("SelectedHouse", HouseInfo());
		pBlackboard->ChangeData("HaveHouseSelected", false);
		return Success;
	}

	return Failure;
}

//bool NotCloseToTarget(Blackboard* pBlackboard)
//{
//	//Get data
//	AgentInfo* pAgent = nullptr; //Can't use auto because won't know the type...
//	Elite::Vector2 targetPos {0,0};
//	float closeToRadius = 0.f;
//	bool dataAvailable =
//		pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("Target", targetPos) &&
//		pBlackboard->GetData("CloseToRadius", closeToRadius);
//
//	if (!dataAvailable)
//		return false;
//
//	if (!pAgent)
//		return false;
//
//	return Elite::Distance(pAgent->Position, targetPos) > closeToRadius;
//}
//
//bool IsBoxNotOpen(Blackboard* pBlackboard)
//{
//	//Get data
//	auto isBoxOpen = false;
//	auto dataAvailable = pBlackboard->GetData("IsBoxOpen", isBoxOpen);
//	if (!dataAvailable)
//		return false;
//
//	if (isBoxOpen)
//		printf("       Hooray, the box is already open! \n");
//
//	return !isBoxOpen;
//}
//
//BehaviorState ChangeToSeek(Blackboard* pBlackboard)
//{
//	//Get data
//	ISteeringBehaviour* pSeek = nullptr;
//	SteeringAgent* pAgent = nullptr;
//	auto dataAvailable =
//		pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("SeekBehavior", pSeek);
//	if (!dataAvailable)
//		return Failure;
//
//	if (!pSeek || !pAgent)
//		return Failure;
//
//	if (pAgent->GetSteeringBehaviour() != pSeek)
//	{
//		printf("Come over here \n");
//		pAgent->SetSteeringBehaviour(pSeek);
//	}
//	return Success;
//}
//
//BehaviorState ChangeToWander(Blackboard* pBlackboard)
//{
//	//Get data
//	ISteeringBehaviour* pWander = nullptr;
//	SteeringAgent* pAgent = nullptr;
//	auto dataAvailable =
//		pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("WanderBehaviour", pWander);
//	if (!dataAvailable)
//		return Failure;
//
//	if (!pWander || !pAgent)
//		return Failure;
//
//	if (pAgent->GetSteeringBehaviour() != pWander)
//	{
//		pAgent->SetSteeringBehaviour(pWander);
//		printf("Just miding my own business... \n");
//	}
//	return Success;
//}
//
//BehaviorState OpenBox(Blackboard* pBlackboard)
//{
//	printf("       Opening Box \n");
//	return Success;
//}
//
//BehaviorState PickupItem(Blackboard* pBlackboard)
//{
//	pBlackboard->ChangeData("TargetSet", false);
//
//	printf("		Taking item! \n");
//	return Success;
//}