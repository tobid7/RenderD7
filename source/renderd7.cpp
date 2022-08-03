#include <renderd7/renderd7.hpp>
#include <renderd7/log.hpp>
#include <regex>

#define D7_NOTHING C2D_Color32(0, 0, 0, 0)
#define CFGVER "3"
Log renderd7log;
float animtime;
bool isndspinit = false;
bool running = true;
std::stack<std::unique_ptr<RenderD7::Scene>> RenderD7::Scene::scenes;
std::stack<std::unique_ptr<RenderD7::Ovl>> overlays;
bool usedbgmsg = false;
std::string dspststus = "Not Initialisized!";

int cobj___;
int maxobj__;

//INI::INIFile cfgfile;
std::unique_ptr<INI::INIFile> cfgfile = nullptr;
INI::INIStructure cfgstruct;
std::string cfgpath;

// RD7 SuperReselution
bool rd7_superreselution;
u8 consoleModel = 0;
u8 sysRegion = CFG_REGION_USA;
bool is_citra = false;
//---------------------------------------
bool rd7settings = false;

std::string D_app_name;

u32 d7_hDown;
u32 d7_hHeld;
u32 d7_hUp;
touchPosition d7_touch;

C2D_TextBuf TextBuf;
C2D_Font Font;

// Fps Calc
static float current_fps = 0.0f;
static unsigned int frames = 0;
static u64 last_time = 0;
float d11framerate = 0;
//-----------------

//Metrik-------------------------------------
u32 mt_color;
u32 mt_txtcolor;

std::string mt_fps;
std::string mt_cpu;
std::string mt_gpu;
std::string mt_cmd;

bool shouldbe_disabled = false;
int cnttttt = 0;
int mt_screen;
//int mt_width = mt_screen ? 320 : 400;
float mt_txtSize;
bool metrikd = false;
//double mt_fpsgraph[320];
std::vector<int> mt_fpsgraph(320);

//Metrik-CSV
std::string csvpc;
bool mt_dumpcsv = false; //Logs the Fps and stuff to csv. It saves every second to not loose performence.
bool mt_csvloop = false; //Saves In Every Frame but slows down performens. mt_dumpcsv must be enabled.
std::ofstream mt_csv;
std::string mt_cname;
//-------------------------------------------
bool currentScreen = false;

C3D_RenderTarget* Top;
C3D_RenderTarget* TopRight;
C3D_RenderTarget* Bottom;

#define DSEVENBLACK C2D_Color32(0, 0 ,0, 255)
#define DSEVENWHITE C2D_Color32(255, 255, 255, 255)

u64 delta_time;

//Screen Fade
bool fadeout = false, fadein = false, fadeout2 = false, fadein2 = false;
int fadealpha = 0;
int fadecolor = 0;


std::string _FMT_(const std::string& fmt_str, ...)
{
	va_list ap;
	char* fp = NULL;
	va_start(ap, fmt_str);
	vasprintf(&fp, fmt_str.c_str(), ap);
	va_end(ap);
	std::unique_ptr<char, decltype(free)*> formatted(fp, free);
	return std::string(formatted.get());
}

std::string Date(void)
{
	time_t unixTime;
	struct tm timeStruct;
	time(&unixTime);
	localtime_r(&unixTime, &timeStruct);
	return _FMT_("%04i-%02i-%02i_%02i-%02i-%02i", timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday,
		timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec);
}

void screenoff()
{
    gspLcdInit();\
    GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTH);\
    gspLcdExit();
}

void screenon()
{
    gspLcdInit();\
    GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);\
    gspLcdExit();
}

float RenderD7::GetDeltaTime()
{
	return delta_time;
}

