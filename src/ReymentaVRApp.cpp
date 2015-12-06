#include "ReymentaVRApp.h"

ReymentaVRApp::ReymentaVRApp()
	: mLightWorldPosition(vec4(1, 1, 1, 1))
{
	CameraPersp host;
	host.setEyePoint(vec3(0, 0, 1));
	host.lookAt(vec3(0));
	mRift = hmd::OculusRift::create(hmd::OculusRift::Params().hostCamera(host).screenPercentage(1.25f));

	try {
		mShader = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("phong.vert")).fragment(loadAsset("phong.frag")));
	}
	catch (const std::exception& e) {
		console() << e.what() << std::endl;
		quit();
	}
	mTeapot = gl::Batch::create(geom::Teapot().subdivisions(12), mShader);

	gl::enableVerticalSync(false);
	gl::disableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enable(GL_CLIP_DISTANCE0, true);
}

// -------- SPOUT -------------
void ReymentaVRApp::setup()
{
    g_Width = 320; // set global width and height to something
    g_Height = 240; // they need to be reset when the receiver connects to a sender
    //setWindowSize(g_Width, g_Height);
    setFullScreen(false);
    setFrameRate(60.0f);
    bInitialized = false;
}

void ReymentaVRApp::update()
{
    unsigned int width, height;

    // -------- SPOUT -------------
    if (!bInitialized) {

        // This is a receiver, so the initialization is a little more complex than a sender
        // The receiver will attempt to connect to the name it is sent.
        // Alternatively set the optional bUseActive flag to attempt to connect to the active sender. 
        // If the sender name is not initialized it will attempt to find the active sender
        // If the receiver does not find any senders the initialization will fail
        // and "CreateReceiver" can be called repeatedly until a sender is found.
        // "CreateReceiver" will update the passed name, and dimensions.
        SenderName[0] = NULL; // the name will be filled when the receiver connects to a sender
        width = g_Width; // pass the initial width and height (they will be adjusted if necessary)
        height = g_Height;

        // Optionally set for DirectX 9 instead of default DirectX 11 functions
        // spoutreceiver.SetDX9(true);  

        // Initialize a receiver
        if (spoutreceiver.CreateReceiver(SenderName, width, height, true)) { // true to find the active sender
            // Optionally test for texture share compatibility
            // bMemoryMode informs us whether Spout initialized for texture share or memory share
            bMemoryMode = spoutreceiver.GetMemoryShareMode();

            // Is the size of the detected sender different from the current texture size ?
            // This is detected for both texture share and memoryshare
            if (width != g_Width || height != g_Height) {
                // Reset the global width and height
                g_Width = width;
                g_Height = height;
                // Reset the local receiving texture size
                spoutTexture = gl::Texture::create(g_Width, g_Height);
                // reset render window
                setWindowSize(g_Width, g_Height);
            }
            bInitialized = true;
        }
        else {
            // Receiver initialization will fail if no senders are running
            // Keep trying until one starts
        }
    } // endif not initialized
    // ----------------------------
	mTime = getElapsedSeconds();
	gl::clear();

	hmd::ScopedRiftBuffer bind{ mRift };
	std::array<mat4, 6> worldToEyeClipMatrices;

	// Calc clip space conversion matrices for both eyes
	for (auto eye : mRift->getEyes()) {
		gl::ScopedMatrices push;
		mRift->enableEye(eye);
		auto idx = 3 * static_cast<size_t>(eye);
		worldToEyeClipMatrices.at(idx) = mRift->getViewMatrix() * mRift->getModelMatrix();
		worldToEyeClipMatrices.at(idx + 1) = mRift->getProjectionMatrix();

		// non-instanced scene
		gl::lineWidth(3.0f);
		gl::drawCoordinateFrame(2);
		gl::drawSphere(vec3(mLightWorldPosition), 0.05f, 36);
	}

	{
		gl::ScopedViewport port{ vec2(0), mRift->getFboSize() };
		gl::ScopedModelMatrix push;
		gl::rotate((float)mTime, vec3(-0.3f, -1.0f, 0.2f));
		gl::scale(vec3(0.5f));
		gl::translate(0.0f, -0.5f, 0.0f);

		auto normalMatrix = glm::transpose(glm::inverse(gl::getModelMatrix()));
		worldToEyeClipMatrices.at(2) = worldToEyeClipMatrices.at(5) = normalMatrix;
		mShader->uniform("uLightPosition", mLightWorldPosition);
		mShader->uniform("uWorldToEyeClipMatrices", worldToEyeClipMatrices.data(), 6);
		mTeapot->drawInstanced(2);
	}
	getWindow()->setTitle(std::string("(" + toString(floor(getAverageFps())) + " fps)"));
}
// -------- SPOUT -------------
void ReymentaVRApp::shutdown()
{
    spoutreceiver.ReleaseReceiver();
}

