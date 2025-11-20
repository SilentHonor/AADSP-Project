#include "haos_emulation.h"
#include "haos.h"
#include "odt_modules.h"

int main (int argc, const char * argv[])
{
	HAOS_init(argc, argv);
	HAOS_add_modules(odt);
	HAOS_run();
}