bool RenderD7::DrawImageFromSheet(RenderD7::Sheet* sheet, size_t index, float x, float y, float scaleX, float scaleY)
{
    if (sheet->spritesheet != nullptr)
    {
		if (C2D_SpriteSheetCount(sheet->spritesheet) >= index)
        {
			return C2D_DrawImageAt(C2D_SpriteSheetGetImage(sheet->spritesheet, index), x, y, 0.5f, nullptr, scaleX, scaleY);
		}
	}
    return false;
}
void RenderD7::Init::NdspFirm(bool useit)
{
    if (useit)
    {
        if ( access( "sdmc:/3ds/dspfirm.cdc", F_OK ) != -1 ) 
        {
            ndspInit();
            isndspinit = true;
			dspststus = "Initialisized success!";
        }
        else
        {
			dspststus = "Not found: dspfirm.cdc";
            renderd7log.Write("RenderD7: SoundEngine Error! ndspfirm not found!");
			RenderD7::AddOvl(std::make_unique<RenderD7::DSP_NF>());
        }
    }
}
void RenderD7::Exit::NdspFirm()
{
    if (isndspinit)
    {
        ndspExit();
    }
}
void RenderD7::Msg::Display(std::string titletxt, std::string subtext, C3D_RenderTarget *target)
{
	shouldbe_disabled = true;
	cnttttt = 0;
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, DSEVENBLACK);
	C2D_TargetClear(Bottom, DSEVENBLACK);
    RenderD7::ClearTextBufs();
	RenderD7::OnScreen(Top);
	RenderD7::Draw::Rect(0, 0, 400, 240, RenderD7::Color::Hex("#111111"));
	RenderD7::OnScreen(Bottom);
	RenderD7::Draw::Rect(0, 0, 320, 240, RenderD7::Color::Hex("#111111"));
	RenderD7::OnScreen(target);
	RenderD7::Draw::Rect(0, 0, 400, 26, RenderD7::Color::Hex("#333333", 200));
    RenderD7::Draw::Text(5, 2, 0.7f, DSEVENWHITE, titletxt);
	RenderD7::Draw::Text(5, 30, 0.6f, DSEVENWHITE, subtext);
	C3D_FrameEnd(0);
}
void RenderD7::Msg::DisplayWithProgress(std::string titletext, std::string subtext, float current, float total, u32 prgbarcolor)
{
	shouldbe_disabled = true;
	cnttttt = 0;
	RenderD7::ClearTextBufs();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, DSEVENBLACK);
	C2D_TargetClear(Bottom, DSEVENBLACK);
	char str[256];
	snprintf(str, sizeof(str), "(%.2f%%)", ((float)current/(float)total) * 100.0f);
	RenderD7::OnScreen(Top);
	RenderD7::Draw::Rect(0, 0, 400, 240, RenderD7::Color::Hex("#111111"));
	RenderD7::Draw::Rect(0, 0, 400, 26, RenderD7::Color::Hex("#333333", 200));
	RenderD7::Draw::Text(5, 2, 0.7f, DSEVENWHITE, titletext);
	RenderD7::Draw::Text(5, 30, 0.6f, DSEVENWHITE, subtext);
	RenderD7::Draw::Rect(30, 120, 342, 30, RenderD7::Color::Hex("#333333"));
	RenderD7::Draw::Rect(31, 121, (int)(((float)current / (float)total) * 338.0f), 28, prgbarcolor);
	RenderD7::Draw::TextCentered(5, 124, 0.7f, RenderD7::Color::Hex("#111111"), str, 390); 
	RenderD7::OnScreen(Bottom);
	RenderD7::Draw::Rect(0, 0, 320, 240, RenderD7::Color::Hex("#111111"));
	C3D_FrameEnd(0);
}
void RenderD7::SetupLog()
{
    renderd7log.Init("RenderD7/RenderD7.log");
}

void RenderD7::Error::DisplayError(std::string toptext, std::string errortext, int timesec)
{
	shouldbe_disabled = true;
	cnttttt = 0;
    RenderD7::ClearTextBufs();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, DSEVENBLACK);
	C2D_TargetClear(Bottom, DSEVENBLACK);
	RenderD7::OnScreen(Top);
    RenderD7::Draw::Text(0, 0, 0.7f, DSEVENWHITE, toptext);
	RenderD7::Draw::Text(0, 30, 0.6f, DSEVENWHITE, errortext);
	C3D_FrameEnd(0);
    for (int i = 0; i < 60*timesec; i++) {
		RenderD7::Draw::Rect(0, 236, (int)(((float)i / (float)60*timesec) * 400.0f), 4, RenderD7::Color::Hex("#00ff00"));
		gspWaitForVBlank();
		
		
	}
}

#include <renderd7/BitmapPrinter.hpp>

void RenderD7::Error::DisplayFatalError(std::string toptext, std::string errortext)
{
	shouldbe_disabled = true;
	cnttttt = 0;
	bool error___ = true;
	RenderD7::ClearTextBufs();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, DSEVENBLACK);
	C2D_TargetClear(Bottom, DSEVENBLACK);
	RenderD7::BitmapPrinter errorss(400, 240);
	errorss.DrawRectFilled(0, 0, 400, 240, 0, 0, 0, 255);
	errorss.DrawDebugText(4, 4, 2, RenderD7::Color::Hex("#ff0000"), toptext);
	errorss.DrawDebugText(4, 26, 1, RenderD7::Color::Hex("#000000"), errortext);
	errorss.DrawDebugText(4, 230, 1, RenderD7::Color::Hex("#000000"), "Press Start to Exit!");
	RenderD7::Image img;
	img.LoadFromBitmap(errorss.GetBitmap());
	RenderD7::OnScreen(Top);
	img.Draw(0, 0);
    /*RenderD7::Draw::TextCentered(0, 0, 0.7f, DSEVENWHITE, toptext, 400);
	RenderD7::Draw::TextCentered(0, 100, 0.6f, DSEVENWHITE, errortext, 400);
	RenderD7::Draw::TextCentered(0, 200, 0.6f, DSEVENWHITE, "Press Start to Exit!", 400);*/
	C3D_FrameEnd(0);
    while (error___)
	{
		if(d7_hDown & KEY_START)
		{
			RenderD7::ExitApp();
		}
    }
}

void RenderD7::Scene::doDraw() {
	if(!RenderD7::Scene::scenes.empty())
		RenderD7::Scene::scenes.top()->Draw();
}

void RenderD7::Scene::doLogic(u32 hDown, u32 hHeld, u32 hUp, touchPosition touch) {
	if(!RenderD7::Scene::scenes.empty())
		RenderD7::Scene::scenes.top()->Logic(hDown, hHeld, hUp, touch);
}

void RenderD7::Scene::Load(std::unique_ptr<Scene> scene, bool fade)
{ 
	Scene::scenes.push(std::move(scene));
}

