#ifndef SUBDUE_SCHEDULE_STORE_H
#define SUBDUE_SCHEDULE_STORE_H

#include "Options.h"

#include <String.h>
#include <SupportDefs.h>

#include <time.h>

class BPath;


struct SavedSchedule {
	ActionMode action = ActionMode::Subdue;
	time_t fireTime = 0;
	BString targetName;
	BString targetSignature;
	bool force = true;
};


class ScheduleStore {
public:
	static status_t Save(const SavedSchedule& schedule);
	static status_t Load(SavedSchedule& schedule);
	static void Clear();

private:
	static status_t _Path(BPath& path, bool createDirectory);
};

#endif
