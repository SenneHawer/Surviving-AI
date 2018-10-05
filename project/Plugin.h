#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "BehaviorTree.h"
#include "SteeringBehaviours.h"
#include "SenUtils.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void ProcessEvents(const SDL_Event& e) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//My Variables
	BehaviorTree* m_pBehaviourTree = nullptr;
	AgentInfo m_AgentInfo;
	std::vector<SteeringBehaviour*> m_pSteeringBehavioursVec = {};
	bool m_CanSeeEnemy;
	sen::Memory m_Memory;

	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;
	vector<EnemyInfo> GetEnemiesInFOV() const;
	vector<EntityInfo> GetItemsInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	bool m_DropItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose


	//Helper Functions
	void InitBlackBoard();

};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}