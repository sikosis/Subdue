#include "MainWindow.h"

#include "AudioMuter.h"
#include "ScheduleStore.h"

#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Notification.h>
#include <OptionPopUp.h>
#include <Roster.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 580, 410), "Subdue", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fTicker(NULL),
	fHideRunner(NULL),
	fScheduled(false),
	fQuitSent(false),
	fScheduledAction(ActionMode::Subdue),
	fTargetTeam(-1),
	fFireTime(0),
	fForceIfNeeded(true),
	fForceDeadline(0)
{
	_BuildInterface();
	_RefreshApplications();
	BMessage tick(kTick);
	fTicker = new BMessageRunner(BMessenger(this), tick, 1000000);
	CenterOnScreen();
}


MainWindow::~MainWindow()
{
	delete fTicker;
	delete fHideRunner;
}


void
MainWindow::_BuildInterface()
{
	BStringView* title = new BStringView("title", "Subdue");
	title->SetFont(be_bold_font, B_FONT_SIZE);
	BStringView* subtitle = new BStringView("subtitle",
		"Quiet an application or your audio on your schedule.");

	fAction = new BOptionPopUp("action", "Action", new BMessage(kActionChanged));
	fAction->AddOptionAt("Subdue an application", (int32)ActionMode::Subdue, 0);
	fAction->AddOptionAt("Smother system audio", (int32)ActionMode::Smother, 1);
	fAction->SetValue((int32)ActionMode::Subdue);

	fApplication = new BOptionPopUp("application", "Application", NULL);
	fRefresh = new BButton("refresh", "Refresh", new BMessage(kRefreshApps));

	fTiming = new BOptionPopUp("timing", "When", new BMessage(kTimingChanged));
	fTiming->AddOptionAt("After (minutes)", (int32)TimingMode::After, 0);
	fTiming->AddOptionAt("At time", (int32)TimingMode::At, 1);
	fTiming->AddOptionAt("Top of hour", (int32)TimingMode::TopOfHour, 2);
	fTiming->SetValue((int32)TimingMode::After);
	fValue = new BTextControl("value", "Value", "30", NULL);
	fForce = new BCheckBox("force",
		"Force-close if the application does not quit after 10 seconds", NULL);
	fForce->SetValue(B_CONTROL_ON);
	fScheduleButton = new BButton("schedule", "Schedule Subdue",
		new BMessage(kSchedule));
	fScheduleButton->MakeDefault(true);
	fStatus = new BStringView("status", "Choose an action and a time.");
	fCountdown = new BStringView("countdown", "");
	fCountdown->SetFont(be_bold_font, B_FONT_SIZE);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(title)
		.Add(subtitle)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fAction)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fApplication)
			.Add(fRefresh)
		.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fTiming)
			.Add(fValue)
		.End()
		.Add(fForce)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fStatus)
			.AddGlue()
			.Add(fCountdown)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fScheduleButton)
		.End();
}


void
MainWindow::_RefreshApplications()
{
	int32 previous = fApplication->Value();
	while (fApplication->CountOptions() > 0)
		fApplication->RemoveOptionAt(0);

	BList teams;
	be_roster->GetAppList(&teams);
	int32 optionIndex = 0;
	for (int32 index = 0; index < teams.CountItems(); index++) {
		team_id team = (team_id)(addr_t)teams.ItemAt(index);
		if (team == (team_id)getpid())
			continue;
		app_info info;
		if (be_roster->GetRunningAppInfo(team, &info) != B_OK
			|| (info.flags & B_BACKGROUND_APP) != 0) {
			continue;
		}
		const char* name = info.ref.name != NULL ? info.ref.name : info.signature;
		fApplication->AddOptionAt(name, team, optionIndex++);
	}
	if (previous >= 0)
		fApplication->SetValue(previous);
	else if (fApplication->CountOptions() > 0) {
		const char* ignored;
		int32 first;
		if (fApplication->GetOptionAt(0, &ignored, &first))
			fApplication->SetValue(first);
	}
}


void
MainWindow::_UpdateActionControls()
{
	bool subdue = fAction->Value() == (int32)ActionMode::Subdue;
	fApplication->SetEnabled(subdue);
	fRefresh->SetEnabled(subdue);
	fForce->SetEnabled(subdue);
	fScheduleButton->SetLabel(subdue ? "Schedule Subdue" : "Schedule Smother");
	fStatus->SetText(subdue ? "Choose an application and a time."
		: "Choose when to mute the system audio mixer.");
}