void RenderD7::Scene::Back() {
	if(RenderD7::Scene::scenes.size() > 0)
		RenderD7::Scene::scenes.pop();
}

void frameloop()
{
	frames++;
	delta_time = osGetTime() - last_time;
	if (delta_time >= 1000) {
		current_fps = frames/(delta_time/1000.0f)+1;
		frames = 0;
		last_time = osGetTime();
	}
	d11framerate = current_fps;
	//for (int i = 0; i < 320; i++) mt_fpsgraph[i] = current_fps;

}
float getframerate()
{
	return d11framerate;
}

std::string RenderD7::GetFramerate()
{
	return (std::to_string((int)d11framerate).substr(0, 2));
}

bool RenderD7::MainLoop()
{
    if (!aptMainLoop()) return false;
    hidScanInput();
	d7_hDown = hidKeysDown();
	d7_hUp = hidKeysUp();
	d7_hHeld = hidKeysHeld();
	hidTouchRead(&d7_touch);

    RenderD7::ClearTextBufs();
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	
	C2D_TargetClear(Top, C2D_Color32(0, 0, 0, 0));
	C2D_TargetClear(Bottom, C2D_Color32(0, 0, 0, 0));
    frameloop();
    RenderD7::Scene::doDraw();
    RenderD7::Scene::doLogic(d7_hDown, d7_hHeld, d7_hUp, d7_touch);
    cnttttt++;
	if (cnttttt > 90)
	{
		shouldbe_disabled = false;
		cnttttt = 0;
	}
	
    return running;
}

void RenderD7::ClearTextBufs(void)
{
	C2D_TextBufClear(TextBuf);
}

void MetrikThread(RenderD7::Parameter param) {
    while (true) {
        RenderD7::DrawMetrikOvl();
        RenderD7::Thread::sleep(1000 * 1); // wait; also, this is needed to allow for concurrency (refer to the documentation for m3d::Thread::sleep())
    }
}

void RenderD7::Init::Graphics()
{
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(size_t(maxobj__));
	C2D_Prepare();
	Top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	TopRight = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	Bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	TextBuf = C2D_TextBufNew(4096);
	Font = C2D_FontLoadSystem(CFG_REGION_USA);
}

void RenderD7::Exit::Graphics()
{
	C2D_TextBufDelete(TextBuf);
	C2D_Fini();
	C3D_Fini();
}

