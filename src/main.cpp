/* 
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// I'm trying to comment the source but my english isn't perfect, so be 
//tolerant and please correct all the mistakes you find. ;)

#if defined( MACOSX )
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#include "stellarium.h"
#include "star_mgr.h"
#include "draw.h"
#include "constellation.h"
#include "constellation_mgr.h"
#include "planet_mgr.h"
#include "nebula.h"
#include "nebula_mgr.h"
#include "s_texture.h"
#include "stellarium_ui.h"
#include "parsecfg.h"
#include "navigation.h"
#include "s_gui.h"
#include "hip_star_mgr.h"
#include "shooting.h"

using namespace std;

stellariumParams global;
Star_mgr * VouteCeleste;              // Stars from HR catalog
Hip_Star_mgr * HipVouteCeleste;       // Class to manage the Hipparcos catalog
Constellation_mgr * ConstellCeleste;  // Constellation boundary and name
Nebula_mgr * messiers;                // Class to manage the messier objects
Planet_mgr * SolarSystem;             // Class to manage the planets
s_texture * texIds[200];              // Common Textures

ShootingStar * TheShooting = NULL;

static int timeAtmosphere=0;

void init();

// ***************  Print a beautiful console logo !! ******************
void drawIntro(void)
{
    printf("\n");
    printf("    -----------------------------------------\n");
    printf("    | ## ### ### #   #   ### ###  #  # # # #|\n");
    printf("    | #   #  ##  #   #   ### ##   #  # # ###|\n");
    printf("    |##   #  ### ### ### # # # #  #  ### # #|\n");
    printf("    |                     %s|\n",APP_NAME);
    printf("    -----------------------------------------\n\n");
    printf("Copyright (C) 2002 Fabien Chereau chereau@free.fr\n\n");
    printf("Please send bug report & comments to stellarium@free.fr\n\n");
};

// ************************  Main display loop  ************************
void glutDisplay(void)
{  
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(global.Fov, (double)global.X_Resolution / 
		   global.Y_Resolution, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    
    // Update the position of observation and time etc...
    Update_time(*SolarSystem);
    Update_variables();

    // Execute all the drawing function in the correct order from the more 
    // far to nearest objects

    // ---- Equatorial Coordinates
    Switch_to_equatorial();          // Switch in Equatorial coordinates
    DrawMilkyWay();                  // Draw the milky way --> init the buffers

    if (global.FlagNebula && (!global.FlagAtmosphere || 
			      global.SkyBrightness<0.1)) 
	messiers->Draw();            // Draw the Messiers Objects
    if (global.FlagConstellationDrawing)
        ConstellCeleste->Draw();     // Draw all the constellations
    if ((!global.FlagAtmosphere && global.FlagStars) || 
        (global.SkyBrightness<0.2 && global.FlagStars)) 
    {
	HipVouteCeleste->Draw(); 
	//VouteCeleste->Draw();      // Draw the stars
    }

	if (!TheShooting) TheShooting = new ShootingStar(global.Timefr);
	if (TheShooting->IsDead())
	{
		delete TheShooting;
		TheShooting = new ShootingStar(global.Timefr);
	}
	TheShooting->Draw(global.Timefr);

    if (global.FlagPlanets || global.FlagPlanetsHintDrawing) 
	SolarSystem->Draw();         // Draw the planets
    if (global.FlagAtmosphere && global.SkyBrightness>0) 
	DrawAtmosphere2();

    SolarSystem->DrawMoonDaylight();

    if (global.FlagEquatorialGrid)
    { 
	DrawMeridiens();             // Draw the meridian lines
        DrawParallels();             // Draw the parallel lines
    }
    if (global.FlagEquator) 
	DrawEquator();               // Draw the celestial equator line
    if (global.FlagEcliptic) 
	DrawEcliptic();              // Draw the ecliptic line
    if (global.FlagConstellationName)
        ConstellCeleste->DrawName(); // Draw the constellations's names
    if (global.FlagSelect)           // Draw the star pointer
        DrawPointer(global.SelectedObject.XYZ,
		    global.SelectedObject.Size,
		    global.SelectedObject.RGB,
		    global.SelectedObject.type);
    
    // ---- AltAzimutal Coordinates
    Switch_to_altazimutal();         // Switch in AltAzimutal coordinates
    if (global.FlagAzimutalGrid)
    {
	DrawMeridiensAzimut();       // Draw the "Altazimuthal meridian" lines
        DrawParallelsAzimut();       // Draw the "Altazimuthal parallel" lines
    }
    if (global.FlagAtmosphere)       // Calc the atmosphere
    {   
        if (++timeAtmosphere>2 && global.SkyBrightness>0)
        {
	    timeAtmosphere=0;
            CalcAtmosphere();
        }
    }
    if (global.FlagHorizon && global.FlagGround)    
        DrawDecor(2);                               // Draw the mountains
    if (global.FlagGround) DrawGround();            // Draw the ground
    if (global.FlagFog) DrawFog();                  // Draw the fog
    if (global.FlagCardinalPoints) DrawCardinaux(); // Daw the cardinal points

    // ---- 2D Displays
    renderUi();

    glutSwapBuffers();               // Swap the 2 buffers
}

// ************************  On resize  *******************************
void glutResize(int w, int h)
{   
    if (!h) return;
    global.X_Resolution = w;
    global.Y_Resolution = h;
    clearUi();
    initUi();
    glViewport(0, 0, global.X_Resolution, global.Y_Resolution);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(global.Fov, (double) global.X_Resolution / 
		   global.Y_Resolution, 1, 10000);   // Update the ratio
    glutPostRedisplay();
    glMatrixMode(GL_MODELVIEW);
}

// *******************  Handle the flags keys  **************************
void processNormalKeys(unsigned char key, int x, int y) 
{
    HandleNormalKey(key,GUI_DOWN);
}

// ************************  Initialisation  ******************************
void loadCommonTextures(void)
{   
    printf("Loading common textures...\n");
    texIds[2] = new s_texture("voielactee256x256",TEX_LOAD_TYPE_PNG_SOLID);
    texIds[3] = new s_texture("fog");
    texIds[4] = new s_texture("ciel");  
    texIds[6] = new s_texture("n");
    texIds[7] = new s_texture("s");
    texIds[8] = new s_texture("e");
    texIds[9] = new s_texture("w");
    texIds[10]= new s_texture("zenith");
    texIds[11]= new s_texture("nadir");
    texIds[12]= new s_texture("pointeur2");
    texIds[25]= new s_texture("etoile32x32");
    texIds[26]= new s_texture("pointeur4");
    texIds[27]= new s_texture("pointeur5");
    texIds[30]= new s_texture("spacefont");

    switch (global.LandscapeNumber)
    {
    case 1 : 
        texIds[31]= new s_texture("landscapes/sea1",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/sea2",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/sea3",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/sea4",TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/sea5",TEX_LOAD_TYPE_PNG_SOLID);
        break;
    case 2 : 
        texIds[31]= new s_texture("landscapes/mountain1",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/mountain2",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/mountain3",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/mountain4",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/mountain5",
				  TEX_LOAD_TYPE_PNG_SOLID);
        break;
    case 3 : 
        texIds[31]= new s_texture("landscapes/snowy1",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[32]= new s_texture("landscapes/snowy2",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[33]= new s_texture("landscapes/snowy3",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[34]= new s_texture("landscapes/snowy4",
				  TEX_LOAD_TYPE_PNG_ALPHA);
        texIds[1] = new s_texture("landscapes/snowy5",
				  TEX_LOAD_TYPE_PNG_SOLID);
        break;
    default : 
        printf("ERROR : Bad landscape number, change it in config.txt\n");
        exit(1);
    }

    texIds[47]= new s_texture("saturneAnneaux128x128",TEX_LOAD_TYPE_PNG_ALPHA);
    texIds[48]= new s_texture("halo");
    texIds[50]= new s_texture("haloLune");
    if (messiers->ReadTexture()==0) 
	printf("Error while loading messier Texture\n");
}

// ********************  Handle the special keys  **********************
void pressKey(int key, int, int) 
{   
    // Direction and zoom deplacements
    switch (key) 
    {
    case GLUT_KEY_LEFT :
	global.deltaAz = -1;
	global.FlagTraking=false; 
	break;
    case GLUT_KEY_RIGHT : 
	global.deltaAz =  1;
	global.FlagTraking=false;
	break;
    case GLUT_KEY_UP :
	global.deltaAlt =  1;
	global.FlagTraking=false;
	break;
    case GLUT_KEY_DOWN :
	global.deltaAlt = -1;
	global.FlagTraking=false;
	break;
    case GLUT_KEY_PAGE_UP :
	global.deltaFov= -1;
	break;
    case GLUT_KEY_PAGE_DOWN :
	global.deltaFov=  1;
	break;
    case GLUT_KEY_F1 : 
	glutFullScreen();
	break;
    case GLUT_KEY_F2 :
	glutReshapeWindow(640, 480);
	break;
    }
}

// *******************  Stop mooving and zooming  **********************
void releaseKey(int key, int, int)
{   
    // When a deplacement key is released stop mooving
    switch (key)
    {
    case GLUT_KEY_LEFT :
    case GLUT_KEY_RIGHT : 
	global.deltaAz = 0;
	break;
    case GLUT_KEY_UP : 
    case GLUT_KEY_DOWN :
	global.deltaAlt = 0;
	break;
    case GLUT_KEY_PAGE_UP :
    case GLUT_KEY_PAGE_DOWN :
	global.deltaFov = 0;
	break;
    }
}

// ***************  Handle the mouse deplacement  *********************
void MoveMouse(int x, int y)
{
    HandleMove(x,y);
}

// *******************  Handle the mouse clic  *************************
void Mouse(int button, int state, int x, int y)
{
    HandleClic(x,y,state,button);
}

// *******************  Read the conuration file  *************************
void loadParams(void)
{
    global.SkyBrightness=0;
    global.Fov=60.;
    global.X_Resolution=800;
    global.Y_Resolution=600;
    global.RaZenith=0;
    global.DeZenith=0;
    global.AzVision=0;
    global.deltaFov=0;
    global.deltaAlt=0;
    global.deltaAz=0;
    global.Fps=40;
    global.FlagSelect=false;
    global.FlagRealMode=false;
    global.FlagConfig = false;
    global.StarTwinkleAmount = 2;
    global.FlagTraking = false;
    global.TimeDirection = 1;

    double tempLatitude=0;
    double tempLongitude=0;
    global.Altitude = 100;
    char * tempDate = NULL;
    char * tempTime = NULL;

    printf("Loading configuration file...\n");
    // array of cfgStruct for reading on config file
    cfgStruct cfgini[] = 
    {// parameter               type        address of variable
        {"STAR_TWINKLE_AMOUNT", CFG_FLOAT,  &global.StarTwinkleAmount},
        {"STAR_SCALE",          CFG_FLOAT,  &global.StarScale},
        {"X_RESOLUTION",        CFG_INT,    &global.X_Resolution},
        {"Y_RESOLUTION",        CFG_INT,    &global.Y_Resolution},
        {"FULLSCREEN",          CFG_BOOL,   &global.Fullscreen},
        {"BBP_MODE",            CFG_INT,    &global.bppMode},
        {"FLAG_FPS",            CFG_BOOL,   &global.FlagFps},       
        {"FLAG_STARS",          CFG_BOOL,   &global.FlagStars},
        {"FLAG_STAR_NAME",      CFG_BOOL,   &global.FlagStarName},
        {"MAX_MAG_STAR_NAME",   CFG_FLOAT,  &global.MaxMagStarName},
        {"FLAG_PLANETS",        CFG_BOOL,   &global.FlagPlanets},
        {"FLAG_PLANETS_NAME",   CFG_BOOL,   &global.FlagPlanetsHintDrawing},
        {"FLAG_NEBULA",         CFG_BOOL,   &global.FlagNebula},
        {"FLAG_NEBULA_NAME",    CFG_BOOL,   &global.FlagNebulaName},
        {"FLAG_GROUND",         CFG_BOOL,   &global.FlagGround},
        {"FLAG_HORIZON",        CFG_BOOL,   &global.FlagHorizon},
        {"FLAG_FOG",            CFG_BOOL,   &global.FlagFog},
        {"FLAG_ATMOSPHERE",     CFG_BOOL,   &global.FlagAtmosphere},
        {"FLAG_CONSTELLATION_DRAWING",CFG_BOOL, &global.FlagConstellationDrawing},
        {"FLAG_CONSTELLATION_NAME",   CFG_BOOL, &global.FlagConstellationName},
        {"FLAG_AZIMUTAL_GRID",  CFG_BOOL,   &global.FlagAzimutalGrid},
        {"FLAG_EQUATORIAL_GRID",CFG_BOOL,   &global.FlagEquatorialGrid},
        {"FLAG_EQUATOR",        CFG_BOOL,   &global.FlagEquator},
        {"FLAG_ECLIPTIC",       CFG_BOOL,   &global.FlagEcliptic},
        {"FLAG_CARDINAL_POINTS",CFG_BOOL,   &global.FlagCardinalPoints},
        {"FLAG_REAL_TIME",      CFG_BOOL,   &global.FlagRealTime},
        {"FLAG_ACCELERED_TIME", CFG_BOOL,   &global.FlagAcceleredTime},
        {"FLAG_VERY_FAST_TIME", CFG_BOOL,   &global.FlagVeryFastTime},
        {"FLAG_MENU",           CFG_BOOL,   &global.FlagMenu},
        {"FLAG_HELP",           CFG_BOOL,   &global.FlagHelp},
        {"FLAG_INFOS",          CFG_BOOL,   &global.FlagInfos},
        {"FLAG_MILKY_WAY",      CFG_BOOL,   &global.FlagMilkyWay},
        {"FLAG_FOLLOW_EARTH",   CFG_BOOL,   &global.FlagFollowEarth},
        {"DATE",                CFG_STRING, &tempDate},
        {"TIME",                CFG_STRING, &tempTime},
        {"FLAG_UTC_TIME",       CFG_BOOL,   &global.FlagUTC_Time},
        {"LANDSCAPE_NUMBER",    CFG_INT,    &global.LandscapeNumber}, 
        {NULL, CFG_END, NULL}   /* no more parameters */
    };
    
    char tempName[255];
    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"config.txt");
    if (cfgParse(tempName, cfgini, CFG_SIMPLE) == -1)
    {   
	printf("An error was detected in the config file\n");
        return;
    }


    cfgStruct cfgini2[] = 
    {// parameter               type        address of variable
        {"LATITUDE",            CFG_DOUBLE, &tempLatitude},
        {"LONGITUDE",           CFG_DOUBLE, &tempLongitude},
        {"ALTITUDE",            CFG_INT,    &global.Altitude},
        {"TIME_ZONE",           CFG_INT,    &global.TimeZone},
        {NULL, CFG_END, NULL}   /* no more parameters */
    };

    strcpy(tempName,global.ConfigDir);
    strcat(tempName,"location.txt");
    if (cfgParse(tempName, cfgini2, CFG_SIMPLE) == -1)
    {
	printf("An error was detected in the location file\n");
        return;
    }

    // init the time parameters with current time and date
    int d,m,y,hour,min,sec;
    time_t rawtime;
    tm * ptm;
    time ( &rawtime );
    ptm = gmtime ( &rawtime );
    y=ptm->tm_year+1900;
    m=ptm->tm_mon+1;
    d=ptm->tm_mday;
    hour=ptm->tm_hour;
    min=ptm->tm_min;
    sec=ptm->tm_sec;

    // If no date given -> default today
    if (tempDate!=NULL && (sscanf(tempDate,"%d/%d/%d\n",&m,&d,&y) != 3))
    {   
	printf("ERROR, bad date format : please change config.txt\n\n");
        exit(1);
    }
    if (tempTime!=NULL && (sscanf(tempTime,"%d:%d:%d\n",&hour,&min,&sec) != 3))
    {   
	printf("ERROR, bad time format : please change config.txt\n\n");
        exit(1);
    }

    if (m>12 || m<1 || d<1 || d>31)
    {   
	printf("ERROR, bad month value : please change config.txt\n\n");
        exit(0);
    }

    if (hour>23 || hour<0 || min<0 || min>59 || sec<0 || sec>59)
    {   
	printf("ERROR, bad time value : please change config.txt\n\n");
        exit(0);
    }

    if (tempDate) 
	delete tempDate;
    if (tempTime) 
    {   
        hour-=global.TimeZone;
        delete tempTime;    
    }
    // calc the julian date and store it in the global variable JDay
    global.JDay=DateOps::dmyToDay(d,m,y);
    global.JDay+=((double)hour*HEURE+((double)min+(double)sec/60)*MINUTE);


    // arrange the latitude/longitude to fit with astroOps conventions
    tempLatitude=90.-tempLatitude;
    tempLongitude=-tempLongitude;
    if (tempLongitude<0)
	tempLongitude=360.+tempLongitude;
    // Set the change in the position
    global.ThePlace.setLongitude(tempLongitude);
    global.ThePlace.setLatitude(tempLatitude);
}

