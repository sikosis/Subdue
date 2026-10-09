#include "DeskbarView.h"

#include "Protocol.h"

#include <Application.h>
#include <Archivable.h>
#include <Deskbar.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include <math.h>


SubdueDeskbarView::SubdueDeskbarView(BRect frame)
	:
	BView(frame, kSubdueDeskbarName, B_FOLLOW_NONE, B_WILL_DRAW),
	fRunner(NULL),
	fConnected(false),
	fScheduled(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


SubdueDeskbarView::SubdueDeskbarView(BMessage* archive)
	:
	BView(archive),
	fRunner(NULL),
	fConnected(false),
	fScheduled(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


SubdueDeskbarView::~SubdueDeskbarView()
{
	delete fRunner;
}


BArchivable*
SubdueDeskbarView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, kSubdueDeskbarName))
		return NULL;
	return new SubdueDeskbarView(archive);
}


status_t
SubdueDeskbarView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status != B_OK)
		return status;
	archive->AddString("add_on", kSubdueSignature);
	archive->AddString("class", kSubdueDeskbarName);
	return B_OK;
}


status_t
SubdueDeskbarView::Install()
{
	BDeskbar deskbar;
	if (!deskbar.IsRunning())
		return B_NO_INIT;
	if (deskbar.HasItem(kSubdueDeskbarName))
		return B_OK;

	float height = deskbar.MaxItemHeight();
	SubdueDeskbarView* view
		= new SubdueDeskbarView(BRect(0, 0, 91, height - 1));
	status_t status = deskbar.AddItem(view);
	delete view;
	return status;
}


status_t
SubdueDeskbarView::Remove()
{
	BDeskbar deskbar;
	if (!deskbar.IsRunning())
		return B_NO_INIT;
	if (!deskbar.HasItem(kSubdueDeskbarName))
		return B_OK;
	return deskbar.RemoveItem(kSubdueDeskbarName);
}


void
SubdueDeskbarView::AttachedToWindow()
{
	BView::AttachedToWindow();
	AdoptParentColors();
	SetDrawingMode(B_OP_OVER);
	SetToolTip("Subdue — no schedule");

	delete fRunner;
	BMessage pulse(kPulse);
	fRunner = new BMessageRunner(BMessenger(this), pulse, 1000000);
	_QueryState();
}


void
SubdueDeskbarView::DetachedFromWindow()
{
	delete fRunner;
	fRunner = NULL;
	BView::DetachedFromWindow();
}


void
SubdueDeskbarView::Draw(BRect updateRect)
{
	(void)updateRect;
	BRect bounds = Bounds();
	float iconSize = floorf(bounds.Height() - 6);
	if (iconSize < 8)
		iconSize = 8;
	BRect clock(3, (bounds.Height() - iconSize) / 2,
		3 + iconSize, (bounds.Height() + iconSize) / 2);

	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	SetPenSize(1.4f);
	StrokeEllipse(clock);
	BPoint center(clock.left + clock.Width() / 2,
		clock.top + clock.Height() / 2);
	StrokeLine(center, BPoint(center.x, clock.top + 3));
	StrokeLine(center, BPoint(clock.right - 3, center.y));

	const char* text = fScheduled ? fCountdown.String() : "Subdue";
	font_height metrics;
	GetFontHeight(&metrics);
	float baseline = (bounds.Height() - metrics.ascent - metrics.descent) / 2
		+ metrics.ascent;
	DrawString(text, BPoint(clock.right + 5, baseline));
}


void
SubdueDeskbarView::MouseDown(BPoint where)
{
	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	BMessage* current = Window() != NULL ? Window()->CurrentMessage() : NULL;
	if (current != NULL)
		current->FindInt32("buttons", &buttons);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		_ShowMenu(where);
		return;
	}
	_SendCommand(kMsgShowWindow, true);
}


void
SubdueDeskbarView::_ShowMenu(BPoint where)
{
	BPopUpMenu* menu = new BPopUpMenu("Subdue", false, false);
	BMenuItem* show = new BMenuItem("Show Subdue",
		new BMessage(kMsgShowWindow));
	BMenuItem* cancel = new BMenuItem("Cancel Schedule",
		new BMessage(kMsgCancelSchedule));
	BMenuItem* quit = new BMenuItem("Quit Subdue",
		new BMessage(kMsgQuitApp));

	BMessenger target(this);
	show->SetTarget(target);
	cancel->SetTarget(target);
	quit->SetTarget(target);
	cancel->SetEnabled(fConnected && fScheduled);
	quit->SetEnabled(fConnected);
	menu->AddItem(show);
	menu->AddItem(cancel);
	menu->AddSeparatorItem();
	menu->AddItem(quit);
	menu->SetAsyncAutoDestruct(true);
	menu->Go(ConvertToScreen(where), true, true, true);
}


status_t
SubdueDeskbarView::_SendCommand(uint32 command, bool launchIfMissing)
{
	BMessenger target(kSubdueSignature);
	if (!target.IsValid()) {
		if (launchIfMissing)
			return be_roster->Launch(kSubdueSignature);
		return B_BAD_VALUE;
	}
	BMessage message(command);
	return target.SendMessage(&message);
}


void
SubdueDeskbarView::_QueryState()
{
	BMessenger target(kSubdueSignature);
	if (!target.IsValid()) {
		_SetDisconnected();
		return;
	}
	BMessage query(kMsgQueryState);
	if (target.SendMessage(&query, BMessenger(this), 100000) != B_OK)
		_SetDisconnected();
}


void
SubdueDeskbarView::_SetDisconnected()
{
	bool changed = fConnected || fScheduled || fCountdown != "";
	fConnected = false;
	fScheduled = false;
	fCountdown = "";
	fTarget = "";
	SetToolTip("Subdue — click to open");
	if (changed)
		Invalidate();
}


void
SubdueDeskbarView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kPulse:
			_QueryState();
			break;
		case kMsgShowWindow:
			_SendCommand(kMsgShowWindow, true);
			break;
		case kMsgCancelSchedule:
			_SendCommand(kMsgCancelSchedule);
			break;
		case kMsgQuitApp:
			_SendCommand(kMsgQuitApp);
			break;
		case kMsgState: {
			fConnected = true;
			message->FindBool("scheduled", &fScheduled);
			const char* countdown = "";
			const char* target = "";
			message->FindString("countdown", &countdown);
			message->FindString("target", &target);
			fCountdown = countdown;
			fTarget = target;
			BString tooltip("Subdue");
			if (fScheduled)
				tooltip.SetToFormat("Subdue — %s — %s", fTarget.String(),
					fCountdown.String());
			else
				tooltip << " — no schedule";
			SetToolTip(tooltip.String());
			Invalidate();
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}


extern "C" _EXPORT BView*
instantiate_deskbar_item()
{
	BDeskbar deskbar;
	float height = deskbar.MaxItemHeight();
	return new SubdueDeskbarView(BRect(0, 0, 91, height - 1));
}
