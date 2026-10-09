#ifndef SUBDUE_MAIN_WINDOW_H
#define SUBDUE_MAIN_WINDOW_H

#include "Options.h"

#include <String.h>
#include <Window.h>

class BButton;
class BCheckBox;
class BMessageRunner;
class BOptionPopUp;
class BStringView;
class BTextControl;

class MainWindow : public BWindow {
public:
	MainWindow();
	virtual ~MainWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	bool ArmFromOptions(const Options& options);
	bool RestoreSchedule();
	void ShowAndActivate();
	void HideForBackground();
	void CancelSchedule();
	void AppendScheduleState(BMessage& state) const;

private:
	enum {
		kActionChanged = 'actn',
		kTimingChanged = 'time',
		kRefreshApps = 'rfsh',
		kSchedule = 'schd',
		kTick = 'tick',
		kHide = 'hide'
	};

	void _BuildInterface();
	void _RefreshApplications();
	void _UpdateActionControls();
	void _UpdateTimingControl();
	void _ScheduleFromControls();
	bool _Arm(ActionMode action, TimingMode timing, int32 first, int32 second,
		team_id team, const char* name, const char* signature, bool force);
	bool _ArmAt(ActionMode action, time_t fireTime, team_id team,
		const char* name, const char* signature, bool force,
		bool hideAfterDelay);
	void _Cancel();
	void _Tick();
	void _PerformAction();
	void _Finish(const char* message, bool success = true);
	void _SetScheduledControls(bool scheduled);
	time_t _CalculateFireTime(TimingMode timing, int32 first, int32 second) const;
	BString _Countdown() const;
	bool _FindApplication(const char* requested, team_id& team, BString& name,
		BString& signature) const;

	BOptionPopUp* fAction;
	BOptionPopUp* fApplication;
	BOptionPopUp* fTiming;
	BTextControl* fValue;
	BCheckBox* fForce;
	BButton* fRefresh;
	BButton* fScheduleButton;
	BStringView* fStatus;
	BStringView* fCountdown;
	BMessageRunner* fTicker;
	BMessageRunner* fHideRunner;

	bool fScheduled;
	bool fQuitSent;
	ActionMode fScheduledAction;
	team_id fTargetTeam;
	BString fTargetName;
	BString fTargetSignature;
	time_t fFireTime;
	bool fForceIfNeeded;
	bigtime_t fForceDeadline;
};

#endif
