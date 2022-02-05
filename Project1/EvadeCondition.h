#pragma once

#include "ISteeringCondition.h"
#include "EvadeBehaviour.h"

class EvadeCondition : public ISteeringCondition
{
public:
	EvadeCondition(EvadeBehaviour* behaviour);
	virtual ~EvadeCondition();

	virtual bool CanUse(const std::unordered_map<unsigned int, Entity*>& entities) override;
	virtual bool CanContinueToUse(const std::unordered_map<unsigned int, Entity*>& entities) override;
	virtual void Update(float deltaTime) override;
	virtual void OnStart() override;
	virtual void OnStop() override;
	virtual ISteeringBehaviour* GetBehaviour() override { return behaviour; }

private:
	Entity* TryFindBullet(const std::unordered_map<unsigned int, Entity*>& entities);

	EvadeBehaviour* behaviour;

};