Result RenderD7::Init::Main(std::string app_name)
{
    gfxInitDefault();
	//consoleInit(GFX_TOP, NULL);
	Result res = cfguInit();
	if (R_SUCCEEDED(res)) {
		CFGU_SecureInfoGetRegion(&sysRegion);
		CFGU_GetSystemModel(&consoleModel);
		cfguExit();
	}
	printf("cfgu\n");
	if (rd7_superreselution)
	{
		gfxSetWide(consoleModel != 3);
	}
	printf("rd7sr\n");
    aptInit();
    romfsInit();
    cfguInit();
	printf("stuff\n");
    if (cobj___){maxobj__ = cobj___;}
    if (!cobj___){maxobj__ = C2D_DEFAULT_MAX_OBJECTS;}
    D_app_name = app_name;
    cfgpath = "sdmc:/RenderD7/Apps/";
    cfgpath += D_app_name;
	csvpc = "sdmc:/RenderD7/Apps/";
	csvpc += D_app_name;
	csvpc += "/mt";
	mkdir("sdmc:/RenderD7/", 0777);
    mkdir("sdmc:/RenderD7/Apps", 0777);
    mkdir(cfgpath.c_str(), 0777);
	mkdir(csvpc.c_str(), 0777);
	bool renew = false;
	printf("folderset\n");
	if (FS::FileExist(cfgpath + "/config.ini"))
	{
		cfgfile = std::make_unique<INI::INIFile>(cfgpath + "/config.ini");
		cfgfile->read(cfgstruct);
		std::string version = cfgstruct["info"]["version"];
		if (version != CFGVER) renew = true;
	}
	printf("vercheck\n");
	renderd7log.Write("Point At: " + std::to_string(__LINE__) + " : " + GetFileName<std::string>(__FILE__));
	if (!FS::FileExist(cfgpath + "/config.ini") || renew)
	{
		cfgfile = std::make_unique<INI::INIFile>(cfgpath+ "/config.ini");
		renderd7log.Write("Point At: " + std::to_string(__LINE__) + " : " + GetFileName<std::string>(__FILE__));
		cfgfile->read(cfgstruct);
		renderd7log.Write("Point At: " + std::to_string(__LINE__) + " : " + GetFileName<std::string>(__FILE__));
		cfgstruct["info"]["version"] = CFGVER;
		cfgstruct["info"]["renderd7ver"] = RENDERD7VSTRING;
		cfgstruct["settings"]["doscreentimeout"] = "0";
		cfgstruct["settings"]["forcetimeoutLB"] = "1";
		cfgstruct["settings"]["forceFrameRate"] = "60";
		cfgstruct["settings"]["super-reselution"] = "0";
		cfgstruct["settings"]["renderer"] = "c3d_c2d";
		renderd7log.Write("Point At: " + std::to_string(__LINE__) + " : " + GetFileName<std::string>(__FILE__));
		cfgstruct["metrik-settings"]["enableoverlay"] = "0";
		cfgstruct["metrik-settings"]["Screen"] = "0";
		cfgstruct["metrik-settings"]["txtColor"] = "#ffffff";
		cfgstruct["metrik-settings"]["txtColorA"] = "255";
        cfgstruct["metrik-settings"]["ColorA"] = "255";
        cfgstruct["metrik-settings"]["Color"] = "#000000";
        cfgstruct["metrik-settings"]["txtSize"] = "0.7f";
		cfgstruct["metrik-settings"]["dumpcsv"] = "0";
		cfgstruct["metrik-settings"]["dumpcsvloop"] = "0";
		cfgfile->write(cfgstruct);
	}
	if (renew)
		printf("renew\n");
	renderd7log.Write("Point At: " + std::to_string(__LINE__) + " : " + GetFileName<std::string>(__FILE__));
	cfgfile = std::make_unique<INI::INIFile>(cfgpath+ "/config.ini");
	cfgfile->read(cfgstruct);
	std::string Fps = cfgstruct["settings"]["forceFrameRate"];
	C3D_FrameRate(RenderD7::Convert::StringtoFloat(Fps));
	metrikd = RenderD7::Convert::FloatToBool(RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["enableoverlay"]));
	mt_txtcolor = RenderD7::Color::Hex(cfgstruct["metrik-settings"]["txtColor"], (u8)RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["txtColorA"]));
    mt_color = RenderD7::Color::Hex(cfgstruct["metrik-settings"]["Color"], (u8)RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["ColorA"]));
    mt_txtSize = RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["txtSize"]);
    mt_screen = RenderD7::Convert::StringtoInt(cfgstruct["metrik-settings"]["Screen"]);
	rd7_superreselution = RenderD7::Convert::FloatToBool(RenderD7::Convert::StringtoFloat(cfgstruct["settings"]["super-reselution"]));
	mt_dumpcsv = RenderD7::Convert::FloatToBool(RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["dumpcsv"]));
	mt_csvloop = RenderD7::Convert::FloatToBool(RenderD7::Convert::StringtoFloat(cfgstruct["metrik-settings"]["dumpcsvloop"]));
	printf("read\n");
	//Check if citra
	s64 citracheck = 0;
	svcGetSystemInfo(&citracheck, 0x20000, 0);
	is_citra = citracheck ? true : false;
	printf("citra\n");
	//Speedup
    osSetSpeedupEnable(true);
	printf("boost\n");
	if (!is_citra && rd7_superreselution)
	{
		if (consoleModel != 3) gfxSetWide(true);
	}
	printf("rd7sr\n");
	//consoleInit(GFX_BOTTOM, NULL);
	if (mt_dumpcsv)
	{
		mt_cname = csvpc;
		mt_cname += "/";
		mt_cname += Date();
		mt_cname += ".csv";
		
		FILE* logfile = fopen((mt_cname.c_str()), "w");
		fclose(logfile);
		
		mt_csv.open((mt_cname), std::ofstream::app);
		mt_csv << "FPS,CPU,GPU,CMD\n";
		mt_csv.close();
	}
	printf("csv\n");
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	Top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	TopRight = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	Bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	TextBuf = C2D_TextBufNew(4096);
	Font = C2D_FontLoadSystem(CFG_REGION_USA);
	printf("Graphical Interface\n");
    //RenderD7::Msg::Display("RenderD7", "RenderD7 init success!\nWaiting for MainLoop!", Top);
	return 0;
}

Result RenderD7::Init::Reload()
{
	C2D_TextBufDelete(TextBuf);
	C2D_Fini();
	C3D_Fini();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(size_t(maxobj__));
	C2D_Prepare();
	Top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	TopRight = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	Bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	TextBuf = C2D_TextBufNew(4096);
	Font = C2D_FontLoadSystem(CFG_REGION_USA);

	return 0;
}

void RenderD7::ToggleRD7SR()
{
	shouldbe_disabled = true;
	cnttttt = 0;
	// Display black screen
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, RenderD7::Color::Hex("#000000"));
	RenderD7::OnScreen(Top);
	C3D_FrameEnd(0);
	// Toggle 400px/800px mode
	gfxSetWide(gfxIsWide() ? false : true);
	rd7_superreselution = gfxIsWide();
	RenderD7::Init::Reload();
}

bool RenderD7::IsRD7SR()
{
	return gfxIsWide();
}

void RenderD7::Exit::Main()
{
	cfgfile->write(cfgstruct);
    if (RenderD7::Threads::threadrunning) RenderD7::Threads::Exit();
    C2D_TextBufDelete(TextBuf);
	C2D_Fini();
	C3D_Fini();
    aptExit();
	gfxExit();
    romfsExit();
    cfguExit();
	romfsExit();
}

