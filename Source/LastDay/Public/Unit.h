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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnHit(uint64_t damage);

	uint64_t id;
	std::string name;
	uint64_t team;
	uint64_t hp; // 应该用一个component来承载
	uint64_t armor;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