void
MainWindow::_UpdateTimingControl()
{
	TimingMode timing = (TimingMode)fTiming->Value();
	fValue->SetEnabled(timing != TimingMode::TopOfHour);
	fValue->SetText(timing == TimingMode::After ? "30"
		: timing == TimingMode::At ? "23:15" : "");
}


time_t
MainWindow::_CalculateFireTime(TimingMode timing, int32 first, int32 second) const
{
	time_t now = time(NULL);
	if (timing == TimingMode::After)
		return first > 0 ? now + (time_t)first * 60 : 0;

	struct tm local;
	localtime_r(&now, &local);
	if (timing == TimingMode::TopOfHour) {
		local.tm_hour += 1;
		local.tm_min = 0;
		local.tm_sec = 0;
	} else {
		if (first < 0 || first > 23 || second < 0 || second > 59)
			return 0;
		local.tm_hour = first;
		local.tm_min = second;
		local.tm_sec = 0;
		time_t candidate = mktime(&local);
		if (candidate <= now)
			local.tm_mday += 1;
	}
	return mktime(&local);
}


void
MainWindow::_ScheduleFromControls()
{
	TimingMode timing = (TimingMode)fTiming->Value();
	int32 first = 0;
	int32 second = 0;
	if (timing == TimingMode::After) {
		char* end = NULL;
		long value = strtol(fValue->Text(), &end, 10);
		if (fValue->Text() == end || *end != '\0' || value <= 0
			|| value > INT32_MAX) {
			fStatus->SetText("Enter a positive number of minutes.");
			return;
		}
		first = value;
	} else if (timing == TimingMode::At) {
		int parsedFirst;
		int parsedSecond;
		char extra;
		if (sscanf(fValue->Text(), "%d:%d%c", &parsedFirst, &parsedSecond,
			&extra) != 2 || parsedFirst < 0 || parsedFirst > 23
			|| parsedSecond < 0 || parsedSecond > 59) {
			fStatus->SetText("Enter a valid 24-hour time such as 23:15.");
			return;
		}
		first = parsedFirst;
		second = parsedSecond;
	}

	ActionMode action = (ActionMode)fAction->Value();
	team_id team = -1;
	BString name("System audio");
	BString signature;
	if (action == ActionMode::Subdue) {
		team = fApplication->Value();
		app_info info;
		if (team < 0 || be_roster->GetRunningAppInfo(team, &info) != B_OK) {
			fStatus->SetText("Choose a running application first.");
			return;
		}
		name = info.ref.name != NULL ? info.ref.name : info.signature;
		signature = info.signature;
	}
	_Arm(action, timing, first, second, team, name.String(), signature.String(),
		fForce->Value() == B_CONTROL_ON);
}


bool
MainWindow::_Arm(ActionMode action, TimingMode timing, int32 first, int32 second,
	team_id team, const char* name, const char* signature, bool force)
{
	time_t fireTime = _CalculateFireTime(timing, first, second);
	if (fireTime == 0) {
		fStatus->SetText("That time could not be scheduled.");
		return false;
	}
	return _ArmAt(action, fireTime, team, name, signature, force, true);
}


bool
MainWindow::_ArmAt(ActionMode action, time_t fireTime, team_id team,
	const char* name, const char* signature, bool force, bool hideAfterDelay)
{
	fScheduled = true;
	fQuitSent = false;
	fScheduledAction = action;
	fTargetTeam = team;
	fTargetName = name;
	fTargetSignature = signature;
	fFireTime = fireTime;
	fForceIfNeeded = force;
	fForceDeadline = 0;
	_SetScheduledControls(true);

	SavedSchedule saved;
	saved.action = action;
	saved.fireTime = fireTime;
	saved.targetName = name;
	saved.targetSignature = signature;
	saved.force = force;
	status_t saveStatus = ScheduleStore::Save(saved);

	char date[64];
	struct tm local;
	localtime_r(&fFireTime, &local);
	strftime(date, sizeof(date), "%a %d %b at %H:%M", &local);
	BString status;
	status.SetToFormat("%s is scheduled for %s.", fTargetName.String(), date);
	if (saveStatus != B_OK)
		status << " Warning: restart recovery is unavailable.";
	fStatus->SetText(status.String());
	fCountdown->SetText(_Countdown().String());

	delete fHideRunner;
	fHideRunner = NULL;
	if (hideAfterDelay) {
		BMessage hide(kHide);
		fHideRunner = new BMessageRunner(BMessenger(this), hide, 5000000, 1);
	}
	return true;
}


