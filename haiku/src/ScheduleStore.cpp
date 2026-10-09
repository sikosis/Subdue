#include "ScheduleStore.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>


static const int32 kScheduleFormatVersion = 1;


status_t
ScheduleStore::_Path(BPath& path, bool createDirectory)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status != B_OK)
		return status;
	status = path.Append("Subdue");
	if (status != B_OK)
		return status;
	if (createDirectory) {
		status = create_directory(path.Path(), 0755);
		if (status != B_OK && status != B_FILE_EXISTS)
			return status;
	}
	return path.Append("schedule");
}


status_t
ScheduleStore::Save(const SavedSchedule& schedule)
{
	BPath path;
	status_t status = _Path(path, true);
	if (status != B_OK)
		return status;

	BMessage state('SbSc');
	state.AddInt32("version", kScheduleFormatVersion);
	state.AddInt32("action", (int32)schedule.action);
	state.AddInt64("fire_time", (int64)schedule.fireTime);
	state.AddString("target_name", schedule.targetName);
	state.AddString("target_signature", schedule.targetSignature);
	state.AddBool("force", schedule.force);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	return status == B_OK ? state.Flatten(&file) : status;
}


status_t
ScheduleStore::Load(SavedSchedule& schedule)
{
	BPath path;
	status_t status = _Path(path, false);
	if (status != B_OK)
		return status;

	BFile file(path.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage state;
	status = state.Unflatten(&file);
	if (status != B_OK)
		return status;

	int32 version;
	int32 action;
	int64 fireTime;
	const char* targetName;
	const char* targetSignature;
	bool force;
	if (state.FindInt32("version", &version) != B_OK
		|| version != kScheduleFormatVersion
		|| state.FindInt32("action", &action) != B_OK
		|| (action != (int32)ActionMode::Subdue
			&& action != (int32)ActionMode::Smother)
		|| state.FindInt64("fire_time", &fireTime) != B_OK
		|| fireTime <= 0
		|| state.FindString("target_name", &targetName) != B_OK
		|| state.FindString("target_signature", &targetSignature) != B_OK
		|| state.FindBool("force", &force) != B_OK) {
		return B_BAD_DATA;
	}

	schedule.action = (ActionMode)action;
	schedule.fireTime = (time_t)fireTime;
	schedule.targetName = targetName;
	schedule.targetSignature = targetSignature;
	schedule.force = force;
	return B_OK;
}


void
ScheduleStore::Clear()
{
	BPath path;
	if (_Path(path, false) != B_OK)
		return;
	BEntry entry(path.Path());
	if (entry.Exists())
		entry.Remove();
}