void RenderD7::DrawTObjects(std::vector<RenderD7::TObject> tobjects, u32 color, u32 txtcolor, int selection, u32 selbgcolor, u32 selcolor)
{
	for(int i = 0; i < (int)tobjects.size(); i++)
	{
		if (selection == i)
		{
			RenderD7::Draw::Rect(tobjects[i].x - 2, tobjects[i].y - 2, tobjects[i].w + 4, tobjects[i].h + 4, selbgcolor);
			RenderD7::Draw::Rect(tobjects[i].x, tobjects[i].y, tobjects[i].w, tobjects[i].h, color);
			RenderD7::Draw::Rect(tobjects[i].x, tobjects[i].y, tobjects[i].w, tobjects[i].h, selcolor);
			RenderD7::Draw::Text(tobjects[i].x + (tobjects[i].w/2) - RenderD7::Draw::GetTextHeight(tobjects[i].txtsize , tobjects[i].text) + tobjects[i].correctx, tobjects[i].y + (tobjects[i].h/2) - RenderD7::Draw::GetTextHeight(tobjects[i].txtsize, tobjects[i].text) + tobjects[i].correcty, tobjects[i].txtsize, txtcolor, tobjects[i].text);
		}
		else
		{
			RenderD7::Draw::Rect(tobjects[i].x, tobjects[i].y - 1, tobjects[i].w, tobjects[i].h, color);
			RenderD7::Draw::Text(tobjects[i].x + (tobjects[i].w/2) - RenderD7::Draw::GetTextHeight(tobjects[i].txtsize , tobjects[i].text) + tobjects[i].correctx, tobjects[i].y + (tobjects[i].h/2) - RenderD7::Draw::GetTextHeight(tobjects[i].txtsize, tobjects[i].text) + tobjects[i].correcty, tobjects[i].txtsize, txtcolor, tobjects[i].text);
		}
	}
}

void RenderD7::DrawTLBtns(std::vector<RenderD7::TLBtn> btns, u32 color, int selection, u32 selbgcolor, u32 selcolor)
{
	for(int i = 0; i < (int)btns.size(); i++)
	{
		if (selection == i)
		{
			RenderD7::Draw::Rect(btns[i].x - 2, btns[i].y - 2, btns[i].w + 4, btns[i].h + 4, selbgcolor);
			RenderD7::Draw::Rect(btns[i].x, btns[i].y, btns[i].w, btns[i].h, color);
			RenderD7::Draw::Rect(btns[i].x, btns[i].y, btns[i].w,  btns[i].h, selcolor);
	        }
		else
		{
			RenderD7::Draw::Rect(btns[i].x, btns[i].y - 1, btns[i].w, btns[i].h, color);
		}
	}
}

void RenderD7::ExitApp()
{
	if (!rd7settings) running = false;
}

bool RenderD7::touchTObj(touchPosition touch, RenderD7::TObject button)
{
   if (touch.px >= button.x && touch.px <= (button.x + button.w) && touch.py >= button.y && touch.py <= (button.y + button.h))	return true;
    else	return false;
}

bool RenderD7::touchTLBtn(touchPosition touch, RenderD7::TLBtn button)
{
   if (touch.px >= button.x && touch.px <= (button.x + button.w) && touch.py >= button.y && touch.py <= (button.y + button.h))	return true;
    else	return false;
}

int RenderD7::GetRandomInt(int b, int e)
{
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(b, e);
	int r = distribution(generator); 
	return r;
}

void RenderD7::DrawSTObject(std::vector<RenderD7::TObject> tobject, int tobjectindex, u32 color, u32 txtcolor)
{
	RenderD7::Draw::Rect(tobject[tobjectindex].x, tobject[tobjectindex].y, tobject[tobjectindex].w, tobject[tobjectindex].h, color);
	RenderD7::Draw::Text(tobject[tobjectindex].x + (tobject[tobjectindex].w/2) - RenderD7::Draw::GetTextHeight(tobject[tobjectindex].txtsize , tobject[tobjectindex].text) + tobject[tobjectindex].correctx, tobject[tobjectindex].y + (tobject[tobjectindex].h/2) - RenderD7::Draw::GetTextHeight(tobject[tobjectindex].txtsize, tobject[tobjectindex].text) + tobject[tobjectindex].correcty, tobject[tobjectindex].txtsize, txtcolor, tobject[tobjectindex].text);
}

bool dirEntryPredicate(const RenderD7::DirContent &lhs, const RenderD7::DirContent &rhs) {
	if (!lhs.isDir && rhs.isDir) return false;
	if (lhs.isDir && !rhs.isDir) return true;

	return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
}

void RenderD7::GetDirContentsExt(std::vector<RenderD7::DirContent> &dircontent, const std::vector<std::string> &extensions) {
	struct stat st;

	dircontent.clear();

	DIR *pdir = opendir(".");

	if (pdir != nullptr) {
		while(true) {
			RenderD7::DirContent dirEntry;

			struct dirent *pent = readdir(pdir);
			if (pent == NULL) break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;
			dirEntry.isDir = (st.st_mode & S_IFDIR) ? true : false;

			if (dirEntry.name.compare(".") != 0 && (dirEntry.isDir || RenderD7::NameIsEndingWith(dirEntry.name, extensions))) {
				dircontent.push_back(dirEntry);
			}
		}

		closedir(pdir);
	}
	sort(dircontent.begin(), dircontent.end(), dirEntryPredicate);
}

void RenderD7::GetDirContents(std::vector<RenderD7::DirContent> &dircontent) {
	RenderD7::GetDirContentsExt(dircontent, {});
}

bool RenderD7::FS::FileExist(const std::string& path)
{
	FILE *test = fopen(path.c_str(), "r");
    if(test != NULL)
    {
        fclose(test);
	
        return true;
    }

    return false;
}

