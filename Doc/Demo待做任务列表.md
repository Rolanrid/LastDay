# Demo 待做任务列表

> 引擎：UE5.7.4　｜　开发方式：由模块责任人决定　｜　原则：优先复用 UE 内置功能
> 任务粒度：每个任务约 **30分钟** 工作量

---

## 模块总览与依赖关系

```
A.项目基础 ──→ B.玩家角色 ──→ C.FPS/RTS混合输入
     │              │                    │
     └──────→ D.资源系统 ←──────┘        │
                  │                    │
                  └──→ E.建造系统 ──→ F.单位系统
                           │              │
                  G.敌人系统 ←────────────┘
                       │
                  H.波次/昼夜系统
                       │
                  I.胜负判定
                       │
                  J.完整HUD
```

---

## A. 项目基础 与 灰盒场景

### A1. 创建项目和初始关卡 —— Rolanrid
- [x] **A1-1**　创建 UE5 空白项目，选择 BP 模板（First Person 模板可选，但后面需清理不需要的内容）
- [x] **A1-2**　创建空关卡 `DemoMap`，设为默认地图
- [ ] ~~**A1-3**　搭建 100m×100m 灰盒地面：拖一个 `Plane` 或 `Cube`，Scale 设为 100×1×100，用灰色材质~~（跳过，复用模板场景）
- [ ] ~~**A1-4**　在四周放置 4 个拉伸方块作为边界围墙（或直接用碰撞体做空气墙）~~（跳过，复用模板场景）
- [x] **A1-5**　放置 Ground Plane 的 `NavMeshBoundsVolume` 覆盖整个地面（后续 AI 寻路用）

### A2. 基础光照与天空
- [ ] **A2-1**　放置 `DirectionalLight` + `SkyLight` + `SkyAtmosphere` 作为基础天空
- [ ] **A2-2**　调整光照强度，让场景白天状态清晰明亮
- [ ] **A2-3**　新建一个空 Actor 或使用关卡蓝图保存"白天光照参数"和"夜晚光照参数"两组变量（Light Intensity、Sky Color、Fog 等），后续 H 模块切换

### A3. GameMode 与 GameState
- [ ] **A3-1**　创建 `BP_GameMode_Demo`，继承 GameModeBase，设置 DefaultPawn（暂为空，后面替换）
- [ ] **A3-2**　创建 `BP_GameState_Demo`，继承 GameStateBase。添加变量：`当前波次(int)`、`当前阶段(enum: 着陆/白天/黄昏/夜晚/战后)`、`剩余敌人数量(int)`
- [ ] **A3-3**　将 GameMode 和 GameState 在项目设置中配置为默认

### A4. 输入配置（Enhanced Input）
- [ ] **A4-1**　创建 `IMC_Default`（InputMappingContext），配置所有按键映射（见下方键位表）
- [ ] **A4-2**　创建 InputAction 资产：`IA_Move`、`IA_Look`、`IA_Jump`、`IA_Shoot`、`IA_AltRTS`、`IA_Select`、`IA_Command`、`IA_Build1`、`IA_Build2`、`IA_Interact`
- [ ] **A4-3**　设置 IA_AltRTS 为"按住触发"，IA_Jump/IA_Shoot/IA_Build 为"按下触发"

> **复用的 UE 功能**：UE5 Enhanced Input System 完整接管输入，无需手写按键检测。

---

## B. 玩家角色

### B1. 角色基础
- [ ] **B1-1**　创建 `BP_PlayerCharacter`，继承 Character。添加 `CapsuleComponent`(自带)、`CameraComponent`(挂载到 Capsule 头部)
- [ ] **B1-2**　角色 Mesh 用 UE 自带的 `Capsule` 或 `Cube` 静态网格体（灰盒外观）
- [ ] **B1-3**　将 `CameraComponent` 设为第一人称位置（Z 偏移=胶囊体高度-10），并旋转跟随鼠标

### B2. 移动与跳跃（复用 UE CharacterMovementComponent）
- [ ] **B2-1**　在 `BP_PlayerCharacter` 中绑定 `IA_Move`、`IA_Look`、`IA_Jump` 事件到 Enhanced Input
- [ ] **B2-2**　Move 逻辑：读取 InputActionValue XY，调用 `Add Movement Input`（复用 Character 内置移动）
- [ ] **B2-3**　Look 逻辑：读取 InputActionValue XY，调用 `Add Controller Yaw Input` / `Add Controller Pitch Input`
- [ ] **B2-4**　Jump 逻辑：调用 Character 自带的 `Jump` 函数
- [ ] **B2-5**　调参：WalkSpeed=600、JumpZ=420、关闭 UseControllerRotationRoll/Pitch（仅 Yaw 跟随）

