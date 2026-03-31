// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"

// Sets default values
AUnit::AUnit()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

uint64_t AUnit::GetTeam() const
{
	return team;
}

void AUnit::SetTeam(uint64_t in_team)
{
	team = in_team;
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUnit::Hitted(uint64_t damage)
{
	if (damage > armor) {
		damage -= armor;
	} else {
		damage = 1;
	}
	if (damage > hp) {
		hp = 0;
		Destroy();
	} else {
		hp -= damage;
	}
}