bool RenderD7::IsNdspInit()
{
	if (isndspinit)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void RenderD7::DrawList1(RenderD7::ScrollList1 &l, float txtsize, C3D_RenderTarget *t)
{
	RenderD7::OnScreen(t);
	RenderD7::Draw::Rect(0, 0, 400, 240, RenderD7::Color::Hex("#dddddd"));
	RenderD7::Draw::Text(0, 0, 0.8f, RenderD7::Color::Hex("#ffffff"), l.Text);
}


void RenderD7::DrawMetrikOvl()
{
    switch (mt_screen)
    {
        case 0:
        RenderD7::OnScreen(Top);
		break;
        case 1:
        RenderD7::OnScreen(Bottom);
		break;
        default:
        RenderD7::OnScreen(Bottom);
		break;
    }
	std::string __C = RENDERD7VSTRING;
	std::string info = "RenderD7 " + __C + " Debug Overlay";
	float infoy = 240 - RenderD7::Draw::GetTextHeight(mt_txtSize, info);
	mt_fps = "FPS: " + RenderD7::GetFramerate();
	mt_cpu = "CPU: " + std::to_string(C3D_GetProcessingTime()*(d11framerate/10)).substr(0, 4) + "%/" + std::to_string(C3D_GetProcessingTime()).substr(0, 4) + "ms";
	mt_gpu = "GPU: " + std::to_string(C3D_GetDrawingTime()*(d11framerate/10)).substr(0, 4) + "%/" + std::to_string(C3D_GetDrawingTime()).substr(0, 4) + "ms";
	mt_cmd = "CMD: " + std::to_string(C3D_GetCmdBufUsage()*100.0f).substr(0, 4) + "%/" + std::to_string(C3D_GetCmdBufUsage()).substr(0, 4) + "ms";
	RenderD7::Draw::Rect(0, 0, RenderD7::Draw::GetTextWidth(mt_txtSize, mt_fps), RenderD7::Draw::GetTextHeight(mt_txtSize, mt_fps), mt_color);
	RenderD7::Draw::Rect(0, 50, RenderD7::Draw::GetTextWidth(mt_txtSize, mt_cpu), RenderD7::Draw::GetTextHeight(mt_txtSize, mt_cpu), mt_color);
	RenderD7::Draw::Rect(0, 70, RenderD7::Draw::GetTextWidth(mt_txtSize, mt_gpu), RenderD7::Draw::GetTextHeight(mt_txtSize, mt_gpu), mt_color);
	RenderD7::Draw::Rect(0, 90, RenderD7::Draw::GetTextWidth(mt_txtSize, mt_cmd), RenderD7::Draw::GetTextHeight(mt_txtSize, mt_cmd), mt_color);
	RenderD7::Draw::Rect(0, infoy, RenderD7::Draw::GetTextWidth(mt_txtSize, info), RenderD7::Draw::GetTextHeight(mt_txtSize, info), mt_color);
	RenderD7::Draw::Text(0, 0, mt_txtSize, mt_txtcolor, mt_fps);
    RenderD7::Draw::Text(0, 50, mt_txtSize, mt_txtcolor, mt_cpu);
    RenderD7::Draw::Text(0, 70, mt_txtSize, mt_txtcolor, mt_gpu);
	RenderD7::Draw::Text(0, 90, mt_txtSize, mt_txtcolor, mt_cmd);
	RenderD7::Draw::Text(0, infoy, mt_txtSize, mt_txtcolor, info);
	
    /*for (int z = 0; z < (int)mt_fpsgraph.size(); z++)
    {
		//mt_fpsgraph[z] = (int)d11framerate;
        C2D_DrawLine(z, 239 - mt_fpsgraph[z], mt_txtcolor, z + 1, 239 - mt_fpsgraph[z + 1], mt_txtcolor, 1, 1);
    }*/
}

RenderD7::DSP_NF::DSP_NF()
{

}

void RenderD7::DSP_NF::Draw(void) const
{
	RenderD7::OnScreen(Top);
	RenderD7::Draw::Rect(0, msgposy, 400, 70, RenderD7::Color::Hex("#111111"));
	RenderD7::Draw::Rect(0, msgposy, 400, 25, RenderD7::Color::Hex("#222222"));
	RenderD7::Draw::Text(2, msgposy+3, 0.7f, RenderD7::Color::Hex("#ffffff"), "Warning! Code: 00027");
	RenderD7::Draw::Text(2, msgposy+30, 0.6f, RenderD7::Color::Hex("#ffffff"), "You can't use Sound effects because the file\n<<sdmc:/3ds/dspfirm.cdc>> was not found!");
}

void RenderD7::DSP_NF::Logic()
{
	this->delay++/*=(int)RenderD7::GetDeltaTime()*/;
	if (msgposy > 170 && delay < 5*60) msgposy--/*=(int)RenderD7::GetDeltaTime()*/;
	
	if (delay >= 5*60)
	{
		msgposy++/*=(int)RenderD7::GetDeltaTime*/;
		if(msgposy > 400) this->Kill();
	}
}

void OvlHandler()
{
	if (!overlays.empty())
	{
		overlays.top()->Draw();
	}
	if (!overlays.empty())
	{
		overlays.top()->Logic();
	}
	if (!overlays.empty())
	{
		if (overlays.top()->IsKilled()) overlays.pop(); 
	}
	
}
int lp = 0;
void RenderD7::FrameEnd()
{
	if (metrikd && !shouldbe_disabled)RenderD7::DrawMetrikOvl();
	if (!shouldbe_disabled) OvlHandler();
	/*if (d7_hHeld & KEY_R && d7_hDown & KEY_SELECT)
	{
		RenderD7::LoadSettings();
	}*/
	if (mt_dumpcsv && lp == 60)
	{
		std::string _mt_fps = RenderD7::GetFramerate();
		std::string _mt_cpu = std::to_string(C3D_GetProcessingTime()).substr(0, 4);
		std::string _mt_gpu = std::to_string(C3D_GetDrawingTime()).substr(0, 4);
		std::string _mt_cmd = std::to_string(C3D_GetCmdBufUsage()).substr(0, 4);
		mt_csv.open((mt_cname), std::ofstream::app);
		std::string fmt_ = _mt_fps + "," + _mt_cpu + "," + _mt_gpu + "," + _mt_cmd + "\n";
		mt_csv << fmt_;
		mt_csv.close();
		lp = 0;
	}
	lp++;
	
	C3D_FrameEnd(0);
}

RenderD7::RSettings::RSettings()
{
	cfgfile = std::make_unique<INI::INIFile>(cfgpath+ "/config.ini");
	cfgfile->read(cfgstruct);
	rd7settings = true;
}

RenderD7::RSettings::~RSettings()
{
	cfgfile->write(cfgstruct);
}

void RenderD7::RSettings::Draw(void) const
{
	if (m_state == RSETTINGS)
	{
		RenderD7::OnScreen(Top);
		RenderD7::Draw::Rect(0, 0, 400, 21, RenderD7::Color::Hex("#111111"));
		RenderD7::Draw::Rect(0, 21, 400, 220, RenderD7::Color::Hex("#eeeeee"));
		RenderD7::Draw::Text(0, 0, 0.7f, DSEVENWHITE, "RenderD7->Settings");
		RenderD7::Draw::TextRight(400, 0, 0.7f, RenderD7::Color::Hex("#ffffff"), RENDERD7VSTRING);
		RenderD7::Draw::Text(0, 30, 0.7f, DSEVENBLACK, "RD7SR: " + rd7srstate);
		RenderD7::Draw::Text(0, 50, 0.7f, DSEVENBLACK, "Metrik to Csv: " + csvstate);
		RenderD7::Draw::Text(0, 70, 0.7f, DSEVENBLACK, "Metrik Overlay: " + mtovlstate);
		RenderD7::Draw::Text(0, 90, 0.7f, DSEVENBLACK, "Force FPS: " + fpsstate);
		RenderD7::Draw::Text(0, 110, 0.7f, DSEVENBLACK, "Metrik Screen: " + mtscreenstate);
		/*RenderD7::Draw::Text(0, 130, 0.7f, DSEVENBLACK, "Metrik Text RGB: " + mttxtcolstate);
		RenderD7::Draw::Text(0, 150, 0.7f, DSEVENBLACK, "Metrik Alpha: " + mtcola);
		RenderD7::Draw::Text(0, 170, 0.7f, DSEVENBLACK, "Metrik Text Alpha: " + mttxtcola);*/
		RenderD7::OnScreen(Bottom);
		std::string verc = "Config Version: ";
		verc += CFGVER;
		RenderD7::Draw::Rect(0, 0, 320, 240, RenderD7::Color::Hex("#eeeeee"));
		RenderD7::Draw::Text(0, 0, 0.7f, RenderD7::Color::Hex("#111111"), verc);
		RenderD7::DrawTObjects(buttons, RenderD7::Color::Hex("#111111"), RenderD7::Color::Hex("#eeeeee"));

	}
	else if (m_state == RINFO)
	{
		std::string rd7ver = RENDERD7VSTRING;
		std::string rd7cfgver = CFGVER;
		std::string citras = is_citra ? "true" : "false";
		std::string buildtime = V_TIME;
		std::string commit = V_STRING;
		RenderD7::OnScreen(Top);
		RenderD7::Draw::Rect(0, 0, 400, 21, RenderD7::Color::Hex("#111111"));
		RenderD7::Draw::Rect(0, 21, 400, 220, RenderD7::Color::Hex("#eeeeee"));
		RenderD7::Draw::Text(0, 0, 0.7f, DSEVENWHITE, "RenderD7->Info");
		RenderD7::Draw::TextRight(400, 0, 0.7f, RenderD7::Color::Hex("#ffffff"), RENDERD7VSTRING);
		RenderD7::Draw::Text(0, 30, 0.7f, DSEVENBLACK, "App: " + D_app_name);
		RenderD7::Draw::Text(0, 50, 0.7f, DSEVENBLACK, "RenderD7: " + rd7ver);
		RenderD7::Draw::Text(0, 70, 0.7f, DSEVENBLACK, "Config-Version: " + rd7cfgver);
		RenderD7::Draw::Text(0, 90, 0.7f, DSEVENBLACK, "Citra: " + citras);
		RenderD7::Draw::Text(0, 110, 0.7f, DSEVENBLACK, "RenderD7-Build-Time: \n" + buildtime);
		RenderD7::Draw::Text(0, 150, 0.7f, DSEVENBLACK, "RenderD7-Commit: " + commit);
		/*RenderD7::Draw::Text(0, 130, 0.7f, DSEVENBLACK, "Metrik Text RGB: " + mttxtcolstate);
		RenderD7::Draw::Text(0, 150, 0.7f, DSEVENBLACK, "Metrik Alpha: " + mtcola);
		RenderD7::Draw::Text(0, 170, 0.7f, DSEVENBLACK, "Metrik Text Alpha: " + mttxtcola);*/
		RenderD7::OnScreen(Bottom);
		std::string verc = "Config Version: ";
		verc += CFGVER;
		RenderD7::Draw::Rect(0, 0, 320, 240, RenderD7::Color::Hex("#eeeeee"));
		RenderD7::Draw::Text(0, 0, 0.7f, RenderD7::Color::Hex("#111111"), verc);
		RenderD7::DrawTObjects(buttons, RenderD7::Color::Hex("#111111"), RenderD7::Color::Hex("#eeeeee"));

	}
}
std::string RenderD7::Kbd(int lenght, SwkbdType tp)
{
	shouldbe_disabled = true;
	RenderD7::FrameEnd();
	SwkbdState state;
	char temp[lenght + 1] = { 0 };
	
	swkbdInit(&state, tp, 2, lenght);
	swkbdSetValidation(&state, SWKBD_NOTBLANK_NOTEMPTY, SWKBD_FILTER_PROFANITY, 0);
	SwkbdButton ret = swkbdInputText(&state, temp, sizeof(temp));
	temp[lenght] = '\0';
	return (ret == SWKBD_BUTTON_CONFIRM ? temp : "60");
}
void RenderD7::RSettings::Logic(u32 hDown, u32 hHeld, u32 hUp, touchPosition touch)
{
	if (m_state == RSETTINGS) {
	rd7srstate = rd7_superreselution ? "true" : "false";
	csvstate = mt_dumpcsv ? "true" : "false";
	mtovlstate = metrikd ? "true" : "false";
	fpsstate = cfgstruct["settings"]["forceFrameRate"];
	mtscreenstate = mt_screen ? "Bottom" : "Top";
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[0]) && !metrikd)
	{
		RenderD7::ToggleRD7SR();
		cfgstruct["settings"]["super-reselution"] = rd7_superreselution ? "1" : "0";
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[1]))
	{
		mt_dumpcsv = mt_dumpcsv ? false : true;
		cfgstruct["metrik-settings"]["dumpcsv"] = mt_dumpcsv ? "1" : "0";
		if (mt_dumpcsv)
		{
			mt_cname = csvpc;
			mt_cname += "/";
			mt_cname += Date();
			mt_cname += ".csv";
			FILE* logfile = fopen((mt_cname.c_str()), "w");
			fclose(logfile);
			mt_csv.open((mt_cname), std::ofstream::app);
			mt_csv << "FPS,CPU,GPU,CMD\n";
			mt_csv.close();
		}
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[2]))
	{
		metrikd = metrikd ? false : true;
		cfgstruct["metrik-settings"]["enableoverlay"] = metrikd ? "1" : "0";
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[3]) && !metrikd)
	{
		cfgstruct["settings"]["forceFrameRate"] = Kbd(2, SWKBD_TYPE_NUMPAD);
		C3D_FrameRate(RenderD7::Convert::StringtoFloat(cfgstruct["settings"]["forceFrameRate"]));
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[4]))
	{
		mt_screen = mt_screen ? 0 : 1;
		cfgstruct["metrik-settings"]["screen"] = mt_screen ? "1" : "0";
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[5]))
	{
		RenderD7::AddOvl(std::make_unique<RenderD7::DSP_NF>());
	}
	if (d7_hDown & KEY_TOUCH && RenderD7::touchTObj(d7_touch, buttons[6]))
	{
		m_state = RINFO;
	}
	if (d7_hDown & KEY_B)
	{
		cfgfile->write(cfgstruct);
		rd7settings = false;
		RenderD7::Scene::Back();
	}
	
	}
	if (m_state == RINFO)
	{
		if (d7_hDown & KEY_B)
		{
			m_state = RSETTINGS;
		}
	}
	
}

