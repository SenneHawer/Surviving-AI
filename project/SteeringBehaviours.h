#pragma once
#include <Exam_HelperStructs.h>
#include <IExamInterface.h>

//class IExamInterface;

/////////////////////////////////////////
//BASE CLASS
//**********
class SteeringBehaviour
{
public:
	SteeringBehaviour(IExamInterface* pInterface) :m_pInterface(pInterface) {}
	virtual ~SteeringBehaviour() {}

	virtual SteeringPlugin_Output CalculateSteering(AgentInfo agentInfo) = 0;
protected:
	IExamInterface* m_pInterface = nullptr;
};

///////////////////////////////////////
//SEEK
//****
class Seek : public SteeringBehaviour
{
public:
	Seek(IExamInterface* pInterface) :SteeringBehaviour(pInterface) {}
	virtual ~Seek() {}

	SteeringPlugin_Output CalculateSteering(AgentInfo agentInfo);

	void SetTarget(Elite::Vector2* targetPosition) { m_pTargetPosition = targetPosition; }

private:
	Elite::Vector2* m_pTargetPosition = nullptr;
};

///////////////////////////////////////
//PIPELINE
//********
class Pipeline : public SteeringBehaviour
{
public:
	Pipeline(IExamInterface* pInterface) :SteeringBehaviour(pInterface)
	{
		m_pSeek = new Seek(pInterface);
		m_pSeek->SetTarget(m_pTargetPosition);
	}
	virtual ~Pipeline()
	{
		SAFE_DELETE(m_pSeek);
	}

	SteeringPlugin_Output CalculateSteering(AgentInfo agentInfo);

	void SetTarget(Elite::Vector2* targetPosition) { m_pTargetPosition = targetPosition; }

private:
	Elite::Vector2* m_pTargetPosition = nullptr;
	Seek* m_pSeek = nullptr;

	int m_MaxConstraintSteps = 10;
	float m_MaxPriority = 50.0f;
	float m_AvoidMargin = 2.0f;
	Elite::Vector2 m_SuggestedTarget;

	//HelperFunctions
	std::vector<EnemyInfo> GetEnemiesInFOV() const;
	float FindSmallestPriority(Elite::Vector2 target, AgentInfo agentInfo, const std::vector<EnemyInfo>& enemies);
	float Pipeline::FindNewTarget(Elite::Vector2 target, AgentInfo agent, EnemyInfo entity);
};