void ReymentaVRApp::mouseDown(MouseEvent event)
{
    if (event.isRightDown()) { // Select a sender
        // SpoutPanel.exe must be in the executable path
        spoutreceiver.SelectSenderPanel(); // DirectX 11 by default
    }
}
// ----------------------------
void ReymentaVRApp::keyDown(KeyEvent event)
{
	hideCursor();
	switch (event.getCode()) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_r:
		mRift->recenterPose();
		break;
	case KeyEvent::KEY_m:
		mRift->enableMirrored(!mRift->isMirrored());
		break;
	case KeyEvent::KEY_s:
		mRift->enableMonoscopic(!mRift->isMonoscopic());
		break;
	case KeyEvent::KEY_t:
		mRift->enablePositionalTracking(!mRift->isTracked());
		break;
	}
}


void ReymentaVRApp::draw()
{
	/*
    unsigned int width, height;
    char txt[256];

    gl::setMatricesWindow(getWindowSize());
    gl::clear();
    gl::color(Color(1, 1, 1));

    // Save current global width and height - they will be changed
    // by receivetexture if the sender changes dimensions
    width = g_Width;
    height = g_Height;

    //
    // Try to receive the texture at the current size 
    //
    // NOTE : if ReceiveTexture is called with a framebuffer object bound, 
    // include the FBO id as an argument so that the binding is restored afterwards
    // because Spout uses an fbo for intermediate rendering
    if (bInitialized) {
        if (spoutreceiver.ReceiveTexture(SenderName, width, height, spoutTexture->getId(), spoutTexture->getTarget())) {
            //  Width and height are changed for sender change so the local texture has to be resized.
            if (width != g_Width || height != g_Height) {
                // The sender dimensions have changed - update the global width and height
                g_Width = width;
                g_Height = height;
                // Update the local texture to receive the new dimensions
                spoutTexture = gl::Texture::create(g_Width, g_Height);
                // reset render window
                setWindowSize(g_Width, g_Height);
                return; // quit for next round
            }

            // Otherwise draw the texture and fill the screen
            gl::draw(spoutTexture, getWindowBounds());

            // Show the user what it is receiving
            gl::enableAlphaBlending();
            sprintf_s(txt, "Receiving from [%s]", SenderName);
            gl::drawString(txt, vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
            sprintf_s(txt, "fps : %2.2d", (int)getAverageFps());
            gl::drawString(txt, vec2(getWindowWidth() - toPixels(100), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
            gl::drawString("RH click to select a sender", vec2(toPixels(20), getWindowHeight() - toPixels(40)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
            gl::disableAlphaBlending();
            return; // received OK
        }
    }

    gl::enableAlphaBlending();
    gl::drawString("No sender detected", vec2(toPixels(20), toPixels(20)), Color(1, 1, 1), Font("Verdana", toPixels(24)));
    gl::disableAlphaBlending();*/
    // ----------------------------
}
void prepareSettings(App::Settings *settings)
{
	hmd::RiftManager::initialize();

	settings->disableFrameRate();
	settings->setTitle("Oculus Rift Sample");
	settings->setWindowSize(1920 / 2, 1080 / 2);
}

CINDER_APP(ReymentaVRApp, RendererGl(RendererGl::Options().msaa(0)), prepareSettings)
