#ifndef BEHAVIOR_TREE_H
#define BEHAVIOR_TREE_H

//Includes
#include "Blackboard.h"

//-----------------------------------------------------------------
// BEHAVIOR TREE HELPERS
//-----------------------------------------------------------------
enum BehaviorState
{
	Failure,
	Success,
	Running
};

//-----------------------------------------------------------------
// BEHAVIOR INTERFACES (BASE)
//-----------------------------------------------------------------
class IBehavior
{
public:
	IBehavior(){}
	virtual ~IBehavior(){}
	virtual BehaviorState Execute(Blackboard* pBlackBoard) = 0;

protected:
	BehaviorState m_CurrentState = Failure;
};

//-----------------------------------------------------------------
// BEHAVIOR TREE COMPOSITES (IBehavior)
//-----------------------------------------------------------------
#pragma region COMPOSITES
class BehaviorComposite : public IBehavior
{
public:
	explicit BehaviorComposite(std::vector<IBehavior*> childrenBehaviors)
	{
		m_ChildrenBehaviors = childrenBehaviors;
	}
	virtual ~BehaviorComposite()
	{
		for (auto pb : m_ChildrenBehaviors)
			SAFE_DELETE(pb);
		m_ChildrenBehaviors.clear();
	}
	virtual BehaviorState Execute(Blackboard* pBlackBoard) override = 0;

protected:
	std::vector<IBehavior*> m_ChildrenBehaviors = {};
};

class BehaviorSelector : public BehaviorComposite
{
public:
	explicit BehaviorSelector(std::vector<IBehavior*> childrenBehaviors) :
		BehaviorComposite(childrenBehaviors)
	{}
	virtual ~BehaviorSelector()
	{}

	virtual BehaviorState Execute(Blackboard* pBlackBoard) override;
};

class BehaviorSequence : public BehaviorComposite
{
public:
	explicit BehaviorSequence(std::vector<IBehavior*> childrenBehaviors) :
		BehaviorComposite(childrenBehaviors)
	{}
	virtual ~BehaviorSequence()
	{}

	virtual BehaviorState Execute(Blackboard* pBlackBoard) override;
};

class BehaviorPartialSequence : public BehaviorSequence
{
public:
	explicit BehaviorPartialSequence(std::vector<IBehavior*> childrenBehaviors)
		: BehaviorSequence(childrenBehaviors)
	{}
	virtual ~BehaviorPartialSequence() {};
	virtual BehaviorState Execute(Blackboard* pBlackBoard) override;

private:
	unsigned int m_CurrentBehaviorIndex = 0;
};
#pragma endregion

//-----------------------------------------------------------------
// BEHAVIOR TREE CONDITIONAL (IBehavior)
//-----------------------------------------------------------------
class BehaviorConditional : public IBehavior
{
public:
	explicit BehaviorConditional(std::function<bool(Blackboard*)> fp) : m_fpConditional(fp)
	{}
	virtual BehaviorState Execute(Blackboard* pBlackBoard) override;

private:
	std::function<bool(Blackboard*)> m_fpConditional = nullptr;
};

//-----------------------------------------------------------------
// BEHAVIOR TREE ACTION (IBehavior)
//-----------------------------------------------------------------
class BehaviorAction : public IBehavior
{
public:
	explicit BehaviorAction(std::function<BehaviorState(Blackboard*)> fp) : m_fpAction(fp)
	{}
	virtual BehaviorState Execute(Blackboard* pBlackBoard) override;

private:
	std::function<BehaviorState(Blackboard*)> m_fpAction = nullptr;
};

//-----------------------------------------------------------------
// BEHAVIOR TREE (BASE)
//-----------------------------------------------------------------
class BehaviorTree final
{
public:
	explicit BehaviorTree(Blackboard* pBlackBoard, IBehavior* pRootComposite) 
		: m_pBlackBoard(pBlackBoard), m_pRootComposite(pRootComposite)
	{};
	~BehaviorTree()
	{
		SAFE_DELETE(m_pRootComposite);
		SAFE_DELETE(m_pBlackBoard);
	};

	BehaviorState Update()
	{
		if (m_pRootComposite == nullptr)
			return m_CurrentState = Failure;

		return m_CurrentState = m_pRootComposite->Execute(m_pBlackBoard);
	}
	Blackboard* GetBlackboard() const
	{ return m_pBlackBoard; }

private:
	BehaviorState m_CurrentState = Failure;
	Blackboard* m_pBlackBoard = nullptr;
	IBehavior* m_pRootComposite = nullptr;
};
#endif