bool
MainWindow::ArmFromOptions(const Options& options)
{
	team_id team = -1;
	BString name("System audio");
	BString signature;
	if (options.action == ActionMode::Subdue
		&& !_FindApplication(options.application.String(), team, name, signature)) {
		BString message;
		message.SetToFormat("Could not find a running application matching “%s”.",
			options.application.String());
		fStatus->SetText(message.String());
		return false;
	}
	int32 first = options.timing == TimingMode::At ? options.hour : options.minutes;
	return _Arm(options.action, options.timing, first, options.minute, team,
		name.String(), signature.String(), options.force);
}


bool
MainWindow::RestoreSchedule()
{
	SavedSchedule saved;
	if (ScheduleStore::Load(saved) != B_OK) {
		ScheduleStore::Clear();
		return false;
	}

	team_id team = -1;
	BString name(saved.targetName);
	BString signature(saved.targetSignature);
	if (saved.action == ActionMode::Subdue) {
		BString foundName;
		BString foundSignature;
		if (!_FindApplication(saved.targetSignature.String(), team, foundName,
				foundSignature)
			&& !_FindApplication(saved.targetName.String(), team, foundName,
				foundSignature)) {
			ScheduleStore::Clear();
			fStatus->SetText("The saved schedule was removed because its application is not running.");
			return false;
		}
		name = foundName;
		signature = foundSignature;
	}

	return _ArmAt(saved.action, saved.fireTime, team, name.String(),
		signature.String(), saved.force, false);
}


bool
MainWindow::_FindApplication(const char* requested, team_id& team, BString& name,
	BString& signature) const
{
	BList teams;
	be_roster->GetAppList(&teams);
	for (int32 index = 0; index < teams.CountItems(); index++) {
		team_id candidate = (team_id)(addr_t)teams.ItemAt(index);
		app_info info;
		if (be_roster->GetRunningAppInfo(candidate, &info) != B_OK)
			continue;
		const char* candidateName = info.ref.name != NULL ? info.ref.name : info.signature;
		if (strcasecmp(candidateName, requested) == 0
			|| strcasecmp(info.signature, requested) == 0) {
			team = candidate;
			name = candidateName;
			signature = info.signature;
			return true;
		}
	}
	return false;
}


BString
MainWindow::_Countdown() const
{
	int64 remaining = fFireTime - time(NULL);
	if (remaining < 0)
		remaining = 0;
	int64 hours = remaining / 3600;
	int64 minutes = (remaining % 3600) / 60;
	int64 seconds = remaining % 60;
	BString text;
	if (hours > 0)
		text.SetToFormat("%" B_PRId64 ":%02" B_PRId64 ":%02" B_PRId64,
			hours, minutes, seconds);
	else
		text.SetToFormat("%02" B_PRId64 ":%02" B_PRId64, minutes, seconds);
	return text;
}


void
MainWindow::_Tick()
{
	if (!fScheduled)
		return;
	if (fScheduledAction == ActionMode::Subdue) {
		team_info info;
		if (get_team_info(fTargetTeam, &info) != B_OK) {
			BString message;
			if (fQuitSent)
				message.SetToFormat("%s quit successfully.", fTargetName.String());
			else
				message.SetToFormat("%s is no longer running.", fTargetName.String());
			_Finish(message.String());
			return;
		}
	}
	if (!fQuitSent && time(NULL) >= fFireTime)
		_PerformAction();
	else if (fQuitSent && system_time() >= fForceDeadline) {
		if (fForceIfNeeded) {
			status_t status = kill_team(fTargetTeam);
			BString message;
			if (status == B_OK)
				message.SetToFormat("%s was force-closed.", fTargetName.String());
			else
				message = "The application could not be force-closed.";
			_Finish(message.String(), status == B_OK);
		} else {
			BString message;
			message.SetToFormat("%s did not quit within 10 seconds.",
				fTargetName.String());
			_Finish(message.String(), false);
		}
		return;
	}
	if (fScheduled && !fQuitSent)
		fCountdown->SetText(_Countdown().String());
}


