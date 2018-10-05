#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "Blackboard.h"
#include "SteeringBehaviours.h"
#include "Behaviors.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Mol";
	info.Student_FirstName = "Senne";
	info.Student_LastName = "Hawer";
	info.Student_Class = "2DAE1";

	//AgentInfo
	m_AgentInfo = m_pInterface->Agent_GetInfo();

	//Memory
	for (int i = 0; i < 5; ++i)
	{
		m_Memory.InventoryMap[i] = -1;
	}

	//BlackBoard
	InitBlackBoard();

	//Behaviour Tree

}

//Called only once
void Plugin::DllInit()
{
	//Can be used to figure out the source of a Memory Leak
	//Possible undefined behavior, you'll have to trace the source manually 
	//if you can't get the origin through _CrtSetBreakAlloc(0) [See CallStack]
	//_CrtSetBreakAlloc(0);

	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded

	for (auto pb : m_pSteeringBehavioursVec)
		SAFE_DELETE(pb);
	m_pSteeringBehavioursVec.clear();

	SAFE_DELETE(m_pBehaviourTree);
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
							//params.LevelFile = "LevelTwo.gppl";
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.OverrideDifficulty = false; //Override Difficulty?
	params.Difficulty = 1.f; //Difficulty Override: 0 > 1 (Overshoot is possible, >1)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::ProcessEvents(const SDL_Event& e)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	switch (e.type)
	{
	case SDL_MOUSEBUTTONUP:
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(x), static_cast<float>(y));
			m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
		}
		break;
	}
	case SDL_KEYDOWN:
	{
		if (e.key.keysym.sym == SDLK_SPACE)
		{
			m_CanRun = true;
		}
		else if (e.key.keysym.sym == SDLK_LEFT)
		{
			m_AngSpeed -= Elite::ToRadians(10);
		}
		else if (e.key.keysym.sym == SDLK_RIGHT)
		{
			m_AngSpeed += Elite::ToRadians(10);
		}
		else if (e.key.keysym.sym == SDLK_g)
		{
			m_GrabItem = true;
		}
		else if (e.key.keysym.sym == SDLK_u)
		{
			m_UseItem = true;
		}
		else if (e.key.keysym.sym == SDLK_r)
		{
			m_RemoveItem = true;
		}
		else if (e.key.keysym.sym == SDLK_d)
		{
			m_DropItem = true;
		}
		break;
	}
	case SDL_KEYUP:
	{
		if (e.key.keysym.sym == SDLK_SPACE)
		{
			m_CanRun = false;
		}
		break;
	}
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto steering = SteeringPlugin_Output();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	//auto agentInfo = m_pInterface->Agent_GetInfo();
	m_AgentInfo = m_pInterface->Agent_GetInfo();

	//Retrieve the current location of our CheckPoint
	auto checkpointLocation = m_pInterface->World_GetCheckpointLocation();

	//Use the navmesh to calculate the next navmesh point
	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);

	//OR, Use the mouse target
	//auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target); //Uncomment this to use mouse position as guidance
	//auto nextTargetPos = m_Target;
	
	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	//Entity Info Demo
	//****************
	/*for (auto entity : vEntitiesInFOV)
	{
	if (entity.Type == eEntityType::ENEMY)
	{
	//Gather Enemy Info
	EnemyInfo eInfo = {};
	m_pInterface->Enemy_GetInfo(entity, eInfo);

	//Set Enemy Tag
	m_pInterface->Enemy_SetTag(eInfo, eInfo.Tag + 1);
	}
	else if (entity.Type == eEntityType::ITEM)
	{
	//Gather Item Info
	ItemInfo iInfo = {};
	m_pInterface->Item_Grab(entity, iInfo);

	//Retrieve Additional Data (Metadata)
	if (iInfo.Type == eItemType::PISTOL) //Available Metadata: ammo/dps/range
	{
	//CheapVariant can be casted to UINT/INT/FLOAT/BOOL
	int ammo = m_pInterface->Item_GetMetadata(iInfo, "ammo"); //INT
	float dps = m_pInterface->Item_GetMetadata(iInfo, "dps"); //FLOAT
	float range = m_pInterface->Item_GetMetadata(iInfo, "range"); //FLOAT
	}
	else if (iInfo.Type == eItemType::FOOD)
	{
	int energy = m_pInterface->Item_GetMetadata(iInfo, "energy"); //INT
	}
	else if (iInfo.Type == eItemType::MEDKIT)
	{
	int health = m_pInterface->Item_GetMetadata(iInfo, "health"); //INT
	}
	}
	}
	*/

	//INVENTORY USAGE DEMO
	//********************

	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(0, item);
		}
	}

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(0);
	}

	if (m_DropItem)
	{
		//Drop an item > Get dropped at players position (Rehashed)
		m_pInterface->Inventory_DropItem(0);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(0);
	}

	//Simple Seek Behaviour (towards Target)
	//steering.LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	//steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	//steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

	//if (Distance(nextTargetPos, agentInfo.Position) < 2.f)
	//{
	//	steering.LinearVelocity = Elite::ZeroVector2;
	//}

	m_Target = nextTargetPos;

	//ENEMIES
	auto vEnemies = GetEnemiesInFOV();
	if (!vEnemies.empty())
	{
		m_CanSeeEnemy = true;
	}
	else
		m_CanSeeEnemy = false;

	//ITEMS
	bool canSeeItems;
	auto vItems = GetItemsInFOV();
	if (vItems.size() == 0) canSeeItems = false;
	else
		canSeeItems = true;

	//if (m_AgentInfo.Bitten)
	//	std::cout << "Bitten" << std::endl;

	////TESTING
	//auto playerAngle = Elite::GetOrientationFromVelocity(m_AgentInfo.LinearVelocity);
	//if (playerAngle < 0)
	//{
	//	playerAngle += M_PI * 2;
	//}
	//std::cout << playerAngle * (180.0f / M_PI) << std::endl;


	bool haveHouseSelected;

	//*** Change blackboard data ***
	auto pBlackboard = m_pBehaviourTree->GetBlackboard();
	if (pBlackboard)
	{
		pBlackboard->GetData("HaveHouseSelected", haveHouseSelected);

		pBlackboard->ChangeData("AgentInfo", m_AgentInfo);
		if (!haveHouseSelected)
			pBlackboard->ChangeData("TargetPosition", m_Target);

		pBlackboard->ChangeData("CanSeeEnemy", m_CanSeeEnemy);
		pBlackboard->ChangeData("CanSeeItem", canSeeItems);
		pBlackboard->ChangeData("ElapsedSec", dt);
	}

	//*** Update BehaviorTree ***
	m_pBehaviourTree->Update();

	//*** Get Steering Output ***
	pBlackboard->GetData("SteeringOutput", steering);

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrientate = true; //Setting AutoOrientate to TRue overrides the AngularVelocity

	bool canRun;
	pBlackboard->GetData("CanRun", canRun);

	steering.RunMode = m_CanRun || canRun; //The OR makes sure I can still run with spacebar
								//If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

								 //SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

								 //@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;
	m_DropItem = false;



	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