### B3. 射击系统（Hitscan + 射线检测）
- [ ] **B3-1**　在摄像机前方附加一个方块 StaticMesh 作为"枪"的视觉模型（纯装饰，不需要骨骼）
- [ ] **B3-2**　绑定 `IA_Shoot` 事件，在事件中执行 `LineTraceByChannel`（起点=摄像机位置，终点=摄像机位置+前向*射程，Visibility通道）
- [ ] **B3-3**　命中后：调用命中 Actor 的"受到伤害"接口 / 自定义事件。输出 Debug 射线（红色=命中，蓝色=未命中），后续做伤害时复用
- [ ] **B3-4**　添加射击间隔限制：用 `DoOnce` + `Delay` 或 float 变量 `上次射击时间`，射速=0.15秒/发

### B4. 玩家血量
- [ ] **B4-1**　在 `BP_PlayerCharacter` 中添加变量：`当前HP(Float,默认100)`、`最大HP(Float,默认100)`
- [ ] **B4-2**　创建自定义事件 `受到伤害(Damage:Float)`，内部：当前HP -= Damage → 判断 ≤0 则广播"玩家死亡"事件
- [ ] **B4-3**　创建自定义事件 `玩家死亡`，由 GameMode 监听以触发失败流程

> **复用的 UE 功能**：CharacterMovementComponent（移动/跳跃/重力）、LineTraceByChannel（射击命中）、Enhanced Input（输入管理）

---

## C. FPS + RTS 混合输入 —— 建议Demo中不实现

### C1. 模式切换
- [ ] **C1-1**　在 `BP_PlayerCharacter` 中添加变量：`当前模式(Enum: FPS/RTS)`、`是否按住Alt(Bool)`
- [ ] **C1-2**　绑定 `IA_AltRTS` 的 `Started` 事件 → 切换到 RTS模式。`Completed` 事件 → 切换回 FPS模式
- [ ] **C1-3**　RTS模式时：禁用 Look 输入和 Shoot 输入；显示鼠标光标（`Set Show Mouse Cursor = true`），启用鼠标点击事件
- [ ] **C1-4**　FPS模式时：恢复 Look/Shoot 输入；隐藏鼠标光标，鼠标锁定到屏幕中心

### C2. 框选单位
- [ ] **C2-1**　在 RTS 模式下，鼠标左键按下 → 记录"起始屏幕坐标"；拖动鼠标 → 实时更新"结束屏幕坐标"
- [ ] **C2-2**　鼠标左键释放 → 执行框选检测：用 `Get Hit Result Under Cursor` 获取起始和结束的世界坐标 → 构建选择矩形
- [ ] **C2-3**　用 `Get All Actors of Class`（单位基类）→ 遍历判断每个单位是否在矩形内（`Is In Box` 或手动比较XY坐标）→ 加入选中列表
- [ ] **C2-4**　选中单位添加高亮：调用单位的自定义事件"被选中"→ 单位修改材质颜色（如黄色）

### C3. 指挥单位移动
- [ ] **C3-1**　RTS 模式下，鼠标右键点击地面 → `Get Hit Result Under Cursor` 获取目标世界位置
- [ ] **C3-2**　遍历当前选中单位列表 → 对每个单位调用"移动到(目标位置)"事件
- [ ] **C3-3**　右键点击敌人时 → 改为攻击指令（调用单位的"攻击目标(敌人引用)"事件）

### C4. 选中取消
- [ ] **C4-1**　单击空白地面（无拖动）→ 清空选中列表，取消所有单位高亮
- [ ] **C4-2**　切回 FPS 模式时 → 自动清空选中列表

> **复用的 UE 功能**：GetHitResultUnderCursor、GetAllActorsOfClass、Collision/Overlap检测

---

## D. 资源系统

