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

void ReymentaVRApp::setup()
{
	int wr;
	// parameters
	mParameterBag = ParameterBag::create();
	mParameterBag->mLiveCode = true;
	mParameterBag->mRenderThumbs = true;
	// utils
	mBatchass = Batchass::create(mParameterBag);
	CI_LOG_V("reymenta setup");
	mFirstLaunch = true;
	wr = mBatchass->getWindowsResolution();
	//setWindowSize(mParameterBag->mMainWindowWidth, mParameterBag->mMainWindowHeight);
	// setup shaders and textures
	mBatchass->setup();
	//setWindowPos(ivec2(0, 0));
	mParameterBag->iResolution.x = mParameterBag->mRenderWidth;
	mParameterBag->iResolution.y = mParameterBag->mRenderHeight;
	mParameterBag->mRenderResolution = ivec2(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	mParameterBag->mRenderResoXY = vec2(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
	mParameterBag->mRenderPosXY = ivec2(mParameterBag->mRenderX, mParameterBag->mRenderY);
	mParameterBag->mMode = 0;

	// Load our textures and transition shader in the main thread.
	try {
		gl::Texture::Format fmt;
		fmt.setWrap(GL_REPEAT, GL_REPEAT);

		mChannel0 = gl::Texture::create(loadImage(loadAsset("presets/tex16.png")), fmt);
		mChannel1 = gl::Texture::create(loadImage(loadAsset("presets/tex06.jpg")), fmt);
		mChannel2 = gl::Texture::create(loadImage(loadAsset("presets/tex09.jpg")), fmt);
		mChannel3 = gl::Texture::create(loadImage(loadAsset("presets/tex02.jpg")), fmt);

		mShaderTransition = gl::GlslProg::create(loadAsset("common/shadertoy.vert"), loadAsset("common/shadertoy.frag"));
	}
	catch (const std::exception& e) {
		// Quit if anything went wrong.
		CI_LOG_EXCEPTION("Failed to load common textures and shaders:", e);
		quit(); return;
	}
	// instanciate the audio class
	mAudio = AudioWrapper::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the spout class
	mSpout = SpoutWrapper::create(mParameterBag, mBatchass->getTexturesRef());
	// instanciate the console class
	mConsole = AppConsole::create(mParameterBag, mBatchass);

	mTimer = 0.0f;

	// imgui
	margin = 3;
	inBetween = 3;
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15 mPreviewWidth = 160;mPreviewHeight = 120;
	w = mParameterBag->mPreviewFboWidth + margin;
	h = mParameterBag->mPreviewFboHeight * 2.3;
	largeW = (mParameterBag->mPreviewFboWidth + margin) * 4;
	largeH = (mParameterBag->mPreviewFboHeight + margin) * 5;
	largePreviewW = mParameterBag->mPreviewWidth + margin;
	largePreviewH = (mParameterBag->mPreviewHeight + margin) * 2.4;
	displayHeight = mParameterBag->mMainDisplayHeight - 50;
	mouseGlobal = false;

	showConsole = showGlobal = showAudio = showMidi = showChannels = showShaders = showOSC = true;
	showTest = showTheme = showFbos = showTextures = false;

	// set ui window and io events callbacks
	ui::initialize();

	// RTE mBatchass->getShadersRef()->setupLiveShader();
	mBatchass->tapTempo();
}

void ReymentaVRApp::update()
{
	//mSpout->update();
	mBatchass->update();
	mAudio->update();
	mParameterBag->iFps = getAverageFps();
	mParameterBag->sFps = toString(floor(mParameterBag->iFps));
	getWindow()->setTitle(std::string("(" + mParameterBag->sFps + " fps)"));
	// Bind textures.
	if (mChannel0) mChannel0->bind(0);
	if (mChannel1) mChannel1->bind(1);
	if (mChannel2) mChannel2->bind(2);
	if (mChannel3) mChannel3->bind(3);

	// Render the current shader to a frame buffer.
	/*if (mBatchass->getShadersRef()->getShaderCurrent() && mBufferCurrent) {
		gl::ScopedFramebuffer fbo(mBufferCurrent);

		// Bind shader.
		gl::ScopedGlslProg shader(mBatchass->getShadersRef()->getShaderCurrent());
		setUniforms();

		// Clear buffer and draw full screen quad (flipped).
		gl::clear();
		gl::drawSolidRect(Rectf(0, (float)getWindowHeight(), (float)getWindowWidth(), 0));
		}

		// Render the next shader to a frame buffer.
		if (mBatchass->getShadersRef()->getShaderNext() && mBufferNext) {
		gl::ScopedFramebuffer fbo(mBufferNext);

		// Bind shader.
		gl::ScopedGlslProg shader(mBatchass->getShadersRef()->getShaderNext());
		setUniforms();

		// Clear buffer and draw full screen quad (flipped).
		gl::clear();
		gl::drawSolidRect(Rectf(0, (float)getWindowHeight(), (float)getWindowWidth(), 0));
		}

		// Perform a cross-fade between the two shaders.
		double time = getElapsedSeconds() - mParameterBag->mTransitionTime;
		double fade = math<double>::clamp(time / mParameterBag->mTransitionDuration, 0.0, 1.0);

		if (fade <= 0.0) {
		// Transition has not yet started. Keep drawing current buffer.
		gl::draw(mBufferCurrent->getColorTexture(), getWindowBounds());
		}
		else if (fade < 1.0) {
		// Transition is in progress.
		// Use a transition shader to avoid having to draw one buffer on top of another.
		gl::ScopedTextureBind tex0(mBufferCurrent->getColorTexture(), 0);
		gl::ScopedTextureBind tex1(mBufferNext->getColorTexture(), 1);

		gl::ScopedGlslProg shader(mShaderTransition);
		mShaderTransition->uniform("iSrc", 0);
		mShaderTransition->uniform("iDst", 1);
		mShaderTransition->uniform("iFade", (float)fade);

		gl::drawSolidRect(getWindowBounds());
		}
		else if (mBatchass->getShadersRef()->getShaderNext()) {
		// Transition is done. Swap shaders.
		gl::draw(mBufferNext->getColorTexture(), getWindowBounds());
		mBatchass->getShadersRef()->swapShaders();

		}
		else {
		// No transition in progress.
		gl::draw(mBufferCurrent->getColorTexture(), getWindowBounds());
		}*/
	// rift
	mTime = getElapsedSeconds();
	gl::clear();

	hmd::ScopedRiftBuffer bind{ mRift };
	std::array<mat4, 6> worldToEyeClipMatrices;

	if (mBatchass->getShadersRef()->getShaderNext() && mBufferNext) {
		//gl::ScopedFramebuffer fbo(mBufferNext);

		// Bind shader.

/*
		// Clear buffer and draw full screen quad (flipped).
		//gl::clear();
		gl::drawSolidRect(Rectf(0, (float)getWindowHeight()/4, (float)getWindowWidth()/4, 0));*/
		// Bind shader.
		gl::ScopedGlslProg shader(mBatchass->getShadersRef()->getShaderNext());
		setUniforms();
		//gl::draw(mBufferNext->getColorTexture(), getWindowBounds());

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
	}

}
// -------- SPOUT -------------
void ReymentaVRApp::cleanup()
{
	CI_LOG_V("shutdown");
	// save warp settings
	mBatchass->getWarpsRef()->save();
	// save params
	mParameterBag->save();
	ui::Shutdown();
	// close spout
	mSpout->shutdown();
	quit();
}
void ReymentaVRApp::mouseMove(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseMove(event);
}
void ReymentaVRApp::mouseDown(MouseEvent event)
{
	mMouse.x = (float)event.getPos().x;
	mMouse.y = (float)event.getPos().y;
	mMouse.z = (float)event.getPos().x;
	mMouse.w = (float)event.getPos().y;
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseDown(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseDown(event);

}

void ReymentaVRApp::mouseDrag(MouseEvent event)
{
	mMouse.x = (float)event.getPos().x;
	mMouse.y = (float)event.getPos().y;
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseDrag(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseDrag(event);

}
void ReymentaVRApp::mouseUp(MouseEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->mouseUp(event);
	if (mParameterBag->mMode == mParameterBag->MODE_AUDIO) mAudio->mouseUp(event);
}
void ReymentaVRApp::keyUp(KeyEvent event)
{
	if (mParameterBag->mMode == mParameterBag->MODE_WARP) mBatchass->getWarpsRef()->keyUp(event);

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
void ReymentaVRApp::resize()
{
	mBatchass->getWarpsRef()->resize();
	// Create/resize frame buffers (no multisampling)
	mBufferCurrent = gl::Fbo::create(getWindowWidth(), getWindowHeight());
	mBufferNext = gl::Fbo::create(getWindowWidth(), getWindowHeight());
}

void ReymentaVRApp::fileDrop(FileDropEvent event)
{
	// Send all file requests to the loading thread.
	size_t count = event.getNumFiles();
	//for (size_t i = 0; i < count && mRequests->isNotFull(); ++i) {
	//mRequests->pushFront(event.getFile(i));
	for (size_t i = 0; i < count; ++i) {
		mBatchass->getShadersRef()->addRequest(event.getFile(i));
	}
	// TO MIGRATE
	int index;
	string ext = "";
	// use the last of the dropped files
	const fs::path &mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);
	index = (int)(event.getX() / (margin + mParameterBag->mPreviewFboWidth + inBetween));// +1;
	//mBatchass->log(mFile + " dropped, currentSelectedIndex:" + toString(mParameterBag->currentSelectedIndex) + " x: " + toString(event.getX()) + " PreviewFboWidth: " + toString(mParameterBag->mPreviewFboWidth));

	if (ext == "wav" || ext == "mp3")
	{
		mAudio->loadWaveFile(mFile);
	}
	else if (ext == "png" || ext == "jpg")
	{
		if (index < 1) index = 1;
		if (index > 3) index = 3;
		//mTextures->loadImageFile(mParameterBag->currentSelectedIndex, mFile);
		mBatchass->getTexturesRef()->loadImageFile(index, mFile);
	}
	else if (ext == "glsl")
	{
		if (index < 4) index = 4;
		int rtn = mBatchass->getShadersRef()->loadPixelFragmentShaderAtIndex(mFile, index);
		if (rtn > -1 && rtn < mBatchass->getShadersRef()->getCount())
		{
			mParameterBag->controlValues[22] = 1.0f;
			// TODO  send content via websockets
			/*fs::path fr = mFile;
			string name = "unknown";
			if (mFile.find_last_of("\\") != std::string::npos) name = mFile.substr(mFile.find_last_of("\\") + 1);
			if (fs::exists(fr))
			{

			std::string fs = loadString(loadFile(mFile));
			if (mParameterBag->mOSCEnabled) mOSC->sendOSCStringMessage("/fs", 0, fs, name);
			}*/
			// save thumb
			timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ saveThumb(); });
		}
	}
	else if (ext == "mov" || ext == "mp4")
	{
		/*
		if (index < 1) index = 1;
		if (index > 3) index = 3;
		mBatchass->getTexturesRef()->loadMovieFile(index, mFile);*/
	}
	else if (ext == "fs")
	{
		//mShaders->incrementPreviewIndex();
		mBatchass->getShadersRef()->loadFragmentShader(mPath);
	}
	else if (ext == "xml")
	{
		mBatchass->getWarpsRef()->loadWarps(mFile);
	}
	else if (ext == "patchjson")
	{
		// try loading patch
		try
		{
			JsonTree patchjson;
			try
			{
				patchjson = JsonTree(loadFile(mFile));
				mParameterBag->mCurrentFilePath = mFile;
			}
			catch (cinder::JsonTree::Exception exception)
			{
				CI_LOG_V("patchjsonparser exception " + mFile + ": " + exception.what());

			}
			//Assets
			int i = 1; // 0 is audio
			JsonTree jsons = patchjson.getChild("assets");
			for (JsonTree::ConstIter jsonElement = jsons.begin(); jsonElement != jsons.end(); ++jsonElement)
			{
				string jsonFileName = jsonElement->getChild("filename").getValue<string>();
				int channel = jsonElement->getChild("channel").getValue<int>();
				if (channel < mBatchass->getTexturesRef()->getTextureCount())
				{
					CI_LOG_V("asset filename: " + jsonFileName);
					mBatchass->getTexturesRef()->setTexture(channel, jsonFileName);
				}
				i++;
			}

		}
		catch (...)
		{
			CI_LOG_V("patchjson parsing error: " + mFile);
		}
	}
	else if (ext == "txt")
	{
		// try loading shader parts
		if (mBatchass->getShadersRef()->loadTextFile(mFile))
		{

		}
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		if (index < 1) index = 1;
		if (index > 3) index = 3;
		mBatchass->getTexturesRef()->createFromDir(mFile + "/", index);
		// or create thumbs from shaders
		mBatchass->getShadersRef()->createThumbsFromDir(mFile + "/");
	}
	/*if (!loaded && ext == "frag")
	{

	//mShaders->incrementPreviewIndex();

	if (mShaders->loadPixelFrag(mFile))
	{
	mParameterBag->controlValues[22] = 1.0f;
	timeline().apply(&mTimer, 1.0f, 1.0f).finishFn([&]{ save(); });
	}
	if (mCodeEditor) mCodeEditor->fileDrop(event);
	}*/
	mParameterBag->isUIDirty = true;
}
void ReymentaVRApp::saveThumb()
{

}
void ReymentaVRApp::draw()
{

}
void ReymentaVRApp::setUniforms()
{
	auto shader = gl::context()->getGlslProg();
	if (!shader)
		return;

	// Calculate shader parameters.
	vec3  iResolution(vec2(getWindowSize()), 1);
	mParameterBag->iChannelTime[0] = (float)getElapsedSeconds();
	mParameterBag->iChannelTime[1] = (float)getElapsedSeconds() - 1;
	mParameterBag->iChannelTime[2] = (float)getElapsedSeconds() - 2;
	mParameterBag->iChannelTime[3] = (float)getElapsedSeconds() - 3;
	//
	if (mParameterBag->mUseTimeWithTempo)
	{
		mParameterBag->iGlobalTime = mParameterBag->iTempoTime*mParameterBag->iTimeFactor;
	}
	else
	{
		mParameterBag->iGlobalTime = (float)getElapsedSeconds();
	}
	mParameterBag->iGlobalTime *= mParameterBag->iSpeedMultiplier;

	vec3  iChannelResolution0 = mChannel0 ? vec3(mChannel0->getSize(), 1) : vec3(1);
	vec3  iChannelResolution1 = mChannel1 ? vec3(mChannel1->getSize(), 1) : vec3(1);
	vec3  iChannelResolution2 = mChannel2 ? vec3(mChannel2->getSize(), 1) : vec3(1);
	vec3  iChannelResolution3 = mChannel3 ? vec3(mChannel3->getSize(), 1) : vec3(1);

	time_t now = time(0);
	tm*    t = gmtime(&now);
	vec4   iDate(float(t->tm_year + 1900),
		float(t->tm_mon + 1),
		float(t->tm_mday),
		float(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec));

	// Set shader uniforms.
	shader->uniform("iResolution", iResolution);
	shader->uniform("iGlobalTime", mParameterBag->iGlobalTime);
	shader->uniform("iChannelTime[0]", mParameterBag->iChannelTime[0]);
	shader->uniform("iChannelTime[1]", mParameterBag->iChannelTime[1]);
	shader->uniform("iChannelTime[2]", mParameterBag->iChannelTime[2]);
	shader->uniform("iChannelTime[3]", mParameterBag->iChannelTime[3]);
	shader->uniform("iChannelResolution[0]", iChannelResolution0);
	shader->uniform("iChannelResolution[1]", iChannelResolution1);
	shader->uniform("iChannelResolution[2]", iChannelResolution2);
	shader->uniform("iChannelResolution[3]", iChannelResolution3);
	shader->uniform("iMouse", mMouse);
	shader->uniform("iChannel0", 0);
	shader->uniform("iChannel1", 1);
	shader->uniform("iChannel2", 2);
	shader->uniform("iChannel3", 3);
	shader->uniform("iDate", iDate);
}

#pragma warning(pop) // _CRT_SECURE_NO_WARNINGS
void prepareSettings(App::Settings *settings)
{
	hmd::RiftManager::initialize();

	settings->disableFrameRate();
	settings->setTitle("Oculus Rift Sample");
	settings->setWindowSize(1920 / 2, 1080 / 2);
}

CINDER_APP(ReymentaVRApp, RendererGl(RendererGl::Options().msaa(0)), prepareSettings)