vector<EnemyInfo> Plugin::GetEnemiesInFOV() const
{
	vector<EnemyInfo> vEnemiesInFOV = {};

	EnemyInfo enemyInfo = {};
	EntityInfo entityInfo = {};

	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, entityInfo))
		{
			if (entityInfo.Type == eEntityType::ENEMY)
			{
				if (m_pInterface->Enemy_GetInfo(entityInfo, enemyInfo))
					vEnemiesInFOV.push_back(enemyInfo);
			}
		continue;
		}
		break;
	}

	return vEnemiesInFOV;
}

vector<EntityInfo> Plugin::GetItemsInFOV() const
{
	vector<EntityInfo> vItemsInFOV = {};
  
	EntityInfo entityInfo = {};

	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, entityInfo))
		{
			if (entityInfo.Type == eEntityType::ITEM)
			{
				vItemsInFOV.push_back(entityInfo);
			}
			continue;
		}
		break;
	}

	return vItemsInFOV;
}

//HELPER FUNCTIONS
void Plugin::InitBlackBoard()
{
	//*****************************
	//* Create SteeringBehaviours *
	//*****************************
	auto pSeekBehavior = new Seek(m_pInterface);
	pSeekBehavior->SetTarget(&m_Target);
	m_pSteeringBehavioursVec.push_back(pSeekBehavior);

	auto pPipelineBehaviour = new Pipeline(m_pInterface);
	pPipelineBehaviour->SetTarget(&m_Target);
	m_pSteeringBehavioursVec.push_back(pPipelineBehaviour);

	//*********************
	//* Create Blackboard *
	//*********************

	auto pBlackboard = new Blackboard();
	//AgentInfo
	pBlackboard->AddData("AgentInfo", m_AgentInfo);

	//TargetPosition
	pBlackboard->AddData("TargetPosition", m_Target);

	//ElapsedSec
	pBlackboard->AddData("ElapsedSec", float());

	//RUN
	pBlackboard->AddData("CanRun", bool());
	pBlackboard->AddData("RunCountDown", float());

	//ENEMIES
	////EnemiesInFOV
	pBlackboard->AddData("CanSeeEnemy", bool());
	////ClosestEnemy
	pBlackboard->AddData("ClosestEnemy", EnemyInfo());

	//ITEMS
	////ItemInFOV
	pBlackboard->AddData("CanSeeItem", bool());
	////ClosestItem
	pBlackboard->AddData("ClosestItem", EntityInfo());
	////SelectedItem
	pBlackboard->AddData("SelectedItem", ItemInfo());

	//HOUSES
	pBlackboard->AddData("CanSeeHouse", bool());
	pBlackboard->AddData("PrevHousePos", Elite::Vector2());
	pBlackboard->AddData("PrevPrevHousePos", Elite::Vector2());
	pBlackboard->AddData("HaveHouseSelected", bool());
	pBlackboard->AddData("SelectedHouse", HouseInfo());

	//STATS
	////HealthNeeded
	pBlackboard->AddData("HealthNeeded", int());
	////EnergyNeeded
	pBlackboard->AddData("EnergyNeeded", int());

	//MEMORY
	pBlackboard->AddData("Memory", m_Memory);

	//Interface
	pBlackboard->AddData("Interface", m_pInterface);

	//SeekBehaviour
	pBlackboard->AddData("SeekBehaviour", static_cast<SteeringBehaviour*>(pSeekBehavior));

	//PipelineBehaviour
	pBlackboard->AddData("PipelineBehaviour", static_cast<SteeringBehaviour*>(pPipelineBehaviour));

	//Steering Output
	auto pSteeringOutput = SteeringPlugin_Output();
	pBlackboard->AddData("SteeringOutput", pSteeringOutput);

	//*********************
	//* Create Blackboard *
	//*********************
	m_pBehaviourTree = new BehaviorTree(pBlackboard,
		new BehaviorSelector({
			new BehaviorSelector//STATS
				({
					new BehaviorSequence
						({
							new BehaviorConditional(B_CheckHealth),
							new BehaviorConditional(B_TakeRightMedKit)
						}),
					new BehaviorSequence
						({
							new BehaviorConditional(B_CheckEnergy),
							new BehaviorConditional(B_TakeRightFood)
						})
				}),
			new BehaviorSelector //ENEMIES
				({
					new BehaviorSequence
					({
						new BehaviorConditional(B_CheckForEnemies),
						new BehaviorConditional(B_GetClosestEnemy),

						new BehaviorSelector
						({
							new BehaviorSequence //SHOOT
								({
									new BehaviorConditional(B_CheckGun),
									new BehaviorConditional(B_AimNew),
									new BehaviorConditional(B_Shoot)
								}),

							new BehaviorSequence //AVOID
								({
									new BehaviorConditional(B_FindPointAround),
									new BehaviorConditional(B_StartRunning),
									new BehaviorConditional(B_SeekNew)
								})
						}),
					}),
					new BehaviorConditional(B_StopRunning)
				}),
			new BehaviorSequence //HOUSES
				({
					new BehaviorConditional(B_CanSeeHouse),
					new BehaviorConditional(B_SeekNew),
					new BehaviorConditional(B_Arrived)
				}),
			new BehaviorSequence //ITEMS
				({
					new BehaviorConditional(B_CheckForItems),
					new BehaviorConditional(B_GetClosestItem),
					new BehaviorConditional(B_SeekNew),
					new BehaviorConditional(B_IsItemInRange),
					new BehaviorSelector
					({ 
						//GUN
						new BehaviorSequence
						({
								new BehaviorConditional(B_IsGun),
								new BehaviorSelector
								({
									new BehaviorSequence
									({
										new BehaviorConditional(B_HasGun),
										new BehaviorConditional(B_CheckAmmo)
									}),
									new BehaviorConditional(B_AddGun)
								}),
						}),
						//MEDKIT
						new BehaviorSequence
						({
								new BehaviorConditional(B_IsMedkit),
								new BehaviorSelector
								({
									new BehaviorConditional(B_EmptySlot),
									new BehaviorSequence
									({
										new BehaviorConditional(B_HasMedkit),
										new BehaviorConditional(B_GetLowestMedKit),
									}),
								}),
						}),
						//FOOD
						new BehaviorSequence
						({
								new BehaviorConditional(B_IsFood),
								new BehaviorSelector
								({
									new BehaviorConditional(B_EmptySlot),
									new BehaviorSequence
									({
										new BehaviorConditional(B_HasFood),
										new BehaviorConditional(B_GetLowestFood)
									})
								})
						}),
						new BehaviorConditional(B_IsGarbage)
					}),
				}),
			new BehaviorSelector //CHECKPOINT
				({
					new BehaviorSequence
					({
						new BehaviorConditional(B_OnBitten),
						new BehaviorConditional(B_StartRunning),
						new BehaviorConditional(B_SeekNew)
					}),
					new BehaviorConditional(B_StopRunning),
					//new BehaviorConditional(B_StopRunning),
					new BehaviorConditional(B_SeekNew)

				})
			})
		);
}


