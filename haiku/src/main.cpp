#include "Options.h"
#include "SubdueApp.h"

#include <stdio.h>


int
main(int argc, char** argv)
{
	Options options = Options::Parse(argc, argv);
	if (options.help || !options.error.IsEmpty()) {
		if (!options.error.IsEmpty())
			fprintf(stderr, "Error: %s\n\n", options.error.String());
		printf("%s", Options::Usage());
		return options.error.IsEmpty() ? 0 : 2;
	}

	SubdueApp app(options);
	app.Run();
	return 0;
}