### D1. 源晶矿脉
- [ ] **D1-1**　创建 `BP_SourceCrystal`，继承 Actor。添加 `StaticMeshComponent`，用 UE 自带发光材质或纯色方块（绿色/青色）
- [ ] **D1-2**　添加变量：`最大储量(Float,默认200)`、`当前储量(Float,默认200)`
- [ ] **D1-3**　创建自定义事件 `被采集(采集量:Float)`：当前储量 -= 采集量 → 如果 ≤0 则销毁自身（矿脉枯竭）
- [ ] **D1-4**　在 DemoMap 中手动放置 3-5 个矿脉，坐标分散在场景各处

### D2. 采集机制
- [ ] **D2-1**　玩家射击命中矿脉时 → 判断 Hit.Actor 是 `BP_SourceCrystal` → 调用矿脉的"被采集(10)"，同时玩家资源+10
- [ ] **D2-2**　创建 `BP_ResourceManager` 组件，挂载到 GameState 或 PlayerState，管理玩家资源数值
- [ ] **D2-3**　在 GameState 中添加变量：`玩家源晶(Int)`，并添加事件：`增加源晶(数量:Int)`、`消耗源晶(数量:Int):Bool(是否足够)`

### D3. 采集器自动产出
- [ ] **D3-1**　在采集器建筑中（见E模块），用 `Set Timer by Event`，每1秒调用一次"自动产出"
- [ ] **D3-2**　自动产出逻辑：检测3米范围内是否有活跃的矿脉 → 从矿脉扣1点 → 玩家源晶+1

> **复用的 UE 功能**：SetTimerByEvent、LineTraceByChannel（射击检测命中）

---

## E. 建造系统

### E1. 建造预览
- [ ] **E1-1**　 FPS 模式下，按 1 键 → 进入炮塔建造模式，显示炮塔预览（半透明，绿色=可放置，红色=不可放置）
- [ ] **E1-2**　 Tick 中从摄像机做 LineTrace → 在地面命中点放置预览 Actor，合法位置变绿，不合法（重叠）变红

### E2. 炮塔放置
- [ ] **E2-1**　左键确认放置：资源足够时消耗资源 → 在预览位置生成 `BP_Turret`（继承 ATurret C++ 类）→ 清空预览
- [ ] **E2-2**　右键取消建造：清空预览，不消耗资源
- [ ] **E2-3**　建造过程中禁止射击

### E3. 炮塔实现（基于已有 C++ 类 ATurret/UTurretSocketComponent）
- [ ] **E3-1**　创建 `BP_Turret` 继承 `ATurret`，设置 `TurretRoot` 静态网格体（灰盒方块充当炮塔底座）
- [ ] **E3-2**　在蓝图添加 `UTurretSocketComponent` 子组件作为炮管，设置 `FireAngle`（扇形检测角）、`FireRange`（射程）、`ProjectileSpeed`
- [ ] **E3-3**　设置 ATurret 的 `DetectionRadius`（敌人检测半径，默认 2000）
- [ ] **E3-4**　创建 `BP_TurretProjectile` 继承 `ABaseProjectile`，设置碰撞球、静态网格体（炮弹外观）、`ProjectileMovement` 速度
- [ ] **E3-5**　在 `BP_Turret` 中指定 `ProjectileClass` = `BP_TurretProjectile`
- [ ] **E3-6**　在 DemoMap 中放置 2-3 个 `BP_Turret` 作为初始防御

> **已有的 C++ 实现**：ATurret 负责检测范围内的敌人，每个 UTurretSocketComponent 自动扇形搜索目标、发射 ABaseProjectile 炮弹。

---



---

## G. 敌人系统

### G1. 抗体生物基础
- [ ] **G1-1**　创建 `BP_Enemy`，继承 Character。添加 `CapsuleComponent` + `Sphere StaticMesh（颜色=红色）`
- [ ] **G1-2**　添加变量：`当前HP(Float,默认50)`、`攻击力(Float,默认5)`、`移动速度(Float,默认300)`、`攻击范围(Float,默认3m)`、`当前目标建筑(引用)`
- [ ] **G1-3**　在 BeginPlay 中设置 `CharacterMovement.MaxWalkSpeed = 移动速度`

### G2. 寻找建筑目标
- [ ] **G2-1**　BeginPlay 或刷新生效时：`Get All Actors of Class (BP_Collector 和 BP_Barracks)` → 找最近的一个 → 设为 `当前目标建筑`
- [ ] **G2-2**　Tick 中：如果 `当前目标建筑` 有效 → `Simple Move To Actor（目标建筑）`（持续靠近）
- [ ] **G2-3**　如果 `当前目标建筑` 被销毁 → 重新寻找最近的存活建筑

