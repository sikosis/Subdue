#ifndef SUBDUE_APP_H
#define SUBDUE_APP_H

#include "Options.h"

#include <Application.h>

class MainWindow;

class SubdueApp : public BApplication {
public:
	explicit SubdueApp(const Options& options);

	virtual void ReadyToRun();
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

private:
	Options fOptions;
	MainWindow* fWindow;
};

#endif