void RenderD7::LoadSettings(){
	RenderD7::Scene::Load(std::make_unique<RenderD7::RSettings>());
}

void RenderD7::AddOvl(std::unique_ptr<RenderD7::Ovl> overlay)
{
	overlays.push(std::move(overlay));
}

RenderD7::Console::Console()
{
    this->x = 0;
    this->y = 0;
    this->w = 320;
    this->h = 240;
    this->color = {0, 0, 0, 255};
}
RenderD7::Console::Console(int x, int y, int w, int h, u8 a)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->color = {0, 0, 0, a};
    
}
RenderD7::Console::Console(int x, int y, int w, int h, RenderD7::Color::rgba col)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->color = col;
}
RenderD7::Console::Console(int x, int y, int w, int h, std::string name, RenderD7::Color::rgba col, RenderD7::Color::rgba barcol, RenderD7::Color::rgba outlinecol)
{
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->color = col;
    this->outlinecol = outlinecol;
    this->barcolor = barcol;
    this->m_name = name;
}
RenderD7::Console::~Console()
{

}
void RenderD7::Console::On(C3D_RenderTarget *t_cscreen)
{
     this->cscreen = t_cscreen;
}
bool RenderD7::Console::Update()
{
     bool dr_sc = true;
     return dr_sc;
}