// ****************  Initialisation of glut and openGL  ****************
void init() 
{   
    glutKeyboardFunc(processNormalKeys);
    glutSpecialFunc(pressKey);
    glutSpecialUpFunc(releaseKey);
    glutDisplayFunc(glutDisplay);
    glutIdleFunc(glutDisplay);
    glutReshapeFunc(glutResize);
    glutMouseFunc(Mouse);
    glutMotionFunc(MoveMouse);
    glutPassiveMotionFunc(MoveMouse);
}

// ***************************  Main  **********************************
int main (void)
{   
    // Set the data directories : test the default installation dir
    // and try to find the files somewhere else if not found there
    char dataRoot[255];
    char tempName[255];
    strcpy(tempName,CONFIG_DATA_DIR);
    strcat(tempName,"/data/catalog.fab");
    FILE * tempFile = fopen(tempName,"r");
    strcpy(dataRoot,CONFIG_DATA_DIR);
    if(!tempFile)
    {
        tempFile = fopen("./data/catalog.fab","r");
        strcpy(dataRoot,".");
        if(!tempFile)
        {
            strcpy(dataRoot,"..");
            tempFile = fopen("../data/catalog.fab","r");
            if(!tempFile)
            {
                strcpy(dataRoot,"$HOME");
                tempFile = fopen("$HOME/.stellarium/data/catalog.fab","r");
                if(!tempFile)
                {
                    // Failure....
                    printf("ERROR : I can't find the datas directories in :\n");
                    printf("%s/ nor in ./ nor in ../ nor $HOME/.stellarium/\n",CONFIG_DATA_DIR);
                    printf("You may fully install the software (on POSIX systems)\n");
                    printf("or go in the stellarium pakage directory.\n");
                    exit(-1);
                }
            }
        }
    }
    fclose(tempFile);
    if (strcmp(dataRoot,CONFIG_DATA_DIR))
	printf("------>Found data files in %s\n",dataRoot);
    strcpy(global.DataDir,dataRoot);
    strcpy(global.TextureDir,dataRoot);
    strcpy(global.ConfigDir,dataRoot);
    strcat(global.DataDir,"/data/");
    strcat(global.TextureDir,"/textures/");
    strcat(global.ConfigDir,"/config/");
    
    
    drawIntro();                     // Print the console logo
    loadParams();                    // Load the params from config.txt
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    if (global.Fullscreen)           // FullScreen Mode
    {
	char str[20];                // Init screen size
        sprintf(str,"%dx%d:%d",global.X_Resolution,
		global.Y_Resolution,global.bppMode);
        glutGameModeString(str);     // define resolution, color depth
        if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE))   // enter full screen
        {
	    glutEnterGameMode();
        }
        else                         // Error
        {   
	    printf("\n\nERROR : Unsuported screen mode, change the config.txt file\n");
            exit(1);
        }
    }
    else                             // Windowed mode
    {  
	glutCreateWindow(APP_NAME);
        glutFullScreen();            // Windowed fullscreen mode
        //global.X_Resolution = glutGet(GLUT_SCREEN_WIDTH);
        //global.Y_Resolution = glutGet(GLUT_SCREEN_HEIGHT);
        //glutReshapeWindow(global.X_Resolution, global.Y_Resolution);
    }

    glutIgnoreKeyRepeat(1);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glDisable(GL_DEPTH_TEST);
    // init the blending function parameters
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    init();                          // Set the callbacks
    
    VouteCeleste = new Star_mgr();
    if (!VouteCeleste) exit(1);

    HipVouteCeleste = new Hip_Star_mgr();
    if (!HipVouteCeleste) exit(1);

    ConstellCeleste = new Constellation_mgr();
    if (!ConstellCeleste) exit(1);

    messiers = new Nebula_mgr();
    if (!messiers) exit(1);

    SolarSystem = new Planet_mgr();
    if (!SolarSystem) exit(1);

    loadCommonTextures();            // Load the common used textures
    SolarSystem->loadTextures();
    
    strcpy(tempName,global.DataDir);
    strcat(tempName,"catalog.fab");
    VouteCeleste->Load(tempName);    // Load stars
   
    strcpy(tempName,global.DataDir);
    strcat(tempName,"hipparcos.dat"); 
    HipVouteCeleste->Load(tempName); // Load hipparcos stars
    
    strcpy(tempName,global.DataDir);
    strcat(tempName,"constellationsmaj.fab");
    ConstellCeleste->Load(tempName,VouteCeleste);     // Load constellations      
    SolarSystem->Compute(global.JDay,global.ThePlace);// Compute planet data
    InitMeriParal();                 // Precalculation for the grids drawing
    InitAtmosphere();
    
    strcpy(tempName,global.DataDir);
    strcat(tempName,"messier.fab");
    messiers->Read(tempName);        // read the messiers object data
    initUi();                        // initialisation of the User Interface
    global.XYZVision.Set(0,1,0);
    glutMainLoop();                  // Start drawing
    return 0;
}