### G3. 攻击建筑
- [ ] **G3-1**　Tick 中检测：与目标建筑的距离 ≤ 攻击范围 → 停止移动 → 按攻击间隔对建筑造成伤害（调用建筑的"受到伤害"事件）
- [ ] **G3-2**　攻击时朝向建筑，可添加简单的"冲撞"动画效果（对 Sphere Mesh 做 Scale 脉冲）

### G4. 敌人受到伤害与死亡
- [ ] **G4-1**　自定义事件 `受到伤害(Damage)`：HP -= Damage → ≤0 则 Destroy 自身
- [ ] **G4-2**　死亡时：通知 GameState `剩余敌人数量-1`，播放简单的销毁效果（缩放到0 或 变灰后延时删除）

### G5. 敌人与玩家的交互
- [ ] **G5-1**　如果玩家挡在敌人和建筑之间 → 敌人继续向建筑移动（不主动攻击玩家），但被碰撞挡路时触发"推开玩家"或绕行
- [ ] **G5-2**　如果建筑全部被毁 → 敌人切换目标为玩家 → 向玩家移动并攻击

> **复用的 UE 功能**：SimpleMoveToActor（追踪建筑）、CharacterMovement、SphereComponent 碰撞体

---

## H. 波次与昼夜系统

### H1. 阶段状态机
- [ ] **H1-1**　在 `BP_GameState_Demo` 中创建枚举：`EDayPhase{着陆, 白天, 黄昏, 夜晚, 战后}`
- [ ] **H1-2**　创建自定义事件 `切换阶段(新阶段:EDayPhase)`：更新当前阶段变量 → 广播给所有监听者（HUD、光照、敌人刷新等）
- [ ] **H1-3**　创建时间线/序列：着陆(30s) → 白天(180s) → 黄昏(15s) → 夜晚(持续到波次清完) → 战后(60s) → 白天(难度+1) → ... 循环3次
- [ ] **H1-4**　切换"夜晚"时：如果波次已达第3波 → 清完第3波直接跳转到"胜利"判定

### H2. 敌人生成
- [ ] **H2-1**　创建 `BP_EnemySpawner`，继承 Actor。在 DemoMap 地图边缘放置 8-12 个刷新点
- [ ] **H2-2**　在 GameState 中监听切换到"夜晚" → 根据当前波次查表获取刷新数量：波次1=5、波次2=10、波次3=15
- [ ] **H2-3**　随机从刷新点列表中选点，间隔 1-2 秒逐个生成 `BP_Enemy`（避免卡顿）
- [ ] **H2-4**　每生成一个敌人 → `剩余敌人数量+1`

### H3. 昼夜光照切换
- [ ] **H3-1**　在关卡蓝图或自定义 Actor 中引用 DirectionalLight
- [ ] **H3-2**　监听 GameState 的"切换阶段"事件 → 白天时用 Timeline 插值光照到白天参数 → 夜晚时插值到夜晚参数
- [ ] **H3-3**　黄昏时加入快速的颜色渐变（从白到橙到暗）

### H4. 波次结束检测
- [ ] **H4-1**　GameState 中监听：每次"剩余敌人数量-1" → 判断 ≤0 → 播"波次清空"→ 切换到"战后"阶段
- [ ] **H4-2**　HUD 在夜晚期间显示"剩余敌人: X"

> **复用的 UE 功能**：Timeline（光照渐变）、SetTimerByEvent（阶段切换倒数）、SpawnActor from Class

---

## I. 胜负判定

### I1. 失败条件
- [ ] **I1-1**　GameState 监听玩家死亡事件 → 触发"游戏失败"流程
- [ ] **I1-2**　GameState Tick 中检测：所有 BP_Collector 和 BP_Barracks 是否都已被销毁 → 触发"游戏失败"流程

### I2. 胜利条件
- [ ] **I2-1**　第3波夜晚的所有敌人被清空 → 触发"游戏胜利"流程

### I3. 结束界面
- [ ] **I3-1**　创建 `WBP_GameOver` Widget，显示文字"胜利！"或"失败！" + 一个"重新开始"按钮
- [ ] **I3-2**　"重新开始"按钮 → 调用 `Open Level（当前关卡名）` 重新加载
- [ ] **I3-3**　触发胜负时：暂停游戏 (`Set Game Paused = true`)，显示 `WBP_GameOver`，显示鼠标光标

