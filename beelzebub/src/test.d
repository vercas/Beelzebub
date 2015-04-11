extern(C) void screen_write(const char * string, short x, short y);

extern(C) void test_ep()
{
	screen_write("asd", 0, 0);
}

extern(C) void* _Dmodule_ref;
