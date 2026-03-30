// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Unit.generated.h"

UCLASS()
class LASTDAY_API AUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUnit();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	uint64_t GetTeam() const;
	void SetTeam(uint64_t team);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Hitted(uint64_t damage);

private:
	uint64_t id;
	std::string name;
	uint64_t team;
	uint64_t hp;
	uint64_t armor;
};