> **复用的 UE 功能**：OpenLevel、SetGamePaused、Widget Blueprint

---

## J. 完整 HUD

### J1. 主 HUD Widget
- [ ] **J1-1**　创建 `WBP_HUD`，继承 UserWidget。添加到玩家视口（在 PlayerController 的 BeginPlay 中 Create Widget + Add to Viewport）
- [ ] **J1-2**　左上角：资源显示 "源晶: XXX"（绑定到 GameState 的玩家源晶变量）
- [ ] **J1-3**　左下/底部中央：血条（ProgressBar，绑定到 PlayerCharacter 的 "当前HP/最大HP"）
- [ ] **J1-4**　右上角：波次显示 "第X波 / 共3波"
- [ ] **J1-5**　屏幕中央上方：当前阶段提示文字（"白天 - 建造时间"/"夜晚 - 守住！"等）

### J2. 模式提示
- [ ] **J2-1**　屏幕底部偏右：当前模式提示 "FPS模式" 或 "RTS模式"，RTS模式时显示"左键框选 | 右键移动"
- [ ] **J2-2**　屏幕底部偏左：建筑快捷键提示 "1=采集器(50) | 2=兵营(100)"
- [ ] **J2-3**　准星对准可交互建筑时：显示浮动文字"按E生产单位"

### J3. 准星
- [ ] **J3-1**　屏幕中央绘制简单十字准星（用 Canvas 绘制 或 用 Widget 的 Image 贴图）
- [ ] **J3-2**　RTS 模式时隐藏准星，FPS 模式时显示

> **复用的 UE 功能**：Widget Blueprint 整套系统、BindWidget 属性绑定

---

## K. 整合与测试

### K1. 全流程串联
- [ ] **K1-1**　确认从开局着陆 → 3波防御 → 胜利/失败的完整链路无断开
- [ ] **K1-2**　测试建筑、单位、敌人之间的交互是否正常（建造→生产单位→单位打怪→怪物拆建筑）
- [ ] **K1-3**　确认 FPS/RTS 切换在各种场景下不卡死、光标不丢失

### K2. 性能与 Bug 检查
- [ ] **K2-2**　检查所有 Timer 是否正确清理（角色死亡/关卡重开时不残留）
- [ ] **K2-3**　测试 3 波全量敌人(5+10+15=30只)同时存在时的帧率（如卡顿需调整生成间隔或简化 AI Tick）

### K3. 打包验证
- [ ] **K3-1**　Package Project (Windows)，确认打包后运行无 Editor-Only 错误
- [ ] **K3-2**　找一个没装 UE5 的电脑测试打包版本能否正常运行

---

## 任务统计

| 模块 | 任务数 | 预估总时长 |
|------|--------|-----------|
| A. 项目基础 | 12 | 6h |
| B. 玩家角色 | 9 | 4.5h |
| C. FPS/RTS混合输入 | 10 | 5h |
| D. 资源系统 | 5 | 2.5h |
| E. 建造系统（炮塔） | 6 | 3h |
| G. 敌人系统 | 9 | 4.5h |
| H. 波次/昼夜 | 11 | 5.5h |
| I. 胜负判定 | 6 | 3h |
| J. HUD | 7 | 3.5h |
| K. 整合测试 | 5 | 2.5h |
| **合计** | **80** | **约40h** |

> 按每人每周 1.5 小时（1晚×0.5h×3次/周或等效），1 人约需 27 周。
> 多人并行开发可按模块分工，预计 2-3 人可在 **8-12 周** 内完成。

---

## Sprint 开发计划（每个 Sprint 产出可玩版本）

> 核心理念：**每个 Sprint 结束都有一条完整的"开始→游玩→结束"链路。**
> 不是按模块推进，而是按可玩性递增推进。

---

### Sprint 1：走路打靶（预估 8.5h）
> 产出：能启动、能走动、能开枪、能退出

