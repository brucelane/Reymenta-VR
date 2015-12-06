
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

#include "CinderOculus.h"


// -------- SPOUT -------------
#include "Spout.h"
// ----------------------------

using namespace ci;
using namespace ci::app;
using namespace std;

class ReymentaVRApp : public App {

public:
	ReymentaVRApp();

	void setup() override;
	void draw() override;
	void update() override;
	void mouseDown(MouseEvent event) override;
	void keyDown(KeyEvent event) override;

	// -------- SPOUT -------------
	SpoutReceiver spoutreceiver;                // Create a Spout receiver object
	void shutdown();
	bool bInitialized;                          // true if a sender initializes OK
	bool bDoneOnce;                             // only try to initialize once
	bool bMemoryMode;                           // tells us if texture share compatible
	unsigned int g_Width, g_Height;             // size of the texture being sent out
	char SenderName[256];                       // sender name 
	gl::TextureRef spoutTexture;                    // Local Cinder texture used for sharing
	// ----------------------------
private:
	void drawPositionalTrackingCamera() const;
	double			mTime;

	hmd::OculusRiftRef	mRift;

	gl::GlslProgRef	mShader;
	gl::BatchRef	mTeapot;

	vec4			mLightWorldPosition;
};