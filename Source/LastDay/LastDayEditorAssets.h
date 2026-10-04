// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * 确保项目里存在运行期需要的资源（目前只有爆炸用的半透材质）。
 * 只在编辑器下真正干活：材质不存在时自动生成 /Game/FX/M_Explosion 并保存。
 */
void EnsureLastDayEditorAssets();