| 编号 | 任务内容 | 来自 |
|------|---------|------|
| S1-1 | 创建 UE5 空白项目，BP 模板 | A1-1→A1-2 |
| S1-2 | 场景复用 FirstPerson 模板（跳过灰盒地面和围墙） | A1-3→A1-4 跳过 |
| S1-3 | 放置 DirectionalLight + SkyLight，基础白天光照 | A2-1→A2-2 |
| S1-4 | 创建 BP_GameMode_Demo + BP_GameState_Demo，设默认 | A3-1→A3-3 |
| S1-5 | 配置 Enhanced Input：IA_Move/IA_Look/IA_Jump/IA_Shoot，创建 IMC_Default | A4-1→A4-2 |
| S1-6 | 创建 BP_PlayerCharacter，胶囊体+CameraComponent | B1-1→B1-3 |
| S1-7 | 绑定 Move/Look/Jump 到 Enhanced Input，调参 WalkSpeed | B2-1→B2-5 |
| S1-8 | 摄像机前附加方块"枪"，LineTrace 射击+Debug 射线显示 | B3-1→B3-4 |
| S1-9 | 屏幕中央十字准星 Widget | J3-1 |
| S1-10 | 创建 WBP_HUD（先只放准星），PlayerController BeginPlay 添加到视口 | J1-1 |

**可测试：** 进游戏 → 移动跳跃 → 开枪看彩虹线 → ESC退出

---

### Sprint 2：收集挑战（预估 +5h，累计 13.5h）
> 产出：第一次有真正的胜利/失败流程

| 编号 | 任务内容 | 来自 |
|------|---------|------|
| S2-1 | 创建 BP_SourceCrystal（发光方块，有储量），DemoMap 放置 3-5 个 | D1-1→D1-4 |
| S2-2 | 射击命中矿脉 → 调用矿脉"被采集(10)"→ 矿脉储量扣减 | D2-1 |
| S2-3 | GameState 中添加"玩家源晶(Int)"变量 + 增减事件 | D2-2→D2-3 |
| S2-4 | 射击命中矿脉时 → 玩家源晶+10 | 串联 D2-1+D2-3 |
| S2-5 | HUD 左上角显示"源晶: XXX"（绑定 GameState 变量） | J1-1→J1-2 |
| S2-6 | 玩家血量：当前HP/最大HP 变量 + "受到伤害"事件 + 死亡广播 | B4-1→B4-3 |
| S2-7 | HUD 底部血条 ProgressBar（绑定玩家 HP 百分比） | J1-3 |
| S2-8 | 创建 WBP_GameOver（胜利/失败文字+重新开始按钮） | I3-1→I3-3 |
| S2-9 | 胜利条件：源晶 ≥ 100 → 调用 GameOver(胜利)；失败条件：玩家死亡 → GameOver(失败) | I1-1 |

**可测试：** 进游戏 → 找矿射击收集 → 源晶达100 → 胜利界面 → 重新开始

---

### Sprint 3：第一夜（预估 +14h，累计 27.5h）
> 产出：完整的单波次昼夜循环。炮塔自动防守，玩家纯 FPS 协助。

| 编号 | 任务内容 | 来自 |
|------|---------|------|
| S3-1 | 创建 EDayPhase 枚举(着陆/白天/黄昏/夜晚/战后) + GameState 切换阶段事件 | H1-1→H1-2 |
| S3-2 | 阶段序列：白天(90s)→黄昏(15s)→夜晚(清完即结束)→战后(60s)→循环 | H1-3 |
| S3-3 | 监听阶段切换 → Timeline 插值 DirectionalLight 昼夜参数 | H3-1→H3-3 |
| S3-4 | 地图边缘放 8 个 BP_EnemySpawner，夜晚逐个间隔 1-2s 生成 5 只敌人 | H2-1→H2-4 |
| S3-5 | 创建 BP_Enemy（红色球体），50HP，300移速，5攻击力，3m攻击范围 | G1-1→G1-3 |
| S3-6 | 敌人 BeginPlay 寻找最近炮塔为目标，Tick 中 SimpleMoveToActor 追踪 | G2-1→G2-3 |
| S3-7 | 敌人距目标≤攻击范围时停止移动，按攻击间隔造成伤害 | G3-1→G3-2 |
| S3-8 | 敌人"受到伤害"事件 + 死亡摧毁，死亡时 GameState.剩余敌人-1 | G4-1→G4-2 |
| S3-9 | 敌人无炮塔可打时切换目标为玩家 | G5-2 |
| S3-10 | 放置 NavMeshBoundsVolume 覆盖地面 | A1-5 |
| S3-11 | 创建 BP_Turret 继承 ATurret（C++），设置网格体/DetectionRadius | E3-1→E3-2 |
| S3-12 | 在 BP_Turret 中添加 UTurretSocketComponent 作为炮管，设射程/角度/射速 | E3-2 |
| S3-13 | 创建 BP_TurretProjectile 继承 ABaseProjectile，设碰撞体/网格体/运动速度 | E3-4 |
| S3-14 | DemoMap 中放置 2-3 个 BP_Turret 作为初始防御 | E3-6 |
| S3-15 | 胜利条件：波次清空 → 胜利；失败条件：玩家死/所有炮塔被毁 → 失败 | I1-1→I2-1 |
| S3-16 | HUD 补齐：血条、阶段提示文字、波次显示（先显示"第1波"） | J1-3→J1-5 |
| S3-17 | 波次结束检测：剩余敌人≤0→切换"战后"阶段 | H4-1→H4-2 |

