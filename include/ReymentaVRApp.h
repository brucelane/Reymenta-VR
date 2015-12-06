

#pragma warning(push)
#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"
#include "cinder/app/Platform.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Unicode.h"

#include "CinderOculus.h"

// UserInterface
#include "CinderImGui.h"
// parameters
#include "ParameterBag.h"
// audio
#include "AudioWrapper.h"
// spout
#include "SpoutWrapper.h"
// Utils
#include "Batchass.h"
// Console
#include "AppConsole.h"

#include <time.h>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Reymenta;

class ReymentaVRApp : public App {

public:
	ReymentaVRApp();

	void setup() override;
	void cleanup() override;
	void draw() override;
	void update() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void mouseMove(MouseEvent event) override;

	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;

	void resize() override;
	void fileDrop(FileDropEvent event) override;

private:
	void saveThumb();

	void drawPositionalTrackingCamera() const;
	double			mTime;

	hmd::OculusRiftRef	mRift;

	gl::GlslProgRef	mShader;
	gl::BatchRef	mTeapot;

	vec4			mLightWorldPosition;
	//! Sets the ShaderToy uniform variables.
	void setUniforms();
	//! Shader that will perform the transition to the next shader.
	gl::GlslProgRef mShaderTransition;
	//! Buffer containing the rendered output of the currently active shader.
	gl::FboRef      mBufferCurrent;
	//! Buffer containing the rendered output of the shader we are transitioning to.
	gl::FboRef      mBufferNext;
	//! Texture slots for our shader, based on ShaderToy.
	gl::TextureRef  mChannel0;
	gl::TextureRef  mChannel1;
	gl::TextureRef  mChannel2;
	gl::TextureRef  mChannel3;
	//! Our mouse position: xy = current position while mouse down, zw = last click position.
	vec4            mMouse;

	// parameters
	ParameterBagRef				mParameterBag;
	// audio
	AudioWrapperRef				mAudio;
	// utils
	BatchassRef					mBatchass;
	// console
	AppConsoleRef				mConsole;
	// spout
	SpoutWrapperRef				mSpout;
	// timeline
	Anim<float>					mTimer;

	// imgui
	float						color[4];
	float						backcolor[4];
	int							playheadPositions[12];
	float						speeds[12];
	int							w;
	int							h;
	int							displayHeight;
	int							xPos;
	int							yPos;
	int							largeW;
	int							largeH;
	int							largePreviewW;
	int							largePreviewH;
	int							margin;
	int							inBetween;
	//bool						sNewFrame;
	float						f = 0.0f;
	char						buf[64];
	bool						showConsole, showGlobal, showTextures, showTest, showMidi, showFbos, showTheme, showAudio, showShaders, showOSC, showChannels;
	bool						mouseGlobal;
	void						ShowAppConsole(bool* opened);
	// log only the first time
	bool						mFirstLaunch;
};