#include "EventLoop.h"
#include "../StarEngine/jni/Logger.h"
#include "../StarEngine/jni/SceneManaging/SceneManager.h"
#include "../StarEngine/jni/GraphicsManager.h"
#include <unistd.h>
#include "../StarEngine/jni/Input/InputManager.h"
#include "../StarEngine/jni/StarEngine.h"

android_app* EventLoop::mApplicationPtr = nullptr;

EventLoop::EventLoop(android_app* pApplication):
			mEnabled(true),
			mQuit(false),
			mMainGameInitialized(false)
{
	star::StarEngine::GetInstance()->Initialize(0,0);

	mApplicationPtr = pApplication;
	mApplicationPtr->onAppCmd = activityCallback;
	mApplicationPtr->userData = this;
	mMainGame = new Game();
	mTimeManager = new star::TimeManager();
	mContext.mTimeManager = mTimeManager;
	//mApplicationPtr->onAppCmd = activityCallback;
	mApplicationPtr->onInputEvent = inputCallback;
	star::StarEngine::GetInstance()->SetAndroidApp(mApplicationPtr);
}

void EventLoop::run()
{
	int32 lResult;
	int32 lEvents;
	android_poll_source* lSource;

	star::Logger::GetInstance()->Log(star::LogLevel::Info,_T("Starting EventLoop"));

	while(true)
	{
		mTimeManager->StartMonitoring();
		while((lResult = ALooper_pollAll(mEnabled ? 0 : -1, NULL, &lEvents, (void**) &lSource)) >= 0)
		{
			if(lSource != NULL)
			{
				lSource->process(mApplicationPtr, lSource);
			}

			if(mApplicationPtr->destroyRequested)
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Info,_T("Exiting Event"));
				mQuit = true;
				return;
			}
		}

		if((mEnabled)&& (!mQuit) && mMainGameInitialized)
		{

			if(mMainGame->Update(mContext) != STATUS_OK)
			{
				mQuit = true;
				end();
			}
			else if(mMainGame->Draw() != STATUS_OK)
			{
				mQuit = true;
				end();
			}
		}
		usleep(100);
		mTimeManager->StopMonitoring();
	}
}

void EventLoop::end()
{
	star::Logger::GetInstance()->Log(star::LogLevel::Info,_T("Ending App"));
	mMainGame->End();
	ANativeActivity_finish(mApplicationPtr->activity);
}

void EventLoop::activityCallback(android_app* pApplication, int32_t pCommand)
{
	EventLoop& lEventLoop = *(EventLoop*) pApplication->userData;
	if(!lEventLoop.mMainGameInitialized)
	{
		switch(pCommand)
			{
			case APP_CMD_INIT_WINDOW:
				if(pApplication->window != nullptr)
				{
					star::Logger::GetInstance()->Log(star::LogLevel::Info,_T("Eventloop Callback : Window Handle Made"));
					star::GraphicsManager::GetInstance()->Initialize(pApplication);
				}
			break;
			case APP_CMD_GAINED_FOCUS:
				lEventLoop.mEnabled=true;
				if(lEventLoop.mMainGame->Initialize(0,0) != STATUS_OK)
				{
					lEventLoop.mQuit=true;
					lEventLoop.end();
				}
				else
					lEventLoop.mMainGameInitialized=true;
				break;
			case APP_CMD_LOST_FOCUS:
				lEventLoop.mEnabled=false;
				break;
			default:
				break;
			}
	}
	else
	{
		star::Logger::GetInstance()->Log(star::LogLevel::Info,_T("Callback to scenemanager"));
		star::SceneManager::GetInstance()->processActivityEvent(pCommand,pApplication);
	}
}

int32 EventLoop::inputCallback(android_app* pApplication, AInputEvent* pEvent) {
	return star::SceneManager::GetInstance()->processInputEvent(pEvent);
}
