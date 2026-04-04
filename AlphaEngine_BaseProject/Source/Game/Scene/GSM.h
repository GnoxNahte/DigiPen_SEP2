/*!
@file	GSM.h
@author	Ethan Ong
@brief	GSM - GameStateManager based on Game Implementation Techniques
		but uses a Object-oriented approach to have better type-safety.
		Also stores variables in scene instead of a static file variable
		and ties the lifetime of objects to the class
@link	https://gnoxnahte.github.io/DigiPen_SEP2/gsm.html

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include <string>

// Following game implementation techniques enum
enum SceneState
{
	GS_QUIT = -2,
	GS_RESTART = -1,

	GS_LEVEL_EDITOR,
	GS_SPLASH_SCREEN,
	GS_MAIN_MENU,
	GS_GAME,
	

	GS_SCENE_COUNT,
};

// NOTE: 
// Changes from Game Implementation Techniques,
// - Load() change to Constructor
// - Unload() change to Deconstructor
class BaseScene
{
public:
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Exit() = 0;

	virtual ~BaseScene() = default;
};

// @todo change to proper singleton?
class GSM
{
public:
	// combine Init, Update, Exit into 1 Run() method?
	static void Init(SceneState state);
	static void Update();
	static void Exit();

	static void ChangeScene(SceneState state);
	static SceneState GetState();
	static std::string GetStateName(SceneState state);
private: 
	static void LoadState(SceneState state);

	static inline BaseScene* currentScene = nullptr;
	
	static inline SceneState previousState = GS_QUIT;
	static inline SceneState currentState = GS_QUIT;
	static inline SceneState nextState = GS_QUIT;
};

// Called after changing scene
struct SceneChangeEvent
{
	SceneState previousState;
	SceneState currentState;
};