void
MainWindow::_PerformAction()
{
	if (fScheduledAction == ActionMode::Smother) {
		BString error;
		status_t status = AudioMuter::Mute(error);
		_Finish(status == B_OK ? "System audio was muted." : error.String(),
			status == B_OK);
		return;
	}

	status_t status;
	BMessenger target(fTargetSignature.String(), fTargetTeam, &status);
	if (status == B_OK)
		status = target.SendMessage(B_QUIT_REQUESTED);
	if (status != B_OK) {
		_Finish("Could not ask the application to quit.", false);
		return;
	}
	fQuitSent = true;
	fCountdown->SetText("");
	BString message;
	message.SetToFormat("Asked %s to quit; waiting for confirmation.",
		fTargetName.String());
	fStatus->SetText(message.String());
	fForceDeadline = system_time() + 10000000;
}


void
MainWindow::_Finish(const char* message, bool success)
{
	ScheduleStore::Clear();
	fScheduled = false;
	fQuitSent = false;
	fTargetTeam = -1;
	fCountdown->SetText(success ? "Done" : "Failed");
	fStatus->SetText(message);
	_SetScheduledControls(false);

	BNotification notification(success ? B_INFORMATION_NOTIFICATION
		: B_ERROR_NOTIFICATION);
	notification.SetGroup("Subdue");
	notification.SetTitle(success ? "Schedule completed" : "Schedule failed");
	notification.SetContent(message);
	notification.Send();
}


void
MainWindow::_Cancel()
{
	ScheduleStore::Clear();
	delete fHideRunner;
	fHideRunner = NULL;
	fScheduled = false;
	fQuitSent = false;
	fTargetTeam = -1;
	fCountdown->SetText("");
	fStatus->SetText("Schedule cancelled.");
	_SetScheduledControls(false);
}


void
MainWindow::_SetScheduledControls(bool scheduled)
{
	fAction->SetEnabled(!scheduled);
	fApplication->SetEnabled(!scheduled
		&& fAction->Value() == (int32)ActionMode::Subdue);
	fRefresh->SetEnabled(!scheduled
		&& fAction->Value() == (int32)ActionMode::Subdue);
	fTiming->SetEnabled(!scheduled);
	fValue->SetEnabled(!scheduled
		&& fTiming->Value() != (int32)TimingMode::TopOfHour);
	fForce->SetEnabled(!scheduled
		&& fAction->Value() == (int32)ActionMode::Subdue);
	fScheduleButton->SetLabel(scheduled ? "Cancel Schedule"
		: fAction->Value() == (int32)ActionMode::Subdue
			? "Schedule Subdue" : "Schedule Smother");
}


void
MainWindow::ShowAndActivate()
{
	if (IsHidden())
		Show();
	Activate(true);
}


void
MainWindow::HideForBackground()
{
	delete fHideRunner;
	fHideRunner = NULL;
	if (!IsHidden())
		Hide();
}


void
MainWindow::CancelSchedule()
{
	if (fScheduled)
		_Cancel();
}


void
MainWindow::AppendScheduleState(BMessage& state) const
{
	state.AddBool("scheduled", fScheduled);
	state.AddString("target", fScheduled ? fTargetName.String() : "");
	if (!fScheduled)
		state.AddString("countdown", "");
	else if (fQuitSent)
		state.AddString("countdown", "Closing…");
	else
		state.AddString("countdown", _Countdown());
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kActionChanged:
			_UpdateActionControls();
			break;
		case kTimingChanged:
			_UpdateTimingControl();
			break;
		case kRefreshApps:
			_RefreshApplications();
			break;
		case kSchedule:
			if (fScheduled)
				_Cancel();
			else
				_ScheduleFromControls();
			break;
		case kTick:
			_Tick();
			break;
		case kHide:
			delete fHideRunner;
			fHideRunner = NULL;
			if (fScheduled)
				Hide();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
			if (!fScheduled)
				_RefreshApplications();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}


bool
MainWindow::QuitRequested()
{
	if (fScheduled) {
		Hide();
		return false;
	}
	be_roster->StopWatching(BMessenger(this));
	return true;
}
