#include "slowmotion.h"

SlowMotion::SlowMotion() : 
lastTime( SDL_GetTicks() ),
FPS( 0 ),
framesPassed( 0 ),
slowMotionEffect( 1.0f ),
slowMotionEnabled( false ) {

}

void SlowMotion::UpdateFPSCounter() {
	framesPassed++;
	if (lastTime < SDL_GetTicks() - SLOWMOTION_FPS_INTERVAL * 1000)
	{
		lastTime = SDL_GetTicks();
		FPS = framesPassed / SLOWMOTION_FPS_INTERVAL;
		framesPassed = 0;
	}

	ALERT(at_notice, "%i\n", FPS);
}

void SlowMotion::SetSlowMotionEffect(float slowMotionEffect) {
	if (slowMotionEffect < 1.0f) {
		slowMotionEffect = 1.0f;
	}
	else if (slowMotionEffect > 5.0f) {
		slowMotionEffect = 5.0f;
	}

	this->slowMotionEffect = slowMotionEffect;
}

void SlowMotion::ToggleSlowMotion() {
	slowMotionEnabled = !slowMotionEnabled;
}

void SlowMotion::UpdateSlowMotion() {
	UpdateFPSCounter();

	if (!slowMotionEnabled && slowMotionEffect < 1.01f) {
		return;
	}
	if (slowMotionEnabled && slowMotionEffect > 2.5f) {
		return;
	}

	if (!slowMotionEnabled) {
		slowMotionEffect *= 0.999;
		if (slowMotionEffect < 1.01f) {
			ExecuteSlowMotionCommand(0.0);
			return;
		}
	}
	else {
		slowMotionEffect *= 1.001;
		if (slowMotionEffect > 2.5f) {
			slowMotionEffect = 2.5f;
		}
	}

	float slowMotionValue = (1.0f / FPS) / slowMotionEffect;
	ExecuteSlowMotionCommand(slowMotionValue);
}

void SlowMotion::ExecuteSlowMotionCommand(float slowMotionValue) {
	char slowMotionCommand[34];
	sprintf(slowMotionCommand, "host_framerate %.15f\n", slowMotionValue);
	SERVER_COMMAND(slowMotionCommand);
}