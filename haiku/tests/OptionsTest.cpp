#include "Options.h"

#include <stdio.h>


static int sFailures = 0;


static void
Check(bool condition, const char* description)
{
	if (condition)
		return;
	fprintf(stderr, "FAIL: %s\n", description);
	sFailures++;
}


int
main()
{
	{
		const char* arguments[] = {"Subdue"};
		Options options = Options::Parse(1, arguments);
		Check(options.error.IsEmpty(), "GUI launch has no parse error");
		Check(!options.HasSchedule(), "GUI launch has no schedule");
	}
	{
		const char* arguments[] = {"Subdue", "--mute", "--after", "5"};
		Options options = Options::Parse(4, arguments);
		Check(options.error.IsEmpty(), "Smother delay parses");
		Check(options.action == ActionMode::Smother, "Smother action selected");
		Check(options.timing == TimingMode::After && options.minutes == 5,
			"Smother delay retained");
	}
	{
		const char* arguments[] = {"Subdue", "--app", "StyledEdit", "--at",
			"23:15", "--no-force"};
		Options options = Options::Parse(6, arguments);
		Check(options.error.IsEmpty(), "Application clock time parses");
		Check(options.application == "StyledEdit", "Application retained");
		Check(options.hour == 23 && options.minute == 15, "Clock retained");
		Check(!options.force, "No-force retained");
	}
	{
		const char* arguments[] = {"Subdue", "--mute", "--after", "1", "--top-of-hour"};
		Options options = Options::Parse(5, arguments);
		Check(!options.error.IsEmpty(), "Duplicate timing is rejected");
	}
	{
		const char* arguments[] = {"Subdue", "--app", "StyledEdit"};
		Options options = Options::Parse(3, arguments);
		Check(!options.error.IsEmpty(), "Target without timing is rejected");
	}
	{
		const char* arguments[] = {"Subdue", "--mute", "--at", "25:00"};
		Options options = Options::Parse(4, arguments);
		Check(!options.error.IsEmpty(), "Invalid clock is rejected");
	}

	if (sFailures == 0)
		printf("All option parser tests passed.\n");
	return sFailures == 0 ? 0 : 1;
}
