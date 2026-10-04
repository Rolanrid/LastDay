// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastDay.h"
#include "LastDayEditorAssets.h"
#include "Modules/ModuleManager.h"
#include "Misc/CoreDelegates.h"

/**
 * 项目模块：除了默认行为，还负责在编辑器下补齐代码运行所需的资源
 * （目前是自动生成爆炸用的半透材质 /Game/FX/M_Explosion）。
 */
class FLastDayModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if WITH_EDITOR
		if (GIsEditor)
		{
			// 引擎初始化完成后再动资源系统；如果此刻引擎已经就绪就直接执行
			FCoreDelegates::OnPostEngineInit.AddStatic(&EnsureLastDayEditorAssets);
			if (GEngine)
			{
				EnsureLastDayEditorAssets();
			}
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FLastDayModule, LastDay, "LastDay" );

DEFINE_LOG_CATEGORY(LogLastDay)
