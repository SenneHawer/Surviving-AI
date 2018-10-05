#include "stdafx.h"
#include "SteeringBehaviours.h"
#include <IExamInterface.h>

///////////////////////////////////////
//SEEK
//****
SteeringPlugin_Output Seek::CalculateSteering(AgentInfo agentInfo)
{
	SteeringPlugin_Output steering = {};

	steering.LinearVelocity = *m_pTargetPosition - agentInfo.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed;
	//steering.LinearVelocity *= 20;
	//Debug Rendering
	////if (drawDebug)
	m_pInterface->Draw_Direction(agentInfo.Position, steering.LinearVelocity, steering.LinearVelocity.Magnitude(), { 1.0f, 0.0f, 0.0f });

	return steering;
}

///////////////////////////////////////
//PIPELINE
//********
SteeringPlugin_Output Pipeline::CalculateSteering(AgentInfo agentInfo)
{
	SteeringPlugin_Output steering = {};
	auto enemies = GetEnemiesInFOV();
	m_pSeek->SetTarget(m_pTargetPosition);

	float shortestViolation, currentViolation, maxViolation;

	for (UINT i {0}; i < m_MaxConstraintSteps; ++i)
	{
		if (enemies.size() == 0) //No enities in FOV
		{
			m_pInterface->Draw_Segment(agentInfo.Position, *m_pTargetPosition, { 0,1,0 }, 0);
			m_pInterface->Draw_SolidCircle(*m_pTargetPosition, 0.5f, { 0,0 }, { 0,1,0 });
			m_pInterface->Draw_Segment(*m_pTargetPosition, m_pInterface->World_GetCheckpointLocation(), { 0,1,0 }, 0);

			steering = m_pSeek->CalculateSteering(agentInfo);
			return steering;
		}

		shortestViolation = maxViolation = m_MaxPriority;

		currentViolation = FindSmallestPriority(*m_pTargetPosition, agentInfo, enemies);
		if (currentViolation < shortestViolation && currentViolation > 0)
		{
			shortestViolation = currentViolation;
		}
	}

	if (shortestViolation < maxViolation)
	{
		m_pTargetPosition = &m_SuggestedTarget;
	}
	else
	{
		steering = m_pSeek->CalculateSteering(agentInfo);
		return steering;
	}

	//DRAW DEBUG
	{
		m_pInterface->Draw_Segment(agentInfo.Position, *m_pTargetPosition, { 0,1,0 }, 0);
		m_pInterface->Draw_SolidCircle(*m_pTargetPosition, 0.5f, { 0.0f, 0.0f }, { 0.f, 1.f, 0.0f }, 1);
		//m_pInterface->Draw_Segment(currTarget, target, { 0,1,0 }, 0);
	}

	steering = m_pSeek->CalculateSteering(agentInfo);

	return steering;
}

std::vector<EnemyInfo> Pipeline::GetEnemiesInFOV() const
{
	std::vector<EnemyInfo> vEnemiesInFOV = {};

	EnemyInfo enemyInfo = {};
	EntityInfo entityInfo = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, entityInfo))
		{
			if (m_pInterface->Enemy_GetInfo(entityInfo, enemyInfo))
			{
				vEnemiesInFOV.push_back(enemyInfo);
				continue;
			}
		}

		break;
	}

	return vEnemiesInFOV;
}
float Pipeline::FindSmallestPriority(Elite::Vector2 target, AgentInfo agentInfo, const std::vector<EnemyInfo>& enemies)
{
	float currPriority = 0.0f;
	float smallestPriority = m_MaxPriority;

	if (enemies.size() == 0)
		return smallestPriority;

	for (auto enemy : enemies)
	{
		currPriority = FindNewTarget(target, agentInfo, enemy);
		if (currPriority < smallestPriority)
		{
			smallestPriority = currPriority;
		}
	}

	return smallestPriority;
}
float Pipeline::FindNewTarget(Elite::Vector2 target, AgentInfo agent, EnemyInfo enemy)
{
	auto currTarget = target;
	auto agent2Enemy = enemy.Location - agent.Position; //Line Between agent and enemy
	auto dir2Target = (currTarget - agent.Position).GetNormalized(); //Direction to taget
	auto dist2ClosestPoint = Elite::Dot(agent2Enemy, dir2Target);

	if (dist2ClosestPoint < 0.0f || dist2ClosestPoint >= m_MaxPriority)
	{
		return (numeric_limits<float>::max)(); //Brakets needed because windows.h has a max Macro...
	}

	auto closestPointToEnemy = agent.Position + (dist2ClosestPoint * dir2Target);

	auto dirOffset = (closestPointToEnemy - enemy.Location); //Offest to move closestPoint outside obstacle if needed
	auto distFromObstacleCenter = dirOffset.Normalize();

	if (distFromObstacleCenter > enemy.Size + m_AvoidMargin)
	{
		return (numeric_limits<float>::max)();
	}

	auto newTarget = enemy.Location + ((enemy.Size + m_AvoidMargin)*dirOffset);

	auto dist2NewTarget = Elite::Distance(agent.Position, newTarget);
	if (dist2NewTarget >= m_MaxPriority)
	{
		return (numeric_limits<float>::max)();
	}

	m_SuggestedTarget = newTarget;

	return dist2NewTarget;
}
