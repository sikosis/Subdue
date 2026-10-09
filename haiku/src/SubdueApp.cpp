#include "SubdueApp.h"

#include "DeskbarView.h"
#include "MainWindow.h"
#include "Protocol.h"

#include <Autolock.h>
#include <Roster.h>


SubdueApp::SubdueApp(const Options& options)
	:
	BApplication(kSubdueSignature),
	fOptions(options),
	fWindow(NULL)
{
}


void
SubdueApp::ReadyToRun()
{
	fWindow = new MainWindow();
	SubdueDeskbarView::Install();
	be_roster->StartWatching(BMessenger(fWindow),
		B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	bool armed = fOptions.HasSchedule()
		? fWindow->ArmFromOptions(fOptions) : fWindow->RestoreSchedule();
	fWindow->Show();
	if (armed && !fOptions.foreground && fWindow->Lock()) {
		fWindow->HideForBackground();
		fWindow->Unlock();
	}
}


void
SubdueApp::MessageReceived(BMessage* message)
{
	if (fWindow == NULL) {
		BApplication::MessageReceived(message);
		return;
	}

	switch (message->what) {
		case kMsgQueryState: {
			BMessage reply(kMsgState);
			BAutolock lock(fWindow);
			if (lock.IsLocked())
				fWindow->AppendScheduleState(reply);
			else
				reply.AddBool("scheduled", false);
			message->SendReply(&reply);
			break;
		}
		case kMsgShowWindow: {
			BAutolock lock(fWindow);
			if (lock.IsLocked())
				fWindow->ShowAndActivate();
			break;
		}
		case kMsgCancelSchedule: {
			BAutolock lock(fWindow);
			if (lock.IsLocked())
				fWindow->CancelSchedule();
			break;
		}
		case kMsgQuitApp: {
			{
				BAutolock lock(fWindow);
				if (lock.IsLocked())
					fWindow->CancelSchedule();
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BApplication::MessageReceived(message);
	}
}


void
SubdueApp::ArgvReceived(int32 argc, char** argv)
{
	if (fWindow == NULL)
		return;
	BAutolock lock(fWindow);
	if (!lock.IsLocked())
		return;
	Options options = Options::Parse(argc, argv);
	if (options.HasSchedule() && options.error.IsEmpty())
		fWindow->ArmFromOptions(options);
	fWindow->ShowAndActivate();
}


bool
SubdueApp::QuitRequested()
{
	SubdueDeskbarView::Remove();
	return BApplication::QuitRequested();
}