**可测试：** 进游戏→白天熟悉场景→黄昏预警→夜晚5怪冲来→炮塔自动射击→清完/被灭→胜负界面

---

### Sprint 4：建造炮塔（预估 +6h，累计 33.5h）
> 产出：加入炮塔建造系统，玩家可在地图上建造炮塔增强防御。

| 编号 | 任务内容 | 来自 |
|------|---------|------|
| S4-1 | 配置 IA_Build1 | A4-2→A4-3 |
| S4-2 | BP_PlayerCharacter 添加建造模式切换：按 1 进入炮塔预览模式 | E1-1 |
| S4-3 | Tick 中从摄像机做 LineTrace + 地面生成半透明预览网格 | E1-2 |
| S4-4 | 预览时合法位置变绿、重叠变红。左键确认放置（消耗资源），右键取消 | E2-1→E2-3 |
| S4-5 | HUD 底部提示"按1建造炮塔（消耗50源晶）" | J2-2 |
| S4-6 | 建造模式隐藏准星，FPS模式恢复准星 | J3-2 |

**可测试：** 同 Sprint 3 流程，现在可在白天收集资源后按1建造新炮塔增强防线

---

### Sprint 5：三波决战（预估 +3h，累计 36.5h）
> 产出：最终 Demo，完整 3 波递增 + 整合测试 + 打包

| 编号 | 任务内容 | 来自 |
|------|---------|------|
| S5-1 | 波次数据表：波次1=5只/波次2=10只/波次3=15只，夜晚随机选刷新点间隔生成 | H2-2→H2-4 扩展 |
| S5-2 | 第3波清空后直接跳到"胜利"判断 | H1-4 + I2-1 |
| S5-3 | HUD 波次显示改为"第X波 / 共3波"，夜晚显示"剩余敌人:X" | J1-4 + H4-2 |
| S5-4 | 全流程串联检查：着陆→白天→黄昏→夜晚1→战后→白天→夜晚2→战后→白天→夜晚3→胜利 | K1-1 |
| S5-5 | 交互完整性检查：建炮塔→炮塔打怪→怪拆炮塔→玩家死亡/炮塔全毁失败 | K1-2 |
| S5-6 | Timer 清理检查、30只敌人同屏帧率检查 | K2-1→K2-3 |
| S5-7 | Package Project（Windows），无 Editor-Only 错误，在其他电脑可运行 | K3-1→K3-2 |

**可测试：** 完整 Demo──3 波昼夜循环，炮塔防守+建造，胜利/失败完整闭合

---

## Sprint 统计

| Sprint | 任务数 | 增量时长 | 累计时长 | 可玩性 |
|--------|--------|---------|---------|--------|
| S1. 走路打靶 | 10 | 8.5h | 8.5h | 启动→移动→开枪→退出 |
| S2. 收集挑战 | 9 | 5h | 13.5h | + 采集→目标→胜利界面 |
| S3. 第一夜 | 17 | 14h | 27.5h | + 炮塔→怪→单波胜负 |
| S4. 建造炮塔 | 6 | 6h | 33.5h | + 资源采集+炮塔建造 |
| S5. 三波决战 | 7 | 3h | 36.5h | + 3波递增→最终打包 |
| **合计** | **49** | — | **36.5h** | 完整 Demo |

> 按模块统计仍是 80 个子任务（A-K），上表为 Sprint 粒度的合并任务。
> 单人每周 1.5h 约需 24 周；2-3 人并行可在 **8-12 周** 内完成
