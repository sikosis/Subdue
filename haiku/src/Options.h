#ifndef SUBDUE_OPTIONS_H
#define SUBDUE_OPTIONS_H

#include <String.h>

enum class ActionMode {
	Subdue,
	Smother
};

enum class TimingMode {
	None,
	After,
	At,
	TopOfHour
};

struct Options {
	ActionMode action = ActionMode::Subdue;
	TimingMode timing = TimingMode::None;
	BString application;
	int32 minutes = 0;
	int32 hour = 0;
	int32 minute = 0;
	bool force = true;
	bool foreground = false;
	bool help = false;
	BString error;

	bool HasSchedule() const { return timing != TimingMode::None; }

	static Options Parse(int argc, const char* const* argv);
	static const char* Usage();
};

#endif
