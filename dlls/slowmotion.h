#ifndef SLOWMOTION_H
#define SLOWMOTION_H

#include "extdll.h"
#include "eiface.h"
#include "enginecallback.h"
#include <SDL2/SDL_timer.h>

#define SLOWMOTION_FPS_INTERVAL 0.1 // lower for faster refresh rate, higher for accuracy
class SlowMotion {

public:
	SlowMotion();

	void SetSlowMotionEffect(float slowMotionEffect);
	void UpdateSlowMotion();
	void ToggleSlowMotion();

private:
	void UpdateFPSCounter();
	void ExecuteSlowMotionCommand(float slowMotionValue);

	Uint32 lastTime;
	Uint32 FPS;
	Uint32 framesPassed;
	float slowMotionEffect;
	bool slowMotionEnabled ;


};

#endif // SLOWMOTION